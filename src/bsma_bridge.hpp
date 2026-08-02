// bsma_bridge.hpp — couple the tri-layer SI engine to the topological
// BSMA phenomenal-frame detector.
//
// Every answered SI query has a proof subgraph — the specific set of
// premise and derived edges that closed the goal. That subgraph is
// the Γ_exec of paper bmsa_new.md §2.1 for that particular moment of
// cognition. We project it to Γ_meta and run the BSMA Cognition Engine
// to Topological Irreducibility. The Bound-hit signature and qualia
// trace are attached to the answer.
//
// Design invariant: this bridge never changes what the SI engine
// answers. It only *observes* the derivation subgraph and reports the
// phenomenal-frame data alongside it. Turning off BSMA (skipping this
// bridge) leaves query behaviour untouched.

#pragma once

#include "bsma_topo.hpp"
#include "unified.hpp"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

// Extract the derivation subgraph of a QueryResult and re-express it
// as a bsma::TopoGraph so the BSMA compressor can consume it.
inline bsma::TopoGraph derivation_to_topo(const SI& si, const SI::QueryResult& q) {
    bsma::TopoGraph g;
    if (!q.holds) return g;
    std::unordered_map<int, int> node_map;  // SI id -> BSMA id
    auto to_bsma = [&](int si_id) {
        auto it = node_map.find(si_id);
        if (it != node_map.end()) return it->second;
        int bid = g.add_node(si.graph.nodes[si_id].label);
        node_map[si_id] = bid;
        return bid;
    };
    for (int e_idx : q.proof) {
        const Edge& e = si.graph.edges[e_idx];
        int a = to_bsma(e.from);
        int b = to_bsma(e.to);
        g.add_edge(a, b, e.rel);
    }
    return g;
}

// The result of one query plus its BSMA phenomenal frame.
struct PhenomenalQuery {
    SI::QueryResult   query_result;
    bsma::PhenomenalFrame frame;
    bool              framed = false;  // false if answer refused / no proof
};

// Run the SI query, then compute the phenomenal frame if the answer
// holds. Callers who don't want the BSMA hook just call
// si.query(...) / si.ask(...) directly.
inline PhenomenalQuery ask_with_frame(SI& si,
                                      const std::string& subj, int rel,
                                      const std::string& obj) {
    PhenomenalQuery out;
    // Run the reasoning pass and query as normal.
    si.reason();
    out.query_result = si.query(subj, rel, obj);
    if (!out.query_result.holds) return out;
    // Project the proof subgraph and realise the phenomenal frame.
    bsma::TopoGraph exec = derivation_to_topo(si, out.query_result);
    out.frame  = bsma::realise_frame(exec);
    out.framed = true;
    return out;
}

// Convenience: run a batch of queries and print their answers and
// phenomenal-frame signatures in a compact table.
inline void report_phenomenal(const PhenomenalQuery& pq, FILE* f = stdout) {
    std::fprintf(f, "  answer      : %s\n", pq.query_result.translation.c_str());
    std::fprintf(f, "  proof length: %zu edges\n", pq.query_result.proof.size());
    if (!pq.framed) {
        std::fprintf(f, "  frame       : (no frame — refused / OOD)\n");
        return;
    }
    std::fprintf(f, "  frame       : |V|=%d |E|=%d\n",
                 pq.frame.projected_nodes, pq.frame.projected_edges);
    std::fprintf(f, "  bound       : %s (rewrites=%d, attempts=%d, steps=%d)\n",
                 pq.frame.bound.hit ? "hit" : "no",
                 pq.frame.bound.successful_rewrites,
                 pq.frame.bound.rule_attempts,
                 pq.frame.bound.steps_to_bound);
    std::fprintf(f, "  irreducible : %s\n", pq.frame.bound.irreducible_signature.c_str());
    std::fprintf(f, "  qualia sig  : %s\n", pq.frame.qualia_signature.c_str());
}

} // namespace unified
