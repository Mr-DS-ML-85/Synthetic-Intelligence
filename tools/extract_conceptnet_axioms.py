#!/usr/bin/env python3
"""
extract_conceptnet_axioms.py — mechanical commonsense ontology extraction.

Reads a ConceptNet 5.7 assertions dump and emits `subject relation object`
triples in the PCMA `.axioms` format. Same contract as
extract_ast_axioms.py: there is no ML, no model, no training, no
inference and no network access at extraction time. Every output row is
a deterministic rewrite of one input row.

WHY THIS IS NOT A LEARNED RESOURCE
-----------------------------------
ConceptNet is a crowdsourced graph of assertions made by named human
contributors, not a model fitted to a corpus. Each edge carries a
`weight` equal (roughly) to the number of independent contributors who
asserted it. We keep only edges at weight >= 2.0, i.e. those asserted
by at least two independent humans.

Every fact produced here enters the substrate with provenance
`asserted`. None is ever counted as `derived`. The JAS induction cycle
must not use these rows as evidence for a universal claim, because
their support is testimonial rather than experimental.

WHAT IS DELIBERATELY EXCLUDED
------------------------------
  RelatedTo (1.70M edges)   Untyped association. This is the one
                            ConceptNet relation that behaves like a
                            distributional embedding: it says two
                            concepts co-occur without saying how. It
                            carries no structure the physics can use
                            and would flood the graph with hubs.
  HasContext (233K)         Domain tagging, not a claim about the world.
  FormOf (379K)             Lexical inflection, not conceptual
                            structure. Routed to --lexicon instead,
                            where it belongs (see cll.md §8, the open
                            morphological-productivity problem).
  EtymologicallyRelatedTo   Historical, not semantic.
  dbpedia/*                 Encyclopedic instance data about named
                            entities; different provenance class.

Usage:
    python3 tools/extract_conceptnet_axioms.py \
        /path/to/conceptnet-assertions-5.7.0.csv.gz \
        -o data/conceptnet.axioms \
        --lexicon data/conceptnet_forms.lexicon
"""

import argparse
import gzip
import os
import sys
from collections import Counter, defaultdict

# ---------------------------------------------------------------- config

# ConceptNet relation -> PCMA relation (src/primitives.hpp).
# `rev` means the triple is emitted with subject and object swapped,
# because ConceptNet's direction is the inverse of ours.
RELMAP = {
    # --- taxonomy ---
    "IsA":              ("is_a",        False),
    "InstanceOf":       ("is_a",        False),
    "MannerOf":         ("is_a",        False),
    "DefinedAs":        ("equals",      False),
    "Synonym":          ("equals",      False),
    "SimilarTo":        ("mirror_of",   False),
    # --- composition ---
    "PartOf":           ("part_of",     False),
    "HasA":             ("has",         False),
    "MadeOf":           ("has",         False),
    "HasProperty":      ("has",         False),
    # --- causal / temporal ---
    "Causes":           ("causes",      False),
    "CausesDesire":     ("causes",      False),
    "Entails":          ("implies",     False),
    # "X HasPrerequisite Y" means Y must happen before X.
    "HasPrerequisite":  ("precedes",    True),
    "HasSubevent":      ("part_of",     True),
    "HasFirstSubevent": ("part_of",     True),
    "HasLastSubevent":  ("part_of",     True),
    # --- derivation ---
    "DerivedFrom":      ("derives_from", False),
    # --- negation ---
    "Antonym":          ("not",         False),
    "DistinctFrom":     ("not",         False),
    "NotDesires":       ("not",         False),
    "NotCapableOf":     ("not",         False),
    "NotHasProperty":   ("not",         False),
    # --- commonsense function (new relations, see primitives.hpp) ---
    "UsedFor":          ("used_for",    False),
    "CapableOf":        ("capable_of",  False),
    "AtLocation":       ("at_location", False),
    "Desires":          ("desires",     False),
    "CreatedBy":        ("created_by",  False),
    "ReceivesAction":   ("capable_of",  False),
    "MotivatedByGoal":  ("used_for",    False),
    "LocatedNear":      ("at_location", False),
}

# Emitted to the lexicon file, not the ontology.
LEXICAL = {"FormOf"}

DEFAULT_MIN_WEIGHT = 2.0
DEFAULT_PER_SUBJECT_CAP = 24
DEFAULT_MAX_TOKENS = 4

# Concepts that would become hubs with no ontological content.
STOP = {
    "thing", "something", "someone", "somebody", "anything", "stuff",
    "person", "people", "place", "time", "way", "one", "other", "many",
    "some", "more", "most", "much", "such", "same", "another", "it",
    "you", "they", "he", "she", "we", "i", "be", "do", "have", "get",
    "make", "go", "take", "come", "know", "see", "want", "use", "a",
    "an", "the", "of", "and", "or", "not", "no", "yes",
}


def parse_concept(uri):
    """/c/en/hot_dog/n/wn/food -> hot_dog ; non-English -> None."""
    parts = uri.split("/")
    # parts = ['', 'c', 'en', 'hot_dog', 'n', ...]
    if len(parts) < 4 or parts[1] != "c" or parts[2] != "en":
        return None
    return parts[3]


def parse_weight(meta):
    """Pull the numeric weight out of the JSON blob without a JSON parse.

    The field is machine-generated with a fixed shape, so a scan is
    both correct and ~8x faster than json.loads over 3.4M rows.
    """
    i = meta.find('"weight":')
    if i < 0:
        return 0.0
    j = i + 9
    while j < len(meta) and meta[j] == " ":
        j += 1
    k = j
    while k < len(meta) and (meta[k].isdigit() or meta[k] in ".-e"):
        k += 1
    try:
        return float(meta[j:k])
    except ValueError:
        return 0.0


def usable(name, max_tokens):
    if not name or name in STOP:
        return False
    if len(name) < 2 or len(name) > 48:
        return False
    if name.count("_") + 1 > max_tokens:
        return False
    # Reject anything that is not plain lowercase ASCII word characters.
    for c in name:
        if not (c.isascii() and (c.isalnum() or c == "_")):
            return False
    # Reject bare numbers.
    if name.replace("_", "").isdigit():
        return False
    return True


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", help="conceptnet-assertions-*.csv.gz")
    ap.add_argument("-o", "--out", default="data/conceptnet.axioms")
    ap.add_argument("--lexicon", default=None,
                    help="write FormOf inflection pairs here")
    ap.add_argument("--min-weight", type=float, default=DEFAULT_MIN_WEIGHT,
                    help="drop edges below this contributor weight")
    ap.add_argument("--cap", type=int, default=DEFAULT_PER_SUBJECT_CAP,
                    help="max edges kept per (subject, relation)")
    ap.add_argument("--max-tokens", type=int, default=DEFAULT_MAX_TOKENS,
                    help="reject concepts with more underscore-joined words")
    args = ap.parse_args()

    kept = []
    forms = []
    seen = set()
    per_subj = Counter()
    stats = Counter()

    op = gzip.open if args.dump.endswith(".gz") else open
    with op(args.dump, "rt", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            stats["rows"] += 1
            col = line.rstrip("\n").split("\t")
            if len(col) < 5:
                continue
            rel = col[1][3:] if col[1].startswith("/r/") else col[1]

            s = parse_concept(col[2])
            o = parse_concept(col[3])
            if s is None or o is None:
                stats["non_english"] += 1
                continue

            if rel in LEXICAL:
                if usable(s, args.max_tokens) and usable(o, args.max_tokens) \
                        and s != o:
                    forms.append((s, o))
                    stats["lexical"] += 1
                continue

            mapped = RELMAP.get(rel)
            if mapped is None:
                stats["unmapped_" + rel] += 1
                continue

            w = parse_weight(col[4])
            if w < args.min_weight:
                stats["low_weight"] += 1
                continue

            if not usable(s, args.max_tokens) or not usable(o, args.max_tokens):
                stats["unusable_concept"] += 1
                continue
            if s == o:
                stats["self_loop"] += 1
                continue

            name, reverse = mapped
            if reverse:
                s, o = o, s

            key = (s, name, o)
            if key in seen:
                stats["duplicate"] += 1
                continue

            ck = (s, name)
            if per_subj[ck] >= args.cap:
                stats["over_cap"] += 1
                continue

            seen.add(key)
            per_subj[ck] += 1
            kept.append((s, name, o, w))
            stats["kept"] += 1
            stats["rel_" + name] += 1

    kept.sort(key=lambda t: (t[1], -t[3], t[0], t[2]))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        f.write("# conceptnet.axioms — commonsense ontology, mechanically\n")
        f.write("# converted from ConceptNet 5.7 by\n")
        f.write("# tools/extract_conceptnet_axioms.py. No ML, no training.\n")
        f.write("#\n")
        f.write("# PROVENANCE: every row is `asserted`. Support is\n")
        f.write("# testimonial (>= %.1f independent human contributors),\n"
                % args.min_weight)
        f.write("# never experimental. JAS induction must not treat these\n")
        f.write("# rows as evidence for a universal claim.\n")
        f.write("#\n")
        f.write("# ConceptNet is CC-BY-SA 4.0. Attribution:\n")
        f.write("#   Speer, Chin, Havasi (2017), ConceptNet 5.5.\n")
        f.write("#\n")
        f.write("# EXCLUDED: RelatedTo (untyped association -- the one\n")
        f.write("# relation that behaves like a distributional embedding),\n")
        f.write("# HasContext, FormOf (-> lexicon), Etymologically*,\n")
        f.write("# dbpedia/*.\n")
        f.write("#\n")
        f.write("# Format: subject   relation   object\n\n")
        cur = None
        for s, r, o, _w in kept:
            if r != cur:
                f.write("\n# --- %s ---\n" % r)
                cur = r
            f.write("%-34s %-14s %s\n" % (s, r, o))

    if args.lexicon and forms:
        forms = sorted(set(forms))
        with open(args.lexicon, "w", encoding="utf-8") as f:
            f.write("# conceptnet_forms.lexicon — inflected form -> stem.\n")
            f.write("# Lexical data for the CLL morphology layer\n")
            f.write("# (research-paper/cll.md §8). NOT ontology: these are\n")
            f.write("# facts about English spelling, not about the world.\n")
            f.write("# Format: form   stem\n\n")
            for a, b in forms:
                f.write("%-30s %s\n" % (a, b))

    print("rows scanned      %d" % stats["rows"])
    print("kept              %d" % stats["kept"])
    print("  dropped: non-english %d, low-weight %d, unmapped %d,"
          % (stats["non_english"], stats["low_weight"],
             sum(v for k, v in stats.items() if k.startswith("unmapped_"))))
    print("           unusable %d, dup %d, self-loop %d, over-cap %d"
          % (stats["unusable_concept"], stats["duplicate"],
             stats["self_loop"], stats["over_cap"]))
    print("lexicon forms     %d" % stats["lexical"])
    print("\nby relation:")
    for k, v in sorted(((k, v) for k, v in stats.items()
                        if k.startswith("rel_")), key=lambda t: -t[1]):
        print("  %-16s %d" % (k[4:], v))
    print("\nwrote %s" % args.out)
    if args.lexicon:
        print("wrote %s" % args.lexicon)


if __name__ == "__main__":
    main()
