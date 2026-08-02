// primitives.hpp — the nine reasoning primitives from
// research-paper/synthetic-intelligence.md §4.4.
//
// Each primitive is a distinct rule type with its own pattern-matcher.
// Combined they close the composition space that the tri-layer engine
// exposes:
//
//   1. DEDUCTION       — from (P) and (P → Q), derive Q
//   2. TRANSITIVE      — from (A r B) and (B r C), derive (A r C)
//                        (special case of deduction on a chain rule)
//   3. INHERITANCE     — from (X is_a C) and (C ANY_REL Y), derive
//                        (X ANY_REL Y) — class properties flow to members
//   4. ABDUCTION       — from goal G and rule (P → G), hypothesize P
//                        (backward-only; guarded so we don't fabricate)
//   5. NEGATION        — contrapositive: from (¬Q) and (P → Q), derive (¬P)
//                        and duplicate elimination
//   6. CASE_ANALYSIS   — from (X or_of {Y,Z}) and (Y → R) and (Z → R),
//                        derive (X → R)  (disjunction elimination)
//   7. INDUCTION       — from many (X_i is_a C) each with property (has P),
//                        derive (C has P) — schematic generalization
//   8. ANALOGY         — structure-preserving map between subgraphs
//                        (isomorphism-guided edge transfer)
//   9. RECURSION       — implicit: any rule may apply to its own output
//                        (guaranteed by the fixpoint loop in reason())
//
// Combined with the meta-learner (rule induction, unified.hpp §5.3.3),
// which is the operational analogue of RECURSION at the rule-set level,
// the system is closed under the composition operation of §3.4.

#pragma once

#include "unified.hpp"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

// -----------------------------------------------------------------------
// Additional relation IDs for the primitives that need them.
// (Keep them numerically after the base ones in unified.hpp so the two
// files can share the same rel_name lookup on the base range.)
// -----------------------------------------------------------------------
enum ExtRelId : int {
    REL_NOT      = 6,   // "socrates NOT is_a stone"           (unary; stored as edge X --NOT--> P)
    REL_OR       = 7,   // "wet_or_rainy OR (rainy, wet)"      (X --OR--> disjunct)
    REL_HYPOTH   = 8,   // "hypothesised: raining implies..."  (dashed edge tag)

    // --- Mechanistic relations for scientific / medical ontologies. ---
    // These convert the engine from a taxonomist ("X is_a Y") into a
    // mechanistic reasoner ("X inhibits Y", "Y is part of pathway Z").
    // Novelty search — drug repurposing, mechanism analogy, pathway
    // rerouting — operates over these edges, not over is_a.
    REL_BINDS_TO         = 9,   // molecule X binds molecular target Y
    REL_INHIBITS         = 10,  // X reduces activity of Y
    REL_ACTIVATES        = 11,  // X increases activity of Y
    REL_TREATS           = 12,  // drug/intervention X treats condition Y
    REL_CAUSES_DISEASE   = 13,  // gene/protein/agent X causes disease Y
    REL_PART_OF_PATHWAY  = 14,  // node X participates in signalling pathway Y
    REL_EXPRESSED_IN     = 15,  // gene/protein X is expressed in tissue/cell Y
    REL_METABOLIZES      = 16,  // enzyme X metabolises substrate Y
    REL_UPREGULATES      = 17,  // X increases abundance of Y (transcription-level)
    REL_DOWNREGULATES    = 18,  // X decreases abundance of Y

    // --- Symbolic / physics relations for the inference engine. ---
    // These make the graph store equations and symmetries, not just
    // definitions. `is_proportional_to` encodes a physical law
    // (curl_E is_proportional_to dB_dt). `mirror_of` encodes
    // symmetry-partner variables (curl_E mirror_of curl_B; dE_dt
    // mirror_of dB_dt) so the anomaly detector can spot incomplete
    // symmetries — the Maxwell displacement-current move.
    REL_PROPORTIONAL_TO  = 19,   // X is_proportional_to Y  (X ∝ Y)
    REL_MIRROR_OF        = 20,   // X mirror_of Y  (symmetry partners)
    REL_DERIVES_FROM     = 21,   // X derives_from Y (dependent variable)
    REL_FUNDAMENTAL      = 22,   // X is_fundamental (tag: axiom of the theory)
    REL_TRIGGERED_BY     = 23,   // intent_X triggered_by "some_word" — intent lexicon

    // ---- Code-structure relations (mechanically extracted from ASTs) ----
    // The substrate for code construction. Unlike natural-language
    // relations these form a small CLOSED set, are extractable with
    // zero ML from any source tree, and carry a hard correctness
    // oracle (the compiler). REL_CALLS and REL_PRECEDES are strongly
    // directional and therefore take a CPSB gauge offset — dataflow
    // order becomes a physical phase gradient rather than a rule.
    REL_CALLS            = 24,   // function X calls function Y
    REL_RETURNS          = 25,   // function X returns type Y
    REL_TAKES_ARG        = 26,   // function X takes_arg type/param Y
    REL_PRECEDES         = 27,   // X must precede Y (dataflow ordering)
    REL_IMPORTS          = 28,   // file/module X imports Y
    REL_MUTATES          = 29,   // function X mutates field/state Y

    // ---- Commonsense relations (mechanically converted from ConceptNet) ----
    // These carry testimonial support only: each is asserted by two or
    // more independent human contributors, never established by
    // experiment. They enter the graph as `asserted` and are excluded
    // from JAS induction evidence. See
    // tools/extract_conceptnet_axioms.py for the conversion contract.
    REL_USED_FOR         = 30,   // artifact X is used for purpose Y
    REL_CAPABLE_OF       = 31,   // agent X is capable of action Y
    REL_AT_LOCATION      = 32,   // X is typically found at location Y
    REL_DESIRES          = 33,   // agent X desires Y
    REL_CREATED_BY       = 34,   // X is created by agent/process Y
};

// Human-readable name for extended relations. Falls back to unified's.
inline const char* rel_name_ext(int r) {
    switch (r) {
        case REL_NOT:                return "not";
        case REL_OR:                 return "or";
        case REL_HYPOTH:             return "hyp";
        case REL_BINDS_TO:           return "binds_to";
        case REL_INHIBITS:           return "inhibits";
        case REL_ACTIVATES:          return "activates";
        case REL_TREATS:             return "treats";
        case REL_CAUSES_DISEASE:     return "causes_disease";
        case REL_PART_OF_PATHWAY:    return "part_of_pathway";
        case REL_EXPRESSED_IN:       return "expressed_in";
        case REL_METABOLIZES:        return "metabolizes";
        case REL_UPREGULATES:        return "upregulates";
        case REL_DOWNREGULATES:      return "downregulates";
        case REL_PROPORTIONAL_TO:    return "is_proportional_to";
        case REL_MIRROR_OF:          return "mirror_of";
        case REL_DERIVES_FROM:       return "derives_from";
        case REL_FUNDAMENTAL:        return "is_fundamental";
        case REL_TRIGGERED_BY:       return "triggered_by";
        case REL_CALLS:              return "calls";
        case REL_RETURNS:            return "returns";
        case REL_TAKES_ARG:          return "takes_arg";
        case REL_PRECEDES:           return "precedes";
        case REL_IMPORTS:            return "imports";
        case REL_MUTATES:            return "mutates";
        case REL_USED_FOR:           return "used_for";
        case REL_CAPABLE_OF:         return "capable_of";
        case REL_AT_LOCATION:        return "at_location";
        case REL_DESIRES:            return "desires";
        case REL_CREATED_BY:         return "created_by";
        default:                     return rel_name(r);
    }
}

// -----------------------------------------------------------------------
// Primitive kind flag on Rule. We embed it as an extra member so the
// existing struct layout in unified.hpp keeps working.
// -----------------------------------------------------------------------
enum PrimitiveKind {
    PRIM_TRANSITIVE    = 0,  // (a r_ab b) + (b r_bc c) => (a r_ac c)         [default]
    PRIM_DEDUCTION     = 1,  // (P) + (P => Q)          => (Q)                [modus ponens]
    PRIM_INHERITANCE   = 2,  // (X is_a C) + (C r Y)    => (X r Y)            [generic class prop flow]
    PRIM_NEGATION      = 3,  // (P => Q) + (X NOT Q)    => (X NOT P)          [contrapositive]
    PRIM_CASE_ANALYSIS = 4,  // (X OR d) + (each d => R)=> (X => R)           [disjunction elim]
    PRIM_INDUCTION     = 5,  // {X_i is_a C, X_i has P} => (C has P)          [schematic]
    PRIM_ANALOGY       = 6   // structure-mapping across two related subgraphs
};

// A PrimitiveRule extends the base Rule with the kind flag and any
// extra relation slots the specific primitive needs. Stored alongside
// the base rules[] vector via a parallel std::vector<PrimRuleMeta>.
struct PrimRuleMeta {
    PrimitiveKind kind = PRIM_TRANSITIVE;
    int rel_extra = -1;    // used by e.g. NEGATION (the negated rel) or INDUCTION (the property rel)
    int min_witnesses = 3; // INDUCTION only: how many concrete X_i are required
};

// -----------------------------------------------------------------------
// The extended engine. Composes with SI: same graph, same table, same
// meta-learner. Adds the missing primitives on top.
// -----------------------------------------------------------------------
struct ExtSI : public SI {
    std::vector<PrimRuleMeta> prim_meta;   // 1:1 with SI::rules

    using SI::SI;

    // Register a rule of a specific primitive kind. The relation slots
    // are interpreted per the kind; see the comments at each add_prim_*
    // helper below for the meaning.
    void add_primitive(const std::string& name, PrimitiveKind kind,
                       int rel_ab, int rel_bc, int rel_ac,
                       int rel_extra = -1, int min_witnesses = 3) {
        // Ensure prim_meta stays aligned even if base add_rule was called.
        while (prim_meta.size() < rules.size()) prim_meta.push_back({});
        add_rule(name, rel_ab, rel_bc, rel_ac, /*spawn=*/false, /*cost=*/1.0f);
        prim_meta.push_back({kind, rel_extra, min_witnesses});
    }

    // ---- convenience helpers per primitive ------------------------------
    void add_transitive(const std::string& name, int rel_ab, int rel_bc, int rel_ac) {
        add_primitive(name, PRIM_TRANSITIVE, rel_ab, rel_bc, rel_ac);
    }
    void add_deduction(const std::string& name) {
        // Modus ponens works on implication chains — one relation, IMPLIES.
        add_primitive(name, PRIM_DEDUCTION, REL_IMPLIES, REL_IMPLIES, REL_IMPLIES);
    }
    void add_inheritance(const std::string& name, int property_rel) {
        // (X is_a C) + (C property_rel Y) => (X property_rel Y).
        add_primitive(name, PRIM_INHERITANCE, REL_IS_A, property_rel, property_rel);
    }
    void add_negation(const std::string& name, int implies_rel = REL_IMPLIES) {
        // Contrapositive around implies_rel. rel_ab = implies_rel, rel_ac = REL_NOT.
        add_primitive(name, PRIM_NEGATION, implies_rel, REL_NOT, REL_NOT);
    }
    void add_case_analysis(const std::string& name, int consequence_rel = REL_IMPLIES) {
        // (X OR d1) + (d1 => R) + (X OR d2) + (d2 => R) => (X => R).
        add_primitive(name, PRIM_CASE_ANALYSIS, REL_OR, consequence_rel, consequence_rel);
    }
    void add_induction(const std::string& name, int property_rel, int witnesses = 3) {
        // Aggregate over class members' shared property.
        add_primitive(name, PRIM_INDUCTION, REL_IS_A, property_rel, property_rel,
                      /*rel_extra=*/property_rel, /*min_witnesses=*/witnesses);
    }
    void add_analogy(const std::string& name, int mapping_rel = REL_IS_A) {
        // Uses a "mapped_to" style edge, encoded as mapping_rel between an
        // element of domain A and its counterpart in domain B.
        add_primitive(name, PRIM_ANALOGY, mapping_rel, mapping_rel, mapping_rel);
    }

    // ---- per-primitive apply passes ------------------------------------
    // Each returns the number of new edges added.

    // 1. TRANSITIVE — reuses the base engine's fire path via the standard
    //    apply_rules_once() from unified.hpp. We call it from apply_all().

    // 2. DEDUCTION (modus ponens on chains). Same shape as transitive for
    //    IMPLIES — but we tag it PRIM_DEDUCTION so the meta-learner and
    //    the paper's book-keeping can distinguish them.
    int fire_deduction(size_t rule_idx, float& budget, int step) {
        const Rule& r = rules[rule_idx];
        if (budget < r.cost) return 0;
        int added = 0;
        size_t snap = graph.edges.size();
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e1 = graph.edges[i];
            if (e1.rel != r.rel_ab) continue;
            // The inner loop is a JOIN on e1.to == e2.from. Scanning all
            // edges to find those made this O(E^2); out_adj answers it in
            // O(deg). Indexed access (not a cached reference) because
            // add_edge below can reallocate this very vector.
            if (e1.to < 0 || (size_t)e1.to >= graph.out_adj.size()) continue;
            size_t kn = graph.out_adj[(size_t)e1.to].size();
            for (size_t k = 0; k < kn; ++k) {
                size_t j = (size_t)graph.out_adj[(size_t)e1.to][k];
                if (j >= snap) continue;          // preserve snapshot semantics
                const Edge& e2 = graph.edges[j];
                if (e2.rel != r.rel_bc) continue;
                if (graph.has_edge(e1.from, e2.to, r.rel_ac)) continue;
                graph.add_edge(e1.from, e2.to, r.rel_ac,
                               (int)rule_idx, (int)i, (int)j, step);
                ++added;
                budget -= r.cost;
                if (budget < r.cost) return added;
            }
        }
        return added;
    }

    // 3. INHERITANCE — (X is_a C) + (C prop Y) => (X prop Y). This is the
    //    substitution primitive: property Y flows from class C down to any
    //    member X. Works for ANY relation on C, unlike transitive which
    //    requires matching relation types.
    int fire_inheritance(size_t rule_idx, float& budget, int step) {
        const Rule& r = rules[rule_idx];    // rel_ab=IS_A, rel_bc=rel_ac=property
        if (budget < r.cost) return 0;
        int added = 0;
        size_t snap = graph.edges.size();
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e_is_a = graph.edges[i];
            if (e_is_a.rel != REL_IS_A) continue;
            // Join on e_is_a.to == e_prop.from — indexed, see fire_deduction.
            if (e_is_a.to < 0 || (size_t)e_is_a.to >= graph.out_adj.size()) continue;
            size_t kn = graph.out_adj[(size_t)e_is_a.to].size();
            for (size_t k = 0; k < kn; ++k) {
                size_t j = (size_t)graph.out_adj[(size_t)e_is_a.to][k];
                if (j >= snap) continue;
                const Edge& e_prop = graph.edges[j];
                if (e_prop.rel != r.rel_bc) continue;
                if (graph.has_edge(e_is_a.from, e_prop.to, r.rel_ac)) continue;
                graph.add_edge(e_is_a.from, e_prop.to, r.rel_ac,
                               (int)rule_idx, (int)i, (int)j, step);
                ++added;
                budget -= r.cost;
                if (budget < r.cost) return added;
            }
        }
        return added;
    }

    // 4. NEGATION — contrapositive. Given rule (P implies Q) recorded as
    //    an edge (P -- IMPLIES --> Q), and an assertion (X -- NOT --> Q),
    //    derive (X -- NOT --> P). Works with any implication-like relation.
    int fire_negation(size_t rule_idx, float& budget, int step) {
        const Rule& r = rules[rule_idx];
        if (budget < r.cost) return 0;
        int added = 0;
        size_t snap = graph.edges.size();
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e_imp = graph.edges[i];
            if (e_imp.rel != r.rel_ab) continue;   // (P implies Q)
            int P = e_imp.from, Q = e_imp.to;
            for (size_t j = 0; j < snap; ++j) {
                const Edge& e_not = graph.edges[j];
                if (e_not.rel != REL_NOT) continue;
                if (e_not.to != Q) continue;
                int X = e_not.from;
                if (graph.has_edge(X, P, REL_NOT)) continue;
                graph.add_edge(X, P, REL_NOT,
                               (int)rule_idx, (int)i, (int)j, step);
                ++added;
                budget -= r.cost;
                if (budget < r.cost) return added;
            }
        }
        return added;
    }

    // 5. CASE_ANALYSIS — disjunction elimination. Given (X -- OR --> d) for
    //    several d, and each d has (d -- IMPLIES --> R), derive
    //    (X -- IMPLIES --> R). Requires *every* d in X's disjunct set to
    //    reach R for soundness.
    int fire_case_analysis(size_t rule_idx, float& budget, int step) {
        const Rule& r = rules[rule_idx];
        if (budget < r.cost) return 0;
        // Group OR edges by their source X.
        std::unordered_map<int, std::vector<int>> or_by_X; // X -> list of edge idxs
        size_t snap = graph.edges.size();
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e = graph.edges[i];
            if (e.rel == REL_OR) or_by_X[e.from].push_back((int)i);
        }
        int added = 0;
        for (auto& kv : or_by_X) {
            int X = kv.first;
            const auto& or_edges = kv.second;
            if (or_edges.size() < 2) continue;   // need a real disjunction
            // Find R values that every disjunct implies.
            std::unordered_map<int,int> R_hits;
            for (int oe : or_edges) {
                int d = graph.edges[oe].to;
                for (size_t j = 0; j < snap; ++j) {
                    const Edge& e_imp = graph.edges[j];
                    if (e_imp.rel != r.rel_bc) continue;
                    if (e_imp.from != d) continue;
                    R_hits[e_imp.to] += 1;
                }
            }
            for (auto& rh : R_hits) {
                int R = rh.first;
                if (rh.second < (int)or_edges.size()) continue; // must cover all disjuncts
                if (graph.has_edge(X, R, r.rel_ac)) continue;
                graph.add_edge(X, R, r.rel_ac,
                               (int)rule_idx, or_edges.front(), -1, step);
                ++added;
                budget -= r.cost;
                if (budget < r.cost) return added;
            }
        }
        return added;
    }

    // 6. INDUCTION — schematic generalisation. Count witnesses of the form
    //    (X_i is_a C) that all share the property (X_i prop_rel Y). If the
    //    number of shared witnesses hits meta.min_witnesses, derive
    //    (C prop_rel Y).
    int fire_induction(size_t rule_idx, float& budget, int step) {
        const Rule& r = rules[rule_idx];
        if (budget < r.cost) return 0;
        const auto& meta = prim_meta[rule_idx];
        int property = meta.rel_extra;
        int added = 0;
        size_t snap = graph.edges.size();
        // For every (C, Y, count) where we've seen enough instances.
        // Key: (C, Y). Value: how many distinct X_i satisfy both.
        std::unordered_map<uint64_t, int> counts;
        std::unordered_map<uint64_t, int> witness_edge; // any edge idx for provenance
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e_is_a = graph.edges[i];
            if (e_is_a.rel != REL_IS_A) continue;
            int Xi = e_is_a.from, C = e_is_a.to;
            // Join on e_prop.from == Xi — indexed, see fire_deduction.
            // This loop was the single most expensive thing in the
            // engine: 87k is_a edges x 237k edges = 2.1e10 iterations.
            if (Xi < 0 || (size_t)Xi >= graph.out_adj.size()) continue;
            size_t kn = graph.out_adj[(size_t)Xi].size();
            for (size_t k = 0; k < kn; ++k) {
                size_t j = (size_t)graph.out_adj[(size_t)Xi][k];
                if (j >= snap) continue;
                const Edge& e_prop = graph.edges[j];
                if (e_prop.rel != property) continue;
                int Y = e_prop.to;
                uint64_t key = ((uint64_t)(uint32_t)C << 32) | (uint32_t)Y;
                counts[key] += 1;
                witness_edge[key] = (int)i;
            }
        }
        for (auto& kv : counts) {
            if (kv.second < meta.min_witnesses) continue;
            int C = (int)(kv.first >> 32);
            int Y = (int)(kv.first & 0xffffffffull);
            if (graph.has_edge(C, Y, property)) continue;
            graph.add_edge(C, Y, property,
                           (int)rule_idx, witness_edge[kv.first], -1, step);
            ++added;
            budget -= r.cost;
            if (budget < r.cost) return added;
        }
        return added;
    }

    // 7. ANALOGY — structure-preserving edge transfer. Given a mapping
    //    (X -- mapping_rel --> X') between two entities, transfer every
    //    outgoing edge of X (except the mapping itself and other mappings)
    //    to X'. This is the "same-shape" edge-copy that Gentner's structure
    //    mapping formalises. Cheap, and surprisingly powerful for cross-
    //    domain reasoning.
    int fire_analogy(size_t rule_idx, float& budget, int step) {
        const Rule& r = rules[rule_idx];
        if (budget < r.cost) return 0;
        int mapping = r.rel_ab;
        int added = 0;
        size_t snap = graph.edges.size();
        // Collect X -> X' mappings.
        std::vector<std::pair<int,int>> pairs;
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e = graph.edges[i];
            if (e.rel == mapping) pairs.emplace_back(e.from, e.to);
        }
        for (auto& mp : pairs) {
            int X = mp.first, Xp = mp.second;
            // Outgoing edges of X — indexed, see fire_deduction.
            if (X < 0 || (size_t)X >= graph.out_adj.size()) continue;
            size_t kn = graph.out_adj[(size_t)X].size();
            for (size_t k = 0; k < kn; ++k) {
                size_t j = (size_t)graph.out_adj[(size_t)X][k];
                if (j >= snap) continue;
                const Edge& e = graph.edges[j];
                if (e.rel == mapping) continue;             // don't copy the mapping edge itself
                if (e.to == Xp) continue;                    // avoid trivial self-loops
                if (graph.has_edge(Xp, e.to, e.rel)) continue;
                graph.add_edge(Xp, e.to, e.rel,
                               (int)rule_idx, (int)j, -1, step);
                ++added;
                budget -= r.cost;
                if (budget < r.cost) return added;
            }
        }
        return added;
    }

    // The extended orchestrator. Runs each primitive kind on every rule
    // of that kind. Returns edges added in this pass.
    int apply_all_primitives(float& budget, int step) {
        while (prim_meta.size() < rules.size()) prim_meta.push_back({});
        int added = 0;
        for (size_t ri = 0; ri < rules.size(); ++ri) {
            PrimitiveKind kind = prim_meta[ri].kind;
            switch (kind) {
                case PRIM_TRANSITIVE:
                case PRIM_DEDUCTION:
                    added += fire_deduction(ri, budget, step);  break;
                case PRIM_INHERITANCE:
                    added += fire_inheritance(ri, budget, step); break;
                case PRIM_NEGATION:
                    added += fire_negation(ri, budget, step);    break;
                case PRIM_CASE_ANALYSIS:
                    added += fire_case_analysis(ri, budget, step); break;
                case PRIM_INDUCTION:
                    added += fire_induction(ri, budget, step);   break;
                case PRIM_ANALOGY:
                    added += fire_analogy(ri, budget, step);     break;
            }
        }
        return added;
    }

    struct ExtReport { int edges_added; int steps_used; float budget_left; };
    ExtReport reason_ext() {
        float budget = compute_budget();
        int total = 0, step = 0;
        for (; step < max_steps; ++step) {
            int add = apply_all_primitives(budget, step + 1);
            if (add == 0) break;
            total += add;
        }
        return {total, step, budget};
    }
};

} // namespace unified
