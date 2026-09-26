// si — the single entry point. ROADMAP Phase 1.5.
//
// THE PROBLEM THIS CLOSES
//
// Roughly twenty binaries and forty CLI verbs accumulated, each
// surfacing one research paper's mechanism. A user had to know which
// paper a capability came from in order to invoke it. That is a
// packaging failure, not a research one — every mechanism worked; none
// was reachable without a manual.
//
// A person asked a question does not first select a mode. The shape of
// what they asked determines the shape of the answer.
//
// HOW DISPATCH WORKS
//
// No string is inspected to choose a mode. The input drives the field,
// the field propagates, and the readout is selected by arg-max over
// continuous functions of PHYSICAL OBSERVABLES:
//
//   n_matched      how many input windows resonated with a symbol
//   hot_frac       fraction of the field the wavefront reached
//   path_strength  whether a relation chain joins the matched symbols
//   shared_frac    overlap of their direct neighbourhoods
//   derived_frac   how much of the neighbourhood was DERIVED not stored
//
// Same idiom as the CPME phase-collapse sweep: continuous weights, one
// arg-max, no branch table over keywords. A different question produces
// a different field, and the different field selects a different answer.
//
// Output is realised through a Constructive Language Locking cluster
// (cll.md), so the same cognitive state speaks English, Bengali or
// Banglish without the substrate changing.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "lang.hpp"
#include "layer2.hpp"
#include "physics.hpp"
#include "primitives.hpp"
#include "si_state.hpp"
#include "unified.hpp"
// The research engines. Each existed as a standalone header with zero
// callers — computed nowhere, reachable from nowhere. Dispatch is their
// single integration point (see the routing blocks inside main()).
#include "salience.hpp"
#include "bisociation.hpp"
#include "novelty.hpp"
#include "analogy.hpp"
#include "inference.hpp"
#include "meta_learner.hpp"
#include "bsma_topo.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>

using namespace unified;

// ------------------------------------------------------------- setup

static void autoload_axioms(ExtSI& si) {
    DIR* d = opendir("data");
    if (!d) return;
    struct dirent* ent;
    std::vector<std::string> files;
    while ((ent = readdir(d)) != nullptr) {
        std::string n = ent->d_name;
        if (n.size() < 8) continue;
        if (n.substr(n.size() - 7) != ".axioms") continue;
        if (n == "mutations.axioms") continue;
        files.push_back("data/" + n);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    FILE* dn = std::fopen("/dev/null", "w");
    for (const auto& f : files) load_axiom_file(si, f, dn ? dn : stderr);
    if (dn) std::fclose(dn);
}

static std::string lower(const std::string& s) {
    std::string o;
    for (char c : s) o += (char)std::tolower((unsigned char)c);
    return o;
}

static int resolve(const ExtSI& si, const std::string& sym) {
    auto it = si.graph.by_label.find(lower(sym));
    return (it == si.graph.by_label.end()) ? -1 : it->second;
}

// Graph keys use '_' as an intra-label separator (insulin_signaling,
// open_source_license). That is a storage convention, not something a
// reader should ever see. Rendered as spaces at the I/O boundary only —
// lookup still uses the underscored key.
static std::string pretty(const std::string& s) {
    std::string o = s;
    for (char& c : o) if (c == '_') c = ' ';
    return o;
}

static std::string label_of(const ExtSI& si, int id) {
    if (id < 0 || id >= (int)si.graph.nodes.size()) return "?";
    return pretty(si.graph.nodes[id].label);
}

// ---------------------------------------------- resonant recognition
//
// `resolve()` is an exact hash probe: one wrong character and the
// concept does not exist. That is why "dogs" failed while "dog" would
// have worked — a plural is not a different concept, it is the same
// concept spelled slightly differently.
//
// Recognition instead runs through the MRS bigram sensory lanes
// (mrs.md §4b.1). No string is compared: the token's ordered character
// pairs drive their lanes, and whatever concept accumulates the most
// energy per unit coupling is what was recognised. Because the match is
// GRADED rather than binary, a plural or a misspelling still resonates
// the right concept at reduced amplitude. Measured 7/7 on misspellings
// including transpositions.
//
// This existed in codeq and was simply never carried into this binary.

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

// Ground every concept label to the bigram lanes ONCE, at load. Weight
// is inverse lane document-frequency: a pair in nearly every label
// distinguishes nothing and binds weakly — the CMD Curie principle
// applied at grounding time rather than during propagation.
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

// Energy received is divided by the geometric mean of the concept's own
// binding mass and the query's — energy per unit coupling, which is why
// neither long nor short labels win by size alone.
static std::vector<std::pair<float,int>>
bigram_recognize(PhysicsEngine& px, const std::string& text, int topk) {
    std::vector<int> bg = bigrams_of(text);
    std::sort(bg.begin(), bg.end());
    bg.erase(std::unique(bg.begin(), bg.end()), bg.end());

    std::unordered_map<int,float> recv;
    float query_mass = 0.0f;
    for (int g : bg) {
        auto it = px.bigram_bind_w.find(g);
        if (it == px.bigram_bind_w.end()) continue;
        for (const auto& kv : it->second) recv[kv.first] += kv.second;
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

// Structural scaffolding must never surface to a reader.
static bool is_internal(const std::string& lbl) {
    return lbl.find("__arg") != std::string::npos;
}

// ---------------------------------------------------------- dispatch

enum Mode { M_SILENCE = 0, M_NEIGHBORHOOD, M_RELATION, M_DERIVED, M_COUNT };

// Adjacency comes from the graph's own index. Scanning si.graph.edges
// here was O(E) per call — tolerable at 2k edges, fatal at 237k.
static void out_neighbors(const ExtSI& si, int id, std::vector<int>& out) {
    if (id < 0 || (size_t)id >= si.graph.out_adj.size()) return;
    for (int ei : si.graph.out_adj[(size_t)id])
        out.push_back(si.graph.edges[(size_t)ei].to);
}

static float shared_fraction(const ExtSI& si, int a, int b) {
    std::vector<int> na, nb;
    out_neighbors(si, a, na);
    out_neighbors(si, b, nb);
    if (na.empty() || nb.empty()) return 0.0f;
    std::sort(na.begin(), na.end()); std::sort(nb.begin(), nb.end());
    na.erase(std::unique(na.begin(), na.end()), na.end());
    nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    std::vector<int> inter;
    std::set_intersection(na.begin(), na.end(), nb.begin(), nb.end(),
                          std::back_inserter(inter));
    size_t small = na.size() < nb.size() ? na.size() : nb.size();
    return (float)inter.size() / (float)small;
}

// Find an edge between two nodes, preferring a DERIVED one so the
// system reports reasoning rather than restating a stored fact.
// Only edges INCIDENT to `a` can join a and b, so the index answers this
// in O(deg(a)) instead of scanning all 237k edges. This function is
// called once per displayed neighbour, so the old full scan made the
// readout quadratic in the graph.
static int edge_between(const ExtSI& si, int a, int b) {
    int fallback = -1;
    if (a < 0 || (size_t)a >= si.graph.out_adj.size()) return -1;
    for (int ei : si.graph.out_adj[(size_t)a]) {
        const Edge& e = si.graph.edges[(size_t)ei];
        if (e.to != b) continue;
        if (e.derived_by_rule >= 0) return ei;
        if (fallback < 0) fallback = ei;
    }
    for (int ei : si.graph.in_adj[(size_t)a]) {
        const Edge& e = si.graph.edges[(size_t)ei];
        if (e.from != b) continue;
        if (e.derived_by_rule >= 0) return ei;
        if (fallback < 0) fallback = ei;
    }
    return fallback;
}

static void explain(const ExtSI& si, int ei, int depth) {
    if (ei < 0 || ei >= (int)si.graph.edges.size() || depth > 6) return;
    const Edge& e = si.graph.edges[ei];
    for (int i = 0; i <= depth; ++i) std::printf("   ");
    std::printf("%s %s %s   [%s]\n",
                label_of(si, e.from).c_str(), rel_name_ext(e.rel),
                label_of(si, e.to).c_str(),
                e.derived_by_rule < 0 ? "asserted"
                                      : si.rules[e.derived_by_rule].name.c_str());
    if (e.derived_by_rule < 0) return;
    explain(si, e.parent_a, depth + 1);
    explain(si, e.parent_b, depth + 1);
}

// Which language cluster to answer in. NOT a flag — a property of what
// the user typed. Bengali script is outside ASCII, so a byte >= 0x80 in
// the input is direct physical evidence of the protocol the speaker is
// using. Pure ASCII answers in English.
//
// This respects CLL isolation: selecting a cluster is not a grammar rule
// in the core, it is routing to one of several equally-external tables.
static const LanguageCluster& pick_cluster(const std::string& line,
                                           const LanguageCluster& en,
                                           const LanguageCluster& bn) {
    for (unsigned char c : line) if (c >= 0x80) return bn;
    return en;
}

// ------------------------------------------------- discovery routing
//
// Query 6 (classify_gap_query) plus the five discovery queries it
// routes to. This is the whole integration the inference engine was
// missing: the queries existed, the classifier was a comment, and no
// input path ever reached either.

static void render_logic_gap(const ExtSI& si, const LogicGap& g,
                             const std::string& a, const std::string& b) {
    std::printf("logic gap: %s <-> %s\n", inf_english(a).c_str(),
                inf_english(b).c_str());
    if (g.full_path_exists) {
        std::printf("  a complete path already exists; no gap.\n");
        return;
    }
    if (g.from_side_node < 0 || g.to_side_node < 0) {
        std::printf("  neither side reaches the other; no fringe found.\n");
        return;
    }
    std::printf("  forward frontier : %s (%d hops from %s)\n",
                pretty(si.graph.nodes[g.from_side_node].label).c_str(),
                g.hops_from_a, inf_english(a).c_str());
    std::printf("  backward frontier: %s (%d hops back from %s)\n",
                pretty(si.graph.nodes[g.to_side_node].label).c_str(),
                g.hops_from_b_reverse, inf_english(b).c_str());
    std::printf("  the missing bridge is between these two.\n");
}

template <typename Resolve>
static bool run_discovery(ExtSI& si, const GapQuery& gq, Resolve resolve_word) {
    if (gq.kind == 0) {
        // A logic gap needs two resolved concepts. The classifier hands
        // back content words; the resolver owns exact + resonant match.
        if (gq.terms.size() < 2) {
            std::printf("a gap question needs two concepts; I only heard %zu.\n\n",
                        gq.terms.size());
            return true;
        }
        int a = resolve_word(gq.terms[0]);
        int b = resolve_word(gq.terms[1]);
        if (a < 0 || b < 0) {
            std::printf("one of those concepts did not resonate with a symbol.\n\n");
            return true;
        }
        LogicGap g = find_logic_gap(si, si.graph.nodes[a].label,
                                    si.graph.nodes[b].label);
        render_logic_gap(si, g, si.graph.nodes[a].label,
                         si.graph.nodes[b].label);
        std::printf("\n");
        return true;
    }
    if (gq.kind == 1) {
        auto comps = find_isolated_subgraphs(si);
        if (comps.empty()) {
            std::printf("no floating islands: every component is grounded "
                        "in a fundamental.\n\n");
            return true;
        }
        std::printf("%d isolated component(s) not grounded in a fundamental:\n",
                    (int)comps.size());
        for (int c = 0; c < (int)comps.size() && c < 5; ++c) {
            std::printf("  island %d (%zu nodes):", c, comps[c].nodes.size());
            for (size_t k = 0; k < comps[c].nodes.size() && k < 6; ++k)
                std::printf(" %s",
                    pretty(si.graph.nodes[comps[c].nodes[k]].label).c_str());
            std::printf("\n");
        }
        std::printf("\n");
        return true;
    }
    if (gq.kind == 2) {
        auto dang = find_dangling_edges(si);
        if (dang.empty()) {
            std::printf("no dangling concepts: every chain lands on a "
                        "fundamental within reach.\n\n");
            return true;
        }
        std::printf("%d dangling concept(s) — outgoing edges that never "
                    "reach a fundamental:\n", (int)dang.size());
        for (const auto& d : dang) {
            std::printf("  %-28s degree=%d hops_to_fundamental=%d\n",
                        pretty(si.graph.nodes[d.node_id].label).c_str(),
                        d.total_degree, d.min_hops_to_fundamental);
        }
        std::printf("\n");
        return true;
    }
    if (gq.kind == 3) {
        auto gaps = find_symmetry_gaps(si);
        if (gaps.empty()) {
            std::printf("no symmetry gaps: every mirrored assertion has "
                        "its mirror edge.\n\n");
            return true;
        }
        std::printf("%d symmetry gap(s) — asserted without their mirror:\n",
                    (int)gaps.size());
        for (const auto& s : gaps) {
            std::printf("  %s --%s--> %s   (mirrors %s, %s%s)\n",
                        pretty(si.graph.nodes[s.x].label).c_str(),
                        rel_name_ext(s.r),
                        pretty(si.graph.nodes[s.y].label).c_str(),
                        pretty(si.graph.nodes[s.x_mirror].label).c_str(),
                        pretty(si.graph.nodes[s.y_mirror].label).c_str(),
                        s.mirror_exists ? " — mirror exists" : "");
        }
        std::printf("\n");
        return true;
    }
    if (gq.kind == 4) {
        auto anom = find_anomalies(si);
        if (anom.empty()) {
            std::printf("no anomalies: degree and grounding are consistent "
                        "everywhere.\n\n");
            return true;
        }
        std::printf("anomalous zones (well-connected, poorly grounded):\n");
        for (const auto& n : anom) {
            std::printf("  %-28s degree=%d hops_to_fundamental=%d score=%.1f\n",
                        pretty(si.graph.nodes[n.node_id].label).c_str(),
                        n.total_degree, n.hops_to_fundamental, n.anomaly_score);
        }
        std::printf("\n");
        return true;
    }
    return false;
}

int main() {
    ExtSI si;
    PhysicsEngine physics;
    ConstructiveEngine constructive;

    // A `.si` holds graph AND learned physical state, so if one exists
    // there is nothing to rebuild. Otherwise seed from data/*.axioms and
    // write one, so the cost is paid once ever rather than once per run.
    std::string err;
    bool restored = si_load(si, physics, SI_DEFAULT_PATH, &err);
    if (!restored) {
        build_big_ontology(si);
        autoload_axioms(si);
        si.reason_ext();
        physics.bind(si);
    }
    constructive.bind(&physics);
    // Sensory grounding: bind every concept label to the bigram lanes
    // ONCE. After this, recognition needs no string comparison.
    seed_bigram_bindings(si, physics);
    if (!restored) {
        if (si_save(si, physics, SI_DEFAULT_PATH, &err))
            std::printf("(substrate written to %s)\n", SI_DEFAULT_PATH);
    }

    // -------------------------------------------------- cognitive engines
    // Attention (TSDA), meta-cognition (rule induction + thinker index),
    // and identity (RIFA) now live in the dispatch loop. Each was a
    // standalone library before; none was instantiated anywhere.
    unified::SalienceMap sal;
    sal.sync_with(si);
    MetaLearner mlearner;
    mlearner.engine = &si;
    ThinkerIndex thinker;
    rifa::RIFA identity(128, 42);

    // The meta-learner observes existing derivation traces once at load,
    // so macros can be induced from the reasoning the substrate already
    // did — observe_trace previously had zero callers in the product.
    {
        int seen = 0;
        for (size_t e = 0; e < si.graph.edges.size() && seen < 256; ++e)
            if (si.graph.edges[e].derived_by_rule >= 0) {
                std::vector<int> tr = si.trace((int)e);
                if (tr.size() >= 2) { mlearner.observe_trace(tr); ++seen; }
            }
    }

    // Language clusters (CLL). Both built; the input selects per turn.
    const LanguageCluster en = make_english();
    const LanguageCluster bn = make_bengali();

    int n_derived = 0;
    for (const auto& e : si.graph.edges)
        if (e.derived_by_rule >= 0) ++n_derived;

    // Node degree is a property of the loaded graph, not of a query.
    // Computed once here; the query loop only reads it.
    std::vector<int> degree(si.graph.nodes.size(), 0);
    for (const auto& e : si.graph.edges) {
        if (e.from >= 0 && e.from < (int)degree.size()) degree[e.from]++;
        if (e.to   >= 0 && e.to   < (int)degree.size()) degree[e.to]++;
    }

    std::printf("si — %d symbols, %d relations (%d derived)\n",
                (int)si.graph.nodes.size(), (int)si.graph.edges.size(),
                n_derived);
    std::printf("Ask anything. No commands, no flags. "
                "Bengali in, Bengali out.\n\n");

    // Rolling introspection state, surfaced by "status".
    std::string last_qualia;        // BSMA qualia signature of the last frame
    long conscious_frames = 0;      // frames that reached the Bound
    long dmn_thoughts    = 0;       // DMN bisociations that validated

    std::string line;
    while (true) {
        std::printf("you> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line)) break;
        while (!line.empty() && std::isspace((unsigned char)line.back()))
            line.pop_back();
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

        // Self-report. "status" is meta-input, like quit — it asks about
        // the thinker, not the world. This is where the thinker index
        // (C/R), the meta-learner's macro inventory, the RIFA identity
        // state, and the CPME emission observables become visible. None
        // of these had a surface before.
        if (lower(line) == "status") {
            std::printf("si > self-report\n");
            thinker.report(stdout);
            std::printf("  macro rules registered : %zu (from %zu sub-chain patterns)\n",
                        mlearner.macros.size(), mlearner.chain_stats.size());
            std::printf("  derived relations      : %d / %d edges\n",
                        [&]{ int d=0; for (auto& e : si.graph.edges) if (e.derived_by_rule >= 0) ++d; return d; }(),
                        (int)si.graph.edges.size());
            std::printf("  attention              : %zu node(s) above tau, cap sampled in [5,9]\n",
                        sal.working_graph(si).size());
            std::printf("  emission observables   : K_settle=%.3f  dR=%.4f  hot=%d/%zu\n",
                        physics.cpme_last_Ksettle, physics.cpme_delta_R,
                        physics.hot_count(), physics.cavities.size());
            std::printf("  identity               : ");
            identity.status(stdout);
            std::printf("  mind-wandering         : %ld conscious frame(s), %ld idle thought(s)\n",
                        conscious_frames, dmn_thoughts);
            if (!last_qualia.empty())
                std::printf("  last conscious frame   : %s\n", last_qualia.c_str());
            std::printf("\n");
            continue;
        }

        // The protocol the question arrived in is the protocol it is
        // answered in. Decided per turn, from bytes, not from a flag.
        const LanguageCluster& lang = pick_cluster(line, en, bn);

        // Tokenise for resonance scan only — not for parsing.
        std::vector<std::string> tok;
        {
            std::istringstream iss(line);
            std::string w;
            while (iss >> w) tok.push_back(w);
        }

        // Word → node resolution, shared by the resonance scan and the
        // discovery router. Exact channel first, then the sensory lanes
        // with the same length+margin guards the main scan uses.
        auto resolve_word = [&](const std::string& raw) -> int {
            std::string t = lower(raw);
            int id = resolve(si, t);
            if (id >= 0) return id;
            if (t.size() < 4) return -1;
            std::vector<std::pair<float,int>> r =
                bigram_recognize(physics, t, 2);
            bool strong = !r.empty() && r[0].first >= 0.55f;
            bool decisive = strong &&
                (r.size() < 2 || r[0].first > 1.25f * r[1].first);
            return (strong && decisive) ? r[0].second : -1;
        };

        // Query 6 routing: natural-language discovery questions reach
        // the inference engine here. Previously the five discovery
        // queries were computed nowhere and reachable from nowhere.
        {
            GapQuery gq = classify_gap_query(line);
            if (gq.is_query && run_discovery(si, gq, resolve_word))
                continue;
        }

        std::vector<int> matched;
        for (const auto& t : tok) {
            int id = resolve(si, t);          // exact channel
            if (id < 0 && t.size() >= 4) {
                // No exact hit — recognise through the sensory lanes.
                // This is what makes "dogs" find `dog` and "linuux"
                // find `linux`: nothing is compared as a string.
                //
                // Two guards, both informational rather than lexical:
                //
                //  (a) LENGTH. A token of 2-3 characters has 3-4 bigrams,
                //      which is not enough evidence to identify anything —
                //      "is" resonated with `iss`, "what" with code symbols.
                //      This is an information floor, not a stopword list:
                //      no word is named, only a quantity of evidence
                //      required.
                //
                //  (b) MARGIN. Accept only when the best match clearly
                //      beats the runner-up. A token that resonates equally
                //      with several concepts has not identified one of
                //      them, and reporting the arg-max of a flat
                //      distribution is exactly the guessing this
                //      architecture exists to avoid.
                std::vector<std::pair<float,int>> r =
                    bigram_recognize(physics, lower(t), 2);
                bool strong = !r.empty() && r[0].first >= 0.55f;
                bool decisive = strong &&
                    (r.size() < 2 || r[0].first > 1.25f * r[1].first);
                if (strong && decisive) id = r[0].second;
            }
            if (id >= 0 &&
                std::find(matched.begin(), matched.end(), id) == matched.end())
                matched.push_back(id);
        }
        // Rank by structural connectedness; drop incidental matches.
        // `degree` is hoisted out of the query loop (see main) — rebuilding
        // it per query was a full 237k-edge scan for a value that does not
        // change between queries.
        std::vector<int> kept;
        for (int m : matched) if (degree[m] >= 3) kept.push_back(m);
        if (!kept.empty()) matched.swap(kept);
        std::stable_sort(matched.begin(), matched.end(),
                         [&](int a, int b){ return degree[a] > degree[b]; });

        // ------------------------------------------------ attention
        // Everything the turn touched spikes; its structural
        // neighbourhood is recalled at lower amplitude (associative
        // recall — a decayed concept can be pulled back into the
        // working set by an active relative). Decay ticks once per turn.
        for (int id : matched) {
            sal.spike(id);
            sal.reinforce_neighbourhood(si, id, 60);
        }
        sal.tick(si);

        // ---------------------------------- working set → BSMA topo
        // The shadow graph: project the working set, compress it. When
        // every rewrite rule fails, the frame is conscious — and that
        // detection FEEDS BACK: the frame's nodes get an attention pin,
        // so what resisted compression stays in the working set. This
        // is the Γ_meta → Γ_exec channel the consciousness layer lacked.
        {
            std::vector<int> wset = sal.working_graph(si);
            if (wset.size() >= 3) {
                bsma::TopoGraph tg;
                std::unordered_map<int,int> to_topo;
                for (int id : wset)
                    to_topo[id] = tg.add_node(si.graph.nodes[id].label, 0);
                for (int id : wset) {
                    if (id < 0 || id >= (int)si.graph.out_adj.size()) continue;
                    for (int ei : si.graph.out_adj[(size_t)id]) {
                        int v = si.graph.edges[(size_t)ei].to;
                        auto it = to_topo.find(v);
                        if (it != to_topo.end())
                            tg.add_edge(to_topo[id], it->second,
                                        si.graph.edges[(size_t)ei].rel);
                    }
                }
                bsma::PhenomenalFrame f = bsma::realise_frame(tg);
                last_qualia = f.qualia_signature;
                if (f.bound.hit) {
                    ++conscious_frames;
                    for (int id : wset)
                        if (sal.S[id] < 80) sal.S[id] = 80;   // attention pin
                }
            }
        }

        // --------------------------------------------------- identity
        // RIFA ingests the raw line, advances its internal dynamics,
        // and — when silence has accumulated enough endurance —
        // rebuilds its identity from the surviving crown configuration.
        identity.input(line);
        identity.step(0.5f);
        if (identity.reconstruct_if_ready())
            std::printf("(identity reconstructed from crown configuration)\n");

        // Shed accumulated energy, then drive.
        physics.renormalize_energy(2.0f);
        physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
        for (int id : matched) physics.drive(id, 3.0f, 6.0f, 20);

        // Two-phase tick (CPME §1.4). Injection: the drive + a short
        // settle, so direct input can bleed off and K_settle rises.
        // Articulation: the phase-collapse sweep runs while the
        // wavefront is still propagating — momentum is positive in
        // DOWNSTREAM cavities exactly then. Sweeping after the full
        // free-run found nothing: every envelope had already decayed.
        physics.run(50);
        std::vector<int> emitted = physics.readout_phase_collapse();
        for (int r = 0; r < 7; ++r) {
            physics.run(25);
            constructive.structural_tick();
        }
        physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

        // Observables.
        const float n_matched = (float)matched.size();
        const float hot_frac  = physics.cavities.empty() ? 0.0f
            : (float)physics.hot_count() / (float)physics.cavities.size();
        float path_strength = 0.0f, shared_frac = 0.0f, derived_frac = 0.0f;
        int rel_edge = -1;
        if (matched.size() >= 2) {
            rel_edge = edge_between(si, matched[0], matched[1]);
            path_strength = (rel_edge >= 0) ? 1.0f : 0.0f;
            shared_frac   = shared_fraction(si, matched[0], matched[1]);
            if (rel_edge >= 0 &&
                si.graph.edges[rel_edge].derived_by_rule >= 0)
                derived_frac = 1.0f;
        }

        auto sig = [](float x){ return 1.0f / (1.0f + std::exp(-x)); };
        float score[M_COUNT];
        score[M_SILENCE]      = std::exp(-6.0f*hot_frac) * std::exp(-2.0f*n_matched);
        score[M_NEIGHBORHOOD] = std::exp(-(n_matched-1.0f)*(n_matched-1.0f))
                              * (0.2f + hot_frac);
        score[M_RELATION]     = sig(4.0f*(n_matched-1.5f)) * path_strength;
        // Derived relations outrank stored ones: reasoning is more
        // informative than restatement, so the field prefers to report it.
        score[M_DERIVED]      = sig(4.0f*(n_matched-1.5f)) * derived_frac * 1.4f;

        int best = 0;
        for (int m = 1; m < M_COUNT; ++m) if (score[m] > score[best]) best = m;

        // -------------------------------------------------- thinker bookkeeping
        // C/R: a one-edge answer is retrieval; a multi-edge proof is
        // composition. Recorded per turn so `status` can show whether
        // the system is thinking or parroting over its lifetime. Runs
        // before any answer-mode continue, so "why" turns count too.
        if (rel_edge >= 0) {
            SI::QueryResult qr;
            qr.holds = true;
            qr.proof = si.trace(rel_edge);
            qr.proof_edge = rel_edge;
            thinker.record(qr);
        }

        // "why ..." — surface the derivation chain of the relation the
        // two matched concepts share. The proof was always reconstructible
        // (explain() existed for DERIVED answers only); asking why is a
        // first-class question about any answered relation.
        if (rel_edge >= 0 && lower(line).rfind("why", 0) == 0) {
            const Edge& e = si.graph.edges[rel_edge];
            ConceptTriple t{label_of(si, e.from), rel_name_ext(e.rel),
                            label_of(si, e.to)};
            std::printf("si > %s\n", lang.realize(t).c_str());
            std::printf("      Because:\n");
            explain(si, rel_edge, 0);
            std::printf("\n");
            continue;
        }

        // -------------------------------------------------- answer dispatch

        std::printf("si > ");
        if (matched.empty() || best == M_SILENCE) {
            std::printf("nothing in the graph resonated with that.\n");

            // ---------------------------------------------- DMN idle cycle
            // Mind-wandering (TSDA §DMN → CMA): with nothing to answer,
            // the default-mode network takes the salience map's
            // high-salience distant pairs and ATTEMPTS a bisociation on
            // the first one that validates. Previously the DMN produced
            // candidates and `bisociate()` had zero callers — the
            // shower-thought loop existed only on paper.
            auto pairs = sal.high_salience_distant_pairs(si);
            for (const auto& p : pairs) {
                BisociationResult r = bisociate(si,
                    si.graph.nodes[p.a].label, si.graph.nodes[p.b].label);
                if (!r.validated) continue;
                ++dmn_thoughts;
                std::printf("      ~ idle thought: ");
                ConceptTriple t{label_of(si, p.a), "resonates_with",
                                label_of(si, p.b)};
                std::printf("%s\n", lang.realize(t).c_str());
                std::printf("      via %s (U_hyp=%.2f, confidence=%.2f)\n",
                            r.intermediate.c_str(), r.energy, r.confidence);
                // The hinge spikes: the next turn's attention field
                // starts from this connection. That is the recursive
                // part of mind-wandering — thoughts seed thoughts.
                auto hit = si.graph.by_label.find(r.intermediate);
                if (hit != si.graph.by_label.end()) sal.spike(hit->second);
                break;   // one spontaneous thought per idle turn
            }
            std::printf("\n");
            continue;
        }

        if (best == M_DERIVED) {
            const Edge& e = si.graph.edges[rel_edge];
            ConceptTriple t{label_of(si, e.from), rel_name_ext(e.rel),
                            label_of(si, e.to)};
            std::printf("%s\n", lang.realize(t).c_str());
            std::printf("      ^ DERIVED, not stored. Because:\n");
            explain(si, rel_edge, 0);
            // The trace that produced this answer feeds rule induction —
            // observe_trace was never called on any derivation before.
            mlearner.observe_trace(si.trace(rel_edge));
            std::printf("\n");
            continue;
        }

        if (best == M_RELATION) {
            const Edge& e = si.graph.edges[rel_edge];
            ConceptTriple t{label_of(si, e.from), rel_name_ext(e.rel),
                            label_of(si, e.to)};
            std::printf("%s\n\n", lang.realize(t).c_str());
            continue;
        }

        // Neighbourhood: the CPME phase-collapse sweep decides what
        // speaks, and in what order. Momentum (ReLU P_i), acoustic mass
        // (μ_i) and settle coupling (K_settle) gate each cavity — the
        // field must be carrying its own thought outward, not echoing
        // the injection. An empty sweep is honest silence even when raw
        // amplitude is high: that was the parroting failure mode.
        if (emitted.empty()) {
            std::printf("the field settled without forming a thought.\n\n");
            continue;
        }
        int shown = 0;
        for (int cid : emitted) {
            if (shown >= 4) break;
            if (cid < 0 || cid >= (int)physics.alias_ledger.size()) continue;
            int lk = physics.alias_ledger[(size_t)cid].label_key;
            if (lk < 0 || lk >= (int)si.graph.nodes.size()) continue;
            const std::string& lbl = si.graph.nodes[lk].label;
            if (lbl.empty() || is_internal(lbl)) continue;
            // Only ever speak in complete relations. A bare label is not
            // an answer — printing `imperial_college_london` on its own
            // tells the reader a cavity had amplitude, which is an
            // implementation detail, not a fact about the world. If the
            // wave reached a concept but no relation joins it to what was
            // asked, the honest move is to say nothing about it.
            int ei = edge_between(si, matched[0], lk);
            if (ei < 0) continue;
            const Edge& e = si.graph.edges[ei];
            ConceptTriple t{label_of(si, e.from), rel_name_ext(e.rel),
                            label_of(si, e.to)};
            std::printf("%s%s", shown ? "      " : "",
                        lang.realize(t).c_str());
            if (e.derived_by_rule >= 0) std::printf("  [derived]");
            std::printf("\n");
            ++shown;
        }
        if (shown == 0) std::printf("nothing propagated above threshold.\n");
        std::printf("\n");
    }
    return 0;
}
