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
    float confidence = 0.0f;         // gradual: 1/(1+U_hyp) when validated,
                                     // accumulates across repeat attempts
                                     // at the caller (CMA §resource loop)
    std::string status;              // human-readable outcome
};

// Structural degree of a node — both directions. The energy model and
// hinge choice need "how connected is this intermediate", which is a
// property of the graph, not of the search that found the path.
inline int bisoc_degree(const SI& si, int id) {
    if (id < 0 || id >= (int)si.graph.nodes.size()) return 0;
    int d = 0;
    if (id < (int)si.graph.out_adj.size()) d += (int)si.graph.out_adj[(size_t)id].size();
    if (id < (int)si.graph.in_adj.size())  d += (int)si.graph.in_adj[(size_t)id].size();
    return d;
}

// One hop through a hub is CHEAP (many corroborating paths run through
// it); one hop through a rare leaf is EXPENSIVE (that path is the only
// evidence). U_hyp = sum over hops of 1/log(2+deg) — the CMA paper's
// "a bridge through highly connected hubs should be cheaper than
// through rare intermediates", which a bare hop count cannot express.
inline float bisoc_hop_cost(const SI& si, int via_node) {
    return 1.0f / std::log(2.0f + (float)bisoc_degree(si, via_node));
}

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
            r.confidence = r.validated ? 1.0f / (1.0f + r.energy) : 0.0f;
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

    // Hinge = the most CONSTRAINING internal concept on the bridge —
    // the highest-degree internal node, not the geometric midpoint. A
    // long bridge through one dominant hub is really a bisociation
    // THROUGH that hub; the midpoint can be an incidental pass-through.
    if (best.edges.size() >= 2) {
        int best_deg = -1, best_node = -1;
        for (size_t k = 0; k + 1 < best.edges.size(); ++k) {
            int internal = si.graph.edges[best.edges[k]].to;
            int d = bisoc_degree(si, internal);
            if (d > best_deg) { best_deg = d; best_node = internal; }
        }
        if (best_node >= 0)
            r.intermediate = si.graph.nodes[best_node].label;
    } else if (!best.edges.empty()) {
        r.intermediate = si.graph.nodes[si.graph.edges[best.edges[0]].to].label;
    }

    // Energy: structural, not just hop count — each hop costs by the
    // rarity of the node it passes through (bisoc_hop_cost).
    float u = 0.0f;
    for (size_t k = 0; k < best.edges.size(); ++k) {
        int via = si.graph.edges[best.edges[k]].to;
        u += bisoc_hop_cost(si, via);
    }
    if (u < 1.0f) u = 1.0f;          // a bridge is never free
    r.energy = u;
    r.validated = r.energy < u_crit;
    r.confidence = r.validated ? 1.0f / (1.0f + r.energy) : 0.0f;
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
    if (r.confidence > 0.0f) {
        char cbuf[64]; std::snprintf(cbuf, sizeof(cbuf), "%.2f", r.confidence);
        out += "  confidence          : "; out += cbuf; out += "\n";
    }
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
