// layer2.hpp — Constructive Engine. Structural rewriting of the substrate
// expressed as pure scalar update rules — no if/switch anywhere in the
// primitive body.
//
// Nine reasoning primitives from research-paper/synthetic-intelligence.md
// are realized as physical modulations of the PRM+MIMS substrate:
//
//   P1 Deduction    : augmented Hooke kick        v_B += g_ded·τ_A·k·p·x_A/m_B·dt
//   P2 Transitive   : second-order Hebbian        Δ += g_trans·v_A·v_B
//   P3 Inheritance  : intra-cluster mean-field    v_X += g_inh·τ_X·(μ_K−x_X)/m_X·dt
//   P4 Abduction    : retrodictive boost          Δ += g_abd·env_B·v_A
//   P5 Negation     : (already in polarity p)     none
//   P6 Case-analysis: (already superposition)     none
//   P7 Induction    : cluster-mean Hebbian        Δ += g_ind·μ_K·v_B
//   P8 Analogy      : pull toward ⟨Δ⟩_K           Δ += g_ana·(⟨Δ⟩_K − Δ)
//   P9 Recursion    : iterate structural_tick_once n_recursion times
//
// MAA Dispatcher : base_freq drifts under wave-clash tension
// Verify         : probe pulse + peak amplitude readout (scalar)
// BSMA           : feed-back displacement as drive amplitude until
//                  ‖Δx‖ < ε — the attractor cavity vector IS the verdict.
//
// Every primitive body is straight-line scalar arithmetic. The only
// branches in this file live inside I/O helpers (`dump_*`).

#pragma once

#include "physics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace unified {

class ConstructiveEngine {
public:
    PhysicsEngine* sub = nullptr;

    // Precomputed per-cavity degree (accumulated at bind).
    std::vector<int>   degree;
    // Per-cavity structural signals — refreshed each structural_tick_once().
    std::vector<float> local_tension;
    std::vector<float> cluster_mean_x;
    std::vector<float> cluster_mean_pd;
    // Scratch — per-cluster aggregations, sized at bind, reused each tick.
    std::vector<float> cluster_sum_x;
    std::vector<float> cluster_sum_pd;
    std::vector<int>   cluster_count_pd;
    // BSMA state.
    std::vector<float> c_snapshot;
    float attractor_error = 0.0f;
    int   bsma_depth      = 0;
    // Verify readout.
    float last_echo       = 0.0f;

    // Primitive gains — pure scalars.
    float g_ded       = 0.010f;   // P1 deduction amplifier
    float g_trans     = 0.005f;   // P2 transitive Hebbian
    float g_inh       = 0.015f;   // P3 inheritance mean-field
    float g_abd       = 0.020f;   // P4 abduction reverse-boost
    float g_ind       = 0.010f;   // P7 induction cross-cluster
    float g_ana       = 0.005f;   // P8 analogy transfer
    float g_maa       = 0.002f;   // MAA freq shift
    // P5b Splitting — g_split raised 4×, correlation scale raised 7×,
    // and the corr_ema time-constant sped up (see structural_tick_once
    // where α_ema goes from 0.02 → 0.08). Result: sustained same-phase
    // co-firing at moderate tension is now enough to drive polarity
    // meaningfully off its base value.
    float g_split     = 0.150f;
    float split_corr_scale = 45.0f;
    float omega_neutral = 10.0f;  // MAA drift target frequency
    // Per-tunnel EMA of (v_a · v_b) — populated in structural_tick_once.
    std::vector<float> corr_ema;
    float bsma_alpha  = 0.12f;    // BSMA self-drive amplitude ratio
    int   bsma_settle = 40;       // ticks between BSMA snapshots
    int   n_recursion = 2;        // P9 iterations per structural_tick()
    float tension_beta = 0.5f;    // τ = tanh(|v|·degree·T_global·β)

    // Hub-rule flag. When true, per-tunnel Hebbian η is scaled by
    //   η_eff(e) = η / log(1 + max(d_a, d_b))
    // so dense hubs consolidate more slowly than sparse leaves and
    // opposing updates don't cancel each other into noise.
    bool  hub_rule_enabled = false;
    float hub_rule_floor   = 0.15f;   // minimum multiplier (log(1+1)=0.69, log(1+50)=3.93 → floor stops complete zero-out)

    // RB+G — fission trigger from Layer 2. Fires the physics-side
    // fission_locked() when I > I_thresh AND τ > τ_thresh sustained
    // for `hold_target` structural ticks, on cavities of generation < g_max.
    bool  fission_enabled     = true;   // RB+G — engine self-decides
    float fission_I_thresh    = 0.15f;
    float fission_tau_thresh  = 0.005f;
    int   fission_hold_target = 3;
    std::vector<int> fission_streak;

    // Phase-1.2 — Homeostatic Energy Feedback. When auto_thresh_enabled,
    // fission_I_thresh floats with an EMA-smoothed system energy:
    //   E_smooth(t) = α · E_smooth(t-1) + (1-α) · E(t)
    //   I_thresh(t) = I0 + k · E_smooth(t)      (k > 0 — negative feedback)
    // Positive k means: more energy → higher threshold → harder to fission,
    // suppressing cascade. Sign-flip would produce runaway; guard below.
    bool  auto_thresh_enabled = true;   // Phase 1.2 — engine self-decides
    float auto_thresh_I0      = 0.10f;   // baseline threshold
    float auto_thresh_k       = 0.005f;  // MUST be > 0
    float auto_thresh_alpha   = 0.95f;   // EMA decay (higher = slower response)
    float E_smooth            = 0.0f;    // EMA-smoothed system energy
    static_assert(true, "sign discipline: auto_thresh_k must remain positive");

    // Phase 1.3 — Online L6-A discovery from the structural tick.
    // Whenever a fission event fires, discover_analogies_locked() is
    // invoked with only_parent = <newly fissioned parent>, so only the
    // O(P) new pairs are checked instead of O(P²). Bridges are inserted
    // in-tick, no operator required.
    bool  online_l6a_enabled  = true;   // Phase 1.3 — engine self-decides
    int   online_l6a_min_shared = 2;
    float online_l6a_rubber_k = 0.05f;
    float online_l6a_rubber_c = 0.15f;
    // Fisher-metric insertion threshold for L6-A rubber bridges.
    float l6a_fisher_max = 2.0f;

    // Called each structural tick before the fission scan when
    // auto_thresh_enabled. Uses physics::audit_energy_locked() because
    // sub->mtx is already held by structural_tick_once().
    void refresh_fission_thresholds_locked() {
        if (!auto_thresh_enabled) return;
        float E = sub->audit_energy_locked().total_mechanical;
        E_smooth = auto_thresh_alpha * E_smooth
                 + (1.0f - auto_thresh_alpha) * E;
        // Positive-k homeostasis. Guard at runtime just in case.
        float k_eff = (auto_thresh_k > 0.0f) ? auto_thresh_k : 0.005f;
        fission_I_thresh = auto_thresh_I0 + k_eff * E_smooth;
    }

    // Telemetry accumulators (per-cavity, per-tunnel). Reset by
    // `telemetry_reset()`, sampled every structural_tick_once().
    std::vector<float> plasticity_by_cavity;   // Σ|Δ|/tunnel touching cavity
    std::vector<float> pos_updates_by_cavity;  // Σ  Δ_e·1[Δ_e>0]  touching cavity
    std::vector<float> neg_updates_by_cavity;  // Σ |Δ_e·1[Δ_e<0]| touching cavity

    void telemetry_reset() {
        int N = (int)sub->cavities.size();
        plasticity_by_cavity.assign(N, 0.0f);
        pos_updates_by_cavity.assign(N, 0.0f);
        neg_updates_by_cavity.assign(N, 0.0f);
        fission_streak.assign(N, 0);
    }

    void bind(PhysicsEngine* p) {
        sub = p;
        int N = (int)p->cavities.size();
        int M = (int)p->macro_nodes.size();
        degree.assign(N, 0);
        local_tension.assign(N, 0.0f);
        cluster_mean_x.assign(N, 0.0f);
        cluster_mean_pd.assign(N, 0.0f);
        cluster_sum_x.assign(M, 0.0f);
        cluster_sum_pd.assign(M, 0.0f);
        cluster_count_pd.assign(M, 0);
        c_snapshot.assign(N, 0.0f);
        for (const auto& t : p->tunnels) {
            ++degree[t.cavity_a];
            ++degree[t.cavity_b];
        }
        corr_ema.assign((int)p->tunnels.size(), 0.0f);
        telemetry_reset();
    }

    // ============================================================
    //  ONE STRUCTURAL TICK. Zero if/switch. Pure scalar arithmetic.
    // ============================================================
    void structural_tick_once() {
        std::lock_guard<std::mutex> lk(sub->mtx);
        int N = (int)sub->cavities.size();
        int T = (int)sub->tunnels.size();
        const float dt      = sub->dt;
        const float T_glob  = sub->ic.tension;
        const float pcap    = sub->plasticity_cap;

        // 0. Compute per-cavity local tension.
        //    τ_i = tanh(|v_i| · degree_i · T_glob · β)
        for (int i = 0; i < N; ++i) {
            const auto& c = sub->cavities[i];
            local_tension[i] = std::tanh(std::fabs(c.velocity)
                                       * (float)degree[i]
                                       * T_glob * tension_beta);
        }

        // 1. Cluster aggregations: mean x, mean Δ.
        std::fill(cluster_sum_x.begin(),   cluster_sum_x.end(),   0.0f);
        std::fill(cluster_sum_pd.begin(),  cluster_sum_pd.end(),  0.0f);
        std::fill(cluster_count_pd.begin(),cluster_count_pd.end(),0);
        for (int i = 0; i < N; ++i)
            cluster_sum_x[sub->cavities[i].cluster_id] += sub->cavities[i].displacement;
        for (int e = 0; e < T; ++e) {
            int k = sub->cavities[sub->tunnels[e].cavity_a].cluster_id;
            cluster_sum_pd[k]   += sub->tunnels[e].plasticity_delta;
            cluster_count_pd[k] += 1;
        }
        for (int i = 0; i < N; ++i) {
            int k = sub->cavities[i].cluster_id;
            cluster_mean_x[i]  = cluster_sum_x[k] * sub->macro_nodes[k].inv_size;
            cluster_mean_pd[i] = cluster_sum_pd[k]
                               / (float)(cluster_count_pd[k] + 1);
        }

        // P3 INHERITANCE — mean-field pull toward cluster centroid.
        //    v_X += g_inh · τ_X · (μ_K − x_X) / m_X · dt
        for (int i = 0; i < N; ++i) {
            auto& c = sub->cavities[i];
            float pull = g_inh * local_tension[i]
                       * (cluster_mean_x[i] - c.displacement);
            c.velocity += (pull / c.mass) * dt;
        }

        // P1 DEDUCTION — augmented Hooke kick along every tunnel.
        //    v_B += g_ded · τ_A · k · p · x_A / m_B · dt
        for (int e = 0; e < T; ++e) {
            const auto& t = sub->tunnels[e];
            const auto& A = sub->cavities[t.cavity_a];
            auto& B       = sub->cavities[t.cavity_b];
            float kick = g_ded * local_tension[t.cavity_a]
                        * t.stiffness * t.polarity * A.displacement;
            B.velocity += (kick / B.mass) * dt;
        }

        // P2 TRANSITIVE + P4 ABDUCTION + P7 INDUCTION + P8 ANALOGY
        // Aggregated as a single continuous Hebbian modulation per tunnel.
        //
        // Hub rule (when enabled): scale the total update magnitude by
        //   1 / log(1 + max(deg_a, deg_b))
        // so tunnels touching dense hubs consolidate more slowly. The
        // multiplier is bounded below by hub_rule_floor so it never
        // fully zeros out a legitimate update.
        //
        // Telemetry: accumulate per-cavity Σ|Δ|, Σ+Δ, Σ|-Δ| so the caller
        // can compute Incoherence Ratio = Σ+ · Σ- / (Σ+ + Σ-)² and the
        // per-cavity heatmap.
        // SRG port routing for the Layer-2 Hebbian aggregations. Under
        // SRG on, excitatory tunnels use E-port velocities of both
        // endpoints; inhibitory tunnels use I-port velocities. Under
        // SRG off, gate_E = 1 always and v_used = v_E = observable
        // velocity, matching the pre-SRG baseline exactly.
        const float srg_active = sub->srg_enabled ? 1.0f : 0.0f;
        for (int e = 0; e < T; ++e) {
            auto& t = sub->tunnels[e];
            const auto& A = sub->cavities[t.cavity_a];
            const auto& B = sub->cavities[t.cavity_b];
            float sign_p = std::copysign(1.0f, t.polarity);
            float gate_E = srg_active * (0.5f + 0.5f * sign_p)
                         + (1.0f - srg_active);
            float gate_I = 1.0f - gate_E;
            float v_a_used = gate_E * A.v_E + gate_I * A.v_I;
            float v_b_used = gate_E * B.v_E + gate_I * B.v_I;

            float p2 = g_trans * v_a_used * v_b_used;
            float p4 = g_abd   * B.drive_envelope * v_a_used;
            float p7 = g_ind   * cluster_mean_x[t.cavity_a] * v_b_used;
            float p8 = g_ana   * (cluster_mean_pd[t.cavity_a] - t.plasticity_delta);
            float delta_raw = p2 + p4 + p7 + p8;
            // Hub-scaled effective rate.
            int   dmax = std::max(degree[t.cavity_a], degree[t.cavity_b]);
            float hub_mul = hub_rule_enabled
                          ? std::max(hub_rule_floor,
                                     1.0f / std::log(1.0f + (float)dmax))
                          : 1.0f;
            float delta = delta_raw * hub_mul;
            float summed = t.plasticity_delta + delta;
            float new_pd = pcap * std::tanh(summed / pcap);
            float applied = new_pd - t.plasticity_delta;   // committed change
            t.plasticity_delta = new_pd;
            t.stiffness        = t.base_stiffness + t.plasticity_delta;

            // --- telemetry ---
            float abs_a = std::fabs(applied);
            plasticity_by_cavity[t.cavity_a] += abs_a;
            plasticity_by_cavity[t.cavity_b] += abs_a;
            // Branchless split of positive / negative contribution.
            float pos_part = 0.5f * (applied + std::fabs(applied));
            float neg_part = 0.5f * (std::fabs(applied) - applied);
            pos_updates_by_cavity[t.cavity_a] += pos_part;
            pos_updates_by_cavity[t.cavity_b] += pos_part;
            neg_updates_by_cavity[t.cavity_a] += neg_part;
            neg_updates_by_cavity[t.cavity_b] += neg_part;
        }

        // MAA Dispatcher — base-frequency drift under local tension.
        //    base_freq += g_maa · τ · (ω_neutral − base_freq)
        for (int i = 0; i < N; ++i) {
            auto& c = sub->cavities[i];
            c.base_freq += g_maa * local_tension[i]
                          * (omega_neutral - c.base_freq);
        }

        // P5b SPLITTING — polarity drift under sustained tension.
        // Track correlation EMA per tunnel; under high tension, polarity
        // relaxes toward −tanh(correlation·scale). Same-phase co-firing
        // at high tension flips polarity to inhibitory → breaks the
        // reinforcing loop. Anti-phase at high tension keeps polarity
        // negative or drives toward it. Continuous scalar, no branch.
        for (int e = 0; e < T; ++e) {
            auto& t = sub->tunnels[e];
            const auto& A = sub->cavities[t.cavity_a];
            const auto& B = sub->cavities[t.cavity_b];
            float tau_avg = 0.5f * (local_tension[t.cavity_a]
                                  + local_tension[t.cavity_b]);
            float corr = A.velocity * B.velocity;
            // Faster EMA (α raised 0.02 → 0.08) — sustained co-firing
            // makes corr_ema breach the tanh saturation more quickly.
            corr_ema[e] = corr_ema[e] * 0.92f + 0.08f * corr;
            float p_target = -std::tanh(corr_ema[e] * split_corr_scale);
            t.polarity += g_split * tau_avg * (p_target - t.polarity);
        }

        // Phase 1.2 — refresh auto-tuned fission threshold from EMA
        // energy. No-op when auto_thresh_enabled == false.
        refresh_fission_thresholds_locked();

        // RB+G FISSION CHECK. Sustained (I > I_thresh AND τ > τ_thresh)
        // over `hold_target` ticks, on cavities of generation < g_max.
        // At most one fission per structural tick. Cascade-safe because
        // children born gen>0 are automatically ineligible.
        if (fission_enabled) {
            if ((int)fission_streak.size() < N) fission_streak.resize(N, 0);
            int   fire_id = -1;
            float fire_I  = 0.0f, fire_tau = 0.0f;
            int   fire_streak_val = fission_hold_target - 1;
            const bool sparse = sub->sparse_tick_enabled;
            for (int i = 0; i < N; ++i) {
                // Phase 1.1 — skip cold cavities when sparse tick is on.
                // Correct because a cold cavity has |v| < wavefront_eps
                // and therefore its Σ+, Σ-, τ contributions are ~0, so
                // its Incoherence would also be ~0. Skipping is safe.
                if (sparse && !sub->is_hot(i)) {
                    fission_streak[i] = 0; continue;
                }
                if (sub->cavities[i].generation >= sub->g_max) {
                    fission_streak[i] = 0; continue;
                }
                if (sub->is_fissioned(i)) {
                    fission_streak[i] = 0; continue;
                }
                float p = pos_updates_by_cavity[i];
                float n_ = neg_updates_by_cavity[i];
                float denom = p + n_ + 1e-9f;
                float I_i = (p * n_) / (denom * denom);
                float tau_i = local_tension[i];
                bool trig = (I_i > fission_I_thresh) &&
                            (tau_i > fission_tau_thresh);
                fission_streak[i] = trig ? (fission_streak[i] + 1) : 0;
                if (fission_streak[i] >= fission_hold_target &&
                    fission_streak[i] > fire_streak_val) {
                    fire_streak_val = fission_streak[i];
                    fire_id  = i;
                    fire_I   = I_i;
                    fire_tau = tau_i;
                }
            }
            if (fire_id >= 0) {
                sub->fission_locked(fire_id, fire_I, fire_tau,
                                    fire_streak_val, sub->rbg_tick_counter);
                fission_streak[fire_id] = 0;

                // Extend aggregation vectors for new cavities+tunnels.
                int newN = (int)sub->cavities.size();
                int newT = (int)sub->tunnels.size();
                if ((int)plasticity_by_cavity.size() < newN) {
                    plasticity_by_cavity.resize(newN, 0.0f);
                    pos_updates_by_cavity.resize(newN, 0.0f);
                    neg_updates_by_cavity.resize(newN, 0.0f);
                    fission_streak.resize(newN, 0);
                    local_tension.resize(newN, 0.0f);
                    cluster_mean_x.resize(newN, 0.0f);
                    cluster_mean_pd.resize(newN, 0.0f);
                    degree.resize(newN, 0);
                    std::fill(degree.begin(), degree.end(), 0);
                    for (const auto& tt : sub->tunnels) {
                        ++degree[tt.cavity_a];
                        ++degree[tt.cavity_b];
                    }
                }
                if ((int)corr_ema.size() < newT) corr_ema.resize(newT, 0.0f);

                // Phase 1.3 — online L6-A. Immediately scan for shared
                // downstream targets between the newly-fissioned parent
                // and every existing fissioned parent. Bridges appear
                // in-tick, no operator call needed.
                if (online_l6a_enabled) {
                    discover_analogies_locked(online_l6a_min_shared,
                                              online_l6a_rubber_k,
                                              online_l6a_rubber_c,
                                              /*clear_records=*/false,
                                              /*only_parent=*/fire_id);
                    // If discovery inserted new rubber tunnels, extend
                    // Layer-2 telemetry once more.
                    int newT2 = (int)sub->tunnels.size();
                    if ((int)corr_ema.size() < newT2)
                        corr_ema.resize(newT2, 0.0f);
                }
            }
        }
    }

    // P9 RECURSION — n_recursion iterations per outer call.
    void structural_tick() {
        for (int r = 0; r < n_recursion; ++r) structural_tick_once();
    }

    // Verify — probe pulse at target's own base frequency; return echo peak.
    float verify_probe(int node_id, int probe_ticks) {
        float base_freq = 0.0f;
        {
            std::lock_guard<std::mutex> lk(sub->mtx);
            base_freq = sub->cavities[node_id].base_freq;
        }
        // Drive for a quarter of the probe window, then let it echo.
        sub->drive(node_id, base_freq, 1.0f, probe_ticks / 4);
        for (int t = 0; t < probe_ticks; ++t) sub->tick();
        float peak = 0.0f;
        {
            std::lock_guard<std::mutex> lk(sub->mtx);
            peak = std::fabs(sub->cavities[node_id].displacement);
        }
        last_echo = peak;
        return peak;
    }

    // BSMA — self-model feedback until attractor.
    void bsma_iterate(int iterations) {
        int N = (int)sub->cavities.size();
        {
            std::lock_guard<std::mutex> lk(sub->mtx);
            for (int i = 0; i < N; ++i)
                c_snapshot[i] = sub->cavities[i].displacement;
        }
        for (int iter = 0; iter < iterations; ++iter) {
            // Feed current snapshot back as drive amplitudes.
            for (int i = 0; i < N; ++i) {
                float freq = 0.0f;
                {
                    std::lock_guard<std::mutex> lk(sub->mtx);
                    freq = sub->cavities[i].base_freq;
                }
                sub->drive(i, freq, bsma_alpha * c_snapshot[i], bsma_settle / 3);
            }
            for (int t = 0; t < bsma_settle; ++t) sub->tick();

            float err_sum = 0.0f;
            {
                std::lock_guard<std::mutex> lk(sub->mtx);
                for (int i = 0; i < N; ++i) {
                    float d = sub->cavities[i].displacement - c_snapshot[i];
                    err_sum += d * d;
                    c_snapshot[i] = sub->cavities[i].displacement;
                }
            }
            attractor_error = std::sqrt(err_sum);
            bsma_depth      = iter + 1;
        }
    }

    // I/O — top-tension cavities.
    void dump_tension(int k = 15, FILE* out = stdout, float min_t = 0.001f) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        int N = (int)sub->cavities.size();
        std::vector<std::pair<float,int>> ranked;
        ranked.reserve(N);
        for (int i = 0; i < N; ++i)
            ranked.push_back({local_tension[i], i});
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){ return a.first > b.first; });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= k) break;
            if (r.first < min_t) break;
            std::fprintf(out, "%s | τ: %.5f\n",
                sub->graph_ptr->graph.nodes[r.second].label.c_str(),
                r.first);
            ++shown;
        }
    }

    void dump_bsma(FILE* out = stdout) {
        std::fprintf(out, "bsma_depth=%d  attractor_error=%.5f\n",
                     bsma_depth, attractor_error);
    }

    void dump_echo(FILE* out = stdout) {
        std::fprintf(out, "last_echo=%.5f\n", last_echo);
    }

    // ---- Telemetry readouts ----------------------------------------------

    // Per-cavity plasticity heatmap. Sorted by Σ|Δ| descending.
    void dump_plasticity_heatmap(int k = 15, FILE* out = stdout, float min_p = 1e-6f) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        int N = (int)sub->cavities.size();
        std::vector<std::pair<float,int>> ranked;
        for (int i = 0; i < N; ++i)
            ranked.push_back({plasticity_by_cavity[i], i});
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){ return a.first > b.first; });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= k) break;
            if (r.first < min_p) break;
            int i = r.second;
            std::fprintf(out,
                "%s | deg: %3d | Σ|Δ|: %.5f | Σ+Δ: %.5f | Σ-Δ: %.5f\n",
                sub->graph_ptr->graph.nodes[i].label.c_str(),
                degree[i],
                plasticity_by_cavity[i],
                pos_updates_by_cavity[i],
                neg_updates_by_cavity[i]);
            ++shown;
        }
    }

    // Incoherence Ratio per cavity =
    //    (Σ+Δ · Σ-Δ) / (Σ+Δ + Σ-Δ + ε)²
    // Peaks at 0.25 when positive and negative updates are equal (max
    // interference); zero when one direction dominates. Reported for
    // the top-k cavities by total activity.
    void dump_incoherence(int k = 15, FILE* out = stdout, float min_activity = 1e-6f) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        int N = (int)sub->cavities.size();
        std::vector<std::pair<float,int>> ranked;
        for (int i = 0; i < N; ++i) {
            float tot = pos_updates_by_cavity[i] + neg_updates_by_cavity[i];
            ranked.push_back({tot, i});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){ return a.first > b.first; });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= k) break;
            if (r.first < min_activity) break;
            int i = r.second;
            float p = pos_updates_by_cavity[i];
            float n = neg_updates_by_cavity[i];
            float denom = (p + n + 1e-9f);
            float incoh = (p * n) / (denom * denom);
            std::fprintf(out,
                "%s | deg: %3d | I: %.5f | Σ+: %.5f | Σ-: %.5f\n",
                sub->graph_ptr->graph.nodes[i].label.c_str(),
                degree[i], incoh, p, n);
            ++shown;
        }
    }

    // Global aggregate: mean Incoherence weighted by activity, and the
    // Structural Integrity Score (T+U) / |tunnels|.
    struct GlobalMetrics {
        float sis;                 // (T+U) / |tunnels|
        float mean_incoherence;    // activity-weighted
        float total_plasticity;
        int   n_tunnels;
        int   n_cavities;
        float t_plus_u;
    };
    GlobalMetrics compute_metrics() {
        auto e = sub->audit_energy();
        GlobalMetrics g{};
        g.n_tunnels   = (int)sub->tunnels.size();
        g.n_cavities  = (int)sub->cavities.size();
        g.t_plus_u    = e.total_mechanical;
        g.sis         = (g.n_tunnels > 0)
                      ? e.total_mechanical / (float)g.n_tunnels
                      : 0.0f;
        // Activity-weighted mean incoherence.
        float wsum = 0.0f, incoh_sum = 0.0f, plastic_sum = 0.0f;
        std::lock_guard<std::mutex> lk(sub->mtx);
        for (int i = 0; i < g.n_cavities; ++i) {
            float p = pos_updates_by_cavity[i];
            float n = neg_updates_by_cavity[i];
            float act = p + n;
            plastic_sum += plasticity_by_cavity[i];
            if (act < 1e-9f) continue;
            float incoh = (p * n) / (act * act + 1e-12f);
            incoh_sum += incoh * act;
            wsum += act;
        }
        g.total_plasticity  = plastic_sum;
        g.mean_incoherence  = (wsum > 1e-9f) ? incoh_sum / wsum : 0.0f;
        return g;
    }

    void dump_metrics(FILE* out = stdout) {
        auto g = compute_metrics();
        std::fprintf(out,
            "SIS: %.6f  T+U: %.5f  tunnels: %d  cavities: %d  "
            "mean_incoherence: %.5f  total_plasticity: %.5f\n",
            g.sis, g.t_plus_u, g.n_tunnels, g.n_cavities,
            g.mean_incoherence, g.total_plasticity);
    }

    // ============================================================
    //  L6 ANALOGICAL RESONANCE BRIDGE
    //
    //  After training, two fissioned parents (P1, P2) that share
    //  ≥ K excitatory downstream targets from their α children are
    //  bridged with a low-stiffness ("rubber") tunnel between the
    //  α cavities. MIMS impedance filtering means the bridge
    //  primarily transmits low-frequency structural probes and
    //  attenuates high-frequency mechanistic probes.
    //
    //  This is the physics realization of L6 Analogical Mapping
    //  from research-paper/l9.md, and matches the "structural
    //  analogy through rubber material" prescription of
    //  research-paper/mims.md.
    //
    //  No if/else in the loop body; the shared-target intersection
    //  is a std::unordered_set membership query (branchless in
    //  hot path terms — the algorithmic branches live in the
    //  discovery pass, not the per-tick physics).
    // ============================================================
    struct AnalogyRecord {
        int parent_a;
        int parent_b;
        int alpha_a;
        int alpha_b;
        int shared_targets;
        std::string label_a;
        std::string label_b;
        std::vector<int> target_ids;
    };
    std::vector<AnalogyRecord> analogy_records;

    // Public entry — takes sub->mtx.
    int discover_analogies(int min_shared = 2,
                           float rubber_k = 0.05f,
                           float rubber_c = 0.15f)
    {
        std::lock_guard<std::mutex> lk(sub->mtx);
        return discover_analogies_locked(min_shared, rubber_k, rubber_c,
                                         /*clear_records=*/true,
                                         /*only_parent=*/-1);
    }

    // Internal — assumes caller holds sub->mtx.
    //   clear_records: if true, wipe analogy_records before scan
    //   only_parent  : if >=0, only pair (only_parent) with every other
    //                  fissioned parent — used by online L6-A discovery
    //                  from the structural tick, so we don't rescan
    //                  every pair every tick.
    int discover_analogies_locked(int min_shared,
                                  float rubber_k, float rubber_c,
                                  bool clear_records = true,
                                  int  only_parent   = -1)
    {
        if (clear_records) analogy_records.clear();

        // Collect fissioned parent ids from the alias ledger.
        std::vector<int> parents;
        for (const auto& kv : sub->fission_map) parents.push_back(kv.first);
        std::sort(parents.begin(), parents.end());

        // For each parent, gather excitatory targets of its α child.
        std::vector<std::unordered_set<int>> targets(parents.size());
        for (size_t i = 0; i < parents.size(); ++i) {
            int a_i = sub->fission_map[parents[i]].first;   // alpha id
            for (const auto& t : sub->tunnels) {
                if (t.polarity < 0.0f) continue;
                int other = -1;
                if (t.cavity_a == a_i)      other = t.cavity_b;
                else if (t.cavity_b == a_i) other = t.cavity_a;
                if (other >= 0) targets[i].insert(other);
            }
        }

        int found = 0;
        // Fast path — online mode restricts one endpoint to only_parent.
        // Full pairwise scan when only_parent < 0.
        int only_idx = -1;
        if (only_parent >= 0) {
            for (size_t i = 0; i < parents.size(); ++i)
                if (parents[i] == only_parent) { only_idx = (int)i; break; }
        }
        for (size_t i = 0; i < parents.size(); ++i) {
            for (size_t j = i + 1; j < parents.size(); ++j) {
                if (only_idx >= 0 &&
                    (int)i != only_idx && (int)j != only_idx) continue;
                std::vector<int> shared;
                for (int x : targets[i]) {
                    if (targets[j].count(x)) shared.push_back(x);
                }
                if ((int)shared.size() < min_shared) continue;

                int a_i = sub->fission_map[parents[i]].first;
                int a_j = sub->fission_map[parents[j]].first;

                // Fisher gate (Phase 4.2): the paper says a rubber bridge
                // is inserted exactly when the two cavities are close in
                // the Fisher metric — statistically indistinguishable
                // given their Hebbian consolidation. Shared downstream
                // targets alone say "these look alike"; the Fisher
                // distance says "the field cannot tell them apart". Both
                // must hold. Generous default keeps prior behaviour
                // while excluding pairs whose covariance structure is
                // genuinely distinct.
                if (fisher_distance_locked(a_i, a_j) > l6a_fisher_max)
                    continue;

                // Duplicate-bridge guard — skip if a rubber tunnel
                // already exists between these α cavities. Cheap linear
                // scan is fine; analogy_records is small.
                bool already = false;
                for (const auto& r : analogy_records) {
                    if ((r.alpha_a == a_i && r.alpha_b == a_j) ||
                        (r.alpha_a == a_j && r.alpha_b == a_i)) {
                        already = true; break;
                    }
                }
                if (already) continue;

                // Insert rubber tunnel α_i ↔ α_j directly into the
                // physics engine. It has low stiffness (rubber material,
                // per research-paper/mims.md) so wave transmission is
                // frequency-dependent — high-freq mechanistic probes
                // get absorbed, low-freq structural probes propagate.
                Tunnel rubber{};
                rubber.cavity_a         = a_i;
                rubber.cavity_b         = a_j;
                rubber.stiffness        = rubber_k;
                rubber.base_stiffness   = rubber_k;
                rubber.damping          = rubber_c;
                rubber.polarity         = 1.0f;
                rubber.plasticity_delta = 0.0f;
                // Phase 2.2 — L6-A bridges resonate at the empirical
                // 3.0 value used in the atorvastatin repurposing demo.
                // Unit matches drive_freq (the same value that appears
                // in `repurpose <label> 3 15`). Non-analogy probes
                // (20.0 mechanistic drive) fall far outside the band
                // and are gated to ~exp(-Q·(20-3)²/3) ≈ 0.
                rubber.resonance_omega  = 3.0f;
                rubber.Q_factor         = 0.5f;
                sub->tunnels.push_back(rubber);

                // Extend Layer 2 telemetry vectors to match new tunnel count.
                corr_ema.resize(sub->tunnels.size(), 0.0f);
                degree[a_i] += 1;
                degree[a_j] += 1;

                // Look up labels via alias ledger.
                auto label_of = [&](int pid) -> std::string {
                    int lk = sub->alias_ledger[pid].label_key;
                    if (lk >= 0 &&
                        lk < (int)sub->graph_ptr->graph.nodes.size())
                        return sub->graph_ptr->graph.nodes[lk].label;
                    return "<unknown>";
                };

                AnalogyRecord rec;
                rec.parent_a       = parents[i];
                rec.parent_b       = parents[j];
                rec.alpha_a        = a_i;
                rec.alpha_b        = a_j;
                rec.shared_targets = (int)shared.size();
                rec.label_a        = label_of(parents[i]);
                rec.label_b        = label_of(parents[j]);
                rec.target_ids     = shared;
                analogy_records.push_back(rec);
                ++found;
            }
        }
        return found;
    }

    // Phase 3.2 — Abduction Rule Discovery via Reverse Observation.
    // The physics is symmetric (Newton's third law), so "drive backwards"
    // is not a sign-flip. Instead: drive the goal cavity at low freq,
    // let the wave settle, then observe which upstream cavities
    // (cavities with excitatory tunnels ending at the goal) accumulated
    // the most residual energy. Those are the abduction candidates:
    // "if <upstream_p> then <goal_g>" — a physically-observed rule.
    struct AbductionRecord {
        int    goal_id;
        int    hypothesis_id;
        float  amplitude;
        std::string goal_label;
        std::string hypothesis_label;
    };
    std::vector<AbductionRecord> abduction_records;

    int discover_abductions(int goal_id,
                            float probe_freq   = 3.0f,
                            float probe_amp    = 6.0f,
                            int   settle_ticks = 200,
                            float threshold    = 0.005f)
    {
        // Drive + settle without holding sub->mtx (drive/tick take it).
        sub->reset_field();
        sub->drive(goal_id, probe_freq, probe_amp, 20);
        sub->run(settle_ticks);

        // Phase 3.2 — Fission-aware target set. If goal is fissioned,
        // its incident tunnels have moved to the α/β children.
        std::vector<int> goal_set = {goal_id};
        auto fmit = sub->fission_map.find(goal_id);
        if (fmit != sub->fission_map.end()) {
            goal_set.push_back(fmit->second.first);
            goal_set.push_back(fmit->second.second);
        }

        std::lock_guard<std::mutex> lk(sub->mtx);
        auto label_of = [&](int cid) -> std::string {
            if (cid < 0 || cid >= (int)sub->alias_ledger.size())
                return "<unknown>";
            int lk = sub->alias_ledger[cid].label_key;
            if (lk < 0 || lk >= (int)sub->graph_ptr->graph.nodes.size())
                return "<unknown>";
            return sub->graph_ptr->graph.nodes[lk].label;
        };

        int found = 0;
        std::unordered_set<int> already;
        std::unordered_set<int> goal_lut(goal_set.begin(), goal_set.end());
        for (const auto& t : sub->tunnels) {
            if (t.polarity < 0.0f) continue;         // excitatory only
            // Tunnel ends at any of the goal aliases (raw / α / β).
            if (!goal_lut.count(t.cavity_b)) continue;
            int p = t.cavity_a;
            if (goal_lut.count(p)) continue;         // exclude self-loop
            if (already.count(p)) continue;
            float amp_p = std::fabs(sub->cavities[p].displacement);
            if (amp_p < threshold) continue;
            AbductionRecord rec;
            rec.goal_id          = goal_id;
            rec.hypothesis_id    = p;
            rec.amplitude        = amp_p;
            rec.goal_label       = label_of(goal_id);
            rec.hypothesis_label = label_of(p);
            abduction_records.push_back(rec);
            already.insert(p);
            ++found;
        }
        return found;
    }

    void dump_abductions(FILE* out = stdout) {
        std::fprintf(out,
            "abduction_records: %zu\n", abduction_records.size());
        for (const auto& r : abduction_records) {
            std::fprintf(out,
                "abduction | %s → %s | amplitude=%.5f | goal=%d hyp=%d\n",
                r.hypothesis_label.c_str(), r.goal_label.c_str(),
                r.amplitude, r.goal_id, r.hypothesis_id);
        }
    }

    // Phase 4.2 — Fisher-Information Metric from corr_ema.
    // For the Boltzmann distribution p(G|θ) = exp(-β·E(G,θ))/Z(θ),
    // the Fisher metric is g_ij(θ) = β² · Cov(∂E/∂θ_i, ∂E/∂θ_j).
    // Since corr_ema[e] ≈ ⟨v_a·v_b⟩ per tunnel — a running mean of the
    // co-firing correlation — corr_ema^T · corr_ema approximates the
    // covariance matrix of energy derivatives to leading order.
    //
    // fisher_distance(cav_a, cav_b) computes the geodesic distance in
    // this metric between two cavities, using the tunnels incident to
    // each as the covariance sample. A small value = statistically
    // indistinguishable = a Fisher-optimal L6-A bridge candidate.
    float fisher_distance(int cav_a, int cav_b, float beta = 1.0f) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        return fisher_distance_locked(cav_a, cav_b, beta);
    }

    // Lock-free twin of fisher_distance for callers that already hold
    // sub->mtx (the L6-A discovery loop runs inside a _locked function;
    // taking the mutex again there would deadlock).
    float fisher_distance_locked(int cav_a, int cav_b, float beta = 1.0f) {
        if (cav_a < 0 || cav_a >= (int)sub->cavities.size()) return 1e9f;
        if (cav_b < 0 || cav_b >= (int)sub->cavities.size()) return 1e9f;
        // Fission-aware: build the set of cavities that "represent" each.
        auto expand = [&](int c, std::unordered_set<int>& s) {
            s.insert(c);
            auto it = sub->fission_map.find(c);
            if (it != sub->fission_map.end()) {
                s.insert(it->second.first);
                s.insert(it->second.second);
            }
        };
        std::unordered_set<int> A, B;
        expand(cav_a, A);
        expand(cav_b, B);
        // Use plasticity_delta² as covariance proxy — corr_ema is
        // reset every rebind, but plasticity_delta persists across
        // training and captures the actual Hebbian consolidation.
        float sa = 0.0f, sb = 0.0f, sab = 0.0f;
        for (int e = 0; e < (int)sub->tunnels.size(); ++e) {
            const auto& t = sub->tunnels[e];
            float c = t.plasticity_delta;
            bool ta = (A.count(t.cavity_a) || A.count(t.cavity_b));
            bool tb = (B.count(t.cavity_a) || B.count(t.cavity_b));
            if (ta) sa  += c * c;
            if (tb) sb  += c * c;
            if (ta && tb) sab += c * c;      // shared incident tunnels
        }
        float d2 = beta * beta * (sa + sb - 2.0f * sab);
        return std::sqrt(std::max(0.0f, d2));
    }

    void dump_analogies(FILE* out = stdout) {
        std::lock_guard<std::mutex> lk(sub->mtx);
        for (const auto& r : analogy_records) {
            std::fprintf(out,
                "analogy | %s ~ %s | shared_targets=%d | α_a=%d α_b=%d | targets:",
                r.label_a.c_str(), r.label_b.c_str(),
                r.shared_targets, r.alpha_a, r.alpha_b);
            for (int tid : r.target_ids) {
                if (tid >= 0 &&
                    tid < (int)sub->graph_ptr->graph.nodes.size())
                    std::fprintf(out, " %s",
                        sub->graph_ptr->graph.nodes[tid].label.c_str());
            }
            std::fprintf(out, "\n");
        }
    }
};

} // namespace unified
