// analogy.hpp — L9 Layer 6: Analogical Mapping.
//
// Implements the "structure mapping" of research-paper/l9.md §3 (L6)
// and its formal novelty potential
//
//     Phi(A, B) = I(A, B) / exp(-k * d(A, B))
//
// A concept is represented by its outgoing-edge signature: the multiset
// of (relation, target_class) tuples. Two concepts are structurally
// analogous when their signatures overlap substantially, even if they
// are semantically distant on the graph. High isomorphism I combined
// with high shortest-path distance d yields a high novelty potential
// Phi — this is the L9 paper's "candidate breakthrough" flag.
//
// Downstream use: for any (A, B) with high Phi, we can transfer edges
// from A onto B as hypotheses. Maxwell's fluid→electric analogy is the
// paradigmatic case: pressure↔voltage, flow↔current, once the shared
// structure is found, the equations transfer.

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace unified {

// The multiset signature of a node: (relation, endpoint_label, direction)
// triples. Using endpoint *label* rather than endpoint id is deliberate —
// analogy works cross-domain, so we abstract over the specific node.
// The direction is part of the signature: a node's ROLE is defined both
// by what it points at (outgoing) and by what points at it (incoming).
// A source with only outgoing links and a sink with only incoming links
// are not analogous even when their edge multisets coincide.
struct NodeSignature {
    struct Feature {
        int rel;
        std::string label;
        int dir;             // 0 = outgoing, 1 = incoming
        bool operator<(const Feature& o) const {
            if (rel != o.rel) return rel < o.rel;
            if (label != o.label) return label < o.label;
            return dir < o.dir;
        }
        bool operator==(const Feature& o) const {
            return rel == o.rel && label == o.label && dir == o.dir;
        }
    };
    std::vector<Feature> features;

    static NodeSignature of(const SI& si, int node_id) {
        NodeSignature sig;
        if (node_id < 0 || node_id >= (int)si.graph.nodes.size()) return sig;
        // Walk the graph's adjacency indexes, not the edge list — this
        // is O(deg) per node instead of O(E), and it reaches BOTH sides.
        if (node_id < (int)si.graph.out_adj.size()) {
            for (int ei : si.graph.out_adj[(size_t)node_id]) {
                const Edge& e = si.graph.edges[(size_t)ei];
                sig.features.push_back({e.rel, si.graph.nodes[e.to].label, 0});
            }
        }
        if (node_id < (int)si.graph.in_adj.size()) {
            for (int ei : si.graph.in_adj[(size_t)node_id]) {
                const Edge& e = si.graph.edges[(size_t)ei];
                sig.features.push_back({e.rel, si.graph.nodes[e.from].label, 1});
            }
        }
        std::sort(sig.features.begin(), sig.features.end());
        return sig;
    }
    size_t size() const { return features.size(); }
};

// Multiset intersection cardinality between two signatures.
inline int signature_overlap(const NodeSignature& a, const NodeSignature& b) {
    int n = 0;
    size_t i = 0, j = 0;
    while (i < a.features.size() && j < b.features.size()) {
        if (a.features[i] == b.features[j])       { ++n; ++i; ++j; }
        else if (a.features[i] < b.features[j])   { ++i; }
        else                                       { ++j; }
    }
    return n;
}

// Jaccard-style structural isomorphism score in [0, 1].
inline float isomorphism_score(const NodeSignature& a, const NodeSignature& b) {
    if (a.features.empty() && b.features.empty()) return 0.0f;
    int overlap = signature_overlap(a, b);
    int union_size = (int)a.features.size() + (int)b.features.size() - overlap;
    return union_size > 0 ? (float)overlap / (float)union_size : 0.0f;
}

// Shortest-path hop count between two nodes (BFS, no rel filter),
// -1 if unreachable within `cap`.
inline int hop_distance(const SI& si, int a, int b, int cap = 12) {
    if (a == b) return 0;
    std::vector<int> depth((int)si.graph.nodes.size(), -1);
    depth[a] = 0;
    std::vector<int> q; q.push_back(a);
    size_t head = 0;
    while (head < q.size()) {
        int u = q[head++];
        if (depth[u] >= cap) continue;
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

// The L9 novelty potential. Semantically distant (large d) *and*
// structurally aligned (large I) ⇒ large Phi ⇒ Candidate Breakthrough.
// If b is unreachable, we treat d as an infinite floor (very high Phi).
inline float novelty_potential(const SI& si, int A, int B, float k = 0.15f) {
    NodeSignature sa = NodeSignature::of(si, A);
    NodeSignature sb = NodeSignature::of(si, B);
    float I = isomorphism_score(sa, sb);
    int d = hop_distance(si, A, B, /*cap=*/24);
    if (d < 0) d = 24;   // treat unreachable as very-far
    // Phi(A,B) = I / exp(-k d)  = I * exp(k d)
    return I * std::exp(k * (float)d);
}

// Find analogues of a source node: rank all other nodes by
// isomorphism_score, return top-K.
struct AnalogueMatch {
    int node_id;
    float iso_score;
    int hop_distance;
    float novelty_phi;
};

inline std::vector<AnalogueMatch>
find_analogues(const SI& si, const std::string& source_label,
               int top_k = 8, float min_iso = 0.15f) {
    std::vector<AnalogueMatch> out;
    auto it = si.graph.by_label.find(source_label);
    if (it == si.graph.by_label.end()) return out;
    int A = it->second;
    NodeSignature sa = NodeSignature::of(si, A);
    if (sa.features.empty()) return out;
    for (int B = 0; B < (int)si.graph.nodes.size(); ++B) {
        if (B == A) continue;
        NodeSignature sb = NodeSignature::of(si, B);
        if (sb.features.empty()) continue;
        float I = isomorphism_score(sa, sb);
        if (I < min_iso) continue;
        int d = hop_distance(si, A, B, 24);
        float phi = I * std::exp(0.15f * (float)(d < 0 ? 24 : d));
        out.push_back({B, I, d, phi});
    }
    std::sort(out.begin(), out.end(),
              [](const AnalogueMatch& a, const AnalogueMatch& b){
                  return a.novelty_phi > b.novelty_phi;
              });
    if ((int)out.size() > top_k) out.resize(top_k);
    return out;
}

// Rendering.
inline std::string render_analogues(const SI& si, const std::string& source,
                                     const std::vector<AnalogueMatch>& an) {
    std::string out;
    if (an.empty()) {
        out = "No structural analogues found for " + source
            + " above the isomorphism threshold.";
        return out;
    }
    out = "Structural analogues of " + source
        + " ranked by novelty potential Phi = I * exp(k * d):\n";
    for (size_t i = 0; i < an.size(); ++i) {
        const auto& m = an[i];
        char line[256];
        std::snprintf(line, sizeof(line),
            "  %zu. %s  [I=%.2f, hop_d=%d, Phi=%.2f]%s\n",
            i + 1, si.graph.nodes[m.node_id].label.c_str(),
            m.iso_score, m.hop_distance, m.novelty_phi,
            (m.hop_distance < 0 ? " (unreachable — pure structural match)" : ""));
        out += line;
    }
    out += "High Phi = semantically distant + structurally aligned. "
           "These are candidate cross-domain transfers — the L9 paper's "
           "'analogical mapping' layer. Verify each transfer explicitly "
           "before treating it as knowledge.";
    return out;
}

} // namespace unified
