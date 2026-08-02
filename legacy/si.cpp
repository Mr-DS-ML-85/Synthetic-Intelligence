// si — CLI for the full Synthetic Intelligence tri-layer engine.
//
//   ./si syllogism      forward-chaining derivation demo
//   ./si backward       backward-chaining goal-directed demo
//   ./si thinker        measure the thinker index C/R with rule induction
//                       ON vs OFF; shows the paper's §7.6 prediction that
//                       C/R grows monotonically over queries when induction
//                       is on.
//   ./si verify         demonstrate the verifier catching a stale answer
//
// This binary supersedes ./unified for the tri-layer demos. It exercises
// all four components of a thinker (§3.4): primitives, composition engine
// (forward + backward), relevance heuristic (currently trivial — will be
// the DCHL substrate at scale), and verifier.

#include "meta_learner.hpp"
#include "unified.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace unified;

// ---------------------------------------------------------------------
// Build the standard ontology once. Same premises used across demos.
// ---------------------------------------------------------------------
static void build_ontology(SI& si) {
    // Rules — the primitive set (Layer 2 §5.3.1).
    si.add_rule("transitive_is_a",       REL_IS_A,    REL_IS_A,    REL_IS_A);
    si.add_rule("class_has_flows_down",  REL_IS_A,    REL_HAS,     REL_HAS);
    si.add_rule("implication_chain",     REL_IMPLIES, REL_IMPLIES, REL_IMPLIES);
    si.add_rule("part_of_transitive",    REL_PART_OF, REL_PART_OF, REL_PART_OF);
    si.add_rule("has_via_part",          REL_PART_OF, REL_HAS,     REL_HAS);

    // Premises — the fact base.
    si.assert_fact("socrates",  REL_IS_A, "man");
    si.assert_fact("plato",     REL_IS_A, "man");
    si.assert_fact("aristotle", REL_IS_A, "man");
    si.assert_fact("man",       REL_IS_A, "primate");
    si.assert_fact("primate",   REL_IS_A, "mammal");
    si.assert_fact("mammal",    REL_IS_A, "animal");
    si.assert_fact("animal",    REL_IS_A, "living_thing");
    si.assert_fact("mortal",    REL_HAS,  "death");
    si.assert_fact("mammal",    REL_HAS,  "warm_blood");
    si.assert_fact("man",       REL_IS_A, "mortal");
    si.assert_fact("raining",   REL_IMPLIES, "wet_ground");
    si.assert_fact("wet_ground",REL_IMPLIES, "slippery");
    si.assert_fact("slippery",  REL_IMPLIES, "risk_of_falling");
    si.assert_fact("wheel",     REL_PART_OF, "car");
    si.assert_fact("car",       REL_PART_OF, "traffic");
    si.assert_fact("wheel",     REL_HAS,     "rubber");
    si.assert_fact("engine",    REL_PART_OF, "car");
}

// A canonical query batch.
struct QSpec { std::string s; int r; std::string o; };
static std::vector<QSpec> canonical_queries() {
    return {
        {"socrates",  REL_IS_A,    "man"},           // length 1 — retrieval
        {"socrates",  REL_IS_A,    "mortal"},        // 2 steps
        {"socrates",  REL_IS_A,    "primate"},       // 2 steps
        {"socrates",  REL_IS_A,    "mammal"},        // 3 steps
        {"socrates",  REL_IS_A,    "animal"},        // 4 steps
        {"socrates",  REL_IS_A,    "living_thing"},  // 5 steps
        {"plato",     REL_IS_A,    "mortal"},
        {"plato",     REL_IS_A,    "living_thing"},
        {"aristotle", REL_IS_A,    "mammal"},
        {"aristotle", REL_IS_A,    "living_thing"},
        {"socrates",  REL_HAS,     "death"},
        {"plato",     REL_HAS,     "warm_blood"},
        {"aristotle", REL_HAS,     "warm_blood"},
        {"raining",   REL_IMPLIES, "risk_of_falling"},
        {"wet_ground",REL_IMPLIES, "risk_of_falling"},
        {"wheel",     REL_PART_OF, "traffic"},
        {"engine",    REL_PART_OF, "traffic"},
    };
}

// ---------------------------------------------------------------------
// 1. Forward-chaining syllogism.
// ---------------------------------------------------------------------
static int demo_syllogism() {
    SI si;
    build_ontology(si);
    auto rep = si.reason();
    std::printf("=== forward chaining ===\n");
    std::printf("  reason: %d edges derived in %d steps, budget left %.1f\n",
                rep.edges_added, rep.steps_used, rep.budget_left);
    for (const auto& q : canonical_queries()) {
        auto res = si.query(q.s, q.r, q.o);
        std::printf("  Q: %-14s %-9s %-14s -> %s\n",
                    q.s.c_str(), rel_name(q.r), q.o.c_str(),
                    res.translation.c_str());
    }
    return 0;
}

// ---------------------------------------------------------------------
// 2. Backward chaining goal-directed proof.
// ---------------------------------------------------------------------
static int demo_backward() {
    SI si;
    build_ontology(si);
    std::printf("=== backward chaining (goal-directed) ===\n");
    // Do NOT run forward reasoning first. Backward should derive from
    // scratch by pursuing each specific goal.
    std::vector<QSpec> hard = {
        {"socrates",  REL_IS_A,    "living_thing"},
        {"aristotle", REL_HAS,     "warm_blood"},
        {"raining",   REL_IMPLIES, "risk_of_falling"},
        {"wheel",     REL_PART_OF, "traffic"},
    };
    for (const auto& q : hard) {
        auto res = si.ask_backward(q.s, q.r, q.o);
        std::printf("  Q: %-14s %-9s %-14s -> %s\n",
                    q.s.c_str(), rel_name(q.r), q.o.c_str(),
                    res.translation.c_str());
        if (res.holds) si.print_proof(res);
        std::printf("\n");
    }
    return 0;
}

// ---------------------------------------------------------------------
// 3. Thinker index — with and without rule induction.
//
//     Prediction (§7.6): C/R rises monotonically as rule induction
//     compiles frequent sub-chains into macro-rules. Without induction,
//     C/R stays flat.
// ---------------------------------------------------------------------
static void run_stream(SI& si, MetaLearner* meta,
                       const std::vector<QSpec>& queries,
                       ThinkerIndex& idx) {
    for (const auto& q : queries) {
        auto rep = si.reason();
        (void)rep;
        auto res = si.query(q.s, q.r, q.o);
        idx.record(res);
        if (meta && res.holds) meta->observe_trace(res.proof);
    }
}

static int demo_thinker() {
    std::printf("=== thinker index C/R over query stream ===\n\n");

    // Stream: repeat canonical queries multiple times to give rule
    // induction a chance to fire.
    auto qs = canonical_queries();
    std::vector<QSpec> stream;
    for (int r = 0; r < 6; ++r) for (const auto& q : qs) stream.push_back(q);

    // Run A: rule induction OFF.
    SI si_off;
    build_ontology(si_off);
    ThinkerIndex idx_off;
    run_stream(si_off, nullptr, stream, idx_off);
    std::printf("[induction OFF] rules used: %zu\n", si_off.rules.size());
    idx_off.report();

    // Run B: rule induction ON.
    SI si_on;
    build_ontology(si_on);
    MetaLearner meta;
    meta.engine = &si_on;
    ThinkerIndex idx_on;
    run_stream(si_on, &meta, stream, idx_on);
    std::printf("\n[induction ON ] rules used: %zu (was %zu initial + %zu macros)\n",
                si_on.rules.size(), si_on.rules.size() - meta.macros.size(),
                meta.macros.size());
    idx_on.report();
    std::printf("\n");
    meta.dump();

    // Verdict.
    std::printf("\n");
    if (meta.macros.size() > 0)
        std::printf(">> rule induction fired: %zu macro-rules registered\n",
                    meta.macros.size());
    else
        std::printf(">> rule induction did not fire on this stream\n");
    std::printf(">> paper §7.6 predicts C/R with induction >= C/R without;\n"
                "   observed: %.3f vs %.3f\n",
                idx_on.thinker_index(), idx_off.thinker_index());
    return 0;
}

// ---------------------------------------------------------------------
// 4. Verifier — synthesize a stale edge and confirm it fails verification.
// ---------------------------------------------------------------------
static int demo_verify() {
    SI si;
    build_ontology(si);
    si.reason();

    // Pick a real derived edge and verify it succeeds.
    int good = -1;
    for (int i = 0; i < (int)si.graph.edges.size(); ++i) {
        if (si.graph.edges[i].derived_by_rule >= 0) { good = i; break; }
    }
    std::printf("=== verifier ===\n");
    std::printf("  a valid derived edge verifies: %s\n",
                (good >= 0 && verify_edge(si, good)) ? "yes" : "no");

    // Forge a stale edge whose parent chain doesn't type-check.
    int forge = si.graph.add_edge(0, (int)si.graph.nodes.size() - 1,
                                  REL_IS_A, 0, 0, 0, 999);
    std::printf("  forged edge (arbitrary parents) verifies: %s\n",
                verify_edge(si, forge) ? "yes  <-- verifier is broken!"
                                       : "no   <-- verifier correctly rejects");
    return 0;
}

int main(int argc, char** argv) {
    std::string cmd = (argc >= 2) ? argv[1] : "all";
    if (cmd == "syllogism") return demo_syllogism();
    if (cmd == "backward")  return demo_backward();
    if (cmd == "thinker")   return demo_thinker();
    if (cmd == "verify")    return demo_verify();
    if (cmd == "all") {
        int rc = 0;
        rc |= demo_syllogism(); std::printf("\n");
        rc |= demo_backward();  std::printf("\n");
        rc |= demo_thinker();   std::printf("\n");
        rc |= demo_verify();
        return rc;
    }
    std::fprintf(stderr,
        "usage: %s [syllogism|backward|thinker|verify|all]\n", argv[0]);
    return 1;
}
