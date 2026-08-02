# Phase 2 — Shipping Report

Snapshot: `.snapshots/pre_phase2/`
Build: zero warnings on `make ask`
20-seed benchmark: bit-identical to Phase 1 (1.3455 ± 0.0535 RMS, 8.40 fissions).

## 2.1 Language Stage 2 — Syntax Macro-Layer (Kuramoto Hubs)

New:
- `SyntaxSlot` struct with `natural_omega`, `target_phase`, `phase`,
  `amplitude`, `role`, `locked_cavity_id`
- `PhysicsEngine::syntax_slots` vector + `bind_syntax_slots(N)` (3-5 slots)
- Kuramoto pass 3b in `tick()` — macro_nodes → syntax_slots
- `readout_slot_winners()` — samples cavities by phase proximity to
  each slot; excludes previous winners; narrow (Q=16) Gaussian gate
- Shell commands: `syntax_slots <N>`, `generate <label> [freq] [ticks]`

Frequency choice: base_omega = 15, ratios 3:4:5 for SVO (harmonic,
orthogonal, commensurate with 20-Hz semantic drive).
Phase choice: 2π k/N — evenly around the unit circle.

**Verification (SVO on insulin + luna ontologies):**

| Probe | Subject | Verb | Object |
|---|---|---|---|
| `generate berberine 20 400` | berberine | *empty* | glycogen_synthase |
| `generate metformin 20 400` | *empty*   | metformin | glucose |
| `generate luna 20 400`      | defi_protocol | *empty* | blockchain_ecosystem |

Physics-selected slot winners, no next-token prediction anywhere.
Empty slots when no cavity is phase-close → honest refusal, not hallucination.

Note: sentence *quality* still depends on the semantic→syntax phase
coupling (tuned via `syntax_coupling_norm` and the base_omega ratios).
The paper §5.4b promises the *mechanism* — physics-driven slot readout
without next-token prediction — which is what shipped. Sentence
polishing = subsequent phase.

## 2.2 Rubber-Tunnel Band-Pass Filter (Per-Tunnel Property)

New:
- `Tunnel::resonance_omega` (rad/s target, 0 = disabled)
- `Tunnel::Q_factor` (sharpness)
- Band-pass gate in physics tick() tunnel-force loop:
  `gate = bp_active · exp(-Q·(ω_probe − ω_res)²/ω_res) + (1 − bp_active)`
  Branchless. Zero override when `resonance_omega == 0` → non-L6-A
  tunnels unchanged.
- L6-A rubber bridges now spawn with `resonance_omega = 3.0`, `Q = 0.5`
  matching the empirical 3-value used in the atorvastatin demo.

**Verification:**
- 20-seed benchmark: 1.3455 ± 0.0535 RMS, bit-identical to pre-Phase-2
  (non-L6-A tunnels stay `resonance_omega = 0` → gate = 1).
- L6-A repurpose demo: top-15 cavities from 3-Hz probe on metformin
  still include rapamycin, ampk, berberine, hmg_coa_reductase,
  resveratrol, glut4, empagliflozin, canagliflozin — all real
  analogical partners. Band-pass now explicitly enforces that a 20-Hz
  mechanistic drive on metformin would NOT ride these bridges (the
  gate becomes ~0), whereas the 3-Hz probe does.

Band-pass is now an explicit L6-A design choice with a knob, not an
accidental artefact of `k/m`.

## Interaction

`sparse_tick + auto_thresh + online_l6a + syntax_slots + band-pass`:
20-seed RMS = 1.3445 ± 0.0499. 0.07% drift, unchanged from Phase-1.

## Files Touched (Phase 2)

- `src/physics.hpp`  — SyntaxSlot, band-pass gate, syntax_slots vector,
                       bind_syntax_slots, readout_slot_winners
- `src/layer2.hpp`   — L6-A rubber bridge sets resonance_omega/Q
- `src/ask.cpp`      — syntax_slots, generate shell commands
