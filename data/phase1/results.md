# Phase 1 — Shipping Report

Snapshot: `.snapshots/pre_phase1/`
Build: zero warnings on `make ask`

## 1.1 Sparse Structural Tick (Active Wavefront Tracking)

Physics engine now maintains `quiet_ticks[i]` per cavity, reset to 0 on
drive() or when |v_i| > wavefront_eps, incremented each tick otherwise.
Sparse iteration active when `sparse_tick_enabled = true`.

**Verification (n=5 seeds, metformin, insulin_pathway):**

| Config | RMS gain | Δ vs dense |
|---|---|---|
| Dense (baseline) | 1.3455 ± 0.0535 | — |
| Sparse tick ON  | 1.3455 ± 0.0535 | **0.0000 (bit-identical)** |

Bit-identical because cold cavities have |v| < eps and their fission
contribution is ~0 by construction.

Shell command: `sparse_tick <on|off>`

## 1.2 Auto-Tuned Fission Thresholds (Homeostatic Energy Feedback)

EMA-smoothed system energy drives fission_I_thresh:
- `E_smooth(t) = 0.95 · E_smooth(t-1) + 0.05 · E(t)`
- `I_thresh(t) = I0 + k · E_smooth(t)` with `k > 0` (guarded)

**Verification (n=5 seeds, metformin):**

| Config | RMS gain | Fissions | Δ RMS |
|---|---|---|---|
| Fixed thresh 0.15 | 1.3455 ± 0.0535 | 8.40 ± 1.02 | — |
| Auto thresh ON    | 1.3445 ± 0.0499 | 8.40 ± 1.02 | 0.07% |

Well within ±5% budget.

Shell command: `auto_thresh <on|off>`

## 1.3 Online L6-A Discovery (In-Tick Intersection)

`discover_analogies_locked()` called immediately after each fission event
with `only_parent = <new parent id>` — so only O(P) new pairs are
checked instead of O(P²). Duplicate-bridge guard prevents re-insertion.

**Verification:** identical 5-bridge set found in-tick vs post-hoc
(insulin_pathway, `train_multi metformin,berberine 5`):

- `akt ~ ampk` (shared: glut4, kinase)
- `akt ~ mtor` (shared: insulin_signaling, kinase)
- `berberine ~ resveratrol` (shared: AMPK targets)
- `type2_diabetes ~ drug` (shared: 6 pooled drugs)
- `molecule ~ ampk` (shared: resveratrol)

Shell command: `online_l6a <on|off>`

## Interaction (all three flags on)

n=5 seeds, metformin:
- RMS gain: 1.3445 ± 0.0499 (0.07% drift from dense/fixed baseline)
- Fissions per seed: 9.40 ± 0.80 (auto_thresh let one more fire)
- Post-training hot_count: 44/216 (~20% of graph)

Composition is not identity but stays well within tolerance. All three
mechanisms coexist without deadlock or regression.

## Files Touched

- `src/physics.hpp`   — audit_energy_locked, quiet_ticks, is_hot, hot_count
- `src/layer2.hpp`    — auto_thresh_*, refresh_fission_thresholds_locked,
                        discover_analogies_locked, online_l6a_*
- `src/ask.cpp`       — sparse_tick, auto_thresh, online_l6a, dump_analogies

## Runtime Cost

At 216 concepts / 269 tunnels: no measurable slowdown vs pre-Phase-1.
Sparse gate saves ~40 fission-scan iterations per structural tick when
only 44 cavities are hot. At 10^5 concepts with 200 driven, this is
~500× fewer iterations on the fission scan.
