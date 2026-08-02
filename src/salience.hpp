// salience.hpp — TSDA (Topological Salience & Default-Mode Architecture).
//
// Implements the three components of research-paper/tsda.md:
//
//   1. SalienceMap  — per-node integer "attentional heat" S(n) that
//                     spikes on reference and decays each tick.
//   2. Working graph — the subgraph of nodes with S(n) >= tau_attention,
//                     bounded in size (Miller's 7 ± 2). This is what the
//                     engine actively "thinks over"; the rest is
//                     long-term memory that must be recalled.
//   3. Default Mode Network — an idle-cycle procedure that scans for
//                     high-salience but topologically distant node pairs
//                     and attempts spontaneous bisociation between them.
//                     This is the "shower thought" mechanism.
//
// Design: the SalienceMap is a lightweight side-structure on top of an
// SI graph. It does not modify the graph. Ticking, spiking, and querying
// are all O(V) at worst, and typically O(k) where k = |working graph|.

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

struct SalienceMap {
    // Salience for each node in the SI graph, indexed by node id.
    std::vector<int> S;
    int  s_max          = 100;   // spike ceiling on reference
    int  tau_attention  = 20;    // Working Graph threshold
    int  decay_step     = 3;     // per-tick decay
    int  working_cap    = 9;     // Miller 7 ± 2
    long tick_count     = 0;     // total cycles elapsed

    // Ensure S has an entry for every node in the graph.
    void sync_with(const SI& si) {
        if (S.size() < si.graph.nodes.size())
            S.resize(si.graph.nodes.size(), 0);
    }

    // Spike a single node's salience to the ceiling. Called whenever
    // the engine touches the node (query, teach, path result).
    void spike(int node_id) {
        if (node_id < 0) return;
        if ((size_t)node_id >= S.size()) S.resize(node_id + 1, 0);
        S[node_id] = s_max;
    }

    void spike(const SI& si, const std::string& label) {
        auto it = si.graph.by_label.find(label);
        if (it != si.graph.by_label.end()) spike(it->second);
    }

    // One decay tick. Every node loses `decay_step`, floored at 0.
    void tick(const SI& si) {
        sync_with(si);
        ++tick_count;
        for (int& s : S) s = std::max(0, s - decay_step);
    }

    // Extract the working graph = the top-N nodes ranked by salience,
    // subject to S >= tau_attention. Returns node ids sorted highest
    // salience first.
    std::vector<int> working_graph(const SI& si) const {
        std::vector<std::pair<int,int>> pairs;    // (salience, node_id)
        for (int i = 0; i < (int)std::min(S.size(), si.graph.nodes.size()); ++i) {
            if (S[i] >= tau_attention) pairs.push_back({S[i], i});
        }
        std::sort(pairs.begin(), pairs.end(),
                  [](const std::pair<int,int>& a, const std::pair<int,int>& b){
                      return a.first > b.first;
                  });
        if ((int)pairs.size() > working_cap) pairs.resize(working_cap);
        std::vector<int> out;
        out.reserve(pairs.size());
        for (const auto& p : pairs) out.push_back(p.second);
        return out;
    }

    // Return every ordered pair (a, b) of high-salience nodes that are
    // topologically distant (shortest-path length > min_dist or
    // unreachable). This is what the Default Mode Network consumes as
    // input candidates for spontaneous bisociation.
    struct DistantPair { int a; int b; int shortest_hop; };

    std::vector<DistantPair>
    high_salience_distant_pairs(const SI& si, int min_dist = 4,
                                int max_pairs = 32) const {
        auto W = working_graph(si);
        // Precompute out-degree so we can prioritise pairs where at
        // least one side has outgoing edges — pairs from two dead-end
        // terminals can't yield a forward bridge.
        std::vector<int> outdeg((int)si.graph.nodes.size(), 0);
        for (const auto& e : si.graph.edges) ++outdeg[e.from];
        std::vector<DistantPair> out;
        // Two passes so productive pairs come first.
        for (int pass = 0; pass < 2 && (int)out.size() < max_pairs; ++pass) {
            for (size_t i = 0; i < W.size() && (int)out.size() < max_pairs; ++i) {
                for (size_t j = 0; j < W.size() && (int)out.size() < max_pairs; ++j) {
                    if (i == j) continue;
                    int a = W[i], b = W[j];
                    bool productive = (outdeg[a] > 0 || outdeg[b] > 0);
                    if (pass == 0 && !productive) continue;
                    if (pass == 1 && productive) continue;
                    // Avoid duplicate emit across passes.
                    bool already = false;
                    for (const auto& q : out)
                        if (q.a == a && q.b == b) { already = true; break; }
                    if (already) continue;
                    int hop = shortest_hop_count(si, a, b,
                                                 /*hard_cap=*/min_dist + 2);
                    if (hop < 0 || hop >= min_dist) {
                        out.push_back({a, b, hop});
                    }
                }
            }
        }
        return out;
    }

  private:
    // BFS bounded to hard_cap hops. Returns hop count if reachable, -1
    // if not reachable within the cap.
    static int shortest_hop_count(const SI& si, int a, int b, int hard_cap) {
        if (a == b) return 0;
        std::vector<int> depth((int)si.graph.nodes.size(), -1);
        depth[a] = 0;
        std::vector<int> q; q.push_back(a);
        size_t head = 0;
        while (head < q.size()) {
            int u = q[head++];
            if (depth[u] >= hard_cap) continue;
            for (const auto& e : si.graph.edges) {
                if (e.from != u) continue;
                if (depth[e.to] >= 0) continue;
                depth[e.to] = depth[u] + 1;
                if (e.to == b) return depth[e.to];
                q.push_back(e.to);
            }
        }
        return -1;
    }
};

} // namespace unified
