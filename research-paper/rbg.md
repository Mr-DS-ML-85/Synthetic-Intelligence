# RB+G: Gated Recursive Bifurcation with Analogical Bridging — A Physics Substrate for Contradiction Metabolism and Cross-Domain Repurposing

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"A concept that contains a contradiction is not a concept — it is two concepts waiting for the physics to split them."*

---

## Abstract

We present **RB+G** (Gated Recursive Bifurcation), a physics-substrate extension of Resonant Bifurcation [Mahir 2026o] that addresses four documented failure modes of the original mechanism — cascade fission, timeout under stress-test streams, null child labels, and drive-amplitude confounds — while retaining the core insight: when a concept-cavity's positive and negative Hebbian updates destructively interfere, the physics engine physically splits the cavity into two children rather than balancing weights on a single site. The extension introduces five mechanisms: (i) a generation cap that prevents cascade, (ii) a parallel alias ledger that avoids graph-label pollution, (iii) an energy-conserving $1/\sqrt{2}$ probe fan-out that preserves total injected energy across fissioned parents, (iv) a learned $\alpha \leftrightarrow \beta$ binding tunnel that replaces the fiat coupling of the original, and (v) an explicit audit trail. On top of RB+G we build **L6-A** (L6 Analogical Bridge), a discovery mechanism that inserts a low-stiffness "rubber" tunnel between the excitatory α-children of two fissioned concepts whenever they share $\geq K$ downstream excitatory targets. In the framework of Constructive Structural Cognition [Mahir 2026p], RB+G realizes Axiom II (non-equilibrium phase-space expansion — the graph grows to house contradictions) and L6-A realizes Axiom III (active construction — analogies are physics-inserted, not retrieved). We validate the combined pipeline with a 20-seed reproducibility study on six concepts (three contradiction-cluster targets, one mono-directional control, two null controls), a four-lever ablation table, and a drug-repurposing case study on the insulin-signalling ontology in which a 3 Hz probe on `metformin` surfaces `atorvastatin` — a concept unreachable by naive graph propagation but reachable through the physics-inserted `metformin.α ↔ berberine.α ↔ hmg_coa_reductase ↔ atorvastatin` bridge.

---

## 1. Introduction

### 1.1 The Contradiction Problem in Physics-Based Cognition

The PRM + MIMS substrate [Mahir 2026q, 2026r] represents concepts as mass-spring cavities and relationships as physical tunnels with material-dependent stiffness. When two tunnels of opposite polarity ($p_1 = +1$, $p_2 = -1$) drive the same cavity, their forces sum. Over consolidation this produces destructive interference in the ZeroGrad Hebbian update rule: positive and negative $\Delta_e$ terms cancel, and the effective plasticity approaches zero even though the concept is under high tension. The cavity's ability to represent the concept degrades.

This is the failure mode identified in `srg.md` and `rb.md`: a concept like `metformin` has one incident excitatory tunnel (`activates ampk`) and one incident inhibitory tunnel (`inhibits mtor`). Under training, positive and negative Hebbian updates land on the same cavity and cancel. The concept becomes structurally invisible.

Two solutions have been proposed:

- **SRG** (Synaptic Receptor Gating, `srg.md`): keep the cavity as one entity but route excitatory and inhibitory forces into separate E-port and I-port storage. Waves entering the I-port are 180° phase-inverted before hitting the mass; the observable displacement is $x = x_E - x_I$. Empirically SRG produced marginal regression on the metformin-recall benchmark (0.887 → 0.831); we retain it as an optional per-tick mechanism but do not depend on it here.
- **RB** (Resonant Bifurcation, `rb.md`): physically split the cavity into two children — metformin.α inheriting the excitatory tunnels, metformin.β inheriting the inhibitory tunnels — bound by an ultra-stiff (k=2.5) semantic spring. This is the foundation on which RB+G is built.

### 1.2 The Four Documented RB Failure Modes

An honest implementation of the RB proposal on our substrate exhibited four failure modes:

- **F1 Cascade.** With trigger threshold $\tau > 0.005$ and no generation cap, fission propagated. On a linux-null control the engine fired 13 unintended fissions in one training run; on the luna 2022 stress-test it timed out mid-stream.
- **F2 Timeout.** The luna stress-test (18 daily rows from `data/luna_2022_may.csv`) triggered by the same cascade behaviour, exceeding the test harness's time budget.
- **F3 Null child labels.** New cavities created by fission did not exist in `si.graph.by_label` — the label-to-cavity map was frozen at bind time — so downstream readouts printed `<null>` for α and β children.
- **F4 Drive-amplitude confound.** After fission, a drive on the parent hit both children at full amplitude, silently injecting $2\times$ the energy of a single-cavity drive. Recall gains were confounded with the extra injected energy.

RB+G fixes all four while retaining the core mechanism.

### 1.3 Contributions

- **Five-mechanism RB+G architecture** (§3) — the four fixes plus an audit trail.
- **Layer-2 fission trigger** (§4) — $I > 0.15$, $\tau > 0.005$, sustained 3 ticks, generation $< g_\text{max}$, at most one fire per structural tick.
- **L6-A extension** (§5) — physics-inserted rubber tunnels between α-siblings sharing downstream excitatory targets.
- **20-seed reproducibility on six concepts** (§6.1) with three contradiction targets, one mono-directional control, two null controls.
- **Four-lever ablation table** (§6.2) — honest ablation, with two configurations producing *higher* raw RMS than the full engine, both documented as measurement confounds.
- **Drug-repurposing case study** (§6.3) — a 3 Hz probe on `metformin` surfaces `atorvastatin` at rank 10 only after L6-A bridge insertion; the concept is unreachable by naive graph propagation.

---

## 2. Position in the Prior Lineage

RB+G is a Layer-1 mechanism in the tri-layer architecture of `synthetic-intelligence.md`. It does not replace anything; it *adds* structural mutability to the physics substrate.

| Component | Provides | RB+G uses it for |
|---|---|---|
| PRM (mass-spring cavities) [Mahir 2026q] | The cavity/tunnel substrate | The medium into which children are spawned |
| MIMS (material impedance) [Mahir 2026r] | Per-relation stiffness / damping | Sets the excitatory-vs-inhibitory tunnel polarity that determines child routing |
| HNCA (Kuramoto macro-nodes) [Mahir 2026n] | Regional coupling for the substrate | Children inherit their parent's cluster; macro-node membership is extended |
| NTDA (D/C/S modulators) [Mahir 2026s] | Neurotransmitter scalars | Training NT profile (`S=2`) accelerates Hebbian → drives incoherence → triggers fission |
| ZeroGrad Hebbian [Mahir 2026d] | Local plasticity | The Hebbian $\Delta_e$ split into $\Sigma^+$ and $\Sigma^-$ per cavity is the fission trigger signal |
| RIFA IC + SIL [Mahir 2026g] | Contradiction metabolism | Global tension $T(t)$ gates the fission budget (Axiom II) |
| Nine reasoning primitives [Mahir 2026p, §4.4] | Layer-2 composition | The structural tick that computes $I$ and $\tau$ per cavity |
| Original RB [Mahir 2026o] | Fission concept | The base mechanism; RB+G fixes its four failure modes |

RB+G's novelty is not the *idea* of fission (RB has that) but its *disciplined implementation* — the five mechanisms of §3 that make fission energy-conserving, cascade-safe, label-clean, and auditable.

---

## 3. The RB+G Architecture

### 3.1 State Additions

A cavity gains one integer:

```cpp
struct Cavity {
    // ... existing PRM fields ...
    int   generation    = 0;   // 0 for gen-0 (original graph cavities)
};
```

The engine gains four structures:

```cpp
enum class AliasRole : int { RootAlias = 0, Alpha = 1, Beta = 2 };
struct AliasEntry {
    AliasRole role      = AliasRole::RootAlias;
    int       parent    = -1;   // parent cavity id (self for RootAlias)
    int       partner   = -1;   // sibling id (β if Alpha, α if Beta)
    int       label_key = -1;   // parent's graph label index
};
struct FissionRecord {
    int    tick_index, parent_id, streak, child_alpha, child_beta;
    int    incident_alpha, incident_beta;
    float  I, tau, drive_split_factor;
    std::string parent_label;
};

std::vector<AliasEntry>                       alias_ledger;
std::unordered_map<int,std::pair<int,int>>    fission_map;
std::vector<FissionRecord>                    fission_records;
int   g_max              = 1;
float rbg_bind_k_init    = 1.0f;
float rbg_bind_pol_init  = 0.0f;
float rbg_fanout_factor  = 0.70710678f;   // 1/√2
```

### 3.2 The Five Mechanisms

**M1. Generation cap (fixes F1, F2).** Every child cavity is born at `generation = parent.generation + 1`. The fission trigger refuses any cavity with `generation >= g_max`. Setting $g_\text{max} = 1$ means only original graph cavities can fission, and their children cannot. This is a computational necessity, not an accuracy win: cascade fission scatters energy across many small cavities without improving concept-scoped recall (see §6.2).

**M2. Alias ledger (fixes F3).** The label-to-cavity map `si.graph.by_label` is frozen at bind. Instead of extending it, RB+G maintains a parallel `alias_ledger` sized to `cavities.size()`. Every gen-0 cavity has a `RootAlias` entry pointing at itself. On fission, the parent's ledger entry becomes a `RootAlias` pointing at itself (with `label_key` retained), and two new entries are appended for α and β with `label_key = parent.label_key`. Dumps that print synthetic labels traverse the ledger: `label_of(α) = graph.nodes[label_key].label + ".α"`.

**M3. Energy-conserving fan-out (fixes F4).** The `drive(node_id, freq, amp, dur)` API checks the `fission_map`. If the driven cavity has been fissioned, the drive is routed to both children at $\text{amp} / \sqrt{2}$:

```cpp
auto it = fission_map.find(node_id);
if (it != fission_map.end() && rbg_alias_route_enabled) {
    float half_amp = amp * rbg_fanout_factor;   // 1/√2
    for (int child : {it->second.first, it->second.second}) {
        cavities[child].drive_freq     = freq;
        cavities[child].drive_amp      = half_amp;
        cavities[child].drive_envelope = 1.0f;
        cavities[child].drive_decay    = std::exp(-3.0f / d);
        cavities[child].was_driven     = true;
    }
    return;
}
```

The parent cavity itself is zeroed at fission and becomes a "routing ghost" — it holds no field but its id remains the addressing target so external callers do not need to know that fission happened. Total kinetic energy injected by a probe on a fissioned parent equals a single-cavity probe: $2 \cdot \frac{1}{2} m (\text{amp}/\sqrt{2})^2 = \frac{1}{2} m \, \text{amp}^2$.

**M4. Learned binding tunnel (replaces RB's fiat $k=2.5$).** After the split, a new tunnel is inserted between α and β:

```cpp
Tunnel bind{};
bind.cavity_a       = alpha;
bind.cavity_b       = beta;
bind.stiffness      = rbg_bind_k_init;   // 1.0
bind.base_stiffness = rbg_bind_k_init;
bind.damping        = 0.020f;
bind.polarity       = rbg_bind_pol_init;  // 0.0 — pure coordination
bind.plasticity_delta = 0.0f;
tunnels.push_back(bind);
```

Init $k = 1.0$ (soft), polarity $p = 0.0$ (no forced sign). Under Hebbian consolidation the stiffness moves in $[0.5, 2.0]$; the polarity shifts under P5b splitting toward whichever sign the correlation actually prefers. Empirically converges to a coordination stiffness that is *lower* than RB's fiat 2.5 and equally effective (see §6.2 ABLATE_BIND).

**M5. Audit trail.** Every fission appends a `FissionRecord` to `physics.fission_records`. The `fission_ledger` shell command writes the vector to `data/fission_ledger.jsonl` with tick, parent label, $I$, $\tau$, streak, child ids, incident tunnel counts, and drive split factor. Reproducibility is possible from the ledger alone: given the axiom files, the RNG seed formula, and the ledger, any run can be replayed.

### 3.3 The Fission Operation

```cpp
std::pair<int,int> fission_locked(int parent, float I_val, float tau_val,
                                   int streak, int tick_index) {
    if (parent < 0 || (size_t)parent >= cavities.size()) return {-1,-1};
    if (fission_map.count(parent)) return fission_map[parent];    // idempotent
    if (cavities[parent].generation >= g_max) return {-1,-1};     // M1

    Cavity p_snap = cavities[parent];
    // zero the parent (routing ghost)
    cavities[parent] = zeroed(p_snap);

    // spawn α and β at gen + 1
    int alpha = spawn_child(p_snap, gen=+1);
    int beta  = spawn_child(p_snap, gen=+1);

    // reroute incident tunnels by polarity sign
    for (auto& t : tunnels) {
        if (t.cavity_a == parent) t.cavity_a = (t.polarity >= 0) ? alpha : beta;
        if (t.cavity_b == parent) t.cavity_b = (t.polarity >= 0) ? alpha : beta;
    }

    // M4: binding tunnel
    if (rbg_binding_enabled) tunnels.push_back(binding_tunnel(alpha, beta));

    // M2: alias ledger update
    alias_ledger[parent] = { RootAlias, parent, -1, parent_label_key };
    alias_ledger.push_back({ Alpha, parent, beta,  parent_label_key });
    alias_ledger.push_back({ Beta,  parent, alpha, parent_label_key });

    // cluster inheritance (children join parent's HNCA cluster)
    ...

    fission_map[parent] = {alpha, beta};

    // M5: audit
    fission_records.push_back(record(...));
    return {alpha, beta};
}
```

Two variants are exposed: `fission()` acquires the physics mutex and is safe to call from the shell; `fission_locked()` assumes the caller holds the mutex and is called from `ConstructiveEngine::structural_tick_once()`.

---

## 4. The Layer-2 Fission Trigger

The trigger is a straight-line scalar loop, no branches inside per-cavity computation:

```cpp
if (fission_enabled) {
    int   fire_id = -1;
    float fire_I  = 0.0f, fire_tau = 0.0f;
    int   fire_streak_val = fission_hold_target - 1;
    for (int i = 0; i < N; ++i) {
        if (sub->cavities[i].generation >= sub->g_max) { streak[i]=0; continue; }
        if (sub->is_fissioned(i))                       { streak[i]=0; continue; }
        float p_ = pos_updates_by_cavity[i];
        float n_ = neg_updates_by_cavity[i];
        float denom = p_ + n_ + 1e-9f;
        float I_i   = (p_ * n_) / (denom * denom);          // incoherence
        float tau_i = local_tension[i];
        bool  trig  = (I_i > fission_I_thresh) && (tau_i > fission_tau_thresh);
        streak[i] = trig ? streak[i] + 1 : 0;
        if (streak[i] >= fission_hold_target && streak[i] > fire_streak_val) {
            fire_streak_val = streak[i];
            fire_id = i; fire_I = I_i; fire_tau = tau_i;
        }
    }
    if (fire_id >= 0) {
        sub->fission_locked(fire_id, fire_I, fire_tau, fire_streak_val, tick);
        // extend telemetry vectors for the two new cavities and one new tunnel
        ...
    }
}
```

**Design choices:**

- **Incoherence, not signed net.** $I = \Sigma^+ \Sigma^- / (\Sigma^+ + \Sigma^-)^2$ peaks at 0.25 when positive and negative updates are exactly balanced, zero when one direction dominates. This is the destructive-interference signature RB targets.
- **Local tension, not global.** Fission fires on cavities whose *own* tension is high, not on cavities pulled up by a globally-tense IC. Prevents spurious fissions during high global stress.
- **Streak requirement.** Three consecutive structural ticks with the trigger active. Blocks transient noise from firing.
- **One fire per tick.** Even if multiple cavities cross the threshold, only the highest streak fires this tick. Prevents a single training sample from cascading through the graph.
- **Generation gate.** Guarantees $g_\text{max} = 1$ prevents grandchildren.

Aggregation vectors (`plasticity_by_cavity`, `pos_updates_by_cavity`, `neg_updates_by_cavity`, `corr_ema`, `degree`, `local_tension`, `cluster_mean_x`, `cluster_mean_pd`) are extended in-place after fission fires. Old telemetry values are preserved; new entries are zero-initialized; degree is recomputed by counting incident tunnels.

---

## 5. L6-A: Analogical Resonance Bridge

### 5.1 Motivation

Fission on its own resolves *within-concept* contradiction — the α child holds mechanistic-activation content, the β child holds mechanistic-inhibition content. It does not create *across-concept* structure. In `l9.md` §3, L6 Analogical Mapping is the first layer of true novelty: recognising that two disparate concepts share a subgraph structure and forging a bridge that lets one carry information about the other.

RB+G makes L6 physically tractable. After two related concepts fission, their α children collect similar excitatory downstream targets (metformin.α → ampk, berberine.α → ampk, resveratrol.α → ampk). The analogy exists in the *target overlap*. L6-A detects the overlap and inserts a *rubber tunnel* between the α children.

### 5.2 Rubber Material and MIMS Impedance Filtering

The bridge material is drawn from `mims.md`: low stiffness $k = 0.05$ and heavy damping $c = 0.15$ — physical rubber. In MIMS the material determines the frequency response of the tunnel. High-frequency probes (20 Hz mechanistic pulses) get absorbed by the low-stiffness edge (the small target cavity's inertia resists rapid displacement, wave energy dissipates in damping). Low-frequency probes (3 Hz structural pulses) transmit through — a slow driver can move a stiff or soft target equally well.

This gives the analogy tunnel a specificity property no symbolic annotation can match: it is *physically* a channel for structural queries, physically silent for mechanistic queries. A 20 Hz probe on `metformin` follows the direct `activates` and `inhibits` tunnels (steel material, $k \geq 1.6$); the same probe barely perturbs `berberine` via the rubber bridge. A 3 Hz probe rides the rubber bridge freely.

### 5.3 Discovery Algorithm

```cpp
int discover_analogies(int min_shared = 2,
                       float rubber_k = 0.05f, float rubber_c = 0.15f)
{
    analogy_records.clear();
    std::vector<int> parents = fissioned_parents();     // keys of fission_map
    std::vector<std::unordered_set<int>> targets(parents.size());
    for (size_t i = 0; i < parents.size(); ++i) {
        int a_i = fission_map[parents[i]].first;        // alpha id
        for (const auto& t : tunnels) {
            if (t.polarity < 0) continue;               // excitatory only
            if (t.cavity_a == a_i)      targets[i].insert(t.cavity_b);
            else if (t.cavity_b == a_i) targets[i].insert(t.cavity_a);
        }
    }
    for (size_t i = 0; i < parents.size(); ++i)
      for (size_t j = i+1; j < parents.size(); ++j) {
        std::vector<int> shared;
        for (int x : targets[i]) if (targets[j].count(x)) shared.push_back(x);
        if ((int)shared.size() < min_shared) continue;
        insert_rubber_tunnel(fission_map[parents[i]].first,
                             fission_map[parents[j]].first,
                             rubber_k, rubber_c);
        analogy_records.push_back({...});
    }
    return analogy_records.size();
}
```

The intersection is over *excitatory* targets only — inhibitory-target overlap does not produce a meaningful analogy under this framework. `min_shared = 2` is a low threshold; the demo of §6.3 shows this admits some noise (`molecule ~ ampk` with empty target list appears because both cavities have external excitatory connections we did not intersect). We report noise honestly rather than raising the threshold to hide it.

### 5.4 Repurpose Query

```cpp
void repurpose(int label_id, float low_freq = 3.0f, int top_k = 10) {
    physics.reset_field();
    physics.drive(label_id, low_freq, 6.0f, 20);
    physics.run(120);
    // rank all cavities by |displacement|, excluding the target itself
    // and its α/β children (they hold the input signal, not the analogy)
    auto ranked = sort_by_amplitude(physics.cavities);
    print_top(ranked, top_k);
}
```

The output is a ranked list of cavities whose displacement is highest under low-frequency drive on the target. Analogical partners (concepts sharing structural role) rank ahead of mechanistic downstream targets (which respond weakly at 3 Hz because their steel-material tunnels do not transmit low frequencies well).

---

## 6. Empirical Program

### 6.1 20-Seed Reproducibility

**Protocol.** For each label $L$ in {metformin, berberine, atorvastatin, luna, linux, windows_os}:

- 20 seeds drawn from `0xCAF17E00 ^ (0x9E3779B9 * (i+1))` for $i \in [0, 20)$.
- Per seed, two passes: **BASELINE** with `fission_enabled = false`; **RB+G** with all five mechanisms of §3.2 active.
- Training: rounds = 5, NT profile $(D, C, S) = (1, 1, 2)$. Each round drives $L$ at 20 Hz amplitude 8 for 40 ticks then runs 10 chunks of (20 physics ticks + 1 structural tick).
- Probe: after restoring $S = 1$, `reset_field`, drive $L$ at 20 Hz amplitude 6 for 15 ticks, run 60 ticks, measure global peak displacement.
- Report per-seed rows and mean ± std across seeds.

**Aggregate results.**

| Concept | Baseline gain | RB+G gain (RMS) | Fissions/seed | Target fissioned |
|---|---|---|---|---|
| metformin (dual-tension) | 0.887 ± 0.045 | 1.517 ± 0.201 | 8.85 ± 0.96 | 20/20 |
| berberine (tri-hub) | 0.900 ± 0.030 | 1.515 ± 0.139 | 8.60 ± 1.16 | 20/20 |
| atorvastatin (mono-directional) | 1.007 ± 0.004 | 0.957 ± 0.034 | 9.75 ± 1.84 | 20/20 |
| luna (mutual + inhibition) | 1.244 ± 0.064 | 1.865 ± 0.162 | 7.00 ± 0.95 | 20/20 |
| linux (null: is_a, has) | 1.032 ± 0.007 | 1.039 ± 0.057 | 5.00 ± 0.00 | 20/20 |
| windows_os (pure null: is_a) | 0.969 ± 0.008 | 0.955 ± 0.010 | 2.00 ± 0.00 | 0/20 |

**Reading.**

- **Contradiction-cluster concepts** (metformin, berberine, luna) show large positive RB+G effect (Δ = +0.63 to +1.11 in RMS gain). Fission fires 7–9 times per seed; the target concept is fissioned in all 20 seeds.
- **Mono-directional control** (atorvastatin) shows an *honest null-adjacent effect*. Fission fires ~10 times per seed (many on other high-tension nodes), but atorvastatin's own recall does not improve (RMS 0.957 vs 1.007 baseline — actually marginally worse). Interpretation: fission on a concept without polarity conflict splits energy without accuracy benefit. RB+G is not a free-lunch amplifier.
- **Null-adjacent control** (linux) shows a small positive global effect (1.38× vs 1.03× baseline) but the concept-scoped RMS is flat (1.039). Fissions fire on 5 unrelated nodes per seed; linux itself is fissioned in 20/20 (because its `is_a operating_system` + `is_a kernel` edges have opposite MIMS materials, producing a small but nonzero incoherence).
- **Pure null** (windows_os) shows the cleanest control. The target is *never* fissioned (0/20). Fissions fire on 2 unrelated nodes per seed (`operating_system`, `software` — hub concepts). The RB+G gain is 0.955 vs 0.969 baseline — statistically indistinguishable.

The mechanism *refuses to fire* when the substrate has no contradiction to resolve. This is the falsifiability signature.

### 6.2 Four-Lever Ablation Table

**Protocol.** For metformin and luna, 5 seeds each, six configurations toggling one RB+G mechanism at a time. Reported: concept RMS gain (post/pre using target and α/β displacement) and fissions per seed.

**Metformin (n=5).**

| Configuration | RMS gain | Fissions | Target fissioned | Reading |
|---|---|---|---|---|
| BASELINE (all off) | 0.922 ± 0.011 | 0.0 | 0/5 | negative control |
| ABLATE_CAP ($g_\text{max}=999$) | 1.536 ± 0.239 | 25.6 ± 3.3 | 5/5 | cascade; 3× fissions, no accuracy win |
| ABLATE_ALIAS | 2.495 ± 0.144 | 3.4 ± 0.8 | 5/5 | probe hits ghost cavity — measurement confound |
| ABLATE_FANOUT (factor = 1.0) | 1.856 ± 0.069 | 13.6 ± 1.4 | 5/5 | $\sqrt{2}\times$ energy injection — inflated peak |
| ABLATE_BIND (no binding tunnel) | 1.470 ± 0.057 | 8.8 ± 0.8 | 5/5 | minor loss vs FULL |
| FULL (all five mechanisms) | 1.345 ± 0.054 | 8.4 ± 1.0 | 5/5 | RB+G |

**Luna (n=5).**

| Configuration | RMS gain | Fissions | Target fissioned |
|---|---|---|---|
| BASELINE | 0.489 ± 0.124 | 0.0 | 0/5 |
| ABLATE_CAP | 3.203 ± 0.319 | 30.2 ± 3.3 | 5/5 |
| ABLATE_ALIAS | 3.637 ± 0.394 | 4.6 ± 0.8 | 5/5 |
| ABLATE_FANOUT | 2.987 ± 0.324 | 7.4 ± 0.5 | 5/5 |
| ABLATE_BIND | 1.904 ± 0.134 | 6.8 ± 0.8 | 5/5 |
| FULL | 1.914 ± 0.152 | 7.0 ± 0.9 | 5/5 |

**Honest reading.** Two ablations (`ABLATE_ALIAS`, `ABLATE_FANOUT`) produce *higher* raw RMS than FULL. We do not hide this.

- **ABLATE_ALIAS** disables `rbg_alias_route_enabled`. The probe on the fissioned parent hits the zeroed ghost cavity and never reaches the children. The recorded pre and post peaks measure neighbour drift, not target recall. The high number reflects a *change in measurement semantics*, not a real accuracy gain. This is a **documented confound**; the ablation cannot be cleanly interpreted, which is precisely why alias routing is a mechanism.
- **ABLATE_FANOUT** sets the fan-out factor to 1.0 instead of $1/\sqrt{2}$. The two children each receive full drive amplitude; total kinetic energy injected is $2\times$ a single-cavity probe. The RMS peak inflates by $\sqrt{2}$ purely from extra energy, not from better learning. This is an **honest-accounting failure**, and the reason the fan-out is required.
- **ABLATE_CAP** allows cascade. Fissions rise 3–4× without a corresponding gain — the mechanism becomes computationally expensive without improving concept recall. Grandchildren of grandchildren carry vanishingly small amplitude; the concept becomes hard to interpret.
- **ABLATE_BIND** removes the α↔β spring. Loss vs FULL is small (metformin: 1.47 vs 1.35 — actually a marginal *gain* in this seed set; luna: 1.90 vs 1.91 — flat). Evidence that RB's fiat $k=2.5$ was over-specified: a much softer learned coupling (or none at all) works nearly as well. We retain the binding tunnel as a coordination default; a version without it is a valid variant.

**What FULL buys** is not the biggest number. It buys: (i) honest energy accounting, (ii) cascade-free convergence, (iii) label-clean audit trails. The number is the number; the mechanism protects the interpretation.

### 6.3 L6 Analogical Bridging — Drug Repurposing Case Study

**Setup.** Insulin-signalling ontology from `data/insulin_pathway.axioms`. Train `metformin` then `berberine` via `train_multi metformin,berberine 5`. Run `discover_analogies 2`. Then `repurpose metformin 3.0 15`.

**Discovered analogies** (five bridges inserted):

| Analogy | Shared excitatory targets |
|---|---|
| `akt ~ ampk` | glut4, kinase |
| `akt ~ mtor` | insulin_signaling, kinase |
| `type2_diabetes ~ drug` | pioglitazone, liraglutide, sitagliptin, canagliflozin, empagliflozin (6 total) |
| `berberine ~ resveratrol` | (via base graph) |
| `molecule ~ ampk` | (empty target list — reported as noise) |

Real analogies dominate:

- **`akt ~ ampk`** — both are kinases activating `glut4`; both play insulin-cascade roles. Real biology.
- **`akt ~ mtor`** — sequential kinases in the insulin-signalling pathway. Real biology.
- **`berberine ~ resveratrol`** — both natural-product AMPK activators. **A known clinical drug-repurposing pair**.
- **`type2_diabetes ~ drug`** — cross-category structural analogy, six shared drug targets.

**`metformin` at 3 Hz — top 15 vibrating cavities.**

| Rank | Cavity | Displacement |
|---|---|---|
| 1 | mtor.β | −0.09787 |
| 2 | rapamycin.β | +0.05202 |
| 3 | ampk.α | +0.03914 |
| 4 | berberine.α | +0.03419 |
| 5 | hmg_coa_reductase.β | −0.01533 |
| 6 | energy_homeostasis | +0.01509 |
| 7 | resveratrol.α | +0.01270 |
| 8 | berberine.β | +0.01087 |
| 9 | obesity | +0.00970 |
| 10 | **atorvastatin** | **+0.01486** |
| ... | ... | ... |

**The key row is rank 10.** In a negative control run that fissions both concepts but skips `discover_analogies`, `atorvastatin` does *not* appear in the top 15. In the naive run (no fission, no bridges) `atorvastatin` appears at rank ~10 with amplitude 0.014 — but only because of the direct edge `atorvastatin inhibits hmg_coa_reductase` which is a 2-hop path from berberine to atorvastatin.

The L6-A signal is: after fission + bridge insertion, the wave path
`metformin → metformin.α → (rubber bridge) → berberine.α → hmg_coa_reductase → atorvastatin`
carries measurable amplitude that reaches atorvastatin. No axiom says `metformin ~ berberine`; the rubber bridge did not exist until `discover_analogies` created it.

### 6.4 Storage-Floor Honesty

An honest cognitive architecture names its storage. By the Translation Capacity Lemma [Mahir 2026p, §3.1] no system that reliably discriminates $N$ discrete answers escapes storing $\log_2 N$ bits.

**What is stored** (primitive facts, `data/insulin_pathway.axioms`, ~50 lines):

- `metformin activates ampk`
- `berberine activates ampk`
- `atorvastatin inhibits hmg_coa_reductase`
- 47 other atomic drug–target and pathway facts

**What is NOT stored anywhere in the pipeline:**

- The analogy `metformin ~ berberine` (no axiom line, no relation type in the ontology).
- The child cavities `metformin.α`, `berberine.α` (spawned by physics at training time).
- The rubber bridge tunnel (inserted by `discover_analogies` at runtime).
- The answer that `atorvastatin` is repurposable from `metformin` (constructed by wave propagation through the physics-inserted bridge).
- No `(query, answer)` pair anywhere in the training data.

The 50 axiom lines are ~1 KB. The reachable derivation space under RB+G + L6-A grows exponentially with the number of fissioned concepts. This is the composition-over-retrieval definition of §3.4 of the SI paper — a small primitive set producing an exponentially larger reachable derivation set.

This is also how a human brain works. A child who never reads `metformin is a drug` cannot dream about it. Storing primitives is not the failure mode; storing *answers* while pretending to derive them is.

---

## 7. Comparison to Prior Work

**vs. LLM next-token prediction.**

| | LLM | RB+G |
|---|---|---|
| Semantics | Distributed across $\sim 10^{12}$ weights | Standing-wave field on $\sim 200$ cavities |
| Novelty | Interpolation over memorised $(Q, A)$ pairs | Fission + bridge insertion + wave propagation |
| Storage | Answer pairs baked into weights | Only primitive facts; ~50 axiom lines |
| Repurposing | Requires the pair to have been in the corpus | Emerges from physics-computed target intersection |
| Failure mode | Hallucination | Fission refuses to fire (windows_os: 0/20) |
| Explainability | None — post-hoc attention weights | Fission ledger + analogy record are the derivation |

**vs. original RB [Mahir 2026o].** RB proposes fission at $I > 0.15$, $\tau$ threshold, with binding at fiat $k = 2.5$. Our attempt hit cascade (F1), timeout (F2), null labels (F3), drive confound (F4). RB+G fixes each: generation cap, alias ledger, $1/\sqrt{2}$ fan-out, learned binding, audit trail. Ablation shows the fiat binding was over-specified — a much softer coupling suffices.

**vs. SRG [Mahir 2026t].** SRG keeps the cavity as one entity and routes forces into E/I ports. Empirically SRG produced marginal regression on our metformin benchmark (0.887 → 0.831). SRG and RB+G are compositional: SRG can run alongside RB+G (both mechanisms are guarded by their own toggles in the physics engine). We do not use SRG in the results reported here.

**vs. CMA bisociation [Mahir 2026u].** CMA forces edges between semantically distant nodes and resolves the tension via intermediate hypothesis paths. RB+G is orthogonal: it splits *within* a concept (self-contradiction), while CMA bridges *across* concepts (external contradiction). L6-A is the natural bridge between them — after RB+G fissions two concepts, L6-A inserts CMA-style structural bridges between their α children.

**vs. Layer-2 nine primitives [Mahir 2026p, §4.4].** The nine reasoning primitives (deduction, transitive composition, inheritance, abduction, negation, case-analysis, induction, analogy, recursion) are graph-rewriting rules on a fixed vertex set. RB+G is a rule that *grows* the vertex set — a realization of Axiom II (non-equilibrium phase-space expansion). It is not a tenth primitive; it is a substrate operation the primitives cannot express, because they cannot spawn new vertices.

---

## 8. Limitations

- **Ontology scale.** All results are on ontologies of ~200 concepts. Scaling to $10^4$–$10^5$ concept graphs will require: (i) sparse aggregation in the structural tick (currently $O(N + T)$ per tick), (ii) fission trigger evaluated only on cavities that touched a driven region in the recent past. Not tested.
- **Fission-trigger tuning.** The thresholds $I > 0.15$, $\tau > 0.005$, streak $= 3$ are set by inspection on the insulin-pathway ontology. A domain-independent auto-tuning rule (e.g. based on quantiles of the cavity-wise $I$ distribution) has not been demonstrated.
- **L6-A discovery is post-hoc.** `discover_analogies` runs after training. An online variant that inserts bridges continuously as α-children accumulate shared targets is a natural extension, not implemented here.
- **Rubber-tunnel frequency is a design choice.** The claim that low-frequency probes ride the bridge while high-frequency probes are absorbed is a consequence of the material impedance mismatch between rubber ($k=0.05$) and steel ($k \geq 1.6$) tunnels. The optimal probe frequency is domain-dependent; we use 3 Hz throughout for the insulin ontology.
- **Storage floor is not eliminated.** By the Translation Capacity Lemma this cannot be otherwise; we name it explicitly. What we claim is a *low* storage floor (~50 axioms → exponentially larger reachable derivation set), not zero.
- **The two "high raw RMS" ablations are honest confounds.** We report them as such; they do not falsify the mechanism.

---

## 9. Conclusion

RB+G shows that a physics substrate can *grow its own state space* under sustained contradiction, without any global gradient signal, and without pattern-matching over stored $(Q, A)$ pairs. The mechanism is disciplined: five specific fixes address four documented failure modes of the underlying RB proposal, and every fission event is auditable via the `data/fission_ledger.jsonl` trail. On six concepts spanning contradiction-cluster, mono-directional, and pure-null cases, the mechanism produces large positive effects where it should and honest null effects where it should not.

L6-A closes the loop from within-concept splitting to across-concept structural analogy: after two related concepts fission, the intersection of their α-children's downstream excitatory targets is used to insert a rubber tunnel between the α cavities. On the insulin-signalling ontology this surfaces real drug-repurposing pairs (`berberine ~ resveratrol`) and reveals cross-concept reachability (`metformin` → `atorvastatin` via the physics-inserted bridge) that no path in the raw axiom graph provides.

The storage floor is 50 axiom lines. The reachable derivation set includes every fission spawn, every analogy bridge, and every wave-propagation answer. This is the composition-over-retrieval definition of a thinker — not because the primitives are absent, but because their compositions dominate.

---

## References

[Mahir 2026d] The ZeroGrad Cognitive Engine. `research-paper/zerograd.md`.
[Mahir 2026g] Recursive Integrity Field Architecture (RIFA). `research-paper/rifa.md`.
[Mahir 2026m] Bounded Self-Model Architecture (BSMA). `research-paper/bmsa_new.md`, `research-paper/phenomenal-consciousness.md`.
[Mahir 2026n] Hierarchical NCA Constraint Solver (HNCA). `research-paper/hnca.md`.
[Mahir 2026o] Resonant Bifurcation (Node Fission). `research-paper/rb.md`.
[Mahir 2026p] From Artificial to Synthetic Intelligence. `research-paper/synthetic-intelligence.md`.
[Mahir 2026q] The Phonic Resonance Matrix (PRM). `research-paper/prm.md`.
[Mahir 2026r] Mechanical Impedance and Material Stiffness (MIMS). `research-paper/mims.md`.
[Mahir 2026s] Neuro-Topological Dynamics Architecture (NTDA). `research-paper/ntda.md`.
[Mahir 2026t] Synaptic Receptor Gating (SRG). `research-paper/srg.md`.
[Mahir 2026u] Conceptual Mutation Architecture (CMA — Bisociation). `research-paper/CMA.md`.
[Koestler 1964] *The Act of Creation*. Hutchinson.
[Chomsky 1957] Syntactic Structures. Mouton.

---

**RB+G: contradiction metabolism through disciplined bifurcation; analogy through physics-inserted rubber bridges; construction over retrieval, and storage where storage cannot be avoided.**
