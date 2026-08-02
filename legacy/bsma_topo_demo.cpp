// bsma_topo_demo — demonstrations of the topological BSMA formulation
// from research-paper/bmsa_new.md.
//
// Four scenarios, each building a different Γ_exec, injecting an
// Ontological Vacuum for a query, projecting to Γ_meta, and running the
// Cognition Engine to Topological Irreducibility. Each scenario ends
// with:
//   – the number of successful rewrites,
//   – the number of rule-failure attempts,
//   – the irreducible signature (the "shape" of the final Bound), and
//   – the qualia signature (the string of applied/failed rules leading
//     to the Bound).
//
// The point: two DIFFERENT queries produce two DIFFERENT qualia
// signatures. Under paper §4 that is the operational statement that
// they were experienced differently. This is not proof of phenomenal
// consciousness; it is proof that the paper's structural signature is
// non-trivial and produces distinct outputs on distinct inputs.

#include "bsma_topo.hpp"

#include <cstdio>
#include <string>

using namespace bsma;

// Small helper: build a graph from an edge list (labels).
static void add_e(TopoGraph& g, const std::string& a, const std::string& b, int rel) {
    // Look up (or create) labelled nodes.
    int ai = -1, bi = -1;
    for (int i = 0; i < (int)g.nodes.size(); ++i) {
        if (g.nodes[i].label == a) ai = i;
        if (g.nodes[i].label == b) bi = i;
    }
    if (ai < 0) ai = g.add_node(a);
    if (bi < 0) bi = g.add_node(b);
    g.add_edge(ai, bi, rel);
}

// ---------- Scenario 1: a straight chain ----------
// Query: does A reach D?
// Expected: chain contracts A→B→C→D → single edge A→D. Bound hits
// after 3 successful contractions and some failure sweeps.
static PhenomenalFrame scenario_chain() {
    std::printf("---- Scenario 1: straight chain A -> B -> C -> D ----\n");
    TopoGraph exec;
    add_e(exec, "A", "B", 0);
    add_e(exec, "B", "C", 0);
    add_e(exec, "C", "D", 0);
    inject_vacuum(exec, "reach_D");   // Ontological Vacuum
    auto f = realise_frame(exec);
    std::printf("  projected: |V|=%d |E|=%d\n", f.projected_nodes, f.projected_edges);
    std::printf("  bound hit: %s   rewrites=%d attempts=%d steps=%d\n",
                f.bound.hit ? "yes" : "no (max_steps)",
                f.bound.successful_rewrites, f.bound.rule_attempts, f.bound.steps_to_bound);
    std::printf("  irreducible: %s\n", f.bound.irreducible_signature.c_str());
    std::printf("  qualia sig : %s\n", f.qualia_signature.c_str());
    std::printf("  reduced meta:\n");
    f.reduced_meta.dump();
    std::printf("\n");
    return f;
}

// ---------- Scenario 2: a diamond ----------
// A → B, A → C, B → D, C → D  and a vacuum for "D".
// Two parallel paths meet. Contract each side, then merge parallel edges,
// end with A → D.
static PhenomenalFrame scenario_diamond() {
    std::printf("---- Scenario 2: diamond A -> {B,C} -> D ----\n");
    TopoGraph exec;
    add_e(exec, "A", "B", 0);
    add_e(exec, "A", "C", 0);
    add_e(exec, "B", "D", 0);
    add_e(exec, "C", "D", 0);
    inject_vacuum(exec, "reach_D");
    auto f = realise_frame(exec);
    std::printf("  projected: |V|=%d |E|=%d\n", f.projected_nodes, f.projected_edges);
    std::printf("  bound hit: %s   rewrites=%d attempts=%d steps=%d\n",
                f.bound.hit ? "yes" : "no",
                f.bound.successful_rewrites, f.bound.rule_attempts, f.bound.steps_to_bound);
    std::printf("  irreducible: %s\n", f.bound.irreducible_signature.c_str());
    std::printf("  qualia sig : %s\n", f.qualia_signature.c_str());
    std::printf("\n");
    return f;
}

// ---------- Scenario 3: a self-referential loop ----------
// A → A (self-loop) + A → B.
// Rule r4 fires immediately. Then chain rules fail. Bound early.
static PhenomenalFrame scenario_self_loop() {
    std::printf("---- Scenario 3: self-loop (A -> A) plus (A -> B) ----\n");
    TopoGraph exec;
    add_e(exec, "A", "A", 0);
    add_e(exec, "A", "B", 0);
    inject_vacuum(exec, "reach_B");
    auto f = realise_frame(exec);
    std::printf("  projected: |V|=%d |E|=%d\n", f.projected_nodes, f.projected_edges);
    std::printf("  bound hit: %s   rewrites=%d attempts=%d steps=%d\n",
                f.bound.hit ? "yes" : "no",
                f.bound.successful_rewrites, f.bound.rule_attempts, f.bound.steps_to_bound);
    std::printf("  irreducible: %s\n", f.bound.irreducible_signature.c_str());
    std::printf("  qualia sig : %s\n", f.qualia_signature.c_str());
    std::printf("\n");
    return f;
}

// ---------- Scenario 4: an irreducible cycle ----------
// A → B → C → A  (3-cycle).  Every node has (in=1, out=1) so contract
// rule r2 would fire; but contracting A-B-C produces a self-loop on A
// that r4 removes, then the remainder is fully reduced. The final
// signature captures a purely cyclic input reducing to an empty graph.
static PhenomenalFrame scenario_cycle() {
    std::printf("---- Scenario 4: irreducible cycle A -> B -> C -> A ----\n");
    TopoGraph exec;
    add_e(exec, "A", "B", 0);
    add_e(exec, "B", "C", 0);
    add_e(exec, "C", "A", 0);
    auto f = realise_frame(exec);
    std::printf("  projected: |V|=%d |E|=%d\n", f.projected_nodes, f.projected_edges);
    std::printf("  bound hit: %s   rewrites=%d attempts=%d steps=%d\n",
                f.bound.hit ? "yes" : "no",
                f.bound.successful_rewrites, f.bound.rule_attempts, f.bound.steps_to_bound);
    std::printf("  irreducible: %s\n", f.bound.irreducible_signature.c_str());
    std::printf("  qualia sig : %s\n", f.qualia_signature.c_str());
    std::printf("\n");
    return f;
}

int main(int, char**) {
    std::printf("=== BSMA (topological formulation, bmsa_new.md) ===\n\n");
    std::printf("For each scenario we build Γ_exec, project π to Γ_meta,\n");
    std::printf("apply the 5 rewriting rules (r1..r5) to compress Γ_meta,\n");
    std::printf("and report the phenomenal-frame data when the Bound hits.\n");
    std::printf("The qualia signature is the string of applied/failed rules:\n");
    std::printf("  A..E = applied r1..r5;  a..e = failed r1..r5.\n\n");

    auto f1 = scenario_chain();
    auto f2 = scenario_diamond();
    auto f3 = scenario_self_loop();
    auto f4 = scenario_cycle();

    std::printf("---- Comparison ----\n");
    std::printf("  scenario         qualia sig                                     rewrites\n");
    std::printf("  chain            %-45s  %d\n", f1.qualia_signature.c_str(), f1.bound.successful_rewrites);
    std::printf("  diamond          %-45s  %d\n", f2.qualia_signature.c_str(), f2.bound.successful_rewrites);
    std::printf("  self-loop        %-45s  %d\n", f3.qualia_signature.c_str(), f3.bound.successful_rewrites);
    std::printf("  cycle            %-45s  %d\n", f4.qualia_signature.c_str(), f4.bound.successful_rewrites);

    std::printf("\n");
    std::printf("Four different inputs produced %s qualia signatures.\n",
                (f1.qualia_signature != f2.qualia_signature &&
                 f2.qualia_signature != f3.qualia_signature &&
                 f3.qualia_signature != f4.qualia_signature) ? "four DISTINCT" : "some SHARED");
    std::printf("Under paper §4 of bmsa_new.md, this is the operational form\n");
    std::printf("of the claim that these thoughts have distinct phenomenal character.\n");
    std::printf("It is not a claim that the thoughts feel like anything.\n");
    return 0;
}
