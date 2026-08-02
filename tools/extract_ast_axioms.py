#!/usr/bin/env python3
"""
extract_ast_axioms.py — mechanical code-structure ontology extraction.

Walks a source tree and emits `subject relation object` triples in the
PCMA `.axioms` format. There is no ML, no model, no training and no
network access: relations are read directly off the syntactic
structure of the source.

Relations emitted (see src/primitives.hpp):

    calls        function X calls function Y
    returns      function X returns type Y
    takes_arg    function X takes_arg type/param Y
    precedes     call X must precede call Y inside the same body
    imports      file/module X imports Y
    is_a         class X is_a base Y
    has          struct/class X has field Y

`precedes` is the important one: it encodes dataflow order, which the
CPSB gauge turns into a physical phase gradient so the phase-collapse
sweep emits statements in dependency order.

Usage:
    python3 tools/extract_ast_axioms.py src/ -o data/code_pcma.axioms
    python3 tools/extract_ast_axioms.py . --lang py -o data/code_py.axioms
"""

import argparse
import ast
import os
import re
import sys
from collections import Counter, defaultdict

# ---------------------------------------------------------------- config

# Identifiers that carry no ontological content — control keywords,
# primitive types, and ubiquitous library calls that would become hubs.
STOP = {
    # C/C++ keywords and control flow
    "if", "else", "for", "while", "do", "switch", "case", "break",
    "continue", "return", "goto", "sizeof", "typedef", "struct", "class",
    "union", "enum", "namespace", "using", "template", "typename",
    "public", "private", "protected", "virtual", "static", "const",
    "constexpr", "inline", "explicit", "operator", "new", "delete",
    "try", "catch", "throw", "nullptr", "true", "false", "this",
    # primitive types
    "void", "int", "float", "double", "char", "bool", "long", "short",
    "unsigned", "signed", "size_t", "auto", "string", "vector",
    # python keywords / builtins that add nothing
    "def", "lambda", "pass", "raise", "with", "as", "in", "not", "and",
    "or", "is", "None", "True", "False", "self", "cls", "print", "len",
    "range", "str", "list", "dict", "set", "tuple", "type", "isinstance",
    "super", "format", "append", "join", "open",
}

MIN_IDENT_LEN = 3
MAX_IDENT_LEN = 40
DEFAULT_PER_SUBJECT_CAP = 24
DEFAULT_MIN_FREQ = 1

IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def norm(name):
    """Normalise an identifier to a graph label (lowercase, no sigils)."""
    n = name.strip().lstrip("*&").rstrip("*&")
    n = n.split("::")[-1].split(".")[-1]
    n = n.strip("_")
    return n.lower()


def usable(name):
    if not name:
        return False
    if len(name) < MIN_IDENT_LEN or len(name) > MAX_IDENT_LEN:
        return False
    if name in STOP:
        return False
    if name.isdigit():
        return False
    return True


# ---------------------------------------------------------------- python

def _py_calls_in_stmt(stmt):
    """Calls appearing directly in one statement's own expressions.

    Nested block bodies are NOT descended into — those are separate
    scopes handled by _py_block, so that a call inside an `if` branch
    never receives a `precedes` edge relative to a call in the `else`
    branch. The two are mutually exclusive at runtime and ordering
    between them is not a fact about the program.
    """
    out = []
    block_fields = ("body", "orelse", "finalbody", "handlers")
    for node in ast.walk(stmt):
        # Skip descending into nested block bodies.
        if node is not stmt and isinstance(
            node, (ast.If, ast.For, ast.While, ast.With, ast.Try,
                   ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)
        ):
            continue
        if isinstance(node, ast.Call):
            tgt = None
            if isinstance(node.func, ast.Name):
                tgt = norm(node.func.id)
            elif isinstance(node.func, ast.Attribute):
                tgt = norm(node.func.attr)
            if tgt and usable(tgt):
                out.append(tgt)
    del block_fields
    return out


def _py_block(body, fn, triples):
    """Walk one statement list in order.

    `precedes` edges are emitted only between calls in the SAME
    statement list — i.e. genuinely sequential execution. Nested
    blocks recurse with their own independent ordering.
    """
    seq = []
    for stmt in body:
        # mutates: assignment to an attribute or augmented assignment
        if isinstance(stmt, ast.Assign):
            for t in stmt.targets:
                if isinstance(t, ast.Attribute):
                    f = norm(t.attr)
                    if usable(f) and usable(fn):
                        triples.append((fn, "mutates", f))
        elif isinstance(stmt, ast.AugAssign):
            t = stmt.target
            name = None
            if isinstance(t, ast.Attribute):
                name = norm(t.attr)
            elif isinstance(t, ast.Name):
                name = norm(t.id)
            if name and usable(name) and usable(fn):
                triples.append((fn, "mutates", name))

        calls = [c for c in _py_calls_in_stmt(stmt) if c != fn]
        for c in calls:
            triples.append((fn, "calls", c))
        seq.extend(calls)

        # Recurse into nested blocks as independent scopes.
        for field in ("body", "orelse", "finalbody"):
            sub = getattr(stmt, field, None)
            if isinstance(sub, list) and sub and isinstance(sub[0], ast.stmt):
                _py_block(sub, fn, triples)
        for h in getattr(stmt, "handlers", []) or []:
            if getattr(h, "body", None):
                _py_block(h.body, fn, triples)

    for a, b in zip(seq, seq[1:]):
        if a != b:
            triples.append((a, "precedes", b))


def extract_python(path, triples):
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            tree = ast.parse(f.read(), filename=path)
    except (SyntaxError, ValueError):
        return

    module = norm(os.path.splitext(os.path.basename(path))[0])

    for node in ast.walk(tree):
        # imports
        if isinstance(node, ast.Import):
            for a in node.names:
                m = norm(a.name)
                if usable(module) and usable(m):
                    triples.append((module, "imports", m))
        elif isinstance(node, ast.ImportFrom):
            if node.module:
                m = norm(node.module)
                if usable(module) and usable(m):
                    triples.append((module, "imports", m))

        # class is_a base, class has method
        elif isinstance(node, ast.ClassDef):
            cls = norm(node.name)
            for b in node.bases:
                if isinstance(b, ast.Name):
                    base = norm(b.id)
                    if usable(cls) and usable(base):
                        triples.append((cls, "is_a", base))
            for item in node.body:
                if isinstance(item, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    meth = norm(item.name)
                    if usable(cls) and usable(meth):
                        triples.append((cls, "has", meth))

        # function: takes_arg, calls, precedes, mutates
        elif isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            fn = norm(node.name)
            if not usable(fn):
                continue
            for i, a in enumerate(node.args.args):
                p = norm(a.arg)
                if not usable(p):
                    continue
                triples.append((fn, "takes_arg", p))
                # Ordinal slot. Position is a property of the
                # (function, index) PAIR, not of the parameter name —
                # `node_id` is arg0 of one function and arg2 of
                # another. A reified slot node carries it:
                #     drive__arg0  is_a  node_id
                # Sorting slots by index reconstructs the signature,
                # which plain `takes_arg` edges cannot do because the
                # axiom format has no ordering guarantee.
                triples.append(("%s__arg%d" % (fn, i), "is_a", p))
            _py_block(node.body, fn, triples)


# ------------------------------------------------------------------ c/c++

# Matches a function definition header: `type name(args) {`
CPP_FUNC_RE = re.compile(
    r"^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+)*"
    r"([A-Za-z_][A-Za-z0-9_:<>,\s\*&]*?)\s+"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*"
    r"\(([^;{]*)\)\s*(?:const\s*)?(?:noexcept\s*)?\{",
    re.MULTILINE,
)
CPP_INCLUDE_RE = re.compile(r'^\s*#include\s+[<"]([^>"]+)[>"]', re.MULTILINE)
CPP_STRUCT_RE = re.compile(
    r"\b(?:struct|class)\s+([A-Za-z_][A-Za-z0-9_]*)\s*"
    r"(?::\s*(?:public|private|protected)?\s*([A-Za-z_][A-Za-z0-9_:]*))?\s*\{",
)
CPP_CALL_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def _matching_brace(text, open_idx):
    """Return index just past the brace matching text[open_idx] == '{'."""
    depth = 0
    for i in range(open_idx, len(text)):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
    return len(text)


def _strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return text


CPP_ASSIGN_RE = re.compile(
    r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:\.|->)\s*([A-Za-z_][A-Za-z0-9_]*)"
    r"\s*(?:=[^=]|\+=|-=|\*=|/=)"
)


def _cpp_split_top_level(body):
    """Split a brace-delimited body into (top_level_text, [nested_blocks]).

    Statements at brace depth 1 execute sequentially; statements inside
    a nested `{ }` belong to a branch or loop and must not receive
    `precedes` edges relative to their siblings in another branch.
    """
    top = []
    nested = []
    depth = 0
    start_nested = 0
    for i, c in enumerate(body):
        if c == "{":
            depth += 1
            if depth == 2:
                start_nested = i
        elif c == "}":
            if depth == 2:
                nested.append(body[start_nested:i + 1])
            depth -= 1
        elif depth == 1:
            top.append(c)
    return "".join(top), nested


def _cpp_block(body, fn, triples, depth=0):
    """Emit calls/precedes/mutates for one C++ scope.

    Ordering edges are emitted only between calls at the same brace
    depth. Nested scopes recurse independently, so calls in different
    branches of an `if` are never claimed to be ordered.
    """
    if depth > 6:
        return
    top, nested = _cpp_split_top_level(body)

    # mutates: `obj.field = ...` / `obj->field = ...`
    for am in CPP_ASSIGN_RE.finditer(top):
        field = norm(am.group(2))
        if usable(field) and usable(fn):
            triples.append((fn, "mutates", field))

    seq = []
    for cm in CPP_CALL_RE.finditer(top):
        tgt = norm(cm.group(1))
        if usable(tgt) and tgt != fn:
            triples.append((fn, "calls", tgt))
            seq.append(tgt)
    for a, b in zip(seq, seq[1:]):
        if a != b:
            triples.append((a, "precedes", b))

    for block in nested:
        _cpp_block(block, fn, triples, depth + 1)


def extract_cpp(path, triples):
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            raw = f.read()
    except OSError:
        return
    text = _strip_comments(raw)
    unit = norm(os.path.splitext(os.path.basename(path))[0])

    # includes
    for m in CPP_INCLUDE_RE.finditer(text):
        inc = norm(os.path.splitext(os.path.basename(m.group(1)))[0])
        if usable(unit) and usable(inc):
            triples.append((unit, "imports", inc))

    # struct/class: is_a base, has field
    for m in CPP_STRUCT_RE.finditer(text):
        cls = norm(m.group(1))
        base = norm(m.group(2)) if m.group(2) else None
        if not usable(cls):
            continue
        if base and usable(base):
            triples.append((cls, "is_a", base))
        end = _matching_brace(text, m.end() - 1)
        body = text[m.end():end]
        # field declarations: `type name ...;`  (skip method decls)
        for line in body.split(";"):
            line = line.strip()
            if not line or "(" in line:
                continue
            idents = IDENT_RE.findall(line)
            if len(idents) >= 2:
                field = norm(idents[-1])
                if usable(field):
                    triples.append((cls, "has", field))

    # functions: returns, takes_arg, calls, precedes
    for m in CPP_FUNC_RE.finditer(text):
        ret_raw, fname, args_raw = m.group(1), m.group(2), m.group(3)
        fn = norm(fname)
        if not usable(fn):
            continue

        ret_ids = IDENT_RE.findall(ret_raw)
        if ret_ids:
            rt = norm(ret_ids[-1])
            if usable(rt):
                triples.append((fn, "returns", rt))

        for i, arg in enumerate(args_raw.split(",")):
            ids = IDENT_RE.findall(arg)
            if len(ids) >= 2:
                p = norm(ids[-1])
                if usable(p):
                    triples.append((fn, "takes_arg", p))
                    triples.append(("%s__arg%d" % (fn, i), "is_a", p))

        brace = text.find("{", m.end() - 1)
        end = _matching_brace(text, brace)
        body = text[brace:end]

        _cpp_block(body, fn, triples)


# ------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("root", help="source directory to walk")
    ap.add_argument("-o", "--out", default="data/code_extracted.axioms")
    ap.add_argument("--lang", choices=["auto", "py", "cpp"], default="auto")
    ap.add_argument("--cap", type=int, default=DEFAULT_PER_SUBJECT_CAP,
                    help="max triples per (subject, relation) pair")
    ap.add_argument("--min-freq", type=int, default=DEFAULT_MIN_FREQ,
                    help="drop triples seen fewer than this many times")
    args = ap.parse_args()

    py_ext = {".py"}
    cpp_ext = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hh", ".cu"}

    triples = []
    n_files = 0
    for dirpath, dirnames, filenames in os.walk(args.root):
        dirnames[:] = [d for d in dirnames
                       if not d.startswith(".") and d not in
                       ("build", "node_modules", "__pycache__", "deprected")]
        for fn in filenames:
            ext = os.path.splitext(fn)[1].lower()
            path = os.path.join(dirpath, fn)
            if args.lang in ("auto", "py") and ext in py_ext:
                extract_python(path, triples)
                n_files += 1
            elif args.lang in ("auto", "cpp") and ext in cpp_ext:
                extract_cpp(path, triples)
                n_files += 1

    # frequency filter
    counts = Counter(triples)
    kept = [t for t, c in counts.items() if c >= args.min_freq]

    # per-(subject, relation) cap, most frequent first — stops hub blowup
    by_key = defaultdict(list)
    for t in kept:
        by_key[(t[0], t[1])].append(t)
    # Ordinal slot nodes are unique per (function, index) and must
    # never be dropped by the frequency cap — each has exactly one
    # edge and carries irreplaceable positional information.
    final = []
    for key, group in by_key.items():
        group.sort(key=lambda t: -counts[t])
        final.extend(group[: args.cap])

    # self-loops are meaningless in the substrate
    final = [t for t in final if t[0] != t[2]]
    final.sort()

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        f.write("# Mechanically extracted code-structure ontology.\n")
        f.write("# Source: %s   files: %d\n" % (args.root, n_files))
        f.write("# Generated by tools/extract_ast_axioms.py — no ML, no network.\n")
        for s, r, o in final:
            f.write("%-32s %-12s %s\n" % (s, r, o))

    rel_counts = Counter(r for _, r, _ in final)
    print("files scanned : %d" % n_files)
    print("raw triples   : %d" % len(triples))
    print("after filters : %d" % len(final))
    print("by relation   :")
    for r, c in rel_counts.most_common():
        print("    %-12s %d" % (r, c))
    print("written to    : %s" % args.out)


if __name__ == "__main__":
    main()
