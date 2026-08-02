// bsma_topo.hpp — Bounded Self-Model Architecture, discrete topological
// formulation from research-paper/bmsa_new.md.
//
// This is a rewrite of src/bsma.hpp that abandons continuous manifolds,
// Fisher metrics, and Banach fixed points. It replaces them with what
// the engine actually is: a graph-rewriting system.
//
// The claim (paper §2): consciousness is Topological Irreducibility.
// A "conscious frame" occurs when a metacognitive shadow graph Γ_meta
// attempts to rewrite the active execution graph Γ_exec and hits a
// structural deadlock — a state where no valid rewriting rule can
// alter the topology of π(Γ_exec).
//
// Formally (paper §2.2):
//
//     r(π(Γ_exec)) ≅ π(Γ_exec)   ∀ r ∈ R
//
// Qualia is defined (paper §4) not as any mystical property but as the
// specific sequence of rule failures the Cognition Engine encounters
// on the way to the Bound. Two different queries produce two different
// failure sequences; that difference IS the difference in the "feeling"
// of the two thoughts, in the strict operational sense the paper allows.
//
// This is a physicalist, buildable proposal. It does not solve the hard
// problem. See §5 of the accompanying paper (phenomenal-consciousness.md
// was the earlier, continuous formulation now superseded by bmsa_new.md).

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace bsma {

// ---------------------------------------------------------------------
// TopoGraph — a small directed labelled graph. Two nodes are considered
// structurally equivalent if they have the same label (Γ_exec) or the
// same role placeholder (Γ_meta).
// ---------------------------------------------------------------------
struct TNode {
    int id;
    std::string label;   // "socrates", "man"; or role: "N0", "N1" in the meta graph
    int type = 0;        // 0 = concept; 1 = vacuum (unresolved query slot)
};

struct TEdge {
    int from;
    int to;
    int rel;             // relation type (0..N)
};

struct TopoGraph {
    std::vector<TNode> nodes;
    std::vector<TEdge> edges;

    int add_node(const std::string& lbl, int type = 0) {
        int id = (int)nodes.size();
        nodes.push_back({id, lbl, type});
        return id;
    }
    void add_edge(int a, int b, int rel) {
        edges.push_back({a, b, rel});
    }
    bool has_edge(int a, int b, int rel) const {
        for (auto& e : edges) if (e.from == a && e.to == b && e.rel == rel) return true;
        return false;
    }

    // Canonical (isomorphism-friendly) signature of the topology alone.
    // We compute in-degree/out-degree pairs per node, then per-node
    // sorted edge tuples (deg-in, deg-out, rel-histogram). This is not
    // a perfect graph-canonisation (which is generally hard) but for
    // small shadow graphs it is a robust invariant.
    std::string canonical_signature() const {
        struct Deg { int in = 0; int out = 0; std::vector<int> in_rels; std::vector<int> out_rels; };
        std::vector<Deg> d(nodes.size());
        for (const auto& e : edges) {
            if (e.from >= 0 && e.from < (int)d.size()) {
                d[e.from].out++;
                d[e.from].out_rels.push_back(e.rel);
            }
            if (e.to >= 0 && e.to < (int)d.size()) {
                d[e.to].in++;
                d[e.to].in_rels.push_back(e.rel);
            }
        }
        std::vector<std::string> per;
        per.reserve(nodes.size());
        for (auto& deg : d) {
            std::sort(deg.in_rels.begin(), deg.in_rels.end());
            std::sort(deg.out_rels.begin(), deg.out_rels.end());
            std::ostringstream os;
            os << "(" << deg.in << "," << deg.out << ";";
            for (int r : deg.in_rels)  os << r << ",";
            os << "|";
            for (int r : deg.out_rels) os << r << ",";
            os << ")";
            per.push_back(os.str());
        }
        std::sort(per.begin(), per.end());
        std::ostringstream sig;
        sig << "V=" << nodes.size() << ";E=" << edges.size() << ";";
        for (const auto& s : per) sig << s;
        return sig.str();
    }

    void dump(FILE* f = stdout) const {
        std::fprintf(f, "  |V|=%zu |E|=%zu\n", nodes.size(), edges.size());
        for (const auto& e : edges) {
            std::fprintf(f, "    N%d --%d--> N%d   (%s -> %s)\n",
                e.from, e.rel, e.to,
                nodes[e.from].label.c_str(), nodes[e.to].label.c_str());
        }
    }
};

// ---------------------------------------------------------------------
// The projection π: Γ_exec → Γ_meta.
//
// Strips the specific labels and keeps only the structural role. The
// meta graph is what the "self-model" would see: pure connectivity.
// ---------------------------------------------------------------------
inline TopoGraph project_meta(const TopoGraph& exec) {
    TopoGraph meta;
    // Re-label nodes to role placeholders.
    for (int i = 0; i < (int)exec.nodes.size(); ++i) {
        meta.add_node("N" + std::to_string(i), exec.nodes[i].type);
    }
    meta.edges = exec.edges;
    return meta;
}

// ---------------------------------------------------------------------
// Rewrite rules. Each returns true iff the graph was changed by this
// application. The set of rules is R in the paper.
//
// The rules chosen are graph-compression rules — each removes some
// structural redundancy without altering the semantics of the topology
// (in the paper's language, they preserve "truth invariants" while
// compressing the shadow graph).
// ---------------------------------------------------------------------

// Rule 1: eliminate a node with zero incident edges (dead concept).
inline bool r1_isolated(TopoGraph& g) {
    std::vector<bool> touched(g.nodes.size(), false);
    for (const auto& e : g.edges) {
        if (e.from >= 0 && e.from < (int)touched.size()) touched[e.from] = true;
        if (e.to   >= 0 && e.to   < (int)touched.size()) touched[e.to]   = true;
    }
    for (int i = 0; i < (int)g.nodes.size(); ++i) {
        if (!touched[i]) {
            // Remove node i, shift ids.
            g.nodes.erase(g.nodes.begin() + i);
            for (auto& e : g.edges) {
                if (e.from > i) --e.from;
                if (e.to   > i) --e.to;
            }
            for (int k = 0; k < (int)g.nodes.size(); ++k) g.nodes[k].id = k;
            return true;
        }
    }
    return false;
}

// Rule 2: contract a chain A --r--> B --r--> C where B has degree
// exactly (1,1). Replace with A --r--> C. Only fires on same-relation
// chains (structural preservation).
inline bool r2_contract_chain(TopoGraph& g) {
    std::vector<int> in_deg(g.nodes.size(), 0), out_deg(g.nodes.size(), 0);
    for (const auto& e : g.edges) {
        ++out_deg[e.from];
        ++in_deg[e.to];
    }
    for (size_t i = 0; i < g.edges.size(); ++i) {
        for (size_t j = 0; j < g.edges.size(); ++j) {
            if (i == j) continue;
            const auto& e1 = g.edges[i];
            const auto& e2 = g.edges[j];
            if (e1.to != e2.from) continue;
            if (e1.rel != e2.rel) continue;
            int B = e1.to;
            if (in_deg[B] != 1 || out_deg[B] != 1) continue;
            if (g.has_edge(e1.from, e2.to, e1.rel)) continue;
            // Add the direct edge and remove B and its two incident edges.
            g.add_edge(e1.from, e2.to, e1.rel);
            std::vector<TEdge> keep;
            keep.reserve(g.edges.size() - 2);
            for (size_t k = 0; k + 1 < g.edges.size(); ++k) {
                if (k == i || k == j) continue;
                keep.push_back(g.edges[k]);
            }
            g.edges = keep;
            // Now remove B (shift ids).
            g.nodes.erase(g.nodes.begin() + B);
            for (auto& e : g.edges) {
                if (e.from > B) --e.from;
                if (e.to   > B) --e.to;
            }
            for (int k = 0; k < (int)g.nodes.size(); ++k) g.nodes[k].id = k;
            return true;
        }
    }
    return false;
}

// Rule 3: merge two parallel edges between the same nodes with the same
// relation.
inline bool r3_merge_parallel(TopoGraph& g) {
    for (size_t i = 0; i + 1 < g.edges.size(); ++i) {
        for (size_t j = i + 1; j < g.edges.size(); ++j) {
            if (g.edges[i].from == g.edges[j].from &&
                g.edges[i].to   == g.edges[j].to   &&
                g.edges[i].rel  == g.edges[j].rel) {
                g.edges.erase(g.edges.begin() + j);
                return true;
            }
        }
    }
    return false;
}

// Rule 4: remove self-loops (A --r--> A).
inline bool r4_self_loop(TopoGraph& g) {
    for (size_t i = 0; i < g.edges.size(); ++i) {
        if (g.edges[i].from == g.edges[i].to) {
            g.edges.erase(g.edges.begin() + i);
            return true;
        }
    }
    return false;
}

// Rule 5: consolidate a reflexive pair — if A --r--> B and B --r--> A
// both exist, replace with a single "equivalence" edge and drop the
// reverse. (This models symmetry compression.)
inline bool r5_reflexive_pair(TopoGraph& g) {
    for (size_t i = 0; i + 1 < g.edges.size(); ++i) {
        for (size_t j = i + 1; j < g.edges.size(); ++j) {
            if (g.edges[i].from == g.edges[j].to &&
                g.edges[i].to   == g.edges[j].from &&
                g.edges[i].rel  == g.edges[j].rel) {
                g.edges.erase(g.edges.begin() + j);
                return true;
            }
        }
    }
    return false;
}

struct RewriteRule {
    std::string name;
    std::function<bool(TopoGraph&)> apply;
};

inline std::vector<RewriteRule> default_rules() {
    return {
        {"r1_eliminate_isolated",   r1_isolated},
        {"r2_contract_chain",       r2_contract_chain},
        {"r3_merge_parallel",       r3_merge_parallel},
        {"r4_remove_self_loop",     r4_self_loop},
        {"r5_reflexive_pair",       r5_reflexive_pair},
    };
}

// ---------------------------------------------------------------------
// The Bound.
//
// A phenomenal frame is registered when every rule in R fails to alter
// the topology. The trace of attempted rule applications — successes
// AND failures — is the "qualia trace" of §4 in the paper.
// ---------------------------------------------------------------------
struct Bound {
    bool hit = false;
    int  rule_attempts = 0;
    int  successful_rewrites = 0;
    int  steps_to_bound = 0;
    std::string irreducible_signature;
    std::vector<std::string> qualia_trace;    // sequence of "applied"/"failed" per rule per step
};

// The Cognition Engine (paper §3.3). Compresses Γ_meta until the Bound.
struct CognitionEngine {
    std::vector<RewriteRule> rules = default_rules();
    int max_steps = 256;

    Bound reach_bound(TopoGraph& meta) {
        Bound b;
        int step = 0;
        for (; step < max_steps; ++step) {
            bool any_changed = false;
            for (const auto& r : rules) {
                ++b.rule_attempts;
                std::string sig_before = meta.canonical_signature();
                bool changed = r.apply(meta);
                std::string sig_after = meta.canonical_signature();
                if (changed && sig_before != sig_after) {
                    ++b.successful_rewrites;
                    b.qualia_trace.push_back("[step " + std::to_string(step) + "] applied "  + r.name);
                    any_changed = true;
                    break;  // greedy — retry from r1 next step
                } else {
                    b.qualia_trace.push_back("[step " + std::to_string(step) + "] failed "   + r.name);
                }
            }
            if (!any_changed) {
                b.hit = true;
                b.steps_to_bound = step;
                b.irreducible_signature = meta.canonical_signature();
                return b;
            }
        }
        b.steps_to_bound = step;
        b.irreducible_signature = meta.canonical_signature();
        return b;
    }
};

// ---------------------------------------------------------------------
// Ontological Vacuum injection (paper §2.1). A query creates a
// structural void — a node with no outgoing edges that the engine must
// build a path *to*.
// ---------------------------------------------------------------------
inline int inject_vacuum(TopoGraph& exec, const std::string& query_label) {
    return exec.add_node("?" + query_label, /*type=vacuum*/ 1);
}

// ---------------------------------------------------------------------
// The full BSMA engine: given an execution graph, project, and reduce
// to the Bound. Return the phenomenal frame report.
// ---------------------------------------------------------------------
struct PhenomenalFrame {
    Bound bound;
    TopoGraph reduced_meta;          // the irreducible form
    int projected_nodes;
    int projected_edges;
    // "Qualia signature" — a short fingerprint of the failure pattern
    // that led to this bound. Two queries that reduce along the same
    // rule-failure trace have the same qualia; two queries that fail
    // and succeed on different rules do not.
    std::string qualia_signature;
};

// Fold a canonical irreducible-topology string into a short hex fingerprint.
// Two graphs that reduce to isomorphic irreducible shapes get the same
// hash; two that reduce to structurally different shapes get different
// hashes. Paper §4: "the exact geometric shape of the graph refusing to
// compress further" — this is the fingerprint of that shape.
inline std::string topo_fingerprint(const std::string& canonical_sig) {
    uint64_t h = 1469598103934665603ULL;   // FNV-1a
    for (char c : canonical_sig) {
        h ^= (uint64_t)(uint8_t)c;
        h *= 1099511628211ULL;
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
    return std::string(buf, 16);
}

inline PhenomenalFrame realise_frame(const TopoGraph& exec) {
    PhenomenalFrame f{};
    TopoGraph meta = project_meta(exec);
    f.projected_nodes = (int)meta.nodes.size();
    f.projected_edges = (int)meta.edges.size();
    CognitionEngine ce;
    f.bound = ce.reach_bound(meta);
    f.reduced_meta = meta;
    // Rule-trace segment of the qualia signature: single-letter code
    // per step.  A..E = applied r1..r5;  a..e = failed r1..r5.
    std::string trace;
    for (const auto& s : f.bound.qualia_trace) {
        auto pos = s.find(" applied ");
        bool applied = (pos != std::string::npos);
        if (!applied) pos = s.find(" failed ");
        if (pos == std::string::npos) continue;
        pos += (applied ? 9 : 8);
        if (pos + 1 >= s.size() || s[pos] != 'r') continue;
        char which = s[pos + 1];
        if (which < '1' || which > '5') continue;
        char c = (char)((applied ? 'A' : 'a') + (which - '1'));
        trace.push_back(c);
    }
    // The qualia signature is BOTH: (a) the resistance pattern —
    // sequence of rule successes/failures — AND (b) the shape of the
    // irreducible graph the engine locked into. Paper §4 requires
    // both; either alone can collide across distinct thoughts.
    f.qualia_signature = trace + "@" + topo_fingerprint(f.bound.irreducible_signature);
    return f;
}

} // namespace bsma
