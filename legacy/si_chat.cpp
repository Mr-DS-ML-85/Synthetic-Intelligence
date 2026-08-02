// si_chat — Continuous Morphological Dynamics.
//
// CMD architecture — three physics mechanisms, no if-else on relations:
//
//   1. Thermodynamic Habituation (physics.hpp)
//        Per-cavity heat T_i integrates |v|. Coupling collapses via
//        Curie point exp(-T/T_crit). Stopwords cook themselves out.
//
//   2. Byte-Wave Stream Injection (this file)
//        No word tokenisation. Every byte in the input drives a
//        byte-lane cavity at a byte-specific micro-frequency. Word
//        emergence is where constructive interference builds. Known
//        concepts are also n-gram scanned against graph labels — this
//        is not tokenisation, it is a resonance-detection lookup.
//
//   3. Phase-Locked Syntax Emission (this file + physics.hpp bind_syntax_slots)
//        Reply is the ordered readout of the Kuramoto syntax slots.
//        No sentence template. The engine emits what phase-locked into
//        each slot, in slot order. If a slot is empty, no word for it.
//
// One binary. One prompt. State persists across turns. No flags.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "csv_load.hpp"
#include "layer2.hpp"
#include "physics.hpp"
#include "primitives.hpp"
#include "rade.hpp"
#include "stream.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace unified;

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

// N-gram resonance scan: for every contiguous byte-window of the input,
// check whether the lowercase form is a graph cavity. Longest matches
// win. This is NOT tokenisation — it is spatial pattern matching over
// the byte stream, exactly analogous to how the cochlea detects harmonic
// windows in a raw acoustic signal. The engine never asks "what is a
// word"; it asks "does this byte-window resonate with any known concept."
struct Match {
    size_t start, end;
    int cavity_id;
    bool freshly_spawned;
};

static std::string lower_alnum(const std::string& s) {
    std::string out;
    for (char c : s) {
        unsigned char u = (unsigned char)c;
        if (std::isalnum(u) || c == '_' || c == '-' || c == '.')
            out += (char)std::tolower(u);
    }
    return out;
}

static std::vector<Match> resonance_scan(ExtSI& si,
                                        PhysicsEngine& physics,
                                        ConstructiveEngine& constructive,
                                        RADE& rade,
                                        const std::string& line,
                                        std::unordered_set<int>& session_new)
{
    std::vector<Match> matches;
    size_t n = line.size();
    // Longest-match first — spawn any non-covered chunks after known
    // concepts have been claimed.
    std::vector<char> covered(n, 0);
    for (size_t len = 24; len >= 3; --len) {
        if (len > n) continue;
        for (size_t i = 0; i + len <= n; ++i) {
            bool overlap = false;
            for (size_t k = 0; k < len; ++k) if (covered[i+k]) { overlap = true; break; }
            if (overlap) continue;
            // Boundary check: must not start/end mid-alphanumeric.
            if (i > 0 && std::isalnum((unsigned char)line[i-1])) continue;
            if (i + len < n && std::isalnum((unsigned char)line[i+len])) continue;
            std::string window = line.substr(i, len);
            std::string key = lower_alnum(window);
            if (key.empty()) continue;
            auto it = si.graph.by_label.find(key);
            if (it == si.graph.by_label.end()) continue;
            matches.push_back({i, i+len, it->second, false});
            for (size_t k = 0; k < len; ++k) covered[i+k] = 1;
        }
    }
    // Un-covered contiguous alphanumeric runs become auto-spawn candidates.
    size_t i = 0;
    while (i < n) {
        while (i < n && (covered[i] || !std::isalnum((unsigned char)line[i]))) ++i;
        if (i >= n) break;
        size_t j = i;
        while (j < n && !covered[j] && std::isalnum((unsigned char)line[j])) ++j;
        std::string chunk = line.substr(i, j - i);
        std::string key = lower_alnum(chunk);
        if (!key.empty() && key.size() >= 2) {
            int new_id = si.graph.add_or_get_node(key);
            // Rebind physics on the extended graph while preserving state.
            std::vector<float> saved_x(physics.cavities.size(), 0.0f);
            std::vector<float> saved_v(physics.cavities.size(), 0.0f);
            std::vector<float> saved_h(physics.cavities.size(), 0.0f);
            for (size_t c = 0; c < physics.cavities.size(); ++c) {
                saved_x[c] = physics.cavities[c].displacement;
                saved_v[c] = physics.cavities[c].velocity;
                saved_h[c] = physics.cavities[c].heat_T;
            }
            physics.bind(si);
            for (size_t c = 0; c < saved_x.size() && c < physics.cavities.size(); ++c) {
                physics.cavities[c].displacement = saved_x[c];
                physics.cavities[c].velocity     = saved_v[c];
                physics.cavities[c].heat_T       = saved_h[c];
            }
            constructive.bind(&physics);
            rade.bind(&physics, &constructive);
            physics.bind_syntax_slots(3);
            session_new.insert(new_id);
            matches.push_back({i, j, new_id, true});
        }
        i = j;
    }
    // Order by start position — this is the byte-stream reading order,
    // the same order the Kuramoto slots will phase-lock in.
    std::sort(matches.begin(), matches.end(),
              [](const Match& a, const Match& b){ return a.start < b.start; });
    return matches;
}

// Byte-wave stream drive: every character byte drives its own dedicated
// sensory lane (the cochlea — 256 ByteLane oscillators inside the
// engine). Lanes reach the concept field ONLY through Hebbian-learned
// byte→concept bindings, which start at zero. Bytes that occur inside a
// resonance-matched window get a nascent binding to that concept —
// creation is wiring, survival is physics: co-activity grows the weight,
// disuse starves it. The old version hash-mod injected every byte into a
// RANDOM concept cavity, which made the readout pure noise ("minecraft"
// heated "alexandria"). That path is gone.
// MRS §1.2 — ticks advanced between consecutive byte injections. Must
// be ≥ 1 for a chirality gradient to exist at all. Larger values give
// wider phase separation between letters at the cost of turn latency.
// Sized so a word spans most of the phase circle rather than a small
// arc: with common lane ω = 3.0 rad/s and dt = 0.02, a spacing of 15
// ticks gives ω·Δt = 0.9 rad per letter, so ~7 letters cover ~2π.
// At the earlier value of 3 ticks the whole word occupied ~1 rad and
// the letters were unresolvable under the sweep window.
static const int mrs_stream_ticks = 15;

static void byte_wave_drive(PhysicsEngine& physics, const std::string& line,
                            const std::vector<Match>& matches) {
    // MRS §1.2 / HTR §2.1 — bytes must be injected with genuine
    // temporal separation, otherwise every lane records an identical
    // t_arrival and the chirality gradient is zero. Driving the whole
    // line in one loop before any tick (the earlier implementation)
    // left sim_time unadvanced and produced no phase gradient at all.
    // Streaming one byte per few ticks is what makes letter order a
    // physical quantity rather than a no-op.
    for (size_t i = 0; i < line.size(); ++i) {
        physics.drive_byte((unsigned char)line[i], 1.0f, 8);
        physics.run(mrs_stream_ticks);
    }
    for (const auto& m : matches)
        for (size_t i = m.start; i < m.end && i < line.size(); ++i)
            physics.bind_byte((unsigned char)line[i], m.cavity_id);
    // MRS §1.3 — observe byte adjacency within each matched window so
    // lane↔lane gauge tunnels exist for the letter pairs that actually
    // co-occur inside words. Creation is wiring; the learned Ψ and
    // weight are physics. Adjacency is not taken across word
    // boundaries, so the offsets encode intra-word order only.
    for (const auto& m : matches) {
        for (size_t i = m.start; i + 1 < m.end && i + 1 < line.size(); ++i) {
            unsigned char a = (unsigned char)line[i];
            unsigned char b = (unsigned char)line[i + 1];
            physics.bind_lane_pair(a, b);
        }
    }
}

int main(int, char**) {
    ExtSI si;
    build_big_ontology(si);
    autoload_axioms(si);
    si.reason_ext();

    PhysicsEngine physics;
    physics.bind(si);
    physics.bind_syntax_slots(3);
    ConstructiveEngine constructive;
    constructive.bind(&physics);
    RADE rade;
    rade.bind(&physics, &constructive);

    // MRS (research-paper/mrs.md) is OFF by default — it emits babble
    // until lane↔lane phase offsets consolidate (paper §5.1). Enable
    // with SI_MRS=1 to exercise byte-level construction alongside the
    // label-emission path.
    // CSAT is opt-in: its Lyapunov sensitivity breaks the bit-exact
    // reproducibility the seed/ablation harnesses depend on
    // (CSAT §4.1). Conversation may enable it; studies must not.
    const char* csat_env = std::getenv("SI_CSAT");
    physics.csat_enabled = (csat_env && csat_env[0] == '1');

    const char* mrs_env = std::getenv("SI_MRS");
    physics.mrs_enabled = (mrs_env && mrs_env[0] == '1');

    std::unordered_set<int> session_new;

    std::fprintf(stdout,
        "CMD is up. Byte-wave input, thermodynamic habituation, "
        "phase-locked emission. Type. Anything.\n\n");

    std::string line;
    while (true) {
        std::fprintf(stdout, "you> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line)) break;
        while (!line.empty() && std::isspace((unsigned char)line.back())) line.pop_back();
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

        // Phase 3 — shed accumulated energy before this turn's input.
        // Without this, 30% of the previous turn's amplitude survives
        // and outcompetes anything the new input excites.
        float shed = physics.renormalize_energy(2.0f);
        if (shed < 0.999f && getenv("SI_VERBOSE"))
            std::fprintf(stdout, "[field renormalised x%.3f]\n", shed);

        // N-gram resonance scan for known concepts + spawn for unknowns.
        auto matches = resonance_scan(si, physics, constructive, rade,
                                      line, session_new);

        // Byte-wave drive on the sensory lanes — every byte enters the
        // substrate, none are discarded, and none pollute the concept
        // field until experience has bound them.
        byte_wave_drive(physics, line, matches);

        // Concept-level drive on matched cavities.
        physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
        for (const auto& m : matches) physics.drive(m.cavity_id, 3.0f, 6.0f, 20);
        // Longer settle to allow syntax slots to phase-lock.
        for (int r = 0; r < 8; ++r) {
            physics.run(25);
            constructive.structural_tick();
        }
        physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

        // CPME §1.6 — Phase-Collapse Projection replaces the fixed
        // three-slot readout. The carrier phase sweeps [0, 2π); each
        // step arg-maxes on a Gaussian window over per-cavity phase
        // atan2(v, ω₀·x), filtered by momentum · acoustic-mass mask ·
        // settle coupling. Output length is emergent — as many tokens
        // as the field has distinct high-momentum phase peaks above
        // the emission floor. Silence when the field is not resonant.
        auto winners = physics.readout_phase_collapse();
        std::vector<std::string> reply_words;
        for (int cid : winners) {
            if (cid < 0) continue;
            int lk = physics.alias_ledger[cid].label_key;
            if (lk < 0 || lk >= (int)si.graph.nodes.size()) continue;
            std::string lbl = si.graph.nodes[lk].label;
            if (lbl.empty()) continue;
            // Structural bookkeeping nodes are not concepts. The
            // `f__argN` ordinal slots introduced for signature
            // reconstruction exist to carry position, and a reader
            // should never see them — they are the graph's internal
            // scaffolding, like a symbol table entry. Filtered at the
            // I/O boundary only; they remain fully available to
            // reasoning and to the serializer.
            if (lbl.find("__arg") != std::string::npos) continue;
            // I/O boundary — graph keys use '_' as an intra-label
            // separator (insulin_signaling, laws_of_conservation).
            // The graph shape is irrelevant to a human reader; render
            // underscores as spaces so multi-word concepts read as
            // multi-word phrases. Purely presentational; no state
            // change, no physics touched.
            for (char& c : lbl) if (c == '_') c = ' ';
            reply_words.push_back(lbl);
        }

        std::fprintf(stdout, "si > ");
        if (reply_words.empty()) {
            std::fprintf(stdout, "…\n\n");
        } else {
            for (size_t k = 0; k < reply_words.size(); ++k) {
                std::fprintf(stdout, "%s%s",
                    reply_words[k].c_str(),
                    (k + 1 == reply_words.size()) ? "" : " ");
            }
            std::fprintf(stdout, "\n\n");
        }

        // MRS §1.4 — byte-level construction, shown alongside the
        // label emission so the two can be compared directly during
        // consolidation. Silent unless SI_MRS=1.
        if (physics.mrs_enabled) {
            std::string synth = physics.readout_byte_sweep();
            std::fprintf(stdout, "mrs> %s\n\n",
                         synth.empty() ? "(silence)" : synth.c_str());
        }
    }
    return 0;
}
