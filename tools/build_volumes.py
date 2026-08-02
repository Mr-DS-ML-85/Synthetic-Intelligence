#!/usr/bin/env python3
"""
build_volumes.py — assemble the Zenodo depositions.

Each volume is one citable PDF: an editorial front matter followed by
its constituent papers, concatenated with their headings demoted so the
volume reads as one document rather than a stapled pile.

    python3 tools/build_volumes.py            # all volumes
    python3 tools/build_volumes.py 01 05      # by slug prefix
    python3 tools/build_volumes.py --check    # verify coverage only

COVERAGE IS ENFORCED. Every .md under research-paper/ must belong to
exactly one volume. A paper in no volume is silently lost work; a paper
in two volumes gets two DOIs for one document. Both abort the build.
"""

import argparse
import io
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "research-paper")
OUT = os.path.join(SRC, "build")
VOL_MD = os.path.join(OUT, "volumes", "md")
VOL_TEX = os.path.join(OUT, "volumes", "tex")
VOL_PDF = os.path.join(OUT, "volumes", "pdf")
LOG = os.path.join(OUT, "volumes", "log")
CONF = os.path.join(ROOT, "tools", "volumes.conf")
PRE = os.path.join(ROOT, "tools", "paper_preamble.tex")

AUTHOR = ("Irfan Mahir — Independent Researcher, Dhaka, Bangladesh "
          "— irfan@furylogic.com")


def parse_conf(path):
    vols, cur = [], None
    for raw in io.open(path, encoding="utf-8"):
        line = raw.rstrip("\n")
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if s.startswith("[") and s.endswith("]"):
            cur = {"slug": s[1:-1], "title": "", "blurb": "", "note": "",
                   "papers": []}
            vols.append(cur)
            continue
        if cur is None:
            continue
        key, _, val = s.partition(":")
        key, val = key.strip(), val.strip()
        if key == "papers":
            cur["papers"] = val.split()
        elif key in ("title", "blurb", "note"):
            cur[key] = val
    return vols


def all_papers():
    found = []
    for dirpath, _dirs, files in os.walk(SRC):
        if os.path.join(SRC, "build") in dirpath:
            continue
        for f in files:
            if f.endswith(".md"):
                rel = os.path.relpath(os.path.join(dirpath, f), SRC)
                found.append(rel)
    return sorted(found)


def check_coverage(vols):
    assigned = {}
    dupes = []
    for v in vols:
        for p in v["papers"]:
            if p in assigned:
                dupes.append((p, assigned[p], v["slug"]))
            assigned[p] = v["slug"]
    on_disk = set(all_papers())
    listed = set(assigned)

    missing = sorted(on_disk - listed)      # exists, in no volume
    phantom = sorted(listed - on_disk)      # listed, does not exist

    ok = True
    if missing:
        ok = False
        print("ERROR: papers on disk but in NO volume (work would be lost):")
        for p in missing:
            print("   ", p)
    if phantom:
        ok = False
        print("ERROR: papers listed in a volume but missing from disk:")
        for p in phantom:
            print("   ", p)
    if dupes:
        ok = False
        print("ERROR: papers in MORE THAN ONE volume (would get two DOIs):")
        for p, a, b in dupes:
            print("    %s  in [%s] and [%s]" % (p, a, b))
    if ok:
        print("coverage OK: %d papers, %d volumes, each paper in exactly one"
              % (len(on_disk), len(vols)))
    return ok


HEADING = re.compile(r"^(#{1,6})\s")
AUTHOR_BLOCK = re.compile(
    r"^\*\*Irfan Mahir\*\*\s*$|^\*Independent Researcher[^\n]*\*\s*$"
    r"|^irfan@furylogic\.com\s*$")


def prepare(path, demote):
    """Read a paper, strip its redundant author block, demote headings."""
    raw = io.open(os.path.join(SRC, path), encoding="utf-8").read()
    out, in_fence = [], False
    for line in raw.split("\n"):
        if line.startswith("```"):
            in_fence = not in_fence
            out.append(line)
            continue
        if not in_fence:
            # The volume carries one author block on its title page; a
            # repeat under every section is noise.
            if AUTHOR_BLOCK.match(line.strip()):
                continue
            m = HEADING.match(line)
            if m:
                lvl = min(len(m.group(1)) + demote, 6)
                line = "#" * lvl + line[m.end(1):]
        out.append(line)
    return "\n".join(out).strip()


def build_volume(v, only_check=False):
    for d in (VOL_MD, VOL_TEX, VOL_PDF, LOG):
        os.makedirs(d, exist_ok=True)

    parts = []
    parts.append("# " + v["title"] + "\n")
    parts.append("**Irfan Mahir**  \n*Independent Researcher, Dhaka, "
                 "Bangladesh*  \nirfan@furylogic.com\n")
    parts.append("> " + v["blurb"] + "\n")
    parts.append("---\n")
    if v.get("note"):
        # Rendered before the contents so it cannot be skipped. Used for
        # citation hazards — most importantly that a retired PAPER may
        # still describe a LIVE mechanism.
        parts.append("## Read This Before Citing\n")
        parts.append(v["note"].replace("\\n", "\n") + "\n")
        parts.append("---\n")

    # Contents table with a status column, so a reader knows before
    # reading which pieces are load-bearing and which are retired.
    parts.append("## Contents of This Volume\n")
    parts.append("| # | Paper | File |")
    parts.append("| --- | --- | --- |")
    for i, p in enumerate(v["papers"], 1):
        title = ""
        for line in io.open(os.path.join(SRC, p), encoding="utf-8"):
            if line.startswith("# "):
                title = line[2:].strip()
                break
        title = title.replace("|", "\\|")
        parts.append("| %d | %s | `%s` |" % (i, title or p, p))
    parts.append("")
    parts.append("---\n")

    for p in v["papers"]:
        parts.append("\n\\newpage\n")
        parts.append(prepare(p, demote=1))
        parts.append("\n")

    md = "\n".join(parts)
    md_path = os.path.join(VOL_MD, v["slug"] + ".md")
    io.open(md_path, "w", encoding="utf-8").write(md)
    if only_check:
        return True

    tex_path = os.path.join(VOL_TEX, v["slug"] + ".tex")
    cmd = [
        "pandoc", md_path, "--standalone",
        "--from=markdown+tex_math_dollars+tex_math_single_backslash"
        "+pipe_tables+backtick_code_blocks+raw_tex",
        "--to=latex", "--pdf-engine=lualatex",
        "--include-in-header=" + PRE,
        "--metadata", "title=" + v["title"],
        "--metadata", "author=" + AUTHOR,
        "--variable", "documentclass=article",
        "--variable", "papersize=a4",
        "--variable", "fontsize=11pt",
        "--variable", "geometry:margin=1in",
        "--variable", "colorlinks=true",
        "--variable", "linkcolor=black",
        "--variable", "mainfont=Latin Modern Roman",
        "--variable", "mainfontfallback=NotoSerifBengali:mode=harf",
        "--variable", "monofont=DejaVu Sans Mono",
        "--variable", "monofontfallback=NotoSansBengali:mode=harf",
        "--toc", "--toc-depth=2",
        "--highlight-style=tango",
        "-o", tex_path,
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 or not os.path.exists(tex_path):
        print("  TEX FAIL  %s" % v["slug"])
        io.open(os.path.join(LOG, v["slug"] + ".tex.log"), "w").write(r.stderr)
        return False

    logf = os.path.join(LOG, v["slug"] + ".pdf.log")
    for _ in range(2):
        r = subprocess.run(
            ["lualatex", "-interaction=nonstopmode", "-halt-on-error",
             "-output-directory=" + VOL_TEX, tex_path],
            capture_output=True, text=True, cwd=VOL_TEX)
    io.open(logf, "w").write(r.stdout[-40000:])

    produced = os.path.join(VOL_TEX, v["slug"] + ".pdf")
    if os.path.exists(produced):
        os.replace(produced, os.path.join(VOL_PDF, v["slug"] + ".pdf"))
        print("  ok        %-18s (%d papers)" % (v["slug"], len(v["papers"])))
        return True
    print("  PDF FAIL  %-18s  see %s" % (v["slug"], logf))
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("slugs", nargs="*")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    vols = parse_conf(CONF)
    if not check_coverage(vols):
        sys.exit(1)
    if args.check:
        return

    targets = [v for v in vols
               if not args.slugs or any(v["slug"].startswith(s)
                                        for s in args.slugs)]
    ok = sum(build_volume(v) for v in targets)
    for ext in (".aux", ".out", ".toc", ".log"):
        for f in os.listdir(VOL_TEX):
            if f.endswith(ext):
                os.remove(os.path.join(VOL_TEX, f))
    print("\nbuilt %d/%d volumes" % (ok, len(targets)))
    print("pdf: " + VOL_PDF)


if __name__ == "__main__":
    main()
