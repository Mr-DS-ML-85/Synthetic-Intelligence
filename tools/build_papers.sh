#!/usr/bin/env bash
# build_papers.sh — render the paper set to LaTeX and PDF.
#
#   tools/build_papers.sh            # all papers
#   tools/build_papers.sh cll rbg    # only these (basename, no .md)
#
# Output:
#   research-paper/build/tex/<name>.tex
#   research-paper/build/pdf/<name>.pdf
#
# lualatex is required rather than pdflatex: several papers contain
# Bengali script as evidence (see cll.md §3.3) and pdflatex cannot set
# it. Everything is UTF-8 end to end.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/research-paper"
OUT="$SRC/build"
TEX="$OUT/tex"
PDF="$OUT/pdf"
LOG="$OUT/log"
PRE="$ROOT/tools/paper_preamble.tex"

mkdir -p "$TEX" "$PDF" "$LOG"

# Collect targets. Papers live at three depths: the live set, the
# partly-working set, and DEAD (kept so the archive is complete and a
# reader can see exactly what was retired).
collect() {
  if [ "$#" -gt 0 ]; then
    for n in "$@"; do
      find "$SRC" -name "$n.md" -not -path "*/build/*"
    done
  else
    find "$SRC" -name '*.md' -not -path "*/build/*" | sort
  fi
}

ok=0; fail=0; failed_list=()

while IFS= read -r md; do
  [ -z "$md" ] && continue
  rel="${md#$SRC/}"
  name="$(basename "$md" .md)"
  # Namespace by directory so failed_but_parts_works/rifa.md does not
  # collide with rifa.md — they are different documents.
  case "$rel" in
    failed_but_parts_works/DEAD/deprected/*) name="DEAD-deprected-$name" ;;
    failed_but_parts_works/DEAD/*)           name="DEAD-$name" ;;
    failed_but_parts_works/*)                name="partial-$name" ;;
  esac

  title="$(grep -m1 '^# ' "$md" | sed 's/^# *//' || true)"
  [ -z "$title" ] && title="$name"

  # --- markdown -> standalone LaTeX ---
  pandoc "$md" \
      --standalone \
      --from=markdown+tex_math_dollars+tex_math_single_backslash+pipe_tables+backtick_code_blocks+raw_tex \
      --to=latex \
      --pdf-engine=lualatex \
      --include-in-header="$PRE" \
      --metadata title="$title" \
      --metadata author="Irfan Mahir — Independent Researcher, Dhaka, Bangladesh — irfan@furylogic.com" \
      --variable documentclass=article \
      --variable papersize=a4 \
      --variable fontsize=11pt \
      --variable geometry:margin=1in \
      --variable colorlinks=true \
      --variable linkcolor=black \
      --variable urlcolor='[HTML]{0B5394}' \
      --variable mainfont='Latin Modern Roman' \
      --variable mainfontfallback='NotoSerifBengali:mode=harf' \
      --variable sansfont='Latin Modern Sans' \
      --variable sansfontfallback='NotoSansBengali:mode=harf' \
      --variable monofont='DejaVu Sans Mono' \
      --variable monofontfallback='NotoSansBengali:mode=harf' \
      --toc --toc-depth=3 \
      --highlight-style=tango \
      -o "$TEX/$name.tex" 2>"$LOG/$name.tex.log"

  if [ ! -s "$TEX/$name.tex" ]; then
    echo "  TEX FAIL  $rel"; fail=$((fail+1)); failed_list+=("$rel (tex)"); continue
  fi

  # --- LaTeX -> PDF ---
  # Two passes so the table of contents resolves. -halt-on-error keeps a
  # broken paper from hanging the batch on an interactive prompt.
  (
    cd "$TEX"
    for pass in 1 2; do
      lualatex -interaction=nonstopmode -halt-on-error \
               -output-directory="$TEX" "$name.tex" \
               >"$LOG/$name.pdf.log" 2>&1 || true
    done
  )

  if [ -s "$TEX/$name.pdf" ]; then
    mv -f "$TEX/$name.pdf" "$PDF/$name.pdf"
    echo "  ok        $rel"
    ok=$((ok+1))
  else
    echo "  PDF FAIL  $rel   (see $LOG/$name.pdf.log)"
    fail=$((fail+1)); failed_list+=("$rel (pdf)")
  fi
done < <(collect "$@")

# Drop LaTeX intermediates; keep the .tex sources, which are a
# deliverable in their own right for journal submission.
rm -f "$TEX"/*.aux "$TEX"/*.out "$TEX"/*.toc "$TEX"/*.nav \
      "$TEX"/*.snm "$TEX"/*.vrb "$TEX"/*.fls "$TEX"/*.fdb_latexmk 2>/dev/null

echo
echo "built $ok pdf, $fail failed"
if [ "$fail" -gt 0 ]; then
  printf '  %s\n' "${failed_list[@]}"
fi
echo "tex: $TEX"
echo "pdf: $PDF"
