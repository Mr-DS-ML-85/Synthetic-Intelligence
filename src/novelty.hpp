// novelty.hpp — Discovery queries over the mechanistic graph.
//
// This is what makes the engine a discovery tool rather than a
// dictionary. The queries here do not retrieve known facts; they
// enumerate implicit connections and rank them as hypotheses.
//
// Three core queries:
//
//   1. find_paths(A, B, K)
//        All edge-index sequences from A to B of length <= K, sorted
//        shortest first. Used for "how does insulin_signaling reach
//        glucose_uptake?" style trace queries.
//
//   2. shared_targets(A, B)
//        Nodes X such that A --*--> X AND B --*--> X. The intersection
//        query the L9 paper calls "cross-domain isomorphism": if two
//        concepts point at the same downstream node, that node is a
//        mechanism they share.
//
//   3. repurposing_candidates(disease, mechanism_rel)
//        For every drug X such that some known treatment Y also
//        satisfies:
//                (Y --treats--> disease)  AND
//                (Y --mechanism_rel--> shared_target)  AND
//                (X --mechanism_rel--> shared_target)  AND
//                NOT (X --treats--> disease)
//        emit X as a mechanism-based *hypothesis*, ranked by how many
//        shared targets it hits. This is the drug-repositioning query.
//
// Every result carries an explicit "this is a hypothesis, not a proof"
// marker for the discourse renderer, so we never let a candidate leak
// out as an assertion.

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

// ---------- Path enumeration ----------

struct DiscoveryPath {
    std::vector<int> edges;   // sequence of edge indices
    int length() const { return (int)edges.size(); }
};

// Enumerate paths from `a` to `b` of length <= max_hops. ITERATIVE
// DEEPENING over a depth-first walk: depth 1 first, then 2, ... The
// previous single-pass DFS burned the `cap` budget on long chains
// before any short path was seen, and short paths are the ones every
// consumer (bisociation, logic gaps) actually wants. Deep paths are
// still reachable — deepening just orders the work by usefulness.
inline std::vector<DiscoveryPath>
find_paths(const SI& si, const std::string& a, const std::string& b,
           int max_hops = 6, int cap = 32) {
    std::vector<DiscoveryPath> out;
    auto it_a = si.graph.by_label.find(a);
    auto it_b = si.graph.by_label.find(b);
    if (it_a == si.graph.by_label.end() || it_b == si.graph.by_label.end())
        return out;
    int start = it_a->second, goal = it_b->second;
    if (start == goal) return out;

    std::vector<char> on_path((int)si.graph.nodes.size(), 0);
    std::vector<int> cur_edges;

    // Outgoing edges per node from the graph's own index — O(1) setup.
    const auto& outgoing = si.graph.out_adj;

    std::function<void(int,int)> dfs = [&](int u, int hops_left) {
        if ((int)out.size() >= cap) return;
        if (u == goal) {
            out.push_back({cur_edges});
            return;
        }
        if (hops_left <= 0) return;
        if (u < 0 || u >= (int)outgoing.size()) return;   // index guard
        on_path[u] = 1;
        for (int e_idx : outgoing[u]) {
            int v = si.graph.edges[e_idx].to;
            if (on_path[v]) continue;
            cur_edges.push_back(e_idx);
            dfs(v, hops_left - 1);
            cur_edges.pop_back();
            if ((int)out.size() >= cap) break;
        }
        on_path[u] = 0;
    };
    for (int depth = 1; depth <= max_hops && (int)out.size() < cap; ++depth)
        dfs(start, depth);

    std::sort(out.begin(), out.end(),
              [](const DiscoveryPath& x, const DiscoveryPath& y){
                  return x.length() < y.length();
              });
    return out;
}

// ---------- Shared targets (intersection query) ----------

struct SharedTarget {
    int target_node;
    int rel_from_a;    // rel used by a -> target_node
    int rel_from_b;    // rel used by b -> target_node
};

inline std::vector<SharedTarget>
shared_targets(const SI& si, const std::string& a, const std::string& b) {
    std::vector<SharedTarget> out;
    auto it_a = si.graph.by_label.find(a);
    auto it_b = si.graph.by_label.find(b);
    if (it_a == si.graph.by_label.end() || it_b == si.graph.by_label.end())
        return out;
    int na = it_a->second, nb = it_b->second;
    // Collect all a's targets.
    std::unordered_map<int, int> a_targets;  // target -> rel
    for (const auto& e : si.graph.edges) {
        if (e.from == na) a_targets[e.to] = e.rel;
    }
    // Walk b's outgoing edges; any hit is a shared target.
    for (const auto& e : si.graph.edges) {
        if (e.from != nb) continue;
        auto it = a_targets.find(e.to);
        if (it != a_targets.end())
            out.push_back({e.to, it->second, e.rel});
    }
    return out;
}

// ---------- Drug repurposing (abductive candidates) ----------
//
// The drug-repositioning query. For each known treatment `known` of
// `disease` via mechanism_rel (e.g. inhibits some protein target),
// scan the graph for any node X that also satisfies the mechanism
// relation with the same target — and does NOT already have a
// treats-edge to the disease. Return X as a candidate with the
// evidence chain that motivated the hypothesis.

struct DrugCandidate {
    int candidate_node;                       // proposed drug/molecule
    std::vector<int> shared_mechanism_targets;// nodes both act on
    std::vector<int> reference_treatments;    // known treatments that share
    int score;                                // legacy integer count
    float strength;                           // weighted score in [0, ~5]
    // For each shared mechanism target, how many known treatments also
    // hit it (endorsement count). Length matches shared_mechanism_targets.
    std::vector<int> endorsements_per_target;
    bool has_direct_treats;                   // if true, this is not novel
};

// Is X a plausible therapeutic entity? We accept nodes explicitly typed
// as drugs / molecules / natural products / compounds. This filters out
// enzymes / receptors / pathway intermediates that share a mechanism
// edge by chance but are not something you'd administer to a patient.
//
// The type set is a PARAMETER, not a hardcoded string wall — the same
// query shape works for any domain once the caller names its entity
// types (e.g. {"framework", "library"} for a code graph).
inline const std::vector<std::string>& default_therapeutic_types() {
    static const std::vector<std::string> t = {
        "drug", "molecule", "compound", "natural_product",
        "small_molecule", "biologic", "peptide"
    };
    return t;
}

inline bool is_therapeutic_entity(
        const SI& si, int X,
        const std::vector<std::string>& types = default_therapeutic_types()) {
    for (const auto& e : si.graph.edges) {
        if (e.from != X || e.rel != REL_IS_A) continue;
        const std::string& lbl = si.graph.nodes[e.to].label;
        for (const auto& t : types) if (lbl == t) return true;
    }
    return false;
}

// For every X such that X shares at least one `mechanism_rel` target
// with a known `treats` of `disease`, emit a candidate. The candidate
// is marked non-novel if a direct treats edge already exists.
//
// If `require_therapeutic` is true, X must additionally be typed as a
// drug / molecule / compound. Turn it off to see *all* mechanism
// analogues (useful for basic-research questions).
inline std::vector<DrugCandidate>
repurposing_candidates(const SI& si, const std::string& disease,
                       int mechanism_rel = REL_INHIBITS,
                       bool require_therapeutic = true) {
    std::vector<DrugCandidate> out;
    auto it_d = si.graph.by_label.find(disease);
    if (it_d == si.graph.by_label.end()) return out;
    int d = it_d->second;

    // 1) Collect known treatments of disease.
    std::vector<int> known;
    for (const auto& e : si.graph.edges) {
        if (e.rel == REL_TREATS && e.to == d) known.push_back(e.from);
    }
    if (known.empty()) return out;

    // 2) For each known treatment, collect its mechanism targets.
    //    known_target[t] = { treatment_a, treatment_b, ... }
    std::unordered_map<int, std::vector<int>> known_target;
    for (int t : known) {
        for (const auto& e : si.graph.edges) {
            if (e.from == t && e.rel == mechanism_rel)
                known_target[e.to].push_back(t);
        }
    }
    if (known_target.empty()) return out;

    // 3) Scan every node X for mechanism-rel edges that hit a known
    //    target — X is a candidate.
    std::unordered_map<int, DrugCandidate> agg;
    for (const auto& e : si.graph.edges) {
        if (e.rel != mechanism_rel) continue;
        auto it = known_target.find(e.to);
        if (it == known_target.end()) continue;
        int X = e.from;
        // Skip the known treatments themselves.
        bool is_known = false;
        for (int k : known) if (k == X) { is_known = true; break; }
        if (is_known) continue;
        // Optionally require X to be an administerable therapeutic
        // entity (drug / molecule / compound / natural_product). This
        // stops the search from surfacing enzymes or receptors that
        // happen to share a mechanism edge but are not drugs.
        if (require_therapeutic && !is_therapeutic_entity(si, X)) continue;
        auto& c = agg[X];
        c.candidate_node = X;
        c.shared_mechanism_targets.push_back(e.to);
        for (int t : it->second) c.reference_treatments.push_back(t);
    }
    // 4) Note which candidates already have a direct treats edge — those
    //    are known, not novel. Compute weighted strength.
    //
    // Strength model (Weighted Path Scoring):
    //   For each shared target t:
    //     endorsements(t) = # of known-treatments that also hit t
    //     w(t)            = 1 + 0.5 * (endorsements(t) - 1)
    //   base                = sum_t w(t)
    //   independence_bonus  = 1 + 0.15 * (n_targets - 1)
    //   strength            = base * independence_bonus
    //
    // A candidate hitting a single, singly-endorsed target scores 1.0.
    // A candidate hitting two singly-endorsed targets scores 2.30.
    // A candidate hitting one target endorsed by three treatments
    // (strong shared axis) scores 2.0. Hitting two multi-endorsed
    // targets scales past 3.5 — a strong lead.
    for (auto& kv : agg) {
        int X = kv.first;
        for (const auto& e : si.graph.edges) {
            if (e.from == X && e.rel == REL_TREATS && e.to == d) {
                kv.second.has_direct_treats = true; break;
            }
        }
        auto& v = kv.second.shared_mechanism_targets;
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
        auto& refs = kv.second.reference_treatments;
        std::sort(refs.begin(), refs.end());
        refs.erase(std::unique(refs.begin(), refs.end()), refs.end());
        kv.second.score = (int)v.size();

        // Endorsements per target: for each shared target t, count how
        // many known treatments in `known_target[t]` are ALSO known
        // treatments of the disease (i.e., in `known`).
        kv.second.endorsements_per_target.clear();
        float base = 0.0f;
        for (int t : v) {
            auto it_t = known_target.find(t);
            int endorsers = (it_t == known_target.end())
                            ? 0 : (int)it_t->second.size();
            kv.second.endorsements_per_target.push_back(endorsers);
            base += 1.0f + 0.5f * std::max(0, endorsers - 1);
        }
        float independence_bonus = 1.0f
            + 0.15f * (float)std::max(0, (int)v.size() - 1);
        kv.second.strength = base * independence_bonus;
        out.push_back(kv.second);
    }
    // 5) Rank by strength descending; novel candidates before known.
    std::sort(out.begin(), out.end(),
              [](const DrugCandidate& a, const DrugCandidate& b){
                  if (a.has_direct_treats != b.has_direct_treats)
                      return !a.has_direct_treats;
                  return a.strength > b.strength;
              });
    return out;
}

// ---------- Discourse rendering for discovery output ----------

inline std::string render_path(const SI& si, const DiscoveryPath& p) {
    if (p.edges.empty()) return "(empty path)";
    std::string out;
    for (size_t i = 0; i < p.edges.size(); ++i) {
        const Edge& e = si.graph.edges[p.edges[i]];
        if (i == 0) out += si.graph.nodes[e.from].label;
        out += " --" + std::string(rel_name_ext(e.rel)) + "--> ";
        out += si.graph.nodes[e.to].label;
    }
    return out;
}

inline std::string render_shared(const SI& si,
                                 const std::string& a,
                                 const std::string& b,
                                 const std::vector<SharedTarget>& sh) {
    std::string out;
    if (sh.empty()) {
        out = "I found no shared downstream targets between " + a
              + " and " + b + " in the current graph.";
        return out;
    }
    out = "Intersection of " + a + " and " + b + " — I found "
        + std::to_string(sh.size()) + " shared target"
        + (sh.size() == 1 ? "" : "s") + ": ";
    for (size_t i = 0; i < sh.size(); ++i) {
        const std::string& t = si.graph.nodes[sh[i].target_node].label;
        out += t + " (" + a + " " + rel_name_ext(sh[i].rel_from_a)
             + ", " + b + " " + rel_name_ext(sh[i].rel_from_b) + ")";
        if (i + 1 < sh.size()) out += "; ";
    }
    out += ".";
    return out;
}

inline std::string render_repurposing(const SI& si,
                                      const std::string& disease,
                                      const std::vector<DrugCandidate>& cands,
                                      int mechanism_rel) {
    std::string out;
    if (cands.empty()) {
        out = "For " + disease + " I could not find any candidate "
              "molecules that share a " + rel_name_ext(mechanism_rel)
            + " mechanism with a known treatment. Either the disease "
              "has no known treatment in the graph, or no other "
              "molecule targets the same mechanism.";
        return out;
    }
    out = "Mechanism-based candidates for " + disease + " — these are "
          "HYPOTHESES generated by shared-mechanism abduction, not "
          "proven treatments. Ranked by how many downstream targets "
          "each shares with an existing treatment (via "
        + std::string(rel_name_ext(mechanism_rel)) + "):\n";
    for (size_t i = 0; i < cands.size(); ++i) {
        const auto& c = cands[i];
        out += "  " + std::to_string(i + 1) + ". "
             + si.graph.nodes[c.candidate_node].label;
        out += " [score " + std::to_string(c.score) + "]";
        if (c.has_direct_treats) out += " (already known to treat)";
        out += "\n     shared mechanism target"
             + std::string(c.shared_mechanism_targets.size() == 1 ? "" : "s")
             + ": ";
        for (size_t j = 0; j < c.shared_mechanism_targets.size(); ++j) {
            out += si.graph.nodes[c.shared_mechanism_targets[j]].label;
            if (j + 1 < c.shared_mechanism_targets.size()) out += ", ";
        }
        out += "\n     shares with known treatment"
             + std::string(c.reference_treatments.size() == 1 ? "" : "s")
             + ": ";
        for (size_t j = 0; j < c.reference_treatments.size(); ++j) {
            out += si.graph.nodes[c.reference_treatments[j]].label;
            if (j + 1 < c.reference_treatments.size()) out += ", ";
        }
        out += "\n";
    }
    out += "Each candidate above is a testable hypothesis — the graph "
           "implies a mechanism, not a clinical outcome. Verify in a lab, "
           "not from this output.";
    return out;
}

} // namespace unified
