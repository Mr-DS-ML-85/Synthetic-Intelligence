#!/usr/bin/env python3
"""
normalize_papers.py — repair markdown that was pasted rather than written.

Several papers in this set were captured as raw text dumps: a bare
language marker on its own line ("C++", "Python") followed by source at
column zero, with no fences anywhere in the file. GitHub renders that as
mangled prose and LaTeX refuses it outright — `<<` becomes
\\textless\\textless, `\\n` becomes an undefined control sequence, and the
build dies.

Two repairs, both conservative:

  1. FENCE  A bare language-marker line followed by code-looking lines is
            wrapped in a fenced block. The scan stops at the first line
            that looks like prose, so a mis-detection truncates the block
            rather than swallowing the rest of the paper.

  2. MATH   An underscore inside \\text{...} in a math span is escaped.
            `\\text{success_signal}` is a subscript to LaTeX and an error
            in text mode; the author meant a literal underscore.

Run with --dry-run first. Every change is reported with its line number.

    python3 tools/normalize_papers.py --dry-run
    python3 tools/normalize_papers.py
"""

import argparse
import glob
import io
import os
import re
import sys

LANG_MARKERS = {
    "c++": "cpp", "cpp": "cpp", "c": "c",
    "python": "python", "py": "python",
    "rust": "rust", "bash": "bash", "sh": "bash",
    "json": "json", "yaml": "yaml", "text": "text",
}

# A line that almost certainly belongs to source code.
CODE_HINT = re.compile(
    r"(\{|\}|;\s*$|^\s*//|^\s*#include|^\s*#\s|\breturn\b|\bfor\s*\(|"
    r"\bif\s*\(|\bwhile\s*\(|\bdef\s+\w+\(|\bclass\s+\w+|::|->|==|\+=|"
    r"^\s{4,}\S|\bstd::|\bauto\b|\bvoid\b|\bfloat\b|\bint\b)"
)

# A line that is almost certainly running prose.
def is_prose(line):
    s = line.strip()
    if not s:
        return False
    if CODE_HINT.search(line):
        return False
    if len(s) < 40:
        return False
    # Sentence-shaped: starts with a capital, contains a space, ends with
    # sentence punctuation.
    return bool(re.match(r"^[A-Z(\*#]", s)) and " " in s and s[-1] in ".:!?"


def fence_bare_blocks(lines, report, path):
    out = []
    i = 0
    n = len(lines)
    in_fence = False
    while i < n:
        line = lines[i]
        if line.startswith("```"):
            in_fence = not in_fence
            out.append(line)
            i += 1
            continue
        if in_fence:
            out.append(line)
            i += 1
            continue

        marker = line.strip().lower()
        if marker in LANG_MARKERS and i + 1 < n:
            # Look ahead: is the next non-blank line code-shaped?
            j = i + 1
            while j < n and not lines[j].strip():
                j += 1
            if j < n and CODE_HINT.search(lines[j]):
                lang = LANG_MARKERS[marker]
                # Consume until prose or a heading.
                k = j
                blanks = 0
                while k < n:
                    cur = lines[k]
                    if cur.startswith("#") or cur.startswith("```"):
                        break
                    if not cur.strip():
                        blanks += 1
                        if blanks >= 2:
                            break
                        k += 1
                        continue
                    if is_prose(cur):
                        break
                    blanks = 0
                    k += 1
                # Trim trailing blanks from the block.
                end = k
                while end > j and not lines[end - 1].strip():
                    end -= 1
                if end > j:
                    report.append("%s:%d fenced %d lines as `%s`"
                                  % (path, i + 1, end - j, lang))
                    out.append("```" + lang)
                    out.extend(lines[j:end])
                    out.append("```")
                    i = end
                    continue
        out.append(line)
        i += 1
    return out


TEXT_US = re.compile(r"(\\text\{)([^}]*_[^}]*)(\})")


def escape_text_underscores(src, report, path):
    def fix(m):
        inner = m.group(2)
        if "\\_" in inner:
            return m.group(0)
        report.append("%s: escaped underscore in \\text{%s}" % (path, inner))
        return m.group(1) + inner.replace("_", r"\_") + m.group(3)
    return TEXT_US.sub(fix, src)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--root", default="research-paper")
    args = ap.parse_args()

    files = sorted(
        glob.glob(os.path.join(args.root, "**", "*.md"), recursive=True))
    files = [f for f in files if "/build/" not in f]

    report = []
    changed = 0
    for p in files:
        src = io.open(p, encoding="utf-8").read()
        orig = src

        lines = src.split("\n")
        lines = fence_bare_blocks(lines, report, p)
        src = "\n".join(lines)
        src = escape_text_underscores(src, report, p)

        if src != orig:
            changed += 1
            if not args.dry_run:
                io.open(p, "w", encoding="utf-8").write(src)

    for r in report:
        print(" ", r)
    print("\n%s: %d files changed, %d edits"
          % ("DRY RUN" if args.dry_run else "WROTE", changed, len(report)))


if __name__ == "__main__":
    main()
