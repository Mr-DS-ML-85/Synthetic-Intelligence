// rade.hpp — Recursive Discovery of Resonance.
//
// Depth-first tunnel traversal that naturally stops at high-tension
// nodes. Implemented as a continuous wavefront on the tunnel matrix
// with per-step attenuation by (1 − τ_target). A high-tension target
// crushes the wavefront amplitude to near-zero, so the wave dies there
// without an explicit if-branch. Every visited node accumulates a
// tension trace; when the search ends, the peak-tension node is the
// "resolution candidate" and its base_freq / touching-tunnel polarity
// are nudged by the MAA Dispatcher path.
//
// The traversal body has zero if/switch statements.

#pragma once

#include "layer2.hpp"
#include "physics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <utility>
#include <vector>

namespace unified {

class RADE {
public:
    PhysicsEngine*      sub = nullptr;
    ConstructiveEngine* eng = nullptr;

    // Continuous state
    std::vector<float> wavefront;
    std::vector<float> next_wavefront;
    std::vector<float> tension_trace;   // per-cavity accumulated τ during search

    // Parameters
    float step_gain    = 0.20f;
    float decay        = 0.85f;
    float trace_gain   = 0.10f;
    float trace_decay  = 0.98f;
    // Resolve parameters
    float g_resolve_freq = 0.03f;       // extra MAA shot at peak node
    float g_resolve_pol  = 0.15f;       // extra polarity drift on touching tunnels
    // Peak readouts
    int   peak_node    = -1;
    float peak_tension = 0.0f;

    void bind(PhysicsEngine* p, ConstructiveEngine* e) {
        sub = p; eng = e;
        int N = (int)p->cavities.size();
        wavefront.assign(N, 0.0f);
        next_wavefront.assign(N, 0.0f);
        tension_trace.assign(N, 0.0f);
    }

    void reset() {
        std::fill(wavefront.begin(),      wavefront.end(),      0.0f);
        std::fill(next_wavefront.begin(), next_wavefront.end(), 0.0f);
        std::fill(tension_trace.begin(),  tension_trace.end(),  0.0f);
        peak_node = -1;
        peak_tension = 0.0f;
    }

    void seed(int node_id, float amp) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        wavefront[node_id] += amp;
    }

    // ============================================================
    //  One RADE propagation step. Zero if/switch.
    //  Wave propagates through every tunnel with attenuation
    //  (1 − τ_target). High tension crushes the wavefront naturally.
    // ============================================================
    void step_once() {
        std::lock_guard<std::mutex> lk(sub->mtx);
        int N = (int)sub->cavities.size();
        int T = (int)sub->tunnels.size();

        std::fill(next_wavefront.begin(), next_wavefront.end(), 0.0f);

        for (int e = 0; e < T; ++e) {
            const auto& t = sub->tunnels[e];
            float attn_ab = 1.0f - eng->local_tension[t.cavity_b];
            float attn_ba = 1.0f - eng->local_tension[t.cavity_a];
            // Symmetric propagation — both directions carry the wave.
            next_wavefront[t.cavity_b] += wavefront[t.cavity_a]
                                       * std::fabs(t.stiffness) * attn_ab * step_gain;
            next_wavefront[t.cavity_a] += wavefront[t.cavity_b]
                                       * std::fabs(t.stiffness) * attn_ba * step_gain;
        }

        for (int i = 0; i < N; ++i) {
            float w = std::tanh(next_wavefront[i]);
            // Tension trace EMA — records where the wave met resistance.
            tension_trace[i] = tension_trace[i] * trace_decay
                             + trace_gain * eng->local_tension[i] * std::fabs(w);
            // Decay old wavefront.
            wavefront[i] = w * decay;
        }
    }

    // Full search from seed. steps controls depth. Records peak.
    void search(int seed_node, float amp, int steps) {
        reset();
        seed(seed_node, amp);
        for (int s = 0; s < steps; ++s) step_once();
        // Find peak-tension node (I/O, allowed to branch).
        int best = 0;
        float best_v = tension_trace[0];
        for (int i = 1; i < (int)tension_trace.size(); ++i) {
            if (tension_trace[i] > best_v) { best_v = tension_trace[i]; best = i; }
        }
        peak_node    = best;
        peak_tension = best_v;
    }

    // Resolve — nudge peak node's base_freq toward neutral, and drift
    // polarity of every tunnel touching it. Pure scalar body.
    void resolve() {
        if (peak_node < 0) return;
        std::lock_guard<std::mutex> lk(sub->mtx);
        int T = (int)sub->tunnels.size();
        int p = peak_node;
        float tau = eng->local_tension[p];
        sub->cavities[p].base_freq +=
            g_resolve_freq * tau * (eng->omega_neutral - sub->cavities[p].base_freq);
        for (int e = 0; e < T; ++e) {
            auto& t = sub->tunnels[e];
            // Touching-tunnel mask (0/1) — implemented branchlessly as
            // (a == p ? 1 : 0) + (b == p ? 1 : 0), but comparisons in
            // integer form. Cast the boolean.
            float touch_a = (float)(t.cavity_a == p);
            float touch_b = (float)(t.cavity_b == p);
            float touches = touch_a + touch_b;
            float p_target = -std::tanh(eng->corr_ema[e] * eng->split_corr_scale);
            t.polarity += g_resolve_pol * tau * touches * (p_target - t.polarity);
        }
    }

    // Emit peak result — I/O only.
    void dump_peak(FILE* out = stdout) {
        if (peak_node < 0) { std::fprintf(out, "no_search_yet\n"); return; }
        std::fprintf(out, "peak_node=%s  peak_tension=%.5f\n",
                     sub->graph_ptr->graph.nodes[peak_node].label.c_str(),
                     peak_tension);
    }

    // Emit tunnel polarities — the "splits" the engine has decided.
    // Sorted by |polarity − sign(base)| to show which ones drifted.
    void dump_polarities(int k, FILE* out = stdout) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        int T = (int)sub->tunnels.size();
        std::vector<std::pair<float,int>> ranked;
        ranked.reserve(T);
        for (int e = 0; e < T; ++e)
            ranked.push_back({std::fabs(sub->tunnels[e].polarity), e});
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){
                      return std::fabs(a.first - 1.0f) > std::fabs(b.first - 1.0f);
                  });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= k) break;
            const Tunnel& t = sub->tunnels[r.second];
            std::fprintf(out, "%s -> %s | pol: %+.4f | k: %.4f | corr_ema: %+.5f\n",
                sub->graph_ptr->graph.nodes[t.cavity_a].label.c_str(),
                sub->graph_ptr->graph.nodes[t.cavity_b].label.c_str(),
                t.polarity, t.stiffness,
                eng->corr_ema[r.second]);
            ++shown;
        }
    }
};

} // namespace unified
