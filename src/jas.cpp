// jas — the J-A-S cycle. Einstein's epistemology as a running loop.
//
//                  +---------------------------+
//                  |        Axioms (A)         |
//                  +---------------------------+
//                     /                     \
//                    v                       v
//                +-----+                   +-----+
//                |  S  |                   | S'  |   deduced theorems
//                +-----+                   +-----+
//                   |                         |
//                   | experiment              | experiment
//                   v                         v
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                Sense Experience (E)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//        \                                     /
//         +------------- Jump (J) ------------+
//
// THE POINT OF THE DIAGRAM is that J is NOT LOGICAL. No inference rule
// carries you from observations to axioms. The leap is inductive,
// unjustified, and revisable. Deduction runs downward (A -> S);
// experiment tests S against experience; refutation propagates upward
// to revise the axiom that produced it.
//
// Prior phases built every element except J:
//
//   E   raw source                     extract_ast_axioms.py
//   A   the typed concept graph        data/*.axioms
//   S   forward chaining               100 derived edges
//   exp compile-verify                 synth
//
// What was missing is a mechanism that *jumps* — that forms a
// universal claim not entailed by anything, then stakes it on an
// experiment that can destroy it. That is this binary.
//
// A worked instance, and it is deliberately a claim that is FALSE:
//
//   E : observe that most functions in the graph carry takes_arg edges
//   J : LEAP to "every function takes at least one argument"
//   S : deduce "therefore run() with zero arguments will not compile"
//   X : compile px.run() -> it COMPILES
//   => S is false, therefore A is REFUTED, therefore the jump was wrong
//
// A system that could not be wrong here would not be doing science.
// The refutation is the deliverable.
//
// No loss function, no gradient, no fitted parameter, no policy. The
// oracle is g++. Induction is counting; deduction is graph traversal;
// refutation is an exit code.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "physics.hpp"
#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace unified;

static void autoload_axioms(ExtSI& si) {
    DIR* d = opendir("data");
    if (!d) return;
    struct dirent* ent;
    std::vector<std::string> files;
    while ((ent = readdir(d)) != nullptr) {
        std::string n = ent->d_name;
        if (n.size() < 8) continue;
        if (n.substr(n.size() - 7) != ".axioms") continue;
        if (n == "mutations.axioms") continue;
        files.push_back("data/" + n);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    FILE* dn = std::fopen("/dev/null", "w");
    for (const auto& f : files) load_axiom_file(si, f, dn ? dn : stderr);
    if (dn) std::fclose(dn);
}

// ------------------------------------------------------------ the jump

// An induced axiom. Provenance INDUCED — the weakest warrant in the
// substrate. Its only justification is that no experiment has yet
// destroyed it, and it is the first thing retracted when one does.
struct InducedAxiom {
    std::string statement;      // human-readable universal claim
    std::string relation;       // the relation generalised over
    int    support   = 0;       // observations consistent with it
    int    total     = 0;       // observations examined
    double confidence = 0.0;    // support / total — NOT a probability,
                                // just the observed frequency that
                                // prompted the leap
    bool   refuted   = false;
    std::string counterexample;
};

// Which nodes the substrate treats as functions: those with outgoing
// `calls` edges. Read off the graph, not supplied by us.
static std::set<int> observe_functions(const ExtSI& si) {
    std::set<int> f;
    for (const auto& e : si.graph.edges)
        if (e.rel == REL_CALLS) f.insert(e.from);
    return f;
}
static std::set<int> observe_with_args(const ExtSI& si) {
    std::set<int> a;
    for (const auto& e : si.graph.edges)
        if (e.rel == REL_TAKES_ARG) a.insert(e.from);
    return a;
}

// E -> J. Observe the graph, then LEAP to a universal claim.
//
// The leap is deliberately stronger than the evidence: "most functions
// have arguments" (an observation) becomes "every function has at
// least one argument" (an axiom). That gap is the whole point. An
// induction that only restated the frequency would be a summary, not
// a jump, and could never be refuted.
static InducedAxiom jump_universal_arity(const ExtSI& si) {
    InducedAxiom ax;
    ax.relation  = "takes_arg";
    ax.statement = "every function takes at least one argument";

    // Which nodes look like functions? Those with outgoing `calls`
    // edges — that is sense experience, not a definition supplied by
    // us. The system reads it off the graph.
    std::set<int> functions;
    for (const auto& e : si.graph.edges)
        if (e.rel == REL_CALLS) functions.insert(e.from);

    std::set<int> has_args;
    for (const auto& e : si.graph.edges)
        if (e.rel == REL_TAKES_ARG) has_args.insert(e.from);

    for (int f : functions) {
        ++ax.total;
        if (has_args.count(f)) ++ax.support;
    }
    ax.confidence = ax.total ? (double)ax.support / ax.total : 0.0;
    return ax;
}

// ------------------------------------------------------- deduction: S

// A -> S. A deduced consequence of the induced axiom, expressed as a
// concrete testable prediction about a specific symbol.
struct Theorem {
    std::string target;
    std::string prediction;     // what the axiom entails
    bool        expect_compiles;
};

static Theorem deduce(const InducedAxiom& ax, const std::string& target) {
    Theorem th;
    th.target = target;
    if (ax.relation == "takes_arg") {
        // If every function takes >= 1 argument, then calling this one
        // with zero arguments must fail to compile.
        th.prediction = target + "() with zero arguments will NOT compile";
        th.expect_compiles = false;
    }
    return th;
}

// -------------------------------------------------- experiment: S -> E

static bool compiles_zero_args(const std::string& target) {
    const char* src = "/tmp/pcma_jas_probe.cpp";
    FILE* f = std::fopen(src, "w");
    if (!f) return false;
    std::fprintf(f, "#include \"physics.hpp\"\nusing namespace unified;\n");
    std::fprintf(f, "int main(){ PhysicsEngine px; px.%s(); return 0; }\n",
                 target.c_str());
    std::fclose(f);
    std::string cmd = "g++ -std=c++17 -fsyntax-only -isystem src ";
    cmd += src; cmd += " 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
}

// ------------------------------------- refutation -> revision -> re-jump
//
// This is the part that makes the diagram a CYCLE rather than a single
// disproof. Einstein's arrow from experiment back to Axioms does not
// mean "delete the axiom" — it means REVISE it and go round again.
//
// The revision is itself inductive, and it is constrained by the
// counterexamples: the new axiom must exclude the cases that killed
// the old one, and must still say something. A revision that merely
// excluded every counterexample by name would be unfalsifiable
// book-keeping, not a claim; so the restriction must be a STRUCTURAL
// predicate the graph can evaluate for symbols not yet examined.
//
// The predicate used here: does the graph record any takes_arg edge
// for this symbol? That is observable before any experiment, so the
// revised axiom makes a genuine prediction about unseen targets.
struct RevisedAxiom {
    std::string statement;
    std::string restriction;
    bool  applies(const ExtSI& si, const std::string& target) const {
        auto it = si.graph.by_label.find(target);
        if (it == si.graph.by_label.end()) return false;
        for (const auto& e : si.graph.edges)
            if (e.from == it->second && e.rel == REL_TAKES_ARG) return true;
        return false;
    }
};

static RevisedAxiom revise(const InducedAxiom& dead) {
    RevisedAxiom r;
    r.restriction = "the graph records at least one takes_arg edge";
    r.statement   = "every function FOR WHICH the graph records a "
                    "takes_arg edge takes at least one argument";
    (void)dead;
    return r;
}

// ------------------------------------------ the impossibility register
//
// "X cannot be done" is not one kind of claim. Treating it as one is
// how fields stall for decades, and how they also waste decades chasing
// things that are genuinely closed. The substrate therefore records the
// WARRANT of every impossibility claim, because warrant determines what
// a successful counterexample means.
//
//   ASSERTED  someone said so. One counterexample destroys it.
//             ("heavier-than-air flight is impossible" — Kelvin, 1895)
//
//   INDUCED   no one has managed it yet. One counterexample destroys it,
//             and the claim was never more than a survey of failures.
//
//   DERIVED   follows from a stated axiom. A counterexample does not
//             refute the impossibility directly — it refutes the PARENT
//             AXIOM, and the refutation must propagate upward.
//
//   THEOREM   follows from a conservation law. A counterexample here is
//             almost always a SCOPE ERROR, not a violation: the new
//             result satisfies the theorem by a different route. This is
//             the case that matters most, and the case most often
//             misreported as a breakthrough.
//
// The asymmetry is the whole point. A good impossibility theorem does
// not close a field — it tells you exactly where the bypass must lie.
// Minsky and Papert proved a single-layer perceptron cannot compute
// XOR. That theorem was never violated. Multilayer networks left its
// SCOPE. The proof was the map to the exit.

enum class Warrant { ASSERTED, INDUCED, DERIVED, THEOREM };

struct ImpossibilityClaim {
    std::string statement;
    Warrant     warrant;
    std::string parent;        // axiom (DERIVED) or law (THEOREM)
    std::string bypass;        // how it was escaped, if it was
    bool        escaped = false;
    bool        violated = false;   // THEOREM only: true would be a
                                    // genuine breakage, not a bypass
};

static const char* warrant_name(Warrant w) {
    switch (w) {
        case Warrant::ASSERTED: return "ASSERTED";
        case Warrant::INDUCED:  return "INDUCED";
        case Warrant::DERIVED:  return "DERIVED";
        default:                return "THEOREM";
    }
}

// Three real claims from this project's own history, with what
// happened to each. Self-reference is deliberate: a system that
// records its own refuted impossibilities is doing the thing this
// section describes.
static std::vector<ImpossibilityClaim> project_impossibilities() {
    std::vector<ImpossibilityClaim> v;

    ImpossibilityClaim a;
    a.statement = "a bag of distinct bytes cannot distinguish "
                  "`metformin` from `metfromin`";
    a.warrant   = Warrant::THEOREM;
    a.parent    = "a multiset is invariant under permutation";
    a.escaped   = true;
    a.violated  = false;
    a.bypass    = "bigram lanes — ordered PAIRS are a different "
                  "representation, not a cleverer weighting of the same "
                  "one. The theorem still holds for byte multisets; the "
                  "sensory layer simply stopped being one. 7/7 measured.";
    v.push_back(a);

    ImpossibilityClaim b;
    b.statement = "global viscosity modulation cannot break "
                  "ground-state locking";
    b.warrant   = Warrant::THEOREM;
    b.parent    = "uniform scaling preserves all amplitude ratios, and "
                  "arg-max is invariant under monotone transforms";
    b.escaped   = true;
    b.violated  = false;
    b.bypass    = "per-cavity coupling through tanh(T_i/T_crit). The "
                  "proof of rank-preservation is untouched and still "
                  "correct — CSAT left its scope by ceasing to be "
                  "global. The theorem pointed straight at the fix.";
    v.push_back(b);

    ImpossibilityClaim c;
    c.statement = "a cognitive architecture can eliminate the log2(N) "
                  "translation cost";
    c.warrant   = Warrant::THEOREM;
    c.parent    = "Translation Capacity Lemma (source coding)";
    c.escaped   = false;
    c.violated  = false;
    c.bypass    = "NOT escaped, and this is the honest case. MRS "
                  "relocates the bits from a label table into a coupling "
                  "matrix, which changes their ORGANISATION (atomic -> "
                  "factored) and therefore what generalises. The bit "
                  "count is unchanged. Any claim to have removed the "
                  "cost would be a scope error dressed as a result.";
    v.push_back(c);

    ImpossibilityClaim d;
    d.statement = "every function takes at least one argument";
    d.warrant   = Warrant::INDUCED;
    d.parent    = "55% observed frequency";
    d.escaped   = true;
    d.violated  = true;      // induced claims CAN simply be false
    d.bypass    = "tick() and reset_field() compile with zero arguments. "
                  "An induced universal needs no scope analysis — one "
                  "counterexample ends it.";
    v.push_back(d);

    return v;
}

static void report_impossibilities() {
    std::printf("\n================ IMPOSSIBILITY REGISTER ================\n");
    std::printf("Does a flaw reject the ontology? It depends entirely on\n");
    std::printf("the WARRANT of the claim that was flawed.\n\n");

    for (const auto& c : project_impossibilities()) {
        std::printf("  [%s] %s\n", warrant_name(c.warrant), c.statement.c_str());
        std::printf("      rests on : %s\n", c.parent.c_str());
        if (!c.escaped) {
            std::printf("      status   : STANDS — not escaped\n");
        } else if (c.warrant == Warrant::THEOREM && !c.violated) {
            std::printf("      status   : SCOPE ESCAPE (theorem intact)\n");
        } else {
            std::printf("      status   : REFUTED — claim was simply false\n");
        }
        std::printf("      %s\n\n", c.bypass.c_str());
    }

    std::printf("  RULE THE SUBSTRATE APPLIES\n");
    std::printf("  --------------------------\n");
    std::printf("  ASSERTED / INDUCED : one counterexample retracts the\n");
    std::printf("      claim outright. Nothing else is disturbed.\n\n");
    std::printf("  DERIVED : the counterexample does NOT touch the claim.\n");
    std::printf("      It refutes the PARENT AXIOM, and the retraction\n");
    std::printf("      must propagate upward to everything else that\n");
    std::printf("      axiom supports.\n\n");
    std::printf("  THEOREM : a counterexample is a SCOPE ERROR until\n");
    std::printf("      proven otherwise. Before celebrating, the system\n");
    std::printf("      must answer: did the result violate the law, or\n");
    std::printf("      leave the domain the law quantifies over?\n");
    std::printf("      Two of the three theorem-claims above were escaped\n");
    std::printf("      by leaving scope. NONE was violated.\n\n");
    std::printf("  A theorem that is genuinely violated means the AXIOMS\n");
    std::printf("  behind it are wrong — which is the largest event that\n");
    std::printf("  can occur in this architecture, and has not occurred.\n");
}

// ------------------------------------------------------------------ main

int main(int argc, char** argv) {
    ExtSI si;
    build_big_ontology(si);
    autoload_axioms(si);
    si.reason_ext();

    std::vector<std::string> targets;
    for (int i = 1; i < argc; ++i) targets.push_back(argv[i]);
    if (targets.empty()) targets = {"tick", "run", "reset_field", "drive"};

    std::printf("jas — the J-A-S cycle (Einstein's epistemology)\n");
    std::printf("graph: %d symbols, %d relations\n\n",
                (int)si.graph.nodes.size(), (int)si.graph.edges.size());

    // ---- E: sense experience ------------------------------------
    std::printf("[E] sense experience\n");
    std::printf("    reading structure off %d relations extracted from source\n\n",
                (int)si.graph.edges.size());

    // ---- J: the jump --------------------------------------------
    InducedAxiom ax = jump_universal_arity(si);
    std::printf("[J] the jump — inductive, not logical\n");
    std::printf("    observed : %d of %d functions carry takes_arg  (%.1f%%)\n",
                ax.support, ax.total, 100.0 * ax.confidence);
    std::printf("    LEAP TO  : \"%s\"\n", ax.statement.c_str());
    std::printf("    note     : the axiom is STRONGER than the evidence.\n");
    std::printf("               that gap is what makes it refutable.\n\n");

    // ---- A -> S: deduction --------------------------------------
    std::printf("[A->S] deduction — logical, downward\n");
    std::vector<Theorem> theorems;
    for (const auto& t : targets) {
        Theorem th = deduce(ax, t);
        theorems.push_back(th);
        std::printf("    S : %s\n", th.prediction.c_str());
    }
    std::printf("\n");

    // ---- experiment: S back against E ---------------------------
    std::printf("[experiment] testing each theorem against reality\n");
    int confirmed = 0, refuted = 0;
    for (const auto& th : theorems) {
        bool actually_compiles = compiles_zero_args(th.target);
        bool holds = (actually_compiles == th.expect_compiles);
        std::printf("    %-14s predicted %-11s observed %-11s -> %s\n",
                    th.target.c_str(),
                    th.expect_compiles ? "compiles" : "fails",
                    actually_compiles ? "compiles" : "fails",
                    holds ? "CONSISTENT" : "REFUTES AXIOM");
        if (holds) ++confirmed;
        else {
            ++refuted;
            if (ax.counterexample.empty()) ax.counterexample = th.target;
        }
    }
    std::printf("\n");

    // ---- refutation propagates upward ---------------------------
    std::printf("[refutation] upward revision\n");
    if (refuted > 0) {
        ax.refuted = true;
        std::printf("    the axiom is FALSE.\n");
        std::printf("    counterexample: %s() compiles with zero arguments,\n",
                    ax.counterexample.c_str());
        std::printf("    so \"%s\"\n    cannot hold.\n", ax.statement.c_str());
        std::printf("    RETRACTED. The jump was wrong, and an experiment\n");
        std::printf("    — not an internal consistency check — destroyed it.\n");
        // Record the refutation. A failed induction is knowledge too:
        // it is a region of hypothesis space now known to be empty.
        FILE* vf = std::fopen("data/refuted.axioms", "a");
        if (vf) {
            std::fprintf(vf, "# REFUTED by experiment: %s\n",
                         ax.statement.c_str());
            std::fprintf(vf, "# counterexample: %s takes zero arguments\n",
                         ax.counterexample.c_str());
            std::fclose(vf);
            std::printf("    -> logged to data/refuted.axioms\n");
        }
    } else {
        std::printf("    axiom survives this round (%d/%d consistent).\n",
                    confirmed, (int)theorems.size());
        std::printf("    NOT proven — merely not yet destroyed. Induction\n");
        std::printf("    confers no warrant beyond survival.\n");
    }

    // ---- round 2: re-jump on a revised axiom --------------------
    if (refuted > 0) {
        RevisedAxiom rev = revise(ax);
        std::printf("\n[J2] re-jump — the cycle turns\n");
        std::printf("    revised  : \"%s\"\n", rev.statement.c_str());
        std::printf("    restricted by: %s\n", rev.restriction.c_str());
        std::printf("    this excludes the counterexamples WITHOUT naming\n");
        std::printf("    them, so it still predicts on unseen symbols.\n\n");

        // Test on symbols NOT used in round 1 — otherwise the revision
        // is fitted to its own counterexamples and proves nothing.
        std::vector<std::string> holdout = {
            "bind_byte", "fission", "drive_byte", "set_neurotransmitters",
            "bind_syntax_slots", "renormalize_energy"
        };
        std::printf("[experiment 2] held-out symbols, none seen in round 1\n");
        int c2 = 0, r2 = 0, skipped = 0;
        for (const auto& t : holdout) {
            if (!rev.applies(si, t)) {
                std::printf("    %-22s outside the restriction — no prediction\n",
                            t.c_str());
                ++skipped;
                continue;
            }
            bool compiles0 = compiles_zero_args(t);
            // Axiom predicts: takes >=1 arg, so zero-arg call must fail.
            bool holds = !compiles0;
            std::printf("    %-22s predicted fails   observed %-9s -> %s\n",
                        t.c_str(), compiles0 ? "compiles" : "fails",
                        holds ? "CONSISTENT" : "REFUTES");
            if (holds) ++c2; else ++r2;
        }
        std::printf("\n[round 2 verdict]\n");
        if (r2 == 0 && c2 > 0) {
            std::printf("    revised axiom SURVIVES on %d held-out symbols\n", c2);
            std::printf("    (%d fell outside its scope and were not predicted)\n",
                        skipped);
            std::printf("    Still not proven — survival is not warrant.\n");
            std::printf("    But the cycle CONVERGED: a false universal became\n");
            std::printf("    a restricted claim that reality has not destroyed.\n");
            FILE* vf = std::fopen("data/verified.axioms", "a");
            if (vf) {
                std::fprintf(vf, "# INDUCED, survived %d held-out experiments:\n",
                             c2);
                std::fprintf(vf, "# %s\n", rev.statement.c_str());
                std::fclose(vf);
                std::printf("    -> logged to data/verified.axioms (provenance: induced)\n");
            }
        } else if (r2 > 0) {
            std::printf("    revised axiom ALSO refuted (%d counterexamples).\n", r2);
            std::printf("    The cycle would turn again. Convergence is not\n");
            std::printf("    guaranteed — that is a property of induction, not\n");
            std::printf("    a defect of the implementation.\n");
        } else {
            std::printf("    no held-out symbol fell inside the restriction;\n");
            std::printf("    the revision is untested, not confirmed.\n");
        }
    }

    report_impossibilities();

    std::printf("\ncycle: E -> J -> A -> S -> experiment -> E -> J2 -> ...\n");
    std::printf("A system that cannot be refuted here is not doing science.\n");
    return 0;
}
