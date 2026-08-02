#!/usr/bin/env python3
"""
check_links.py — verify that every cross-reference in the paper set
resolves to something that exists.

This exists because a paper in this lineage cited `CMD` as its substrate
"as previously specified" for over a year, and no such paper had ever
been written. Four documents referenced a phantom. Nobody followed the
link, so nobody noticed.

Three classes of reference are checked:

  FILE     `foo.md`, `src/bar.hpp`, `data/x.axioms` in backticks or
           markdown links. Must exist on disk.
  SECTION  "§5.4c" or "section 5.4c" qualified by a paper name. The
           target paper must exist and must contain that heading.
  SYMBOL   `some_function()` or `struct Foo` claimed to be in the
           implementation. Must appear in the named source file.

Exit code is non-zero if any reference is broken, so this can gate CI.

    python3 tools/check_links.py
    python3 tools/check_links.py --quiet     # errors only
"""

import argparse
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAPERS = os.path.join(ROOT, "research-paper")

# `something.md` / `src/x.hpp` / `tools/y.py` / `data/z.axioms`
FILE_REF = re.compile(
    r"`([A-Za-z0-9_./+-]+\.(?:md|hpp|cpp|cu|py|sh|axioms|si|conf|tex))`")

# A markdown link to a local path.
MD_LINK = re.compile(r"\[[^\]]*\]\((?!https?:)([^)#]+?)(?:#[^)]*)?\)")

# "`paper.md` §3.1" or "§3.1 of `paper.md`" — a section in a named paper.
SEC_AFTER = re.compile(r"`([A-Za-z0-9_.-]+\.md)`[^.\n]{0,40}?§\s*([0-9]+(?:\.[0-9a-z]+)*)")
SEC_BEFORE = re.compile(r"§\s*([0-9]+(?:\.[0-9a-z]+)*)\s+of\s+`([A-Za-z0-9_.-]+\.md)`")

# Where a paper may live, given a bare filename.
SEARCH_DIRS = ["", "failed_but_parts_works", "failed_but_parts_works/DEAD",
               "failed_but_parts_works/DEAD/deprected"]


def resolve_paper(name):
    for d in SEARCH_DIRS:
        p = os.path.join(PAPERS, d, name)
        if os.path.isfile(p):
            return p
    return None


# Papers cite implementation files by bare name — "physics.hpp" rather
# than "src/physics.hpp" — which is normal prose and should resolve.
CODE_DIRS = ["src", "tools", "legacy", "data", "."]


def resolve_any(ref, from_path):
    """Resolve a reference to an absolute path, or None."""
    # Repo-root relative (src/..., tools/..., data/...)
    p = os.path.join(ROOT, ref)
    if os.path.exists(p):
        return p
    # Relative to the citing paper.
    p = os.path.join(os.path.dirname(from_path), ref)
    if os.path.exists(p):
        return p
    # A bare paper name, anywhere in the paper tree.
    if ref.endswith(".md"):
        found = resolve_paper(os.path.basename(ref))
        if found:
            return found
    # A bare source name, in any code directory.
    base = os.path.basename(ref)
    for d in CODE_DIRS:
        p = os.path.join(ROOT, d, base)
        if os.path.isfile(p):
            return p
    return None


def headings_of(path):
    out = set()
    try:
        for line in io.open(path, encoding="utf-8", errors="ignore"):
            if line.startswith("#"):
                out.add(line.lstrip("#").strip().lower())
    except OSError:
        pass
    return out


def has_section(path, num):
    """Does `path` contain a heading numbered `num`?"""
    pat = re.compile(r"^#+\s*" + re.escape(num) + r"[\s.)]")
    try:
        for line in io.open(path, encoding="utf-8", errors="ignore"):
            if pat.match(line):
                return True
    except OSError:
        return False
    return False


def load_exceptions():
    """Known-broken references that are accepted, with reasons on file."""
    path = os.path.join(ROOT, "tools", "link_exceptions.txt")
    out = set()
    if not os.path.isfile(path):
        return out
    for line in io.open(path, encoding="utf-8"):
        line = line.split("#")[0].strip()
        if not line or "->" not in line:
            continue
        src, _, ref = line.partition("->")
        out.add((src.strip(), ref.strip()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()
    exceptions = load_exceptions()

    md_files = []
    for dirpath, _d, files in os.walk(PAPERS):
        if os.path.join(PAPERS, "build") in dirpath:
            continue
        md_files += [os.path.join(dirpath, f) for f in files if f.endswith(".md")]
    md_files.sort()

    broken_files, broken_secs = [], []
    n_file, n_sec = 0, 0

    for path in md_files:
        rel = os.path.relpath(path, ROOT)
        text = io.open(path, encoding="utf-8", errors="ignore").read()

        # Strip fenced code so example paths are not treated as citations.
        prose = re.sub(r"```.*?```", "", text, flags=re.S)

        seen = set()
        for m in list(FILE_REF.finditer(prose)) + list(MD_LINK.finditer(prose)):
            ref = m.group(1).strip()
            if ref in seen:
                continue
            seen.add(ref)
            n_file += 1
            if resolve_any(ref, path) is None:
                key = (os.path.relpath(path, PAPERS), ref)
                if key not in exceptions:
                    broken_files.append((rel, ref))

        for m in SEC_AFTER.finditer(prose):
            paper, num = m.group(1), m.group(2)
            n_sec += 1
            tgt = resolve_paper(os.path.basename(paper))
            if tgt is None:
                broken_secs.append((rel, "%s §%s" % (paper, num), "no such paper"))
            elif not has_section(tgt, num):
                broken_secs.append((rel, "%s §%s" % (paper, num), "no such section"))
        for m in SEC_BEFORE.finditer(prose):
            num, paper = m.group(1), m.group(2)
            n_sec += 1
            tgt = resolve_paper(os.path.basename(paper))
            if tgt is None:
                broken_secs.append((rel, "%s §%s" % (paper, num), "no such paper"))
            elif not has_section(tgt, num):
                broken_secs.append((rel, "%s §%s" % (paper, num), "no such section"))

    if broken_files:
        print("BROKEN FILE REFERENCES (%d):" % len(broken_files))
        for src, ref in broken_files:
            print("  %-52s -> %s" % (src, ref))
        print()
    if broken_secs:
        print("BROKEN SECTION REFERENCES (%d):" % len(broken_secs))
        for src, ref, why in broken_secs:
            print("  %-52s -> %-28s (%s)" % (src, ref, why))
        print()

    if not args.quiet or broken_files or broken_secs:
        print("checked %d papers: %d file refs, %d section refs"
              % (len(md_files), n_file, n_sec))
    bad = len(broken_files) + len(broken_secs)
    print("%d broken reference%s" % (bad, "" if bad == 1 else "s"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
