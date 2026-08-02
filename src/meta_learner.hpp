// meta_learner.hpp — rule induction & backward chaining for Layer 2.
//
// Implements the parts of research-paper/synthetic-intelligence.md §5.3
// that let the thinker index grow over the system's lifetime:
//
//   §5.3.3  Rule induction — track derivation sub-chains, register as
//           macro-rules when they cross an MDL threshold.
//   §5.3.4  Verifier — every candidate answer edge is re-checked against
//           the current rule set before Layer 3 emits a translation.
//
// Backward chaining is in unified.hpp (SI::ask_backward). Rule induction
// and verification live here because they operate on the historical
// derivation trace, which is a separate object from the reasoning graph.

#pragma once

#include "unified.hpp"

#include <cstdio>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

// --------------------------------------------------------------------
// A "sub-chain" is a linear sequence of rule applications observed
// during a derivation. Two sub-chains are the same when they match
// the same abstract rule-sequence (rule_a, rule_b, ...) regardless of
// which concrete edges they were applied to.
//
// When a sub-chain of length L crosses the MDL threshold — the number
// of times it has been observed times its bit-cost saved per use — it
// is registered as a new macro-rule that composes L primitive rules
// into a single one-step edge derivation.
// --------------------------------------------------------------------
struct SubChainKey {
    std::vector<int> rule_ids;                     // sequence of primitive rule indices
    bool operator==(const SubChainKey& o) const {
        return rule_ids == o.rule_ids;
    }
};

struct SubChainKeyHash {
    size_t operator()(const SubChainKey& k) const {
        size_t h = 1469598103934665603ULL;
        for (int r : k.rule_ids) {
            h ^= (size_t)(r + 0x9E3779B97F4A7C15ULL);
            h *= 1099511628211ULL;
        }
        return h;
    }
};

struct SubChainStat {
    int  count = 0;
    int  length = 0;
    bool promoted = false;   // already registered as a macro-rule?
    int  macro_rule_idx = -1;
};

// A macro-rule is a Rule whose "cost" is length-1 primitives collapsed
// to one step. We reuse the transitive-composition slot: applying the
// macro rule directly produces the edge that the sub-chain would have.
// Concretely, we compose the sub-chain's rule sequence into a
// derived-relation pair (rel_ab, rel_bc, rel_ac) using the first and
// last rules' relation types.
struct MacroRule {
    std::string name;            // e.g. "macro[transitive_is_a x class_has_flows_down]"
    std::vector<int> component_rule_ids;
    int rel_ab;
    int rel_bc;
    int rel_ac;
    int uses = 0;
};

// --------------------------------------------------------------------
// The MetaLearner.
//
// Attach to an SI engine. After every successful query it observes
// the derivation trace, updates sub-chain counts, and promotes any
// sub-chain that has crossed the MDL threshold to a macro-rule
// registered on the SI engine.
// --------------------------------------------------------------------
struct MetaLearner {
    SI* engine = nullptr;
    std::unordered_map<SubChainKey, SubChainStat, SubChainKeyHash> chain_stats;
    std::vector<MacroRule> macros;

    // MDL bookkeeping.
    //   bit_cost_per_use  = number of Rule-application bits saved per query
    //                       by using a macro instead of the underlying chain.
    //   promotion_threshold = total bits saved required before we register.
    // The default choices below are conservative: a sub-chain of length 2
    // must fire at least 3 times before it's compressed into a macro,
    // length 3 requires 2 firings, etc.
    float bits_saved_per_use(int chain_length) const {
        return (float)std::log2((double)std::max(1, chain_length));
    }
    float promotion_threshold = 2.0f;  // bits

    // Called by SI after every successful ask() with the derivation trace.
    void observe_trace(const std::vector<int>& derivation_edges) {
        if (!engine) return;
        // Filter to only the rule-derived edges (premises are rule_id == -1).
        std::vector<int> rule_ids;
        rule_ids.reserve(derivation_edges.size());
        for (int e : derivation_edges) {
            const Edge& ee = engine->graph.edges[e];
            if (ee.derived_by_rule >= 0)
                rule_ids.push_back(ee.derived_by_rule);
        }
        // Enumerate contiguous sub-chains of length 2..min(N, 6).
        int N = (int)rule_ids.size();
        int max_len = std::min(N, 6);
        for (int len = 2; len <= max_len; ++len) {
            for (int i = 0; i + len <= N; ++i) {
                SubChainKey key;
                key.rule_ids.assign(rule_ids.begin() + i, rule_ids.begin() + i + len);
                auto& stat = chain_stats[key];
                stat.count += 1;
                stat.length = len;
                if (!stat.promoted) {
                    float saved = stat.count * bits_saved_per_use(len);
                    if (saved >= promotion_threshold) {
                        try_promote(key, stat);
                    }
                }
            }
        }
    }

    // Try to compile a sub-chain into a macro-rule and register it with SI.
    void try_promote(const SubChainKey& key, SubChainStat& stat) {
        // The chain is (rule_a, rule_b, ..., rule_k). We compose by
        // matching consecutive rules' rel_ab -> rel_ac chains.
        if (key.rule_ids.empty()) return;
        int first = key.rule_ids.front();
        int last  = key.rule_ids.back();
        if (first < 0 || first >= (int)engine->rules.size()) return;
        if (last  < 0 || last  >= (int)engine->rules.size()) return;

        // The composed rule takes the antecedent shape (rel_ab of the first
        // primitive), the intermediate shape (rel_bc of the last primitive),
        // and produces the conclusion of the last primitive.
        int rel_ab = engine->rules[first].rel_ab;
        int rel_bc = engine->rules[last].rel_bc;
        int rel_ac = engine->rules[last].rel_ac;

        std::string name = "macro[";
        for (size_t i = 0; i < key.rule_ids.size(); ++i) {
            if (i) name += " x ";
            name += engine->rules[key.rule_ids[i]].name;
        }
        name += "]";

        // Register on the engine. Cost is 1 (macro is a single-step rule).
        int old_size = (int)engine->rules.size();
        engine->add_rule(name, rel_ab, rel_bc, rel_ac, /*spawn=*/false, /*cost=*/1.0f);
        stat.promoted = true;
        stat.macro_rule_idx = old_size;
        macros.push_back({name, key.rule_ids, rel_ab, rel_bc, rel_ac, 0});
    }

    void dump(FILE* f = stdout) const {
        std::fprintf(f, "MetaLearner: %zu sub-chain patterns tracked, %zu macros registered\n",
                     chain_stats.size(), macros.size());
        for (const auto& m : macros) {
            std::fprintf(f, "  macro '%s'  (composes %zu primitives)\n",
                         m.name.c_str(), m.component_rule_ids.size());
        }
    }
};

// --------------------------------------------------------------------
// Verifier — §5.3.4.
//
// Walks a candidate answer edge's parent chain back to premises,
// re-applies each rule's pattern, and confirms the derivation still
// type-checks against the current rule set.  Failed verification means
// Layer 3 must emit "unknown", not the (possibly stale) derived answer.
// --------------------------------------------------------------------
inline bool verify_edge(const SI& si, int edge_idx) {
    if (edge_idx < 0 || edge_idx >= (int)si.graph.edges.size()) return false;
    const Edge& e = si.graph.edges[edge_idx];
    if (e.derived_by_rule < 0) return true;  // a premise is trivially valid
    if (e.derived_by_rule >= (int)si.rules.size()) return false;
    const Rule& r = si.rules[e.derived_by_rule];
    if (e.parent_a < 0 || e.parent_b < 0) return false;
    const Edge& pa = si.graph.edges[e.parent_a];
    const Edge& pb = si.graph.edges[e.parent_b];
    // Type check: parent shapes must match the rule pattern, and the
    // conclusion must be exactly what the rule produces.
    if (pa.rel != r.rel_ab) return false;
    if (pb.rel != r.rel_bc) return false;
    if (e.rel  != r.rel_ac) return false;
    if (pa.to  != pb.from) return false;
    if (e.from != pa.from) return false;
    if (e.to   != pb.to)   return false;
    // Recurse.
    return verify_edge(si, e.parent_a) && verify_edge(si, e.parent_b);
}

// --------------------------------------------------------------------
// Thinker index measurement.
//
// C(S)/R(S) as defined in §3.4:
//   R(S) = fraction of queries answered by a length-1 derivation (premise)
//   C(S) = expected derivation length across the query set
// A high C/R means the system is composing; a low C/R means it is retrieving.
// --------------------------------------------------------------------
struct ThinkerIndex {
    int total_queries = 0;
    int retrieval_queries = 0;
    double sum_length = 0.0;

    void record(const SI::QueryResult& r) {
        if (!r.holds) return;   // unknown queries do not count
        total_queries++;
        int L = (int)r.proof.size();
        sum_length += (double)L;
        if (L <= 1) retrieval_queries++;
    }

    float composition_rate() const {
        return total_queries ? (float)(sum_length / total_queries) : 0.f;
    }
    float retrieval_rate() const {
        return total_queries ? (float)retrieval_queries / total_queries : 0.f;
    }
    float thinker_index() const {
        float R = retrieval_rate();
        return composition_rate() / std::max(R + 1e-6f, 1e-6f);
    }
    void report(FILE* f = stdout) const {
        std::fprintf(f,
            "thinker index: C=%.3f  R=%.3f  C/R=%.3f  (%d queries, %d retrievals)\n",
            composition_rate(), retrieval_rate(), thinker_index(),
            total_queries, retrieval_queries);
    }
};

} // namespace unified
