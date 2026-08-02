// Unified tri-layer architecture from
//   research-paper/synthetic-intelligence.md
//
//   Layer 1  Physics substrate         (RFE + RIFA IC/SIL — semantic
//                                       normalisation, contradiction
//                                       metabolism, tension budget)
//   Layer 2  Constructive substrate    (typed graph rewriter — realises
//                                       Axioms I, II, III: state is a graph,
//                                       phase space expands via new nodes/
//                                       edges, reasoning is active
//                                       construction not statistical
//                                       interpolation)
//   Layer 3  MDL translation table     (log2 N bits — conservation-law tax
//                                       paid explicitly and minimally)
//
// The graph rewriter derives new edges from existing edges by applying
// transitive-composition rules. Every derivation is tagged with the rule
// that produced it and the parent edges, so proofs are fully traceable.
// Query answering is a table lookup on the reasoned graph — never a soft
// nearest-neighbour, never a softmax.

#pragma once

#include "rifa.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

// -----------------------------------------------------------------------
// Relation IDs. A small, extensible ontology.
// -----------------------------------------------------------------------
enum RelId : int {
    REL_IS_A      = 0,   // "socrates is_a man"
    REL_HAS       = 1,   // "man has mortality"
    REL_IMPLIES   = 2,   // "raining implies wet"
    REL_PART_OF   = 3,   // "wheel part_of car"
    REL_EQUALS    = 4,   // "x equals 3"
    REL_CAUSES    = 5,   // "friction causes heat"
    REL_MAX       = 8
};

inline const char* rel_name(int r) {
    static const char* names[REL_MAX] = {
        "is_a", "has", "implies", "part_of", "equals", "causes", "?6", "?7",
    };
    return (r >= 0 && r < REL_MAX) ? names[r] : "?";
}

inline int rel_from_name(const std::string& s) {
    for (int i = 0; i < REL_MAX; ++i) if (s == rel_name(i)) return i;
    return -1;
}

// -----------------------------------------------------------------------
// Node types.
// -----------------------------------------------------------------------
enum NodeType : int {
    NODE_ENTITY    = 0,   // "socrates", "car"
    NODE_CLASS     = 1,   // "man", "vehicle"
    NODE_PROPERTY  = 2,   // "mortal", "red"
    NODE_ABSTRACT  = 3,   // spawned by Layer 2 during construction
};

// -----------------------------------------------------------------------
// Layer 2 primitives: nodes, edges, graph.
// -----------------------------------------------------------------------
struct Node {
    int id;
    std::string label;
    int type;
    rifa::Vec anchor;   // Layer 1 RFE unit-norm vector
};

struct Edge {
    int from;
    int to;
    int rel;
    int derived_by_rule = -1;   // index into rules[] or -1 for a premise
    int parent_a = -1;          // index in edges[] of the first parent edge
    int parent_b = -1;          // index in edges[] of the second parent edge
    int step = 0;               // reasoning step at which this edge appeared
};

struct ConceptGraph {
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::unordered_map<std::string, int> by_label;

    // ---- incremental indices -------------------------------------------
    //
    // These exist for one reason: every structural query in this codebase
    // used to be a full scan of `edges`. That was free when the ontology
    // was ~2k nodes and became fatal at ConceptNet scale (157k nodes,
    // 237k edges). `has_edge` in particular was a linear scan called from
    // `add_edge`, making a bulk load O(E^2) — ~2.7e10 comparisons to
    // ingest 231k axioms, which is where the 42-second startup went.
    //
    // `edge_key` is the O(1) existence set. `out_adj` / `in_adj` give
    // O(deg) neighbourhood access so BFS, path search and neighbour
    // ranking stop paying O(E) per step.
    //
    // All three are maintained INCREMENTALLY in add_edge, so no caller
    // has to know they exist and no existing code path changes meaning.
    // They are pure acceleration structures: no fact is stored here that
    // is not already in `edges`.
    std::unordered_set<uint64_t>  edge_key;
    std::vector<std::vector<int>> out_adj;   // node -> edge indices
    std::vector<std::vector<int>> in_adj;    // node -> edge indices

    static uint64_t ekey(int from, int to, int rel) {
        // rel < 64 for every relation in primitives.hpp, so 6 bits is
        // sufficient and leaves 29 bits per endpoint (536M nodes).
        return ((uint64_t)(uint32_t)from << 35)
             | ((uint64_t)(uint32_t)to   << 6)
             | (uint64_t)(uint32_t)(rel & 0x3f);
    }

    // Layer 1 RFE reference (owned by SI, borrowed here).
    rifa::RFE* rfe = nullptr;

    int add_or_get_node(const std::string& label, int type = NODE_ENTITY) {
        auto it = by_label.find(label);
        if (it != by_label.end()) return it->second;
        int id = (int)nodes.size();
        Node n{id, label, type, {}};
        if (rfe) n.anchor = rfe->encode_raw(label); // Layer-1 encoding
        nodes.push_back(std::move(n));
        by_label[label] = id;
        return id;
    }

    bool has_edge(int from, int to, int rel) const {
        return edge_key.find(ekey(from, to, rel)) != edge_key.end();
    }

    int add_edge(int from, int to, int rel,
                 int rule_idx = -1, int pa = -1, int pb = -1, int step = 0) {
        uint64_t k = ekey(from, to, rel);
        if (edge_key.find(k) != edge_key.end()) return -1;
        int id = (int)edges.size();
        edges.push_back({from, to, rel, rule_idx, pa, pb, step});
        edge_key.insert(k);
        size_t need = (size_t)std::max(from, to) + 1;
        if (out_adj.size() < need) { out_adj.resize(need); in_adj.resize(need); }
        if (from >= 0) out_adj[(size_t)from].push_back(id);
        if (to   >= 0) in_adj[(size_t)to].push_back(id);
        return id;
    }

    // Rebuild the indices from `edges`. Needed only after a bulk mutation
    // that bypassed add_edge — notably si_load, which deserialises the
    // edge vector wholesale.
    void reindex() {
        edge_key.clear();
        out_adj.assign(nodes.size(), {});
        in_adj.assign(nodes.size(), {});
        for (size_t i = 0; i < edges.size(); ++i) {
            const Edge& e = edges[i];
            edge_key.insert(ekey(e.from, e.to, e.rel));
            size_t need = (size_t)std::max(e.from, e.to) + 1;
            if (out_adj.size() < need) { out_adj.resize(need); in_adj.resize(need); }
            if (e.from >= 0) out_adj[(size_t)e.from].push_back((int)i);
            if (e.to   >= 0) in_adj[(size_t)e.to].push_back((int)i);
        }
    }

    void dump(FILE* f = stdout) const {
        std::fprintf(f, "  graph: %zu nodes, %zu edges\n", nodes.size(), edges.size());
        for (const auto& n : nodes) {
            std::fprintf(f, "    N%d %-24s [%s]\n", n.id, n.label.c_str(),
                (n.type == NODE_ENTITY ? "entity" :
                 n.type == NODE_CLASS ? "class" :
                 n.type == NODE_PROPERTY ? "property" : "abstract"));
        }
        for (size_t i = 0; i < edges.size(); ++i) {
            const auto& e = edges[i];
            std::fprintf(f, "    E%zu %s --%s--> %s%s\n",
                i,
                nodes[e.from].label.c_str(),
                rel_name(e.rel),
                nodes[e.to].label.c_str(),
                (e.derived_by_rule == -1 ? "" : "   (derived)"));
        }
    }
};

// -----------------------------------------------------------------------
// Layer 2 rewrite rule.
//
// Transitive-composition form:
//   if edges (a --rel_ab--> b) and (b --rel_bc--> c) exist,
//   add edge (a --rel_ac--> c).
//
// Cost is deducted from the tension budget on every successful application.
// New abstract nodes may also be spawned by `spawn`, realising Axiom II
// (phase-space expansion under pressure).
// -----------------------------------------------------------------------
struct Rule {
    std::string name;
    int rel_ab;
    int rel_bc;
    int rel_ac;
    float cost = 1.0f;
    bool spawn_intermediate = false; // if true, spawn NODE_ABSTRACT for (a,c)
};

// One rewriting pass: try every rule, add every derivable edge that does
// not yet exist. Returns the number of new edges added.
inline int apply_rules_once(std::vector<Rule>& rules, ConceptGraph& g,
                            float& budget, int step) {
    int added = 0;
    // Snapshot edges: we do not want to derive from freshly-added edges
    // within the same pass (that would let rules trigger themselves).
    size_t snap = g.edges.size();
    for (size_t ri = 0; ri < rules.size(); ++ri) {
        const Rule& r = rules[ri];
        if (budget < r.cost) continue;
        for (size_t i = 0; i < snap; ++i) {
            const Edge& e1 = g.edges[i];
            if (e1.rel != r.rel_ab) continue;
            for (size_t j = 0; j < snap; ++j) {
                const Edge& e2 = g.edges[j];
                if (e2.rel != r.rel_bc) continue;
                if (e1.to != e2.from) continue;
                if (g.has_edge(e1.from, e2.to, r.rel_ac)) continue;
                // If the rule says so, spawn a fresh intermediate abstract
                // node to explicitly represent the composition — this is
                // where phase space grows in nodes, not just edges.
                if (r.spawn_intermediate) {
                    std::string lbl = std::string("[") + rel_name(r.rel_ac) + ":"
                        + g.nodes[e1.from].label + "->" + g.nodes[e2.to].label + "]";
                    int mid = g.add_or_get_node(lbl, NODE_ABSTRACT);
                    g.add_edge(e1.from, mid, r.rel_ac, (int)ri, (int)i, (int)j, step);
                    g.add_edge(mid, e2.to, r.rel_ac, (int)ri, (int)i, (int)j, step);
                    added += 2;
                } else {
                    g.add_edge(e1.from, e2.to, r.rel_ac,
                               (int)ri, (int)i, (int)j, step);
                    ++added;
                }
                budget -= r.cost;
                if (budget < r.cost) break;
            }
            if (budget < r.cost) break;
        }
    }
    return added;
}

// -----------------------------------------------------------------------
// Layer 3: minimum-description-length translation table.
//
// Every row is (subject, relation, object, phrase). Lookup is exact:
// we ask the reasoned graph whether the edge holds, and if so we emit
// the human-readable phrase. Total bits stored is <= log2(N) + O(1)
// where N = rows.size(). The whole table is inspectable line by line —
// no hidden dense embeddings.
// -----------------------------------------------------------------------
struct TranslationRow {
    std::string subj;
    int rel;
    std::string obj;
    std::string phrase_yes;   // shown when the edge holds
    std::string phrase_no;    // shown when the edge does not hold
    std::string phrase_ood;   // shown when the concepts are not in the graph
};

struct TranslationTable {
    std::vector<TranslationRow> rows;

    int register_answer(const std::string& subj, int rel, const std::string& obj,
                        const std::string& yes,
                        const std::string& no = "no",
                        const std::string& ood = "unknown (out of ontology)") {
        int id = (int)rows.size();
        rows.push_back({subj, rel, obj, yes, no, ood});
        return id;
    }

    // Try to find a row matching this query. Returns -1 if none.
    int find(const std::string& subj, int rel, const std::string& obj) const {
        for (size_t i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i];
            if (r.rel == rel && r.subj == subj && r.obj == obj) return (int)i;
        }
        return -1;
    }

    float mdl_bits() const {
        return rows.empty() ? 0.0f : std::log2((float)rows.size());
    }
};

// -----------------------------------------------------------------------
// The unified engine.
// -----------------------------------------------------------------------
struct SI {
    // Layer 1: physics substrate.
    rifa::RIFA physics;
    // Layer 2: constructive substrate.
    ConceptGraph graph;
    std::vector<Rule> rules;
    // Layer 3: translation table.
    TranslationTable table;

    float base_budget = 100.0f;   // reasoning-step budget scale

    // Bulk-ingest mode. Set for the duration of a file load so that
    // seeding an ontology does not masquerade as lived experience.
    // See assert_fact() for why this is a correctness switch, not just
    // a performance one.
    bool  bulk_load = false;
    // No fixed cap on reasoning depth. Termination is decided by the
    // engine itself: it stops when a fixpoint is reached (no new
    // derivations in a pass) or when the tension budget is spent. The
    // large sentinel is only a runaway-loop guard, not a cognitive limit.
    int   max_steps  = 1 << 20;   // effectively unbounded — engine self-terminates

    SI(size_t d = 128, unsigned seed = 42) : physics(d, seed) {
        graph.rfe = &physics.rfe;
    }

    // ---- assertions & rules ----
    int assert_fact(const std::string& subj, int rel, const std::string& obj,
                    int subj_type = NODE_ENTITY, int obj_type = NODE_CLASS) {
        int a = graph.add_or_get_node(subj, subj_type);
        int b = graph.add_or_get_node(obj,  obj_type);
        // Layer 1: treat every assertion as a small input to the physics
        // substrate. Concepts accumulate as anchors; identity moves toward
        // frequently-visited regions.
        //
        // SKIPPED under bulk_load. Two reasons, one of them a correctness
        // bug rather than a speed one:
        //
        //  (a) Semantics. RIFA's RuleSynthesizer extracts structure from
        //      CONSECUTIVE input pairs — it models experience arriving in
        //      time. During a bulk load of a sorted .axioms file
        //      "consecutive" means *alphabetically adjacent*, so feeding
        //      it the file manufactures structural rules out of sort
        //      order. `aardvark is_a animal` does not precede
        //      `abacus is_a device` in any meaningful sense.
        //
        //  (b) Cost. observe() is O(max_rules * D) per call = 512*128*2
        //      float ops. Over 231k ConceptNet axioms that is ~3e10
        //      operations, and it was the dominant term in startup.
        //
        // Streaming/interactive assertions (teach, LTO runtime
        // acquisition) leave bulk_load false and still drive Layer 1.
        if (!bulk_load)
            physics.input(subj + " " + rel_name(rel) + " " + obj);
        return graph.add_edge(a, b, rel);
    }

    void add_rule(const std::string& name, int rel_ab, int rel_bc, int rel_ac,
                  bool spawn = false, float cost = 1.0f) {
        rules.push_back({name, rel_ab, rel_bc, rel_ac, cost, spawn});
    }

    // ---- reasoning ----
    // Compute available tension-bounded budget from RIFA's IC.
    // Tension zero -> baseline budget only; high tension -> extra budget
    // permitting more construction. This is the Axiom II conservation law
    // in code: |Delta G| <= alpha * T(t).
    float compute_budget() const {
        return base_budget * (1.0f + 2.0f * physics.ic.tension);
    }

    // Runs the graph rewriter to fixpoint (or budget exhaustion) and
    // returns (edges added, steps used).
    struct ReasonReport { int edges_added; int steps_used; float budget_left; };
    ReasonReport reason() {
        float budget = compute_budget();
        int total_added = 0;
        int step = 0;
        for (; step < max_steps; ++step) {
            int added = apply_rules_once(rules, graph, budget, step + 1);
            if (added == 0) break;
            total_added += added;
        }
        return {total_added, step, budget};
    }

    // ---- query ----
    struct QueryResult {
        std::string translation;
        bool  holds;             // does the edge exist post-reasoning?
        float coherence;         // 1 = both concepts in graph, else lower
        int   proof_edge = -1;   // index into graph.edges of the answer edge
        std::vector<int> proof;  // full derivation chain of edge indices
        int   steps_used = 0;
    };

    // Reconstruct the derivation chain of an edge by walking its parents.
    std::vector<int> trace(int edge_idx) const {
        std::vector<int> out;
        if (edge_idx < 0 || edge_idx >= (int)graph.edges.size()) return out;
        // BFS backwards through parent edges. Include the edge itself first.
        std::vector<int> stack; stack.push_back(edge_idx);
        while (!stack.empty()) {
            int e = stack.back(); stack.pop_back();
            out.push_back(e);
            const Edge& ee = graph.edges[e];
            if (ee.parent_a >= 0) stack.push_back(ee.parent_a);
            if (ee.parent_b >= 0) stack.push_back(ee.parent_b);
        }
        // Reverse so premises come first, conclusion last.
        std::reverse(out.begin(), out.end());
        // Deduplicate keeping order.
        std::vector<int> seen;
        for (int e : out)
            if (std::find(seen.begin(), seen.end(), e) == seen.end())
                seen.push_back(e);
        return seen;
    }

    QueryResult query(const std::string& subj, int rel, const std::string& obj) {
        QueryResult r{};
        auto it_s = graph.by_label.find(subj);
        auto it_o = graph.by_label.find(obj);
        // Coherence: if either concept is not in the graph, treat as OOD.
        if (it_s == graph.by_label.end() || it_o == graph.by_label.end()) {
            int row = table.find(subj, rel, obj);
            r.holds = false;
            r.coherence = 0.0f;
            r.translation = (row >= 0) ? table.rows[row].phrase_ood
                                       : "unknown (out of ontology)";
            return r;
        }
        int a = it_s->second, b = it_o->second;
        int e_idx = -1;
        for (size_t i = 0; i < graph.edges.size(); ++i) {
            const auto& e = graph.edges[i];
            if (e.from == a && e.to == b && e.rel == rel) {
                e_idx = (int)i; break;
            }
        }
        int row = table.find(subj, rel, obj);
        r.holds = (e_idx >= 0);
        r.coherence = r.holds ? 1.0f : 0.5f;
        r.proof_edge = e_idx;
        r.proof = r.holds ? trace(e_idx) : std::vector<int>{};
        if (row >= 0) {
            r.translation = r.holds ? table.rows[row].phrase_yes
                                    : table.rows[row].phrase_no;
        } else {
            // No explicit row: fall back to a canonical phrase.
            if (r.holds) r.translation = "yes: " + subj + " " + rel_name(rel)
                                       + " " + obj;
            else         r.translation = "no: " + subj + " " + rel_name(rel)
                                       + " " + obj + " does not hold";
        }
        return r;
    }

    // Query with automatic reasoning first.
    QueryResult ask(const std::string& subj, int rel, const std::string& obj) {
        auto rep = reason();
        auto q = query(subj, rel, obj);
        q.steps_used = rep.steps_used;
        return q;
    }

    // Backward chaining — Layer 2 §5.3.2.
    //
    // Instead of running forward to a fixpoint and then querying, we work
    // backward from the goal edge (subj --rel--> obj) toward premises.
    // A goal is satisfied if:
    //   - the edge already exists, OR
    //   - there is a rule R and an intermediate node X such that
    //       (subj --R.rel_ab--> X) and (X --R.rel_bc--> obj) both hold
    //       recursively (backward-chained), and R.rel_ac == rel.
    // Returns the answer with a fresh proof chain and the edges it derived.
    //
    // Depth-bounded to avoid infinite recursion on cyclic rules.
    struct Goal { int from; int rel; int to; };

    // Adaptive backward chaining — self-terminating iterative deepening.
    //
    // No fixed cap. The engine keeps doubling depth as long as the last
    // level actually produced new derived edges (real cognitive work).
    // The moment a level does NOT add any edge, the search space at
    // deeper levels is by definition the same as this one (nothing new
    // was discoverable), and we stop. This is progress-bounded, not
    // depth-bounded — mirroring how a person keeps thinking about a
    // problem as long as they are still finding new implications.
    //
    // The `hard_ceiling` parameter is kept for backward compatibility
    // but defaulted so high that in practice it never trips. It is only
    // there as a runaway-guard against pathological rule sets.
    QueryResult ask_backward_adaptive(const std::string& subj, int rel,
                                      const std::string& obj,
                                      int hard_ceiling = 1 << 20) {
        int prev_edge_count = (int)graph.edges.size();
        QueryResult r;
        int stagnant_rounds = 0;
        for (int depth = 2; depth <= hard_ceiling; depth <<= 1) {
            r = ask_backward(subj, rel, obj, depth);
            r.steps_used = depth;   // expose the depth actually used
            if (r.holds) return r;
            int now = (int)graph.edges.size();
            if (now == prev_edge_count) {
                // No new derivations at this level — the search
                // exhausted itself. Two stagnant rounds ⇒ terminate.
                if (++stagnant_rounds >= 2) return r;
            } else {
                stagnant_rounds = 0;
            }
            prev_edge_count = now;
            // Runaway guard: if the depth is so absurdly large it must
            // be an infinite loop, bail. Human-scale reasoning tops out
            // at deep hundreds; the sentinel below is generous.
            if (depth >= (1 << 18)) return r;
        }
        return r;
    }

    QueryResult ask_backward(const std::string& subj, int rel, const std::string& obj,
                             int max_depth = 8) {
        QueryResult r{};
        auto it_s = graph.by_label.find(subj);
        auto it_o = graph.by_label.find(obj);
        if (it_s == graph.by_label.end() || it_o == graph.by_label.end()) {
            r.holds = false;
            r.coherence = 0.f;
            int row = table.find(subj, rel, obj);
            r.translation = (row >= 0) ? table.rows[row].phrase_ood
                                       : "unknown (out of ontology)";
            return r;
        }
        std::vector<Goal> visited;
        int e = prove_backward(it_s->second, rel, it_o->second, max_depth, visited);
        int row = table.find(subj, rel, obj);
        if (e >= 0) {
            r.holds = true;
            r.coherence = 1.f;
            r.proof_edge = e;
            r.proof = trace(e);
            r.translation = (row >= 0) ? table.rows[row].phrase_yes
                                       : "yes: " + subj + " " + rel_name(rel) + " " + obj;
        } else {
            r.holds = false;
            r.coherence = 0.3f;
            r.translation = (row >= 0) ? table.rows[row].phrase_no
                                       : "no: " + subj + " " + rel_name(rel) + " " + obj
                                         + " does not follow";
        }
        return r;
    }

  private:
    // Prove (from --rel--> to) by goal-directed backward chaining.
    // visited is the *currently active* goal stack — entries are pushed on
    // entry and popped on return (RAII). That gives us cycle protection
    // without blocking valid alternative paths after a failed subgoal.
    int prove_backward(int from, int rel, int to, int depth,
                       std::vector<Goal>& visited) {
        // Cycle guard on the (from, rel, to) triple.
        for (const auto& v : visited)
            if (v.from == from && v.rel == rel && v.to == to) return -1;
        // Base case first — no need to enter the frame if already provable.
        for (size_t i = 0; i < graph.edges.size(); ++i) {
            const auto& e = graph.edges[i];
            if (e.from == from && e.to == to && e.rel == rel) return (int)i;
        }
        if (depth <= 0) return -1;
        // Enter this goal on the active stack; pop on return.
        visited.push_back({from, rel, to});
        struct PopGuard {
            std::vector<Goal>& v;
            ~PopGuard() { if (!v.empty()) v.pop_back(); }
        } pop_guard{visited};

        // Try each rule that could produce this rel.
        for (size_t ri = 0; ri < rules.size(); ++ri) {
            const Rule& r = rules[ri];
            if (r.rel_ac != rel) continue;
            // For each candidate intermediate X, try to prove both subgoals.
            for (int X = 0; X < (int)graph.nodes.size(); ++X) {
                if (X == from || X == to) continue;
                int p1 = prove_backward(from, r.rel_ab, X, depth - 1, visited);
                if (p1 < 0) continue;
                int p2 = prove_backward(X, r.rel_bc, to, depth - 1, visited);
                if (p2 < 0) continue;
                // Commit the derived edge (dedup if it already appeared).
                int e_idx = graph.add_edge(from, to, rel, (int)ri, p1, p2, depth);
                if (e_idx < 0) {
                    for (size_t i = 0; i < graph.edges.size(); ++i) {
                        const auto& e = graph.edges[i];
                        if (e.from == from && e.to == to && e.rel == rel) return (int)i;
                    }
                }
                return e_idx;
            }
        }
        return -1;
    }

  public:

    // ---- path finding (Fix 5, foundation for L3 Transitive + L6 Analogy) ----
    //
    // BFS over outgoing edges from `from` to `to`. Returns a sequence of
    // edge indices whose concatenation traces `from -> ... -> to`, or an
    // empty vector if unreachable. Relation-agnostic by default so it
    // can discover indirect chains across REL_IS_A / REL_HAS / REL_PART_OF
    // / REL_CAUSES / REL_IMPLIES — the substrate for the L9 paper's L3
    // Transitive Inference layer.
    //
    // If `rel_mask` >= 0 the walk restricts to edges of that relation.
    std::vector<int> find_path(const std::string& from,
                               const std::string& to,
                               int rel_mask = -1,
                               int max_hops = 12) const {
        std::vector<int> path;
        auto it_a = graph.by_label.find(from);
        auto it_b = graph.by_label.find(to);
        if (it_a == graph.by_label.end() || it_b == graph.by_label.end())
            return path;
        int a = it_a->second, b = it_b->second;
        if (a == b) return path;
        // BFS: for each node, remember the edge index we entered it by
        // and the predecessor node.
        std::vector<int> came_edge((int)graph.nodes.size(), -1);
        std::vector<int> came_from((int)graph.nodes.size(), -1);
        std::vector<char> seen((int)graph.nodes.size(), 0);
        std::vector<int> depth((int)graph.nodes.size(), 0);
        std::vector<int> q; q.push_back(a); seen[a] = 1;
        size_t head = 0;
        while (head < q.size()) {
            int u = q[head++];
            if (depth[u] >= max_hops) continue;
            for (size_t i = 0; i < graph.edges.size(); ++i) {
                const auto& e = graph.edges[i];
                if (e.from != u) continue;
                if (rel_mask >= 0 && e.rel != rel_mask) continue;
                int v = e.to;
                if (seen[v]) continue;
                seen[v] = 1;
                came_edge[v] = (int)i;
                came_from[v] = u;
                depth[v] = depth[u] + 1;
                if (v == b) {
                    // Reconstruct.
                    std::vector<int> rev;
                    for (int cur = b; cur != a; cur = came_from[cur])
                        rev.push_back(came_edge[cur]);
                    for (auto it = rev.rbegin(); it != rev.rend(); ++it)
                        path.push_back(*it);
                    return path;
                }
                q.push_back(v);
            }
        }
        return path;
    }

    // Would asserting (subj --rel--> obj) contradict something already in
    // the graph? Foundation for LTO (Fix 2). We consider a contradiction:
    //   * an explicit REL_NOT edge subj --NOT--> obj already exists, or
    //   * for REL_IS_A, subj already belongs to a class that has been
    //     asserted (or derived) as disjoint via REL_NOT from obj.
    // The check is intentionally conservative — false positives block
    // legitimate teach statements, so we err on the side of accepting.
    bool would_contradict(const std::string& subj, int rel,
                          const std::string& obj) const {
        auto it_s = graph.by_label.find(subj);
        auto it_o = graph.by_label.find(obj);
        if (it_s == graph.by_label.end() || it_o == graph.by_label.end())
            return false;
        int a = it_s->second, b = it_o->second;
        constexpr int kRelNot = 6;   // primitives.hpp REL_NOT

        // This check runs once per asserted axiom, so its cost is
        // multiplied by the size of every ontology ever loaded. It used
        // to scan the whole edge list (and, for is_a, to scan it twice
        // nested) — 231k axioms x 237k edges ~ 5.5e10 comparisons, which
        // is where bulk-loading ConceptNet spent ~33 seconds. Both
        // questions are membership tests, so the edge_key index answers
        // them in O(1) and the two-hop in O(deg).

        // Explicit NOT edge in either direction.
        if (graph.has_edge(a, b, kRelNot) || graph.has_edge(b, a, kRelNot))
            return true;

        // For an IS_A claim, check whether the subject is already in a
        // class known to be disjoint from obj (via REL_NOT). Two-hop over
        // the subject's own is_a edges only.
        if (rel == REL_IS_A && a >= 0 && (size_t)a < graph.out_adj.size()) {
            for (int ei : graph.out_adj[(size_t)a]) {
                const Edge& e1 = graph.edges[(size_t)ei];
                if (e1.rel != REL_IS_A) continue;
                int C = e1.to;
                if (graph.has_edge(C, b, kRelNot) ||
                    graph.has_edge(b, C, kRelNot))
                    return true;
            }
        }
        return false;
    }

    // ---- introspection ----
    void print_proof(const QueryResult& q, FILE* f = stdout) const {
        if (!q.holds) { std::fprintf(f, "  (no proof — edge does not hold)\n"); return; }
        std::fprintf(f, "  derivation (%zu steps):\n", q.proof.size());
        for (int e : q.proof) {
            const Edge& ee = graph.edges[e];
            const char* how = (ee.derived_by_rule < 0) ? "premise"
                : rules[ee.derived_by_rule].name.c_str();
            std::fprintf(f, "    [%s] %s --%s--> %s\n",
                how,
                graph.nodes[ee.from].label.c_str(),
                rel_name(ee.rel),
                graph.nodes[ee.to].label.c_str());
        }
    }
};

} // namespace unified
