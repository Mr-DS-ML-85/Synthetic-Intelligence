#!/usr/bin/env python3
"""
extract_axioms.py — refine raw NL corpora (C4 / Pile / RedPajama JSONL)
into a small ontology usable by the PCMA engine.

We use the classical Hearst (1992) lexico-syntactic patterns for is_a
extraction, plus a handful of high-precision has/causes/implies patterns.
The output is deduplicated (subject, relation, object) triples in the
format expected by src/axiom_loader.hpp.

Design:
  - Streaming — never load the whole file into memory.
  - Case-normalised, lowercase.
  - Filter noisy candidates aggressively (short / stopword / non-alpha).
  - Cap the output per (relation, subject) at 3 to prevent hub blowup.

Usage:
  python3 tools/extract_axioms.py \
      --input ~/Desktop/datasets/c4_sample/c4_sample.jsonl \
      --output data/c4_extracted.axioms \
      --max-triples 500
"""
from __future__ import annotations
import argparse, json, re, sys
from collections import defaultdict, Counter
from pathlib import Path

STOPWORDS = set("""
a an the this that these those and or but if then when where why how what who
which whom whose is are was were be been being have has had do does did will
would should could may might must shall can of in on at to for with by from as
about into over under it its i you he she we they me him her us them my your
his our their not no yes so than also only just very more most much many few
here there now then thing things something anything nothing everything
one two three four five six seven eight nine ten first second last next
time way thing part case lot need number list link code line
i_am i_was i_feel i_felt he_is she_is they_are we_are you_are it_is
b_which c_which a_which d_which a_what b_what c_what d_what
""".split())

# Additional filter: any token containing digits followed by letters
# (like 'a_which', 'b_what') is treated as noise.
_NOISE_RE = re.compile(r"^[a-z]_[a-z]+$")

MIN_LEN, MAX_LEN = 3, 24

def clean_token(tok: str) -> str | None:
    tok = tok.strip().lower()
    tok = re.sub(r"[^a-z0-9_\- ]+", "", tok)
    tok = re.sub(r"\s+", "_", tok).strip("_-")
    if not tok: return None
    if len(tok) < MIN_LEN or len(tok) > MAX_LEN: return None
    if tok in STOPWORDS: return None
    if tok.isdigit(): return None
    if _NOISE_RE.match(tok): return None  # e.g. a_which, b_what
    # Reject if any token component is a stopword.
    parts = tok.split("_")
    if any(p in STOPWORDS for p in parts): return None
    # Must contain at least one substantive syllable (>=4 chars)
    if not any(len(p) >= 4 for p in parts): return None
    return tok

# Hearst patterns. Named groups: (subj, obj) or (X, Y) — orientation
# depends on the pattern.
HEARST = [
    # "X is a Y" | "X is an Y"
    (re.compile(r"\b(?P<x>[a-z][a-z0-9_\- ]{2,30}?)\s+(?:is|are)\s+(?:a|an|the)\s+(?P<y>[a-z][a-z0-9_\- ]{2,30}?)\b"), "is_a", "x", "y"),
    # "Y such as X"
    (re.compile(r"\b(?P<y>[a-z][a-z0-9_\- ]{2,30}?)\s+such\s+as\s+(?P<x>[a-z][a-z0-9_\- ]{2,30}?)\b"), "is_a", "x", "y"),
    # "Y including X"
    (re.compile(r"\b(?P<y>[a-z][a-z0-9_\- ]{2,30}?)\s+including\s+(?P<x>[a-z][a-z0-9_\- ]{2,30}?)\b"), "is_a", "x", "y"),
    # "Y like X"
    (re.compile(r"\b(?P<y>[a-z][a-z0-9_\- ]{2,30}?)\s+like\s+(?P<x>[a-z][a-z0-9_\- ]{2,30}?)\b"), "is_a", "x", "y"),
    # "X has Y" — high false positive rate, keep short
    (re.compile(r"\b(?P<x>[a-z][a-z0-9_\- ]{2,20}?)\s+has\s+(?P<y>[a-z][a-z0-9_\- ]{2,20}?)\b"), "has", "x", "y"),
    # "X causes Y"
    (re.compile(r"\b(?P<x>[a-z][a-z0-9_\- ]{2,20}?)\s+causes\s+(?P<y>[a-z][a-z0-9_\- ]{2,20}?)\b"), "causes", "x", "y"),
    # "X implies Y"
    (re.compile(r"\b(?P<x>[a-z][a-z0-9_\- ]{2,20}?)\s+implies\s+(?P<y>[a-z][a-z0-9_\- ]{2,20}?)\b"), "implies", "x", "y"),
    # "X is part of Y"
    (re.compile(r"\b(?P<x>[a-z][a-z0-9_\- ]{2,20}?)\s+is\s+part\s+of\s+(?P<y>[a-z][a-z0-9_\- ]{2,20}?)\b"), "part_of", "x", "y"),
]

def extract_triples(text: str):
    text = text.lower()
    # collapse whitespace so patterns match across newlines
    text = re.sub(r"\s+", " ", text)
    for pat, rel, subj_key, obj_key in HEARST:
        for m in pat.finditer(text):
            s = clean_token(m.group(subj_key))
            o = clean_token(m.group(obj_key))
            if s is None or o is None or s == o:
                continue
            yield (s, rel, o)

def stream_jsonl(path: Path, limit_lines: int | None):
    n = 0
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            n += 1
            if limit_lines is not None and n > limit_lines:
                break
            try:
                obj = json.loads(line)
            except Exception:
                continue
            text = obj.get("text") or obj.get("content") or ""
            if not isinstance(text, str) or len(text) < 40:
                continue
            yield text

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--input",  required=True, help="Path to JSONL corpus")
    p.add_argument("--output", required=True, help="Output .axioms file")
    p.add_argument("--max-triples", type=int, default=500,
                   help="Cap final axiom count after dedup+ranking")
    p.add_argument("--max-per-subject", type=int, default=3,
                   help="Per-subject-per-relation cap (prevents hub blowup)")
    p.add_argument("--limit-lines", type=int, default=None,
                   help="Only read first N jsonl lines (for quick testing)")
    args = p.parse_args()

    counts = Counter()
    for text in stream_jsonl(Path(args.input), args.limit_lines):
        for t in extract_triples(text):
            counts[t] += 1

    # Sort by frequency, apply per-subject cap.
    ordered = sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))
    per_subj = defaultdict(int)
    kept = []
    for (s, rel, o), count in ordered:
        key = (s, rel)
        if per_subj[key] >= args.max_per_subject:
            continue
        # frequency floor — 2+ occurrences to survive
        if count < 2:
            continue
        per_subj[key] += 1
        kept.append((s, rel, o, count))
        if len(kept) >= args.max_triples:
            break

    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as out:
        out.write(f"# {args.output} — extracted by tools/extract_axioms.py\n")
        out.write(f"# source: {args.input}\n")
        out.write(f"# unique triples: {len(counts)}   kept: {len(kept)}\n")
        for s, rel, o, count in kept:
            out.write(f"{s:24s} {rel:12s} {o:24s} # freq={count}\n")

    print(f"[extract_axioms] {len(counts)} unique triples, "
          f"{len(kept)} kept → {args.output}",
          file=sys.stderr)

if __name__ == "__main__":
    main()
