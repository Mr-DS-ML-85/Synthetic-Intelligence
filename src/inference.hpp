// inference.hpp — the Discovery Engine.
//
// This is the move from Knowledge Store to Inference Engine.
// Six queries, each finding what the graph *doesn't* say rather than
// what it does:
//
//   1. find_isolated_subgraphs(si)
//      Connected components that never touch a node tagged as
//      "is_fundamental". These are floating islands — concepts not
//      grounded in the theory's core.
//
//   2. find_dangling_edges(si)
//      Nodes with outgoing edges that never lead (within K hops) to a
//      fundamental node. Same idea, per-node.
//
//   3. find_logic_gap(si, A, B)
//      When A and B don't reach each other, run bidirectional BFS from
//      both. Return the two closest fringe nodes — the exact missing
//      bridge.
//
//   4. find_symmetry_gaps(si)
//      For every asserted edge (X --r--> Y) where BOTH endpoints
//      have `mirror_of` partners X', Y', check whether the symmetric
//      partner edge (X' --r--> Y') is asserted. If not, THAT is the
//      candidate breakthrough — the Maxwell displacement-current
//      pattern.
//
//   5. find_anomalies(si)
//      Nodes with high total degree but low axiomatic grounding
//      (long shortest-hop distance to any fundamental node). These
//      are the "confused zones" — where a Maxwell-scale insight
//      would rearrange the theory.
//
//   6. classify_gap_query(query)
//      Called by the router to route natural-language questions
//      like "what's missing between X and Y" or "any incomplete
//      symmetries" or "where should we look for a breakthrough".

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cstdint>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace unified {

// ---------- helpers ----------

// Is this node explicitly marked as a fundamental / axiomatic node?
// Two conventions: an outgoing edge (X is_fundamental fundamental_law)
// or an `is_a` chain landing on a label ending in "_law" / "axiom".
inline bool is_fundamental_node(const SI& si, int nid) {
    for (const auto& e : si.graph.edges) {
        if (e.from != nid) continue;
        if (e.rel == REL_FUNDAMENTAL) return true;
        if (e.rel == REL_IS_A) {
            const std::string& tl = si.graph.nodes[e.to].label;
            if (tl == "fundamental_law" || tl == "axiom" ||
                tl == "law" || tl == "conservation_law" ||
                tl == "invariant")
                return true;
        }
    }
    return false;
}

// Undirected adjacency for connected-component decomposition.
inline std::vector<std::vector<int>> undirected_adj(const SI& si) {
    std::vector<std::vector<int>> adj(si.graph.nodes.size());
    for (const auto& e : si.graph.edges) {
        adj[e.from].push_back(e.to);
        adj[e.to].push_back(e.from);
    }
    return adj;
}

// ---------- 1. isolated subgraphs ----------

struct IsolatedComponent {
    std::vector<int> nodes;
    bool contains_fundamental = false;
};

inline std::vector<IsolatedComponent> find_isolated_subgraphs(const SI& si) {
    auto adj = undirected_adj(si);
    std::vector<int> comp((int)si.graph.nodes.size(), -1);
    std::vector<IsolatedComponent> comps;
    for (int i = 0; i < (int)si.graph.nodes.size(); ++i) {
        if (comp[i] >= 0) continue;
        IsolatedComponent c;
        std::vector<int> q; q.push_back(i);
        comp[i] = (int)comps.size();
        while (!q.empty()) {
            int u = q.back(); q.pop_back();
            c.nodes.push_back(u);
            if (is_fundamental_node(si, u)) c.contains_fundamental = true;
            for (int v : adj[u]) {
                if (comp[v] < 0) {
                    comp[v] = comp[i];
                    q.push_back(v);
                }
            }
        }
        comps.push_back(std::move(c));
    }
    // Return only the components that are NOT grounded in a
    // fundamental node — these are the floating islands.
    std::vector<IsolatedComponent> floating;
    for (auto& c : comps)
        if (!c.contains_fundamental) floating.push_back(std::move(c));
    // Larger islands first.
    std::sort(floating.begin(), floating.end(),
              [](const IsolatedComponent& a, const IsolatedComponent& b){
                  return a.nodes.size() > b.nodes.size();
              });
    return floating;
}

// ---------- 2. dangling edges ----------

struct DanglingNode {
    int node_id;
    int total_degree;              // in + out
    int min_hops_to_fundamental;   // -1 if unreachable
};

inline std::vector<DanglingNode>
find_dangling_edges(const SI& si, int max_hops = 5, int top_k = 10) {
    // Precompute out-adjacency.
    std::vector<std::vector<int>> out(si.graph.nodes.size());
    std::vector<int> in_deg(si.graph.nodes.size(), 0);
    std::vector<int> out_deg(si.graph.nodes.size(), 0);
    for (const auto& e : si.graph.edges) {
        out[e.from].push_back(e.to);
        ++out_deg[e.from];
        ++in_deg[e.to];
    }
    std::vector<DanglingNode> dangling;
    for (int i = 0; i < (int)si.graph.nodes.size(); ++i) {
        int deg = in_deg[i] + out_deg[i];
        if (deg == 0) continue;
        // BFS from i looking for any fundamental.
        std::vector<int> depth(si.graph.nodes.size(), -1);
        depth[i] = 0;
        std::vector<int> q; q.push_back(i);
        size_t head = 0;
        int hit = -1;
        while (head < q.size()) {
            int u = q[head++];
            if (depth[u] >= max_hops) continue;
            for (int v : out[u]) {
                if (depth[v] >= 0) continue;
                depth[v] = depth[u] + 1;
                if (is_fundamental_node(si, v)) { hit = depth[v]; break; }
                q.push_back(v);
            }
            if (hit >= 0) break;
        }
        // Node is dangling if it has connections but no forward path
        // to any fundamental node within K hops.
        if (hit < 0)
            dangling.push_back({i, deg, -1});
    }
    // Rank by degree descending — the most-connected ungrounded nodes
    // are the most interesting anomalies.
    std::sort(dangling.begin(), dangling.end(),
              [](const DanglingNode& a, const DanglingNode& b){
                  return a.total_degree > b.total_degree;
              });
    if ((int)dangling.size() > top_k) dangling.resize(top_k);
    return dangling;
}

// ---------- 3. logic gap ----------
//
// When A does not reach B, run BFS from A forward and BFS from B
// backward. Find the pair of fringe nodes (one on each side) closest
// to each other by graph-agnostic label distance (Levenshtein) or by
// bibliographic distance. We stay simple: pick the pair whose shortest
// hop count is minimal across the two frontiers.

struct LogicGap {
    int  from_side_node = -1;   // last reachable from A
    int  to_side_node   = -1;   // first node whose reverse reaches B
    int  hops_from_a    = -1;
    int  hops_from_b_reverse = -1;
    bool full_path_exists = false;
    std::vector<int> forward_prefix;   // edges from A to from_side_node
    std::vector<int> backward_suffix;  // edges from to_side_node to B
};

// Reverse adjacency for backward BFS.
inline std::vector<std::vector<int>> reverse_out_adj(const SI& si) {
    std::vector<std::vector<int>> in_edges(si.graph.nodes.size());
    for (size_t i = 0; i < si.graph.edges.size(); ++i) {
        const auto& e = si.graph.edges[i];
        in_edges[e.to].push_back(e.from);
    }
    return in_edges;
}

inline LogicGap find_logic_gap(const SI& si,
                               const std::string& a_label,
                               const std::string& b_label,
                               int max_hops = 6) {
    LogicGap g;
    auto ita = si.graph.by_label.find(a_label);
    auto itb = si.graph.by_label.find(b_label);
    if (ita == si.graph.by_label.end() || itb == si.graph.by_label.end())
        return g;
    int A = ita->second, B = itb->second;
    if (A == B) return g;

    // Forward BFS from A.
    std::vector<int> fdepth(si.graph.nodes.size(), -1);
    std::vector<int> fparent_node(si.graph.nodes.size(), -1);
    std::vector<int> fparent_edge(si.graph.nodes.size(), -1);
    fdepth[A] = 0;
    std::vector<int> fq = {A};
    size_t fh = 0;
    while (fh < fq.size()) {
        int u = fq[fh++];
        if (fdepth[u] >= max_hops) continue;
        for (size_t i = 0; i < si.graph.edges.size(); ++i) {
            const auto& e = si.graph.edges[i];
            if (e.from != u) continue;
            if (fdepth[e.to] >= 0) continue;
            fdepth[e.to] = fdepth[u] + 1;
            fparent_node[e.to] = u;
            fparent_edge[e.to] = (int)i;
            fq.push_back(e.to);
        }
    }
    // Backward BFS from B along reverse edges.
    std::vector<int> bdepth(si.graph.nodes.size(), -1);
    std::vector<int> bparent_node(si.graph.nodes.size(), -1);
    std::vector<int> bparent_edge(si.graph.nodes.size(), -1);
    bdepth[B] = 0;
    std::vector<int> bq = {B};
    size_t bh = 0;
    while (bh < bq.size()) {
        int u = bq[bh++];
        if (bdepth[u] >= max_hops) continue;
        for (size_t i = 0; i < si.graph.edges.size(); ++i) {
            const auto& e = si.graph.edges[i];
            if (e.to != u) continue;
            if (bdepth[e.from] >= 0) continue;
            bdepth[e.from] = bdepth[u] + 1;
            bparent_node[e.from] = u;
            bparent_edge[e.from] = (int)i;
            bq.push_back(e.from);
        }
    }
    // Full path? A node reached by both frontiers.
    int best_meet = -1;
    int best_cost = INT32_MAX;
    for (int v = 0; v < (int)si.graph.nodes.size(); ++v) {
        if (fdepth[v] < 0 || bdepth[v] < 0) continue;
        int c = fdepth[v] + bdepth[v];
        if (c < best_cost) { best_cost = c; best_meet = v; }
    }
    if (best_meet >= 0 && best_meet != -1 && best_cost > 0) {
        // There IS a path; not really a gap.
        g.full_path_exists = true;
        return g;
    }
    // No path — find the pair (u, v) with u forward-reachable, v
    // backward-reachable, minimizing (fdepth[u] + bdepth[v]).
    int best_u = -1, best_v = -1, best_score = INT32_MAX;
    for (int u = 0; u < (int)si.graph.nodes.size(); ++u) {
        if (fdepth[u] < 0) continue;
        for (int v = 0; v < (int)si.graph.nodes.size(); ++v) {
            if (bdepth[v] < 0) continue;
            if (u == v) continue;
            int score = fdepth[u] + bdepth[v];
            if (score < best_score) {
                best_score = score;
                best_u = u; best_v = v;
            }
        }
    }
    if (best_u < 0 || best_v < 0) return g;

    g.from_side_node = best_u;
    g.to_side_node   = best_v;
    g.hops_from_a    = fdepth[best_u];
    g.hops_from_b_reverse = bdepth[best_v];
    // Reconstruct forward prefix.
    for (int cur = best_u; cur != A; cur = fparent_node[cur])
        g.forward_prefix.push_back(fparent_edge[cur]);
    std::reverse(g.forward_prefix.begin(), g.forward_prefix.end());
    // Reconstruct backward suffix (in forward order).
    for (int cur = best_v; cur != B; cur = bparent_node[cur])
        g.backward_suffix.push_back(bparent_edge[cur]);
    // backward_suffix edges are already in forward direction (each
    // edge stored is X→Y with X on the forward side of B).
    return g;
}

// ---------- 4. symmetry gaps ----------

struct SymmetryGap {
    int x, y;             // asserted edge X --r--> Y
    int x_mirror, y_mirror; // mirror partners of X and Y
    int r;                // the relation involved
    // Whether the mirror edge (x_mirror --r--> y_mirror) already exists.
    bool mirror_exists = false;
};

inline std::vector<SymmetryGap> find_symmetry_gaps(const SI& si) {
    // Build mirror map.
    std::unordered_map<int, int> mirror;
    for (const auto& e : si.graph.edges) {
        if (e.rel != REL_MIRROR_OF) continue;
        mirror[e.from] = e.to;
        mirror[e.to]   = e.from;   // symmetric
    }
    if (mirror.empty()) return {};
    std::vector<SymmetryGap> gaps;
    for (const auto& e : si.graph.edges) {
        // We only meaningfully symmetry-check "physical law" style
        // relations. Skip taxonomy edges (is_a, has, etc.) — those
        // don't get mirror partners in the physics sense.
        if (e.rel == REL_IS_A || e.rel == REL_HAS ||
            e.rel == REL_MIRROR_OF || e.rel == REL_FUNDAMENTAL ||
            e.rel == REL_PART_OF) continue;
        auto itx = mirror.find(e.from);
        auto ity = mirror.find(e.to);
        if (itx == mirror.end() || ity == mirror.end()) continue;
        int xm = itx->second, ym = ity->second;
        // Does the mirror edge exist?
        bool has_mirror = si.graph.has_edge(xm, ym, e.rel);
        if (!has_mirror)
            gaps.push_back({e.from, e.to, xm, ym, e.rel, false});
    }
    return gaps;
}

// ---------- 5. anomalies ----------

struct AnomalyNode {
    int node_id;
    int total_degree;
    int hops_to_fundamental;      // -1 if unreachable
    float anomaly_score;          // higher = more anomalous
};

inline std::vector<AnomalyNode>
find_anomalies(const SI& si, int max_hops = 6, int top_k = 8) {
    std::vector<int> in_deg(si.graph.nodes.size(), 0);
    std::vector<int> out_deg(si.graph.nodes.size(), 0);
    for (const auto& e : si.graph.edges) {
        ++out_deg[e.from];
        ++in_deg[e.to];
    }
    // Anomalies only make sense within a theory that has fundamentals.
    // We scope to undirected components that contain at least one
    // fundamental node. Everything outside such components is either
    // an unrelated ontology or a totally-ungrounded fragment.
    auto adj = undirected_adj(si);
    std::vector<int> comp((int)si.graph.nodes.size(), -1);
    std::vector<bool> comp_has_fund;
    for (int i = 0; i < (int)si.graph.nodes.size(); ++i) {
        if (comp[i] >= 0) continue;
        int cid = (int)comp_has_fund.size();
        comp_has_fund.push_back(false);
        std::vector<int> q; q.push_back(i);
        comp[i] = cid;
        while (!q.empty()) {
            int u = q.back(); q.pop_back();
            if (is_fundamental_node(si, u)) comp_has_fund[cid] = true;
            for (int v : adj[u]) {
                if (comp[v] < 0) { comp[v] = cid; q.push_back(v); }
            }
        }
    }
    // Precompute which nodes are pure category / meta labels (targets
    // of `is_fundamental` or `is_a`) — we don't want to surface
    // "fundamental_law" or "theory" as anomalies; they are the tag
    // categories, not the interesting variables.
    std::unordered_set<int> category_target;
    for (const auto& e : si.graph.edges) {
        if (e.rel == REL_FUNDAMENTAL) category_target.insert(e.to);
    }
    std::vector<AnomalyNode> out;
    for (int i = 0; i < (int)si.graph.nodes.size(); ++i) {
        // Skip if this node's component has no fundamental at all —
        // that means the whole component is a different theory /
        // ontology and we can't fairly measure "anomaly" there.
        if (!comp_has_fund[comp[i]]) continue;
        if (category_target.count(i)) continue;   // meta label, not a physics variable
        // Skip fundamental-tagged nodes themselves (they are grounded
        // by definition).
        if (is_fundamental_node(si, i)) continue;
        // Skip nodes that look like abstract theory containers by name.
        const std::string& lbl = si.graph.nodes[i].label;
        if (lbl == "theory" || lbl == "law" || lbl == "axiom" ||
            lbl == "invariant" || lbl == "conservation_law" ||
            lbl == "fundamental_law")
            continue;
        int deg = in_deg[i] + out_deg[i];
        if (deg < 2) continue;   // low-degree nodes aren't "confused zones"
        // BFS to nearest fundamental.
        std::vector<int> depth(si.graph.nodes.size(), -1);
        depth[i] = 0;
        std::vector<int> q = {i};
        size_t head = 0;
        int hit = -1;
        while (head < q.size()) {
            int u = q[head++];
            if (depth[u] >= max_hops) continue;
            for (const auto& e : si.graph.edges) {
                if (e.from != u) continue;
                if (depth[e.to] >= 0) continue;
                depth[e.to] = depth[u] + 1;
                if (is_fundamental_node(si, e.to)) { hit = depth[e.to]; break; }
                q.push_back(e.to);
            }
            if (hit >= 0) break;
        }
        // Anomaly score = degree * (unreachable ? 1.0 : hit/max_hops).
        float remoteness = (hit < 0) ? 1.0f
                                     : (float)hit / (float)max_hops;
        float score = (float)deg * remoteness;
        if (score <= 0.0f) continue;
        out.push_back({i, deg, hit, score});
    }
    std::sort(out.begin(), out.end(),
              [](const AnomalyNode& a, const AnomalyNode& b){
                  return a.anomaly_score > b.anomaly_score;
              });
    if ((int)out.size() > top_k) out.resize(top_k);
    return out;
}

// ---------- rendering helpers ----------

// English-ify a label (only for local use — router.hpp has its own).
inline std::string inf_english(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (char c : s) out.push_back(c == '_' ? ' ' : c);
    return out;
}

} // namespace unified
