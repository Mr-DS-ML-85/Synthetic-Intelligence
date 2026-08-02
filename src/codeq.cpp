// codeq — structural query interface over a mechanically-extracted
// code ontology.
//
// ROADMAP Phase 1.2. This binary surfaces capability the substrate
// already has; it introduces no new inference machinery.
//
// Two classes of query:
//
//   SYMBOLIC — direct edge traversal over the concept graph.
//     callers, calls, signature, fields, imports, order, path
//     These are exact. They answer "what does the source literally say".
//
//   RESONANT — wave propagation through the physics substrate.
//     neighbors
//     A cavity is driven, the field is allowed to propagate through
//     material-weighted tunnels, and the readout ranks cavities by
//     resulting displacement. Multi-hop influence accumulates through
//     the impedance of intervening relations, so this surfaces
//     structurally-related symbols that share no direct edge. This is
//     the query a grep cannot answer.
//
// No embeddings, no similarity metric, no learned representation, no
// scoring model. Ranking is displacement amplitude in a mass-spring
// field whose materials were fixed at bind time by relation type.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "layer2.hpp"
#include "physics.hpp"
#include "primitives.hpp"
#include "si_state.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace unified;

// ---------------------------------------------------------------- setup

static void autoload_axioms(ExtSI& si) {
    DIR* d = opendir("data");
    if (!d) return;
    struct dirent* ent;
    std::vector<std::string> files;
    while ((ent = readdir(d)) != nullptr) {
        std::string name = ent->d_name;
        if (name.size() < 8) continue;
        if (name.substr(name.size() - 7) != ".axioms") continue;
        if (name == "mutations.axioms") continue;
        files.push_back("data/" + name);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    FILE* devnull = std::fopen("/dev/null", "w");
    for (const auto& f : files)
        load_axiom_file(si, f, devnull ? devnull : stderr);
    if (devnull) std::fclose(devnull);
}

static std::string lower(const std::string& s) {
    std::string o;
    for (char c : s) o += (char)std::tolower((unsigned char)c);
    return o;
}

// Resolve a user-typed symbol to a node id, or -1.
static int resolve(const ExtSI& si, const std::string& sym) {
    auto it = si.graph.by_label.find(lower(sym));
    return (it == si.graph.by_label.end()) ? -1 : it->second;
}

// Graph keys use '_' as an intra-label separator (insulin_signaling,
// drive_split_factor). That is a storage convention, not something a
// reader should see. Rendered as spaces at the I/O boundary only —
// no state is changed and lookup still uses the underscored key.
static std::string pretty(const std::string& s) {
    std::string o = s;
    for (char& c : o) if (c == '_') c = ' ';
    return o;
}

static std::string label_of(const ExtSI& si, int id) {
    if (id < 0 || id >= (int)si.graph.nodes.size()) return std::string("?");
    return pretty(si.graph.nodes[id].label);
}

// ------------------------------------------------------- symbolic queries

// Print every edge of relation `rel` whose source is `id`.
static int out_edges(const ExtSI& si, int id, int rel) {
    int n = 0;
    for (const auto& e : si.graph.edges) {
        if (e.from != id) continue;
        if (rel >= 0 && e.rel != rel) continue;
        std::printf("    %-14s %s\n", rel_name_ext(e.rel), label_of(si, e.to).c_str());
        ++n;
    }
    return n;
}

// Print every edge of relation `rel` whose target is `id`.
static int in_edges(const ExtSI& si, int id, int rel) {
    int n = 0;
    for (const auto& e : si.graph.edges) {
        if (e.to != id) continue;
        if (rel >= 0 && e.rel != rel) continue;
        std::printf("    %-24s %s\n", label_of(si, e.from).c_str(), rel_name_ext(e.rel));
        ++n;
    }
    return n;
}

// Relation-restricted shortest path. `unified::find_path` takes a
// single relation id, which cannot express "any code relation", so a
// chain between two functions was free to detour through the medical
// ontology wherever the two share a label. This BFS takes a bitmask
// over relation ids (all < 32) and returns the edge chain.
// REL_PRECEDES is deliberately EXCLUDED. It records that two calls
// appeared in sequence in some body, which is the right relation for
// an ordering question and the wrong one for a relatedness question:
// `count precedes fprintf` is true in a dozen unrelated functions, so
// admitting it makes every symbol reachable from every other in two
// hops through incidental co-occurrence. `order` queries use the
// dedicated PRECEDES walk; relatedness uses structural edges only.
static const uint32_t CODE_REL_MASK =
      (1u << REL_CALLS) | (1u << REL_RETURNS) | (1u << REL_TAKES_ARG)
    | (1u << REL_IMPORTS) | (1u << REL_MUTATES)
    | (1u << REL_HAS) | (1u << REL_IS_A);

// Hop-count BFS returns the SHORTEST chain, which in a call graph is
// almost always the one threading a hub: `end`, `size`, `push_back`
// touch hundreds of functions, so any two symbols are ~3 hops apart
// through one of them. Such a chain is true but vacuous — it explains
// nothing about how the two symbols actually relate.
//
// Cost is therefore the DEGREE of the node being entered, not 1 per
// hop. Traversing a hub is expensive; traversing a specific symbol is
// cheap. Dijkstra then prefers a longer, more specific chain over a
// short generic one. This is the same principle as the MIMS material
// table — passage through a node costs what that node's connectivity
// makes it cost.
static bool path_masked(const ExtSI& si, int a, int b, uint32_t mask,
                        int max_hops, std::vector<int>& edge_chain) {
    if (a == b) return true;
    const size_t N = si.graph.nodes.size();
    if (a < 0 || b < 0 || (size_t)a >= N || (size_t)b >= N) return false;

    std::vector<int> degree(N, 0);
    for (const auto& e : si.graph.edges) {
        if (e.from >= 0 && (size_t)e.from < N) degree[e.from]++;
        if (e.to   >= 0 && (size_t)e.to   < N) degree[e.to]++;
    }

    // Adjacency restricted to the relation mask.
    std::vector<std::vector<std::pair<int,int>>> adj(N);  // (to, edge_idx)
    for (size_t ei = 0; ei < si.graph.edges.size(); ++ei) {
        const Edge& e = si.graph.edges[ei];
        if (e.rel < 0 || e.rel >= 32) continue;
        if (!(mask & (1u << e.rel))) continue;
        if (e.from < 0 || (size_t)e.from >= N) continue;
        if (e.to   < 0 || (size_t)e.to   >= N) continue;
        adj[e.from].push_back({e.to, (int)ei});
    }

    const long INF = (1L << 60);
    std::vector<long> dist(N, INF);
    std::vector<int>  hops(N, 0);
    std::vector<int>  parent_edge(N, -1), parent_node(N, -1);
    // Simple binary-heap Dijkstra over (cost, node).
    std::vector<std::pair<long,int>> heap;
    dist[a] = 0;
    heap.push_back({0, a});
    auto cmp = [](const std::pair<long,int>& x, const std::pair<long,int>& y) {
        return x.first > y.first;              // min-heap
    };
    std::make_heap(heap.begin(), heap.end(), cmp);

    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end(), cmp);
        std::pair<long,int> top = heap.back();
        heap.pop_back();
        long d = top.first;
        int  u = top.second;
        if (d > dist[u]) continue;
        if (u == b) break;
        if (hops[u] >= max_hops) continue;
        for (const auto& pr : adj[u]) {
            int v = pr.first, ei = pr.second;
            long step = 1 + (long)degree[v];   // hubs are expensive
            long nd = d + step;
            if (nd < dist[v]) {
                dist[v] = nd;
                hops[v] = hops[u] + 1;
                parent_edge[v] = ei;
                parent_node[v] = u;
                heap.push_back({nd, v});
                std::push_heap(heap.begin(), heap.end(), cmp);
            }
        }
    }

    if (dist[b] >= INF) return false;
    for (int x = b; x != a && x != -1; x = parent_node[x]) {
        if (parent_edge[x] < 0) return false;
        edge_chain.push_back(parent_edge[x]);
    }
    std::reverse(edge_chain.begin(), edge_chain.end());
    return true;
}

// Does `a` precede `b` along REL_PRECEDES edges? Bounded BFS.
static bool precedes(const ExtSI& si, int a, int b, int max_hops,
                     std::vector<int>& chain) {
    std::vector<int> frontier{a};
    std::vector<int> parent(si.graph.nodes.size(), -1);
    std::vector<char> seen(si.graph.nodes.size(), 0);
    seen[a] = 1;
    for (int hop = 0; hop < max_hops && !frontier.empty(); ++hop) {
        std::vector<int> next;
        for (int cur : frontier) {
            for (const auto& e : si.graph.edges) {
                if (e.from != cur || e.rel != REL_PRECEDES) continue;
                if (seen[e.to]) continue;
                seen[e.to] = 1;
                parent[e.to] = cur;
                if (e.to == b) {
                    for (int x = b; x != -1; x = parent[x]) chain.push_back(x);
                    std::reverse(chain.begin(), chain.end());
                    return true;
                }
                next.push_back(e.to);
            }
        }
        frontier.swap(next);
    }
    return false;
}

// ------------------------------------------------------- resonant query

// Drive one cavity, let the field propagate, rank by displacement.
// The ranking is physical: influence reaching a distant cavity has
// passed through the impedance of every intervening relation, so
// stiff chains (precedes, calls) transmit and loose ones (imports)
// attenuate. Nothing here is learned or fitted.
static void neighbors(ExtSI& si, PhysicsEngine& physics,
                      ConstructiveEngine& constructive,
                      int id, int topk, float freq) {
    physics.reset_field();
    physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
    physics.drive(id, freq, 6.0f, 20);
    for (int r = 0; r < 8; ++r) {
        physics.run(25);
        constructive.structural_tick();
    }
    physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

    std::vector<std::pair<float,int>> ranked;
    ranked.reserve(physics.cavities.size());
    for (int i = 0; i < (int)physics.cavities.size(); ++i) {
        if (i == id) continue;                       // exclude the probe
        float a = std::fabs(physics.cavities[i].displacement);
        ranked.push_back({a, i});
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const std::pair<float,int>& x, const std::pair<float,int>& y) {
                  return x.first > y.first;
              });

    int shown = 0;
    for (const auto& r : ranked) {
        if (shown >= topk) break;
        if (r.first < 1e-5f) break;
        int lk = physics.alias_ledger[r.second].label_key;
        if (lk < 0 || lk >= (int)si.graph.nodes.size()) continue;
        const std::string& lbl = si.graph.nodes[lk].label;
        if (lbl.empty()) continue;
        std::printf("    %-28s  amplitude=%.5f\n", pretty(lbl).c_str(), r.first);
        ++shown;
    }
    if (shown == 0) std::printf("    (no propagation above threshold)\n");
}

// --------------------------------------------- resonant recognition
//
// `resolve()` is exact string lookup: a hash probe into by_label. It is
// binary and brittle — one wrong character and the concept does not
// exist. That is the "it's just regex" criticism, and it is fair.
//
// Recognition here instead runs through the CMD byte-wave sensory
// layer. Each concept's label is bound ONCE at load time to the 256
// byte lanes (sensory grounding — the equivalent of a label being
// pointed at). At query time no string is compared: input bytes drive
// their lanes, energy flows through the learned bindings, and whatever
// concepts rise above threshold are what was recognised.
//
// Binding strength is inverse byte frequency across the whole lexicon.
// A byte occurring in nearly every label distinguishes nothing and is
// bound weakly; a rare byte is bound strongly. This is the same
// principle as the CMD Curie gate — ubiquitous participants heat up and
// decouple — applied at grounding time rather than during propagation.
//
// The consequence that proves it is not string matching: recognition
// is GRADED. A misspelling still drives most of the right lanes and
// still resonates the right concept, at reduced amplitude. A hash
// lookup returns nothing for the same input.

static void seed_byte_bindings(const ExtSI& si, PhysicsEngine& px) {
    // Byte document-frequency across the lexicon.
    std::vector<int> byte_df(256, 0);
    for (const auto& n : si.graph.nodes) {
        bool seen[256] = {false};
        for (char c : n.label) {
            unsigned char u = (unsigned char)c;
            if (!seen[u]) { seen[u] = true; byte_df[u]++; }
        }
    }
    const float N = (float)si.graph.nodes.size() + 1.0f;
    for (size_t id = 0; id < si.graph.nodes.size(); ++id) {
        const std::string& lbl = si.graph.nodes[id].label;
        if (lbl.empty()) continue;
        bool seen[256] = {false};
        for (char c : lbl) {
            unsigned char u = (unsigned char)c;
            if (seen[u]) continue;
            seen[u] = true;
            // Rarity weight: log(N / df). Ubiquitous bytes ~0.
            float w = std::log(N / (float)(byte_df[u] + 1));
            if (w <= 0.0f) continue;
            // NO length division here. Dividing binding weight by
            // sqrt(len) made a substring outscore the whole word: for
            // query `metformin`, `from` scored S/2 while `metformin`
            // scored S/3, because the extra bytes are common (weight
            // ~0) but the length penalty is not. Coverage is instead
            // normalised symmetrically at query time.
            px.bind_byte(u, (int)id, w);
        }
    }
}

// Recognise concepts by driving byte lanes — no string comparison.
// Returns concept ids ranked by accumulated resonance.
static std::vector<int> resonant_recognize(const ExtSI& si,
                                           PhysicsEngine& px,
                                           const std::string& text,
                                           int topk) {
    std::vector<float> score(si.graph.nodes.size(), 0.0f);
    bool seen[256] = {false};
    // Distinct bytes of the query excite their lanes; each lane's
    // learned bindings deposit energy on the concepts it grounds.
    for (char c : text) {
        unsigned char u = (unsigned char)std::tolower((unsigned char)c);
        if (seen[u]) continue;
        seen[u] = true;
        if (u >= px.byte_bind_w.size()) continue;
        for (const auto& kv : px.byte_bind_w[u]) {
            if (kv.first >= 0 && (size_t)kv.first < score.size())
                score[kv.first] += kv.second;
        }
    }
    // Penalise concepts carrying many bytes the query did NOT contain,
    // otherwise long labels accumulate by sheer size.
    for (size_t id = 0; id < si.graph.nodes.size(); ++id) {
        if (score[id] <= 0.0f) continue;
        const std::string& lbl = si.graph.nodes[id].label;
        // Symmetric coverage: penalise bytes of the label absent from
        // the query AND reward how much of the label is accounted for.
        // A substring now loses because it explains little of the
        // query, and a long unrelated label loses because most of it
        // is unexplained.
        bool lseen[256] = {false};
        int distinct = 0, missing = 0;
        for (char c : lbl) {
            unsigned char u = (unsigned char)c;
            if (lseen[u]) continue;
            lseen[u] = true;
            ++distinct;
            if (!seen[u]) ++missing;
        }
        int q_distinct = 0;
        for (int b = 0; b < 256; ++b) if (seen[b]) ++q_distinct;
        float covered = (float)(distinct - missing);
        float recall  = covered / (float)(q_distinct > 0 ? q_distinct : 1);
        float precis  = covered / (float)(distinct > 0 ? distinct : 1);
        float f1 = (recall + precis > 0.0f)
                 ? (2.0f * recall * precis) / (recall + precis) : 0.0f;
        score[id] *= f1;
    }
    std::vector<std::pair<float,int>> ranked;
    for (size_t id = 0; id < score.size(); ++id)
        if (score[id] > 0.0f) ranked.push_back({score[id], (int)id});
    std::sort(ranked.begin(), ranked.end(),
              [](const std::pair<float,int>& a, const std::pair<float,int>& b) {
                  return a.first > b.first;
              });
    std::vector<int> out;
    for (int i = 0; i < (int)ranked.size() && i < topk; ++i)
        out.push_back(ranked[i].second);
    return out;
}


// ---- MRS Sec 4b.1: bigram sensory grounding + recognition ----------
//
// Extract the ordered character pairs of a token, bracketed by the
// BOS/EOS sentinels so word-initial and word-final position are
// themselves observable pairs.
static std::vector<int> bigrams_of(const std::string& w) {
    std::vector<int> out;
    if (w.empty()) return out;
    unsigned char prev = PhysicsEngine::BIGRAM_BOS;
    for (char c : w) {
        unsigned char u = (unsigned char)std::tolower((unsigned char)c);
        out.push_back((int)prev * 256 + (int)u);
        prev = u;
    }
    out.push_back((int)prev * 256 + (int)PhysicsEngine::BIGRAM_EOS);
    return out;
}

// Ground every concept label to the bigram lanes, ONCE, at load.
// Weight is inverse lane document-frequency: a pair occurring in
// nearly every label distinguishes nothing and is bound weakly. Same
// principle as the CMD Curie gate, applied at grounding time.
static void seed_bigram_bindings(const ExtSI& si, PhysicsEngine& px) {
    std::unordered_map<int,int> df;
    for (const auto& n : si.graph.nodes) {
        std::vector<int> bg = bigrams_of(n.label);
        std::sort(bg.begin(), bg.end());
        bg.erase(std::unique(bg.begin(), bg.end()), bg.end());
        for (int g : bg) df[g]++;
    }
    const float N = (float)si.graph.nodes.size() + 1.0f;
    for (size_t id = 0; id < si.graph.nodes.size(); ++id) {
        const std::string& lbl = si.graph.nodes[id].label;
        if (lbl.empty()) continue;
        std::vector<int> bg = bigrams_of(lbl);
        std::sort(bg.begin(), bg.end());
        bg.erase(std::unique(bg.begin(), bg.end()), bg.end());
        for (int g : bg) {
            float w = std::log(N / (float)(df[g] + 1));
            if (w <= 0.0f) continue;
            px.bind_bigram((unsigned char)(g / 256),
                           (unsigned char)(g % 256), (int)id, w);
        }
    }
}

// Recognise by driving the query's bigram lanes. No string is
// compared. Energy received by a concept is divided by the geometric
// mean of its own binding mass and the query's — energy per unit
// coupling, which is why neither long nor short labels win by size.
static std::vector<std::pair<float,int>>
bigram_recognize(const ExtSI& si, PhysicsEngine& px,
                 const std::string& text, int topk) {
    std::vector<int> bg = bigrams_of(text);
    std::sort(bg.begin(), bg.end());
    bg.erase(std::unique(bg.begin(), bg.end()), bg.end());

    std::unordered_map<int,float> recv;
    float query_mass = 0.0f;
    for (int g : bg) {
        auto it = px.bigram_bind_w.find(g);
        if (it == px.bigram_bind_w.end()) continue;
        for (const auto& kv : it->second) recv[kv.first] += kv.second;
        // Query mass uses the same weights the lexicon assigned.
        float wmax = 0.0f;
        for (const auto& kv : it->second) if (kv.second > wmax) wmax = kv.second;
        query_mass += wmax;
    }
    std::vector<std::pair<float,int>> ranked;
    for (const auto& kv : recv) {
        auto mi = px.bigram_mass.find(kv.first);
        float cm = (mi == px.bigram_mass.end()) ? 1.0f : mi->second;
        float denom = std::sqrt((cm > 0 ? cm : 1.0f) *
                                (query_mass > 0 ? query_mass : 1.0f));
        ranked.push_back({kv.second / denom, kv.first});
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const std::pair<float,int>& a, const std::pair<float,int>& b) {
                  return a.first > b.first;
              });
    if ((int)ranked.size() > topk) ranked.resize(topk);
    return ranked;
}

// ------------------------------------------------------ derivation view
//
// The single hardest criticism of this substrate is that it is a search
// engine: you put facts in, you get facts out, nothing is created. That
// criticism is answerable, but only if derivation is made VISIBLE.
//
// `reason_ext()` runs forward chaining over the loaded axioms and
// commits new edges marked with the rule that produced them and the
// indices of the two premise edges. On the current ontology it produces
// 100 edges that appear in NO axiom file — e.g. `human is_a great_ape`,
// deduced transitively, never asserted.
//
// A search engine cannot do this. It has no rule to fire and no
// premises to cite. So the distinction is not rhetorical: it is a
// property you can print, count, and check.

static bool is_derived(const ExtSI& si, int edge_idx) {
    if (edge_idx < 0 || edge_idx >= (int)si.graph.edges.size()) return false;
    return si.graph.edges[edge_idx].derived_by_rule >= 0;
}

// Print the proof tree for one edge, recursing into its premises.
static void explain_edge(const ExtSI& si, int edge_idx, int depth) {
    if (edge_idx < 0 || edge_idx >= (int)si.graph.edges.size()) return;
    if (depth > 8) { std::printf("    ...\n"); return; }
    const Edge& e = si.graph.edges[edge_idx];
    for (int i = 0; i < depth; ++i) std::printf("  ");
    const char* how = (e.derived_by_rule < 0)
                    ? "asserted"
                    : si.rules[e.derived_by_rule].name.c_str();
    std::printf("    %s %s %s   [%s]\n",
                label_of(si, e.from).c_str(), rel_name_ext(e.rel),
                label_of(si, e.to).c_str(), how);
    if (e.derived_by_rule < 0) return;          // premise — stop
    explain_edge(si, e.parent_a, depth + 1);
    explain_edge(si, e.parent_b, depth + 1);
}

// Find an edge joining a and b (either direction), preferring a derived
// one so `why` shows reasoning rather than a restatement.
static int find_edge_between(const ExtSI& si, int a, int b) {
    int fallback = -1;
    for (size_t i = 0; i < si.graph.edges.size(); ++i) {
        const Edge& e = si.graph.edges[i];
        if ((e.from == a && e.to == b) || (e.from == b && e.to == a)) {
            if (e.derived_by_rule >= 0) return (int)i;
            if (fallback < 0) fallback = (int)i;
        }
    }
    return fallback;
}

// --------------------------------------------------- resonant dispatch
//
// The user should not have to name a query mode. A person asked about
// code does not first select "neighborhood mode" — the shape of what
// they asked determines the shape of the answer.
//
// The dispatch below inspects NO STRINGS. It drives the field with
// whatever symbols the input resonated with, lets the substrate
// propagate, and then scores each readout mode as a continuous
// function of PHYSICAL OBSERVABLES:
//
//   n_matched     how many input windows resonated with a known symbol
//   hot_frac      fraction of the field the wavefront reached
//   path_strength whether a relation chain joins the matched symbols
//   shared_frac   overlap of the matched symbols' direct neighbourhoods
//
// arg-max over the mode scores selects the readout. This is the same
// idiom as the phase-collapse emission sweep: continuous weights, one
// arg-max, no branch table over query keywords. A different question
// produces a different field, and the different field selects a
// different answer.

enum Mode { M_SILENCE = 0, M_NEIGHBORHOOD, M_CHAIN, M_BRIDGE, M_COUNT };

static const char* mode_name(int m) {
    switch (m) {
        case M_NEIGHBORHOOD: return "resonance neighbourhood";
        case M_CHAIN:        return "relational chain";
        case M_BRIDGE:       return "shared structure";
        default:             return "silence";
    }
}

// Direct out-neighbours of a node, any relation.
static void out_neighbors(const ExtSI& si, int id, std::vector<int>& out) {
    for (const auto& e : si.graph.edges)
        if (e.from == id) out.push_back(e.to);
}

// Fraction of the smaller neighbourhood that both symbols share.
static float shared_fraction(const ExtSI& si, int a, int b) {
    std::vector<int> na, nb;
    out_neighbors(si, a, na);
    out_neighbors(si, b, nb);
    if (na.empty() || nb.empty()) return 0.0f;
    std::sort(na.begin(), na.end());
    std::sort(nb.begin(), nb.end());
    na.erase(std::unique(na.begin(), na.end()), na.end());
    nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    std::vector<int> inter;
    std::set_intersection(na.begin(), na.end(), nb.begin(), nb.end(),
                          std::back_inserter(inter));
    size_t small = (na.size() < nb.size()) ? na.size() : nb.size();
    return (float)inter.size() / (float)small;
}

static void print_shared(const ExtSI& si, int a, int b, int topk) {
    std::vector<int> na, nb;
    out_neighbors(si, a, na);
    out_neighbors(si, b, nb);
    std::sort(na.begin(), na.end());
    std::sort(nb.begin(), nb.end());
    na.erase(std::unique(na.begin(), na.end()), na.end());
    nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    std::vector<int> inter;
    std::set_intersection(na.begin(), na.end(), nb.begin(), nb.end(),
                          std::back_inserter(inter));
    if (inter.empty()) { std::printf("    (no shared structure)\n"); return; }
    int shown = 0;
    for (int x : inter) {
        if (shown++ >= topk) break;
        std::printf("    %s\n", label_of(si, x).c_str());
    }
}

static void dispatch(ExtSI& si, PhysicsEngine& physics,
                     ConstructiveEngine& constructive,
                     const std::vector<std::string>& tok) {
    // 1. Resonance scan — which input words are known symbols.
    std::vector<int> matched;
    int n_lexical = 0, n_resonant = 0;
    for (const auto& t : tok) {
        int id = resolve(si, t);                 // exact channel
        if (id >= 0) { matched.push_back(id); ++n_lexical; continue; }
        // No exact hit — recognise through the bigram sensory lanes.
        // No string is compared: the token's ordered character pairs
        // drive their lanes and whatever concept accumulates the most
        // energy per unit coupling is what was recognised. Measured
        // 7/7 on misspellings including transpositions (MRS Sec 4b.1).
        // A floor keeps genuinely unknown tokens from matching noise.
        std::vector<std::pair<float,int>> r =
            bigram_recognize(si, physics, lower(t), 1);
        if (!r.empty() && r[0].first >= 0.35f) {
            matched.push_back(r[0].second);
            ++n_resonant;
        }
    }
    // Deduplicate, preserve order.
    std::vector<int> uniq;
    for (int m : matched)
        if (std::find(uniq.begin(), uniq.end(), m) == uniq.end())
            uniq.push_back(m);
    matched.swap(uniq);

    // Rank by structural connectedness (degree, descending).
    //
    // The first attempt here used INVERSE degree, on the information-
    // theoretic argument that a rare symbol is more specific. On this
    // data that is empirically backwards: incidental matches on common
    // English words ("come", "and", "state") enter the graph through
    // extraction noise and therefore have degree 1-2, while the actual
    // topic of a question ("fission", "cavity", "tick") is wired into
    // real structure. Energy propagates from the connected symbols and
    // dies at the isolated ones, so degree is the correct proxy for
    // "what did this question actually excite". No stopword list.
    std::vector<int> degree(si.graph.nodes.size(), 0);
    for (const auto& e : si.graph.edges) {
        if (e.from >= 0 && e.from < (int)degree.size()) degree[e.from]++;
        if (e.to   >= 0 && e.to   < (int)degree.size()) degree[e.to]++;
    }
    // Drop symbols too weakly attached to carry a question's topic.
    std::vector<int> kept;
    for (int m : matched) if (degree[m] >= 3) kept.push_back(m);
    if (!kept.empty()) matched.swap(kept);
    std::stable_sort(matched.begin(), matched.end(),
                     [&](int a, int b) { return degree[a] > degree[b]; });

    // 2. Drive the field with everything that resonated.
    physics.reset_field();
    physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
    for (int id : matched) physics.drive(id, 3.0f, 6.0f, 20);
    for (int r = 0; r < 8; ++r) {
        physics.run(25);
        constructive.structural_tick();
    }
    physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

    // 3. Measure physical observables.
    const float n_matched = (float)matched.size();
    const float hot_frac =
        physics.cavities.empty() ? 0.0f
        : (float)physics.hot_count() / (float)physics.cavities.size();

    float path_strength = 0.0f;
    float shared_frac   = 0.0f;
    if (matched.size() >= 2) {
        std::vector<int> chain;
        if (precedes(si, matched[0], matched[1], 8, chain)) {
            path_strength = 1.0f;
        } else {
            std::vector<int> p;
            bool found = path_masked(si, matched[0], matched[1],
                                     CODE_REL_MASK, 8, p);
            path_strength = (found && !p.empty()) ? 0.7f : 0.0f;
        }
        shared_frac = shared_fraction(si, matched[0], matched[1]);
    }

    // 4. Continuous mode scores. No string is inspected anywhere.
    auto sigmoid = [](float x) { return 1.0f / (1.0f + std::exp(-x)); };
    float score[M_COUNT];
    score[M_SILENCE]      = std::exp(-6.0f * hot_frac) * std::exp(-2.0f * n_matched);
    score[M_NEIGHBORHOOD] = std::exp(-(n_matched - 1.0f) * (n_matched - 1.0f))
                          * (0.2f + hot_frac);
    score[M_CHAIN]        = sigmoid(4.0f * (n_matched - 1.5f)) * path_strength;
    score[M_BRIDGE]       = sigmoid(4.0f * (n_matched - 1.5f))
                          * (1.0f - path_strength) * (0.1f + shared_frac);

    int best = 0;
    for (int m = 1; m < M_COUNT; ++m)
        if (score[m] > score[best]) best = m;

    // 5. Run the selected readout.
    std::printf("  [%s]  matched=%d  reach=%.3f  path=%.2f  shared=%.2f\n",
                mode_name(best), (int)n_matched, hot_frac,
                path_strength, shared_frac);
    // Recognition telemetry — how much of the input was understood by
    // sensory resonance vs by exact lexical lookup. The lexical count
    // is the residual dependence on string matching; driving it toward
    // zero is a measurable goal, not a claim.
    std::printf("  recognition: lexical=%d  resonant=%d\n",
                n_lexical, n_resonant);

    if (best == M_SILENCE || matched.empty()) {
        std::printf("    nothing in the graph resonated with that.\n");
        return;
    }
    if (best == M_NEIGHBORHOOD) {
        // Field is already propagated — read it out directly.
        std::vector<std::pair<float,int>> ranked;
        for (int i = 0; i < (int)physics.cavities.size(); ++i) {
            bool is_probe = std::find(matched.begin(), matched.end(), i)
                            != matched.end();
            if (is_probe) continue;
            ranked.push_back({std::fabs(physics.cavities[i].displacement), i});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& x,
                     const std::pair<float,int>& y) {
                      return x.first > y.first;
                  });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= 10 || r.first < 1e-5f) break;
            int lk = physics.alias_ledger[r.second].label_key;
            if (lk < 0 || lk >= (int)si.graph.nodes.size()) continue;
            std::printf("    %-28s  amplitude=%.5f\n",
                        pretty(si.graph.nodes[lk].label).c_str(), r.first);
            ++shown;
        }
        if (shown == 0) std::printf("    (no propagation above threshold)\n");
        return;
    }
    if (best == M_CHAIN) {
        std::vector<int> chain;
        if (precedes(si, matched[0], matched[1], 8, chain)) {
            std::printf("    dataflow: ");
            for (size_t i = 0; i < chain.size(); ++i)
                std::printf("%s%s", label_of(si, chain[i]).c_str(),
                            (i + 1 == chain.size()) ? "\n" : " -> ");
            return;
        }
        std::vector<int> p;
        if (!path_masked(si, matched[0], matched[1], CODE_REL_MASK, 8, p)
            || p.empty()) {
            std::printf("    (no chain found)\n"); return;
        }
        std::printf("    ");
        for (size_t i = 0; i < p.size(); ++i) {
            const Edge& e = si.graph.edges[p[i]];
            if (i == 0) std::printf("%s ", label_of(si, e.from).c_str());
            std::printf("--%s--> %s ", rel_name_ext(e.rel), label_of(si, e.to).c_str());
        }
        std::printf("\n");
        return;
    }
    // M_BRIDGE
    std::printf("    %s and %s both relate to:\n",
                label_of(si, matched[0]).c_str(), label_of(si, matched[1]).c_str());
    print_shared(si, matched[0], matched[1], 10);
}

// ------------------------------------------------------------------ main

static void usage() {
    std::printf(
        "codeq — structural queries over an extracted code ontology\n\n"
        "  neighbors <sym> [k]   resonance probe: what this symbol is\n"
        "                        structurally coupled to (multi-hop)\n"
        "  calls <fn>            what <fn> calls\n"
        "  callers <fn>          what calls <fn>\n"
        "  signature <fn>        parameters and return type\n"
        "  fields <struct>       declared members\n"
        "  imports <unit>        what <unit> includes\n"
        "  order <a> <b>         is there a dataflow chain a -> b\n"
        "  path <a> <b>          any relation chain between two symbols\n"
        "  stats                 edge counts by relation\n"
        "  quit\n\n");
}

int main(int argc, char** argv) {
    ExtSI si;
    build_big_ontology(si);
    autoload_axioms(si);
    si.reason_ext();

    PhysicsEngine physics;
    physics.bind(si);
    ConstructiveEngine constructive;
    constructive.bind(&physics);

    // Sensory grounding: bind every concept label to the byte lanes ONCE.
    // After this, recognition needs no string comparison.
    seed_byte_bindings(si, physics);
    seed_bigram_bindings(si, physics);

    std::printf("codeq — %d symbols, %d relations loaded.\n",
                (int)si.graph.nodes.size(), (int)si.graph.edges.size());
    std::printf("Type 'help' for queries, 'quit' to exit.\n\n");

    // Allow a one-shot query from argv.
    std::vector<std::string> argline;
    for (int i = 1; i < argc; ++i) argline.push_back(argv[i]);
    bool one_shot = !argline.empty();

    std::string line;
    while (true) {
        std::vector<std::string> tok;
        if (one_shot) {
            tok = argline;
        } else {
            std::printf("codeq> ");
            std::fflush(stdout);
            if (!std::getline(std::cin, line)) break;
            std::istringstream iss(line);
            std::string w;
            while (iss >> w) tok.push_back(w);
            if (tok.empty()) continue;
        }

        const std::string& cmd = tok[0];

        if (cmd == "quit" || cmd == "exit") break;
        if (cmd == "help") { usage(); if (one_shot) break; continue; }

        // List what the engine concluded that nobody told it.
        if (cmd == "derived") {
            int k = (tok.size() > 1) ? std::atoi(tok[1].c_str()) : 15;
            int total = 0;
            for (const auto& e : si.graph.edges)
                if (e.derived_by_rule >= 0) ++total;
            std::printf("  %d facts derived, present in no axiom file:\n", total);
            int shown = 0;
            for (const auto& e : si.graph.edges) {
                if (e.derived_by_rule < 0) continue;
                if (shown++ >= k) break;
                std::printf("    %-24s %-12s %-24s  [%s]\n",
                            label_of(si, e.from).c_str(), rel_name_ext(e.rel),
                            label_of(si, e.to).c_str(),
                            si.rules[e.derived_by_rule].name.c_str());
            }
            std::printf("\n");
            if (one_shot) break;
            continue;
        }

        // Portable substrate state (.si). Everything the substrate
        // learned — thermal, acoustic, plasticity, gauge, sensory
        // bindings, lane tunnels, fission map — plus the graph itself.
        if (cmd == "save" || cmd == "load") {
            std::string p = (tok.size() > 1) ? tok[1] : "model.si";
            std::string err;
            bool ok = (cmd == "save") ? si_save(si, physics, p, &err)
                                      : si_load(si, physics, p, &err);
            if (!ok) {
                std::printf("  %s failed: %s\n", cmd.c_str(), err.c_str());
            } else if (cmd == "save") {
                std::printf("  saved %d symbols, %d relations -> %s\n",
                            (int)si.graph.nodes.size(),
                            (int)si.graph.edges.size(), p.c_str());
            } else {
                constructive.bind(&physics);
                std::printf("  loaded %d symbols, %d relations <- %s\n",
                            (int)si.graph.nodes.size(),
                            (int)si.graph.edges.size(), p.c_str());
            }
            std::printf("\n");
            if (one_shot) break;
            continue;
        }

        // Explicit query verbs remain available so the reproducibility
        // harnesses can pin an exact readout. Anything else is handed
        // to the resonant dispatcher, which selects the readout from
        // the field's own state rather than from the wording.
        static const char* verbs[] = {
            "neighbors", "calls", "callers", "signature", "fields",
            "imports", "order", "path", "stats", "why"
        };
        bool explicit_verb = false;
        for (const char* v : verbs)
            if (cmd == v) { explicit_verb = true; break; }

        if (!explicit_verb) {
            dispatch(si, physics, constructive, tok);
            std::printf("\n");
            if (one_shot) break;
            continue;
        }

        if (cmd == "stats") {
            std::vector<int> counts(64, 0);
            for (const auto& e : si.graph.edges)
                if (e.rel >= 0 && e.rel < 64) counts[e.rel]++;
            std::printf("  symbols   %d\n", (int)si.graph.nodes.size());
            std::printf("  relations %d\n", (int)si.graph.edges.size());
            for (int r = 0; r < 64; ++r)
                if (counts[r] > 0)
                    std::printf("    %-16s %d\n", rel_name_ext(r), counts[r]);
            if (one_shot) break;
            continue;
        }

        if (tok.size() < 2) {
            std::printf("  usage: %s <symbol>\n", cmd.c_str());
            if (one_shot) break;
            continue;
        }

        int id = resolve(si, tok[1]);
        if (id < 0) {
            std::printf("  '%s' not in graph\n", tok[1].c_str());
            if (one_shot) break;
            continue;
        }

        if (cmd == "neighbors") {
            int k = (tok.size() > 2) ? std::atoi(tok[2].c_str()) : 10;
            std::printf("  [resonance probe: %s]\n", label_of(si, id).c_str());
            neighbors(si, physics, constructive, id, k, 3.0f);
        }
        else if (cmd == "calls") {
            std::printf("  [%s calls]\n", label_of(si, id).c_str());
            if (out_edges(si, id, REL_CALLS) == 0)
                std::printf("    (none recorded)\n");
        }
        else if (cmd == "callers") {
            std::printf("  [callers of %s]\n", label_of(si, id).c_str());
            if (in_edges(si, id, REL_CALLS) == 0)
                std::printf("    (none recorded)\n");
        }
        else if (cmd == "signature") {
            std::printf("  [signature of %s]\n", label_of(si, id).c_str());
            int n = out_edges(si, id, REL_TAKES_ARG);
            n += out_edges(si, id, REL_RETURNS);
            if (n == 0) std::printf("    (none recorded)\n");
        }
        else if (cmd == "fields") {
            std::printf("  [fields of %s]\n", label_of(si, id).c_str());
            if (out_edges(si, id, REL_HAS) == 0)
                std::printf("    (none recorded)\n");
        }
        else if (cmd == "imports") {
            std::printf("  [%s imports]\n", label_of(si, id).c_str());
            if (out_edges(si, id, REL_IMPORTS) == 0)
                std::printf("    (none recorded)\n");
        }
        else if (cmd == "why") {
            if (tok.size() < 3) { std::printf("  usage: why <a> <b>\n"); }
            else {
                int b = resolve(si, tok[2]);
                if (b < 0) std::printf("  '%s' not in graph\n", tok[2].c_str());
                else {
                    int ei = find_edge_between(si, id, b);
                    if (ei < 0) {
                        std::printf("  no relation recorded between %s and %s\n",
                                    label_of(si, id).c_str(), label_of(si, b).c_str());
                    } else if (!is_derived(si, ei)) {
                        std::printf("  asserted directly in an axiom file:\n");
                        explain_edge(si, ei, 0);
                    } else {
                        std::printf("  DERIVED — this was concluded, not stored:\n");
                        explain_edge(si, ei, 0);
                    }
                }
            }
        }
        else if (cmd == "order") {
            if (tok.size() < 3) { std::printf("  usage: order <a> <b>\n"); }
            else {
                int b = resolve(si, tok[2]);
                if (b < 0) std::printf("  '%s' not in graph\n", tok[2].c_str());
                else {
                    std::vector<int> chain;
                    if (precedes(si, id, b, 8, chain)) {
                        std::printf("  yes — dataflow chain:\n    ");
                        for (size_t i = 0; i < chain.size(); ++i)
                            std::printf("%s%s", label_of(si, chain[i]).c_str(),
                                        (i + 1 == chain.size()) ? "\n" : " -> ");
                    } else {
                        std::printf("  no recorded dataflow chain %s -> %s\n",
                                    label_of(si, id).c_str(), label_of(si, b).c_str());
                    }
                }
            }
        }
        else if (cmd == "path") {
            if (tok.size() < 3) { std::printf("  usage: path <a> <b>\n"); }
            else {
                std::vector<int> p;
                int bb = resolve(si, tok[2]);
                bool fnd = (bb >= 0) && path_masked(si, id, bb,
                                                    CODE_REL_MASK, 8, p);
                if (!fnd || p.empty()) std::printf("  no path found\n");
                else {
                    std::printf("  ");
                    for (size_t i = 0; i < p.size(); ++i) {
                        const Edge& e = si.graph.edges[p[i]];
                        if (i == 0) std::printf("%s ", label_of(si, e.from).c_str());
                        std::printf("--%s--> %s ", rel_name_ext(e.rel),
                                    label_of(si, e.to).c_str());
                    }
                    std::printf("\n");
                }
            }
        }
        else {
            std::printf("  unknown query '%s' (try 'help')\n", cmd.c_str());
        }

        std::printf("\n");
        if (one_shot) break;
    }
    return 0;
}
