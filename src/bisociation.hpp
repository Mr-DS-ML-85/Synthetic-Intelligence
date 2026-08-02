// bisociation.hpp — CMA Conceptual Mutation Architecture.
//
// Implements research-paper/CMA.md: the Mutation primitive M that
// forces a hypothesised edge E_hyp between two semantically DISTANT
// nodes (d(a,b) > tau_novelty), then puts the engine into a Resolution
// Phase — search for a bridge subgraph that explains the connection.
//
// If a bridge is found within the resource budget and the bisociation
// energy U_hyp is below U_crit, the edge is validated (adopted). If
// not, the hypothesis decays and is discarded — no permanent noise is
// added to the graph.
//
// This is distinct from `novelty.hpp::repurposing_candidates`, which
// looks for candidates that already SHARE a mechanism target. CMA
// instead forces pairs that DON'T share anything and asks: "can the
// engine invent a bridge?"

#pragma once

#include "primitives.hpp"
#include "unified.hpp"
#include "novelty.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace unified {

// A bisociation attempt: forced edge between (a, b), plus the bridge
// path the engine found (if any) and its energy score.
struct BisociationResult {
    std::string a_label;
    std::string b_label;
    int shortest_hop_before;         // d(a, b) in the original graph
    bool bridge_found = false;
    std::vector<int> bridge_edges;   // edge indices making the bridge
    std::string intermediate;        // label of the "hinge" concept, if any
    float energy = 0.0f;             // U_hyp (lower = better)
    bool validated = false;          // U_hyp < U_crit
    std::string status;              // human-readable outcome
};

// Search for a bridge between a and b: any short path a -> ... -> b
// through the graph. If found, extract the mid-hop concept as the
// hinge. This is the "structural bisociation" that the CMA paper
// describes with the gravity/RDMA example: both fracture into
// data_pull.
inline BisociationResult
bisociate(const SI& si, const std::string& a_label,
          const std::string& b_label,
          int tau_novelty = 3,           // require d(a,b) > tau
          int max_bridge_hops = 5,
          float u_crit = 5.0f) {
    BisociationResult r;
    r.a_label = a_label;
    r.b_label = b_label;

    auto ita = si.graph.by_label.find(a_label);
    auto itb = si.graph.by_label.find(b_label);
    if (ita == si.graph.by_label.end() || itb == si.graph.by_label.end()) {
        r.status = "one or both concepts are not in the graph";
        return r;
    }

    // Distance before the mutation.
    auto shortest = find_paths(si, a_label, b_label,
                               /*max_hops=*/max_bridge_hops, /*cap=*/1);
    r.shortest_hop_before = shortest.empty() ? -1
                                            : shortest.front().length();

    // If the concepts are already close, this is not a bisociation.
    if (r.shortest_hop_before >= 0 && r.shortest_hop_before <= tau_novelty) {
        r.status = "concepts are already close (d=" +
                   std::to_string(r.shortest_hop_before) +
                   " <= tau_novelty=" + std::to_string(tau_novelty) +
                   "); no bisociation attempted";
        return r;
    }

    // Resolution Phase: search for a bridge.
    auto paths = find_paths(si, a_label, b_label,
                            /*max_hops=*/max_bridge_hops, /*cap=*/8);
    if (paths.empty()) {
        // No forward bridge found. Try meeting-in-the-middle: search
        // for any node that both a and b point at, either directly or
        // through one hop.
        // Reuse shared_targets for a cheap intersection check.
        auto sh = shared_targets(si, a_label, b_label);
        if (!sh.empty()) {
            // Synthesise a two-leg bridge: a -> shared -> b if b has
            // an incoming edge from shared.
            r.intermediate = si.graph.nodes[sh.front().target_node].label;
            // Energy: proportional to path length. No explicit bridge
            // edges (they exist via different relations from a and b),
            // so we treat this as a "hinge" bisociation.
            r.energy = 2.0f;    // two-leg hinge is cheap
            r.status = "no direct bridge, but " + r.intermediate +
                       " is a shared hinge concept";
            r.validated = r.energy < u_crit;
            return r;
        }
        r.status = "no bridge found within " +
                   std::to_string(max_bridge_hops) + " hops; "
                   "hypothesis decays";
        r.energy = (float)max_bridge_hops + 5.0f;   // above u_crit
        return r;
    }

    // Bridge found. Use the shortest.
    const DiscoveryPath& best = paths.front();
    r.bridge_edges = best.edges;
    r.bridge_found = true;

    // Extract the middle hop as the hinge concept.
    if (!best.edges.empty()) {
        int mid = (int)best.edges.size() / 2;
        int mid_edge = best.edges[mid];
        int hinge = (mid == 0) ? si.graph.edges[mid_edge].to
                              : si.graph.edges[best.edges[mid - 1]].to;
        r.intermediate = si.graph.nodes[hinge].label;
    }

    // Energy: linear in path length. Longer bridges cost more.
    r.energy = (float)best.length();
    r.validated = r.energy < u_crit;
    r.status = r.validated
               ? "bridge found (len=" + std::to_string(best.length())
                 + "), U_hyp=" + std::to_string(r.energy)
                 + " < U_crit=" + std::to_string(u_crit)
                 + " -> hypothesis VALIDATED"
               : "bridge found (len=" + std::to_string(best.length())
                 + ") but U_hyp=" + std::to_string(r.energy)
                 + " >= U_crit=" + std::to_string(u_crit)
                 + " -> hypothesis PRUNED";
    return r;
}

inline std::string render_bisociation(const SI& si,
                                       const BisociationResult& r) {
    std::string out = "Bisociation attempt: " + r.a_label + " <> "
                    + r.b_label + "\n";
    out += "  original distance   : ";
    out += (r.shortest_hop_before < 0 ? "unreachable"
             : std::to_string(r.shortest_hop_before) + " hops");
    out += "\n";
    if (r.bridge_found) {
        out += "  bridge              : "
             + render_path(si, DiscoveryPath{r.bridge_edges}) + "\n";
    }
    if (!r.intermediate.empty()) {
        out += "  hinge concept       : " + r.intermediate + "\n";
    }
    char e[64]; std::snprintf(e, sizeof(e), "%.2f", r.energy);
    out += "  bisociation energy  : "; out += e; out += "\n";
    out += "  outcome             : " + r.status + "\n";
    if (r.validated) {
        out += "  interpretation      : the engine synthesised a bridge "
               "between two distant concepts. This is a candidate "
               "cross-domain insight — treat as a HYPOTHESIS, not fact.";
    } else if (!r.bridge_found) {
        out += "  interpretation      : the engine could not synthesise "
               "a bridge; the hypothesised edge decays. No graph "
               "corruption occurs.";
    }
    return out;
}

} // namespace unified
