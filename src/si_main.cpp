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

    std::string line;
    while (true) {
        std::printf("you> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line)) break;
        while (!line.empty() && std::isspace((unsigned char)line.back()))
            line.pop_back();
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

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

        // Shed accumulated energy, then drive.
        physics.renormalize_energy(2.0f);
        physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
        for (int id : matched) physics.drive(id, 3.0f, 6.0f, 20);
        for (int r = 0; r < 8; ++r) {
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

        std::printf("si > ");
        if (matched.empty() || best == M_SILENCE) {
            std::printf("nothing in the graph resonated with that.\n\n");
            continue;
        }

        if (best == M_DERIVED) {
            const Edge& e = si.graph.edges[rel_edge];
            ConceptTriple t{label_of(si, e.from), rel_name_ext(e.rel),
                            label_of(si, e.to)};
            std::printf("%s\n", lang.realize(t).c_str());
            std::printf("      ^ DERIVED, not stored. Because:\n");
            explain(si, rel_edge, 0);
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

        // Neighbourhood: realise the strongest relation the wave found.
        std::vector<std::pair<float,int>> ranked;
        for (int i = 0; i < (int)physics.cavities.size(); ++i) {
            if (std::find(matched.begin(), matched.end(), i) != matched.end())
                continue;
            ranked.push_back({std::fabs(physics.cavities[i].displacement), i});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){ return a.first > b.first; });

        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= 4 || r.first < 1e-5f) break;
            int lk = physics.alias_ledger[r.second].label_key;
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
