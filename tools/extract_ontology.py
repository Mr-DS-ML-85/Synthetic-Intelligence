#!/usr/bin/env python3
"""
extract_ontology.py — extract (subject, relation, object) triples from
JSONL text datasets (Pile, C4, etc.) via classic Hearst-style patterns.

No ML. No LLM. Pure regex over text. The pipeline is:

  1. Load every .jsonl file under --input-dir and read the "text" field.
  2. Split each text into sentences.
  3. For each sentence, match all supported patterns and emit triples.
  4. Deduplicate and filter noisy triples (too long, too short, ends in
     stopwords, contains punctuation-only tokens).
  5. Write facts.txt with one "subject|REL|object" per line.

Output is consumed by src/facts_loader.hpp on the C++ side.

Patterns (subset of Hearst 1992 + a few common English forms):

  Y such as X                 → X IS_A Y
  Y including X               → X IS_A Y
  Y like X                    → X IS_A Y
  Y, especially X             → X IS_A Y
  X is a Y   / X is an Y      → X IS_A Y
  X are Y                     → X IS_A Y   (plural form)
  X has Y    / X have Y       → X HAS Y
  X causes Y  / X cause Y     → X CAUSES Y
  X is part of Y              → X PART_OF Y
  X are part of Y             → X PART_OF Y
  X implies Y                 → X IMPLIES Y
  if X then Y                 → X IMPLIES Y

Usage:

  python3 tools/extract_ontology.py \
      --input-dir ~/Desktop/datasets \
      --output facts.txt \
      --max-docs 20000 \
      --max-triples 5000
"""

import argparse
import json
import os
import re
import sys
from collections import Counter
from pathlib import Path

# ------------ noise filters ------------

STOPWORDS = {
    "a", "an", "the", "of", "in", "on", "at", "for", "to", "and", "or",
    "but", "with", "by", "from", "as", "into", "than", "then", "over",
    "under", "up", "down", "out", "so", "if", "not", "no", "yes", "very",
    "such", "these", "those", "this", "that", "some", "any", "all", "many",
    "much", "more", "most", "less", "least", "one", "two", "several",
    "few", "each", "every", "he", "she", "it", "they", "we", "you", "i",
    "his", "her", "their", "our", "your", "my", "me", "us", "them", "him",
    "was", "were", "been", "being", "am", "is", "are", "be", "have", "had",
    "has", "having", "do", "did", "does", "done", "doing", "will", "would",
    "could", "should", "may", "might", "must", "can", "cannot", "here",
    "there", "where", "when", "how", "why", "which", "who", "whom", "whose",
    "what",
}

MAX_TERM_LEN = 30
MIN_TERM_LEN = 3
MAX_TERM_WORDS = 4

# Tokens must be alphabetic + optionally underscore/hyphen; no digits,
# no punctuation.
TERM_RE = re.compile(r"^[a-z][a-z_\-]{2," + str(MAX_TERM_LEN - 1) + r"}$")

PARTICIPLE_SUFFIXES = ("ed", "en", "ing", "ted", "ded", "led", "wn", "own")
GENERIC_BLOCK = {
    "thing", "things", "way", "ways", "time", "times", "day", "days",
    "year", "years", "part", "parts", "case", "cases", "kind", "kinds",
    "sort", "sorts", "value", "values", "class", "classes", "type", "types",
    "example", "examples", "member", "members", "set", "sets",
    "list", "lists", "issue", "issues", "reason", "reasons", "point",
    "form", "state", "process", "system", "content",
    "field", "line", "term", "step",
    "sentence", "word", "sentences", "words", "text",
    "shown", "won", "made", "used", "used_to",
    "one", "two", "three", "many", "several", "some", "any",
    "great", "good", "bad", "big", "small", "large", "new", "old",
    "data", "information", "content", "post", "posts", "article",
    "articles", "story", "stories", "video", "videos", "image", "images",
    "site", "sites", "page", "pages", "user", "users", "member",
    "person", "people", "guy", "guys", "girl", "girls", "man", "men",
    "woman", "women", "friend", "friends", "child", "children",
    "kid", "kids", "family", "families", "customer", "customers",
    "member", "members", "student", "students", "participant",
    "participants", "gev", "ifn", "orthogonal", "symplectic",
}
ADVERB_BLOCK = {
    "really", "currently", "actually", "basically", "usually", "obviously",
    "essentially", "generally", "typically", "still", "always", "never",
    "often", "sometimes", "especially", "particularly", "just", "only",
    "even", "quite", "rather", "definitely", "probably", "possibly",
    "perhaps", "maybe", "indeed", "certainly", "clearly", "simply",
    "primarily", "mainly", "mostly", "largely", "widely", "commonly",
    "recently", "previously", "formerly", "originally", "initially",
    "finally", "ultimately", "eventually", "immediately", "suddenly",
}

def is_participle(w: str) -> bool:
    if len(w) < 4:
        return False
    return any(w.endswith(sfx) for sfx in PARTICIPLE_SUFFIXES)

def clean_term(t: str) -> str | None:
    t = t.strip().lower()
    t = re.sub(r"[\.,;:!\?\)\]\}\"']+$", "", t)
    t = re.sub(r"^[\(\[\{\"']+", "", t)
    if len(t) < MIN_TERM_LEN or len(t) > MAX_TERM_LEN:
        return None
    words = t.split()
    if len(words) > MAX_TERM_WORDS:
        return None
    for w in words:
        if w in STOPWORDS:
            return None
        if w in GENERIC_BLOCK:
            return None
        if w in ADVERB_BLOCK:
            return None
        if not re.match(r"^[a-z][a-z\-']*$", w):
            return None
    # Reject single-word participles ("won", "shown", "changed"). Multi-
    # word phrases containing participles are usually OK ("large brain").
    if len(words) == 1 and is_participle(words[0]):
        return None
    canon = "_".join(words)
    if not TERM_RE.match(canon):
        return None
    return canon


# ------------ pattern rules ------------

# Each pattern is (regex, callback). Callback returns list of (s, rel, o).
PATTERNS = []

def _mk(pat, cb):
    PATTERNS.append((re.compile(pat, re.IGNORECASE), cb))

# "X is a Y" / "X is an Y" — general IS_A
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+is\s+(?:a|an)\s+([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(1), "IS_A", m.group(2))],
)
# "X are Y" — plural IS_A (weaker; only accept when X is capitalizable single word)
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+are\s+([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(1), "IS_A", m.group(2))],
)
# "Y such as X" — Hearst
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+such\s+as\s+([a-z][a-z\- ,]{2,80})\b",
    lambda m: [
        (x.strip(), "IS_A", m.group(1))
        for x in re.split(r"\s*,\s*|\s+and\s+", m.group(2))
        if x.strip()
    ],
)
# "Y including X" — Hearst
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+including\s+([a-z][a-z\- ,]{2,80})\b",
    lambda m: [
        (x.strip(), "IS_A", m.group(1))
        for x in re.split(r"\s*,\s*|\s+and\s+", m.group(2))
        if x.strip()
    ],
)
# "Y like X" — Hearst
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+like\s+([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(2), "IS_A", m.group(1))],
)
# "X has Y"
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+(?:has|have)\s+(?:a\s+|an\s+|the\s+)?([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(1), "HAS", m.group(2))],
)
# "X causes Y"
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+(?:causes?|leads?\s+to)\s+([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(1), "CAUSES", m.group(2))],
)
# "X part of Y"
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+(?:is|are)\s+part\s+of\s+(?:a\s+|an\s+|the\s+)?([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(1), "PART_OF", m.group(2))],
)
# "X implies Y"
_mk(
    r"\b([a-z][a-z\- ]{2,40})\s+implies?\s+([a-z][a-z\- ]{2,40})\b",
    lambda m: [(m.group(1), "IMPLIES", m.group(2))],
)


def split_sentences(text: str):
    text = text.replace("\r", "").replace("\n", " ").replace("\t", " ")
    text = re.sub(r"\s+", " ", text)
    # Naive sentence splitter, avoids abbreviations by requiring 2+ chars.
    return [s.strip() for s in re.split(r"(?<=[\.!\?])\s+", text) if s.strip()]


def extract(text: str, triples: Counter):
    for sent in split_sentences(text):
        if len(sent) < 20 or len(sent) > 400:
            continue
        for pat, cb in PATTERNS:
            for m in pat.finditer(sent):
                for s, rel, o in cb(m):
                    cs, co = clean_term(s), clean_term(o)
                    if not cs or not co or cs == co:
                        continue
                    triples[(cs, rel, co)] += 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input-dir", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--max-docs", type=int, default=20000,
                    help="Max documents to scan across all files.")
    ap.add_argument("--max-triples", type=int, default=5000,
                    help="Max distinct triples to emit (highest-frequency first).")
    ap.add_argument("--min-count", type=int, default=2,
                    help="A triple is kept only if seen at least this many times.")
    args = ap.parse_args()

    root = Path(os.path.expanduser(args.input_dir))
    jsonls = list(root.rglob("*.jsonl"))
    if not jsonls:
        sys.exit(f"no *.jsonl files under {root}")

    print(f"scanning {len(jsonls)} files under {root}", file=sys.stderr)
    triples: Counter = Counter()
    docs = 0
    for p in jsonls:
        try:
            with open(p, "r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    if docs >= args.max_docs:
                        break
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        obj = json.loads(line)
                    except Exception:
                        continue
                    text = obj.get("text", "")
                    if not isinstance(text, str) or len(text) < 100:
                        continue
                    extract(text, triples)
                    docs += 1
                    if docs % 1000 == 0:
                        print(f"  {docs} docs, {len(triples)} triples so far",
                              file=sys.stderr)
        except OSError as e:
            print(f"  skipping {p}: {e}", file=sys.stderr)
        if docs >= args.max_docs:
            break

    kept = [(t, c) for t, c in triples.items() if c >= args.min_count]
    kept.sort(key=lambda tc: -tc[1])
    kept = kept[: args.max_triples]

    outp = Path(args.output).expanduser()
    outp.parent.mkdir(parents=True, exist_ok=True)
    with open(outp, "w", encoding="utf-8") as f:
        for (s, rel, o), c in kept:
            f.write(f"{s}|{rel}|{o}|{c}\n")

    print(f"docs scanned : {docs}", file=sys.stderr)
    print(f"triples raw  : {len(triples)}", file=sys.stderr)
    print(f"triples kept : {len(kept)}", file=sys.stderr)
    print(f"written to   : {outp}", file=sys.stderr)


if __name__ == "__main__":
    main()
