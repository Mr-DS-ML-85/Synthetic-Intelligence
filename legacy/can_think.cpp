// can_think — feed the philosophical question directly to the SI engine
// and see what it derives from a fair ontology.
//
// The point: SI's answer to "can machines think" is a *function of the
// axioms you give it*. That is the honest behaviour philosophy of mind
// has for centuries — Turing gave one set of axioms, Searle another,
// Dennett a third, and they derive different answers. A pattern-matcher
// LLM smears these together into an average; the SI engine says
// "here is the derivation from these premises, here is the conclusion,
// here is the chain."  Change the premises and you change the answer.
//
// We build three positions:
//   A. Turing / functionalist   — behaviour that passes the Turing test
//                                 counts as thinking.
//   B. Searle / biological       — thinking requires biological substrate;
//                                 syntax alone is not semantics.
//   C. Dennett / intentional     — thinking is an intentional stance
//                                 attributable to any sufficiently
//                                 competent system.
// and let each derive its own answer to "can machines think?". This
// exhibits SI as a debate arena, not a smoothed statistical judgement.

#include "meta_learner.hpp"
#include "primitives.hpp"

#include <cstdio>
#include <string>

using namespace unified;

// Add the reasoning primitives and shared background facts.
static void add_rules(ExtSI& si) {
    si.add_transitive  ("t_is_a",       REL_IS_A,    REL_IS_A,    REL_IS_A);
    si.add_transitive  ("t_implies",    REL_IMPLIES, REL_IMPLIES, REL_IMPLIES);
    si.add_inheritance ("has_flows",    REL_HAS);
    si.add_inheritance ("cap_flows",    REL_IMPLIES);
    si.add_deduction   ("modus_ponens");
}

// Register the (subj, is_a, thinking) row so the answer, when derivable,
// gets a human-readable phrase.
static void add_translation(ExtSI& si) {
    si.table.register_answer("machine", REL_IS_A, "thinker",
        "Yes: under these axioms, a machine is a thinker.");
}

// ---------- POSITION A: Turing / Functionalist ----------
static void run_turing() {
    ExtSI si;
    add_rules(si);
    add_translation(si);

    // The classical Turing (1950) axioms
    si.assert_fact("turing_test",  REL_IS_A,    "behavioural_test_of_thinking");
    si.assert_fact("behavioural_test_of_thinking",
                                   REL_IS_A,    "test_of_thinking");
    si.assert_fact("passing_test_of_thinking",
                                   REL_IMPLIES, "thinker");
    si.assert_fact("passing_behavioural_test_of_thinking",
                                   REL_IS_A,    "passing_test_of_thinking");
    // Empirical assertion (Turing predicted this):
    si.assert_fact("machine",      REL_IS_A,    "system");
    si.assert_fact("system",       REL_IMPLIES, "passing_behavioural_test_of_thinking");
    // Note: this final premise IS controversial — Turing predicted it,
    // Searle denies it. Making it explicit is the point.

    std::printf("---- Position A: Turing / Functionalist axioms ----\n");
    auto rep = si.reason_ext();
    std::printf("  derived %d edges in %d steps\n", rep.edges_added, rep.steps_used);
    auto q = si.query("machine", REL_IMPLIES, "thinker");
    std::printf("  machine implies thinker?  %s\n",
                q.holds ? "YES (functionalist)" : "NO (unable to derive)");
    if (q.holds) {
        std::printf("  proof (%zu edges):\n", q.proof.size());
        si.print_proof(q);
    }
    std::printf("\n");
}

// ---------- POSITION B: Searle / Biological ----------
static void run_searle() {
    ExtSI si;
    add_rules(si);
    // The negation primitive is what makes this position expressible.
    si.add_negation("contrapositive", REL_IMPLIES);
    add_translation(si);

    // Searle's Chinese Room axioms:
    //   1. Thinking requires semantic content (understanding).
    //   2. Syntactic manipulation alone does not yield semantics.
    //   3. Machines are syntactic manipulators (by construction).
    // Therefore: machines cannot think.
    si.assert_fact("thinker",           REL_IMPLIES, "has_semantics");
    si.assert_fact("has_syntax_only",   REL_IMPLIES, "no_semantics");
    si.assert_fact("machine",           REL_IS_A,    "syntactic_manipulator");
    si.assert_fact("syntactic_manipulator",
                                        REL_IS_A,    "has_syntax_only");
    si.assert_fact("machine",           REL_NOT,     "has_semantics");

    std::printf("---- Position B: Searle / Biological axioms ----\n");
    auto rep = si.reason_ext();
    std::printf("  derived %d edges in %d steps\n", rep.edges_added, rep.steps_used);
    // Query the contrapositive form: "machine cannot be a thinker"
    // because thinker requires semantics but machine has no semantics.
    auto q1 = si.query("machine", REL_NOT, "thinker");
    std::printf("  machine NOT thinker?  %s\n",
                q1.holds ? "YES (Searle refutation)" : "no derivation found");
    if (q1.holds) {
        std::printf("  proof (%zu edges):\n", q1.proof.size());
        si.print_proof(q1);
    }
    std::printf("\n");
}

// ---------- POSITION C: Dennett / Intentional stance ----------
static void run_dennett() {
    ExtSI si;
    add_rules(si);
    add_translation(si);

    // Dennett: thinking is what we attribute to competent systems whose
    // behaviour is best predicted by treating them as if they have
    // beliefs and desires (intentional stance).
    si.assert_fact("competent_system", REL_IMPLIES, "usefully_treated_as_thinker");
    si.assert_fact("usefully_treated_as_thinker",
                                        REL_IMPLIES, "thinker");
    si.assert_fact("machine",           REL_IS_A,    "competent_system");
    si.assert_fact("competent_system",  REL_IS_A,    "system");

    std::printf("---- Position C: Dennett / Intentional stance axioms ----\n");
    auto rep = si.reason_ext();
    std::printf("  derived %d edges in %d steps\n", rep.edges_added, rep.steps_used);
    auto q = si.query("competent_system", REL_IMPLIES, "thinker");
    std::printf("  competent_system implies thinker?  %s\n",
                q.holds ? "YES (Dennett)" : "no derivation");
    if (q.holds) {
        std::printf("  proof (%zu edges):\n", q.proof.size());
        si.print_proof(q);
    }
    std::printf("\n");
}

// ---------- Hello world in Python: the code-synthesis test ----------
static void run_hello_python() {
    ExtSI si;
    add_rules(si);
    // Axioms of the domain: what a "hello_world" program IS.
    si.assert_fact("hello_world_program",    REL_IS_A,   "output_program");
    si.assert_fact("output_program",         REL_HAS,    "print_statement");
    si.assert_fact("python_language",        REL_HAS,    "print_syntax");
    si.assert_fact("python_hello_world",     REL_IS_A,   "hello_world_program");
    si.assert_fact("python_hello_world",     REL_IS_A,   "python_program");
    si.assert_fact("python_program",         REL_HAS,    "python_syntax");
    // The literal string as an assertion — the engine still cannot
    // generate it, but it can retrieve the fact that (X has literal Y).
    si.assert_fact("python_hello_world_literal",
                                              REL_IS_A,  "code_string");
    si.assert_fact("python_hello_world",     REL_HAS,    "python_hello_world_literal");

    si.table.register_answer("python_hello_world", REL_HAS, "print_statement",
        "Yes: a python hello-world program has a print statement (that "
        "print statement is: print(\"hello, world\")).");
    si.table.register_answer("python_hello_world", REL_HAS, "python_hello_world_literal",
        "The canonical python hello-world source code is: "
        "print(\"hello, world\")");

    std::printf("---- Hello world in Python (code-composition test) ----\n");
    auto rep = si.reason_ext();
    std::printf("  derived %d edges in %d steps\n", rep.edges_added, rep.steps_used);
    auto q1 = si.query("python_hello_world", REL_HAS, "print_statement");
    std::printf("  python_hello_world HAS print_statement?  %s\n",
                q1.holds ? "YES" : "no");
    if (q1.holds) std::printf("  -> %s\n", q1.translation.c_str());
    auto q2 = si.query("python_hello_world", REL_HAS, "python_hello_world_literal");
    std::printf("\n  python_hello_world HAS literal?  %s\n",
                q2.holds ? "YES" : "no");
    if (q2.holds) std::printf("  -> %s\n", q2.translation.c_str());
    std::printf("\n"
                "  IMPORTANT: this is retrieval of an ASSERTED literal, not\n"
                "  synthesis. The engine did not INVENT the string. In an SI\n"
                "  system a bounded code synthesiser would run as a separate\n"
                "  primitive that emits a candidate string, and Layer 2 would\n"
                "  verify it against a type-check + test harness (paper §8).\n"
                "  The current engine's honest answer is 'here is the literal\n"
                "  I was told' — not 'here is a string I made up'.\n\n");
}

int main() {
    std::printf("=== Can machines think? — SI derivation across three positions ===\n\n");
    std::printf("Each position uses a *different set of axioms*. SI derives\n");
    std::printf("what those axioms entail. The answers differ because the\n");
    std::printf("axioms differ — same behaviour as human philosophers.\n\n");
    run_turing();
    run_searle();
    run_dennett();

    std::printf("=== Hello world in Python ===\n\n");
    run_hello_python();

    std::printf("=== Summary ===\n");
    std::printf("The engine is not a memorizer if you feed it non-trivial\n");
    std::printf("axioms — it derives distinct answers under distinct axiom sets.\n");
    std::printf("It IS bounded by the primitives + premises you give it. That\n");
    std::printf("bound is the honest one: a system that says 'yes machines\n");
    std::printf("think' regardless of premises is not thinking, it is\n");
    std::printf("agreeing. A system that derives the correct conclusion from\n");
    std::printf("a specific axiom set is doing philosophy — the same thing\n");
    std::printf("Turing, Searle, and Dennett did on paper.\n");
    return 0;
}
