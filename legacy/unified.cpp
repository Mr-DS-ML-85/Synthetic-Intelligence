// unified — CLI driver for the tri-layer Constructive Structural
// Cognition engine specified in
//   research-paper/synthetic-intelligence.md
//
// Commands:
//   ./unified syllogism      classical is_a / has generalisation demo
//   ./unified expand         phase-space expansion via node spawning
//   ./unified ood            out-of-ontology honest refusal
//   ./unified all            run all of the above

#include "unified.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace unified;

// ---------------------------------------------------------------------
// 1. Syllogistic reasoning with zero-shot generalisation.
//
// Premises given: individual is-a chains for several entities.
// Rule supplied: transitive is_a (Barbara syllogism).
// Test: query subject -> class pairs that were never asserted directly.
//
// If the engine works, every derivable relation is inferred without
// having been seen. This is the falsifiable-in-principle test for
// active construction over statistical interpolation.
// ---------------------------------------------------------------------
static int demo_syllogism() {
    SI si;

    // Rules (Layer 2)
    si.add_rule("transitive_is_a",  REL_IS_A,  REL_IS_A,  REL_IS_A);
    si.add_rule("class_has_flows_down", REL_IS_A, REL_HAS, REL_HAS);
    si.add_rule("implication_chain", REL_IMPLIES, REL_IMPLIES, REL_IMPLIES);

    // Premises (Layer 2 seeded from Layer 1 assertions)
    // Chain 1: individuals -> species -> class -> phylum
    si.assert_fact("socrates", REL_IS_A, "man");
    si.assert_fact("plato",    REL_IS_A, "man");
    si.assert_fact("aristotle",REL_IS_A, "man");
    si.assert_fact("man",      REL_IS_A, "mortal");
    si.assert_fact("man",      REL_IS_A, "primate");
    si.assert_fact("primate",  REL_IS_A, "mammal");
    si.assert_fact("mammal",   REL_IS_A, "animal");
    // Property flow: mortal has death
    si.assert_fact("mortal",   REL_HAS,  "death");
    si.assert_fact("mammal",   REL_HAS,  "warm_blood");
    // Chain 2: implication chain
    si.assert_fact("raining",  REL_IMPLIES, "wet_ground");
    si.assert_fact("wet_ground", REL_IMPLIES, "slippery");
    si.assert_fact("slippery", REL_IMPLIES, "risk_of_falling");

    // Translation table (Layer 3). Explicit rows for the queries we
    // plan to ask; nothing hidden, log2(N) bits budget.
    si.table.register_answer("socrates", REL_IS_A, "mortal",
                             "Yes: Socrates is mortal.");
    si.table.register_answer("plato",    REL_IS_A, "mortal",
                             "Yes: Plato is mortal.");
    si.table.register_answer("aristotle",REL_IS_A, "animal",
                             "Yes: Aristotle is an animal.");
    si.table.register_answer("socrates", REL_HAS,  "death",
                             "Yes: Socrates has death.");
    si.table.register_answer("plato",    REL_HAS,  "warm_blood",
                             "Yes: Plato has warm blood.");
    si.table.register_answer("raining",  REL_IMPLIES, "risk_of_falling",
                             "Yes: Raining implies risk of falling.");

    std::printf("=== syllogism demo — active construction ===\n\n");
    std::printf("--- premises ingested ---\n");
    si.graph.dump();
    std::printf("  translation table: %zu rows, %.2f bits (log2 N)\n\n",
                si.table.rows.size(), si.table.mdl_bits());

    auto rep = si.reason();
    std::printf("--- after reasoning: %d edges derived over %d steps ---\n",
                rep.edges_added, rep.steps_used);
    si.graph.dump();

    std::printf("\n--- queries (all previously unseen) ---\n");
    struct Q { std::string s; int r; std::string o; };
    std::vector<Q> qs = {
        {"socrates", REL_IS_A,   "mortal"},
        {"socrates", REL_IS_A,   "animal"},
        {"plato",    REL_IS_A,   "mammal"},
        {"aristotle",REL_IS_A,   "primate"},
        {"socrates", REL_HAS,    "death"},
        {"plato",    REL_HAS,    "warm_blood"},
        {"raining",  REL_IMPLIES, "risk_of_falling"},
    };
    for (const auto& q : qs) {
        auto res = si.query(q.s, q.r, q.o);
        std::printf("  Q: %s %s %s\n", q.s.c_str(), rel_name(q.r), q.o.c_str());
        std::printf("     -> %s\n", res.translation.c_str());
        if (res.holds) si.print_proof(res);
        std::printf("\n");
    }
    return 0;
}

// ---------------------------------------------------------------------
// 2. Phase-space expansion via node spawning (Axiom II).
//
// A rule can spawn a fresh NODE_ABSTRACT when it fires, physically
// growing the graph's dimensionality. This models "the system invents
// new degrees of freedom under structural pressure".
// ---------------------------------------------------------------------
static int demo_expand() {
    SI si;

    // Same transitive rule but this one SPAWNS an intermediate node,
    // rather than adding a direct edge. Every derivation is now a
    // three-vertex construction (a -> [abstract] -> c) instead of a
    // two-vertex composition.
    si.add_rule("spawn_transitive_is_a",  REL_IS_A,  REL_IS_A,  REL_IS_A,
                /*spawn=*/true);

    si.assert_fact("socrates", REL_IS_A, "man");
    si.assert_fact("man",      REL_IS_A, "mortal");
    si.assert_fact("mortal",   REL_IS_A, "being");

    std::printf("=== phase-space expansion demo ===\n\n");
    std::printf("initial |V|=%zu |E|=%zu\n", si.graph.nodes.size(), si.graph.edges.size());
    auto rep = si.reason();
    std::printf("after reasoning: |V|=%zu |E|=%zu   (edges added=%d, steps=%d)\n\n",
                si.graph.nodes.size(), si.graph.edges.size(),
                rep.edges_added, rep.steps_used);
    si.graph.dump();
    std::printf("\nnew NODE_ABSTRACTs are physical growth of the phase space\n"
                "under tension budget %.2f -> %.2f (Axiom II conservation law).\n",
                si.base_budget, rep.budget_left);
    return 0;
}

// ---------------------------------------------------------------------
// 3. OOD honesty (Axiom III consequence).
//
// A query whose concepts do not appear in the ontology returns "unknown"
// rather than confabulating. The refusal is grounded — you can point at
// exactly which concept is unknown.
// ---------------------------------------------------------------------
static int demo_ood() {
    SI si;
    si.add_rule("transitive_is_a", REL_IS_A, REL_IS_A, REL_IS_A);
    si.assert_fact("socrates", REL_IS_A, "man");
    si.assert_fact("man",      REL_IS_A, "mortal");
    si.table.register_answer("socrates", REL_IS_A, "mortal",
                             "Yes: Socrates is mortal.");
    si.reason();

    std::printf("=== OOD honesty demo ===\n\n");
    // Known.
    auto q1 = si.query("socrates", REL_IS_A, "mortal");
    std::printf("  Q: is socrates mortal?    -> %s\n",  q1.translation.c_str());
    // Same ontology, unseen combination the reasoner has not derived (no rule
    // connects socrates to some other class).
    auto q2 = si.query("socrates", REL_IS_A, "flower");
    std::printf("  Q: is socrates a flower?  -> %s\n",  q2.translation.c_str());
    // OOD entirely: neither concept in graph.
    auto q3 = si.query("napoleon", REL_IS_A, "emperor");
    std::printf("  Q: is napoleon an emperor?-> %s   (coherence=%.2f)\n",
                q3.translation.c_str(), q3.coherence);
    return 0;
}

int main(int argc, char** argv) {
    std::string cmd = (argc >= 2) ? argv[1] : "all";
    if (cmd == "syllogism") return demo_syllogism();
    if (cmd == "expand")    return demo_expand();
    if (cmd == "ood")       return demo_ood();
    if (cmd == "all") {
        int rc = 0;
        rc |= demo_syllogism(); std::printf("\n");
        rc |= demo_expand();    std::printf("\n");
        rc |= demo_ood();
        return rc;
    }
    std::fprintf(stderr,
        "usage: %s [syllogism|expand|ood|all]\n", argv[0]);
    return 1;
}
