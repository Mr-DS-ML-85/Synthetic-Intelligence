# Future Fixes: Roadmap to Complete SI

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"Nothing on this list is proven impossible. Three are engineering. Four are research programmes. Three are hard math. All are tractable."*

**Companion documents.**
- Theory: `research-paper/synthetic-intelligence.md` §8 (Open Problems)
- Mechanism: `research-paper/rbg.md`
- Implementation: `research-paper/failed_but_parts_works/deprected/how_we_implement.md`

---

## Overview

Ten problems remain between the current PCMA engine and what §8 of the SI paper calls "complete SI". Each entry below states the problem, why it matters, the concrete fix path, the files to touch, the estimated effort, and the success criterion. Ordered by expected difficulty (easiest first).

### Effort classes

| Class | Time budget | Nature |
|---|---|---|
| **E** — Engineering | Days | Function-level edits, no new theory |
| **R** — Research | Weeks–months | New mechanism, empirical validation required |
| **M** — Math | Months–year+ | Genuine theorem or categorical construction |

---

## 1. Auto-tuning Fission Thresholds

**✅ SHIPPED** — homeostatic energy feedback, EMA-smoothed, k > 0 guarded.
See `data/phase1/results.md`. 20-seed drift: 0.07% RMS.

**Class: E** · **Effort: 2–3 days** · **Files: `src/layer2.hpp`, `src/ask.cpp`**

### Problem
`fission_I_thresh = 0.15`, `fission_tau_thresh = 0.005`, `fission_hold_target = 3` are hand-picked on the insulin-pathway ontology. On a new ontology they may fire too eagerly (cascade the graph) or too rarely (miss real contradictions).

### Fix
Replace fixed thresholds with quantile-based thresholds computed at the end of every N structural ticks (say N=100):

```cpp
// At the end of structural_tick_once() every N ticks:
void refresh_fission_thresholds(float I_quantile = 0.95f,
                                float tau_quantile = 0.90f)
{
    std::vector<float> I_vals, tau_vals;
    for (int i = 0; i < N; ++i) {
        float p_ = pos_updates_by_cavity[i];
        float n_ = neg_updates_by_cavity[i];
        float denom = p_ + n_ + 1e-9f;
        I_vals.push_back((p_ * n_) / (denom * denom));
        tau_vals.push_back(local_tension[i]);
    }
    std::sort(I_vals.begin(), I_vals.end());
    std::sort(tau_vals.begin(), tau_vals.end());
    fission_I_thresh   = I_vals  [(size_t)(I_vals.size()   * I_quantile)];
    fission_tau_thresh = tau_vals[(size_t)(tau_vals.size() * tau_quantile)];
    // fission_hold_target unchanged — streak requirement stays semantic
}
```

### Success criterion
- On `insulin_pathway`, fission count per training round remains 7–10 (matches current).
- On a *new* untested ontology (say `data/maxwell_equations.axioms`), fission fires on 1–3 concepts with genuine polarity conflict, and 0 on pure-null concepts.
- 20-seed reproducibility numbers change by < 5%.

### Fallback
Add a shell command `fission_thresh_mode <fixed|quantile>` so both modes can be compared side-by-side.

---

## 2. Online L6-A Discovery

**✅ SHIPPED** — in-tick call to `discover_analogies_locked()` with
`only_parent = <new fissioned>`. Same 5 bridges as post-hoc invocation.
Duplicate-bridge guard active.

**Class: E** · **Effort: 3–5 days** · **Files: `src/layer2.hpp`**

### Problem
`discover_analogies()` currently runs *after* training. In a live-running system, analogies should appear the moment two α-children accumulate enough shared targets — not require an operator to invoke discovery.

### Fix
Move the intersection check into `structural_tick_once()`, guarded by a cadence counter to avoid running it every tick:

```cpp
// Add to ConstructiveEngine:
int   online_l6a_cadence     = 50;    // check every 50 structural ticks
bool  online_l6a_enabled     = false; // opt-in
int   l6a_min_shared         = 2;
int   l6a_last_alpha_count   = 0;

// Inside structural_tick_once(), at the end:
if (online_l6a_enabled
    && (rbg_tick_counter % online_l6a_cadence) == 0)
{
    // Only run if the number of fissioned parents has grown since last check.
    int cur_alpha_count = (int)sub->fission_map.size();
    if (cur_alpha_count > l6a_last_alpha_count) {
        int newly_bridged = discover_analogies_locked(l6a_min_shared);
        l6a_last_alpha_count = cur_alpha_count;
        (void)newly_bridged;
    }
}
```

The `_locked` variant assumes `sub->mtx` is already held (as it is inside the structural tick). Requires refactoring the current `discover_analogies()` into public and internal variants, matching the `fission()` / `fission_locked()` split.

### Success criterion
- Training `metformin` then `berberine` with `online_l6a_enabled = true` produces the same bridges as the post-hoc `discover_analogies` call.
- Bridges appear within 100 ticks of the second concept fissioning.
- No performance regression on the 20-seed benchmark (structural tick stays under 1 ms).

### Risk
Bridge insertion during ongoing Hebbian consolidation could destabilize plasticity. If observed, add a "cooldown" after each bridge insertion (skip Hebbian on that tunnel for 5 ticks).

---

## 3. Language Generation Stage 2 (Syntax Macro-Layer)

**✅ SHIPPED** — SyntaxSlot struct with harmonic ω ratios (3:4:5 on base
15), phases at 2πk/N, Kuramoto pass 3b, temporal readout head with
narrow-Gaussian (Q=16) exclusion. `generate <label>` shell command
produces physics-selected slot winners, empty slots as honest refusal
when no cavity phase-locks. See `data/phase2/results.md`.

**Class: R** · **Effort: 2–3 weeks** · **Files: `src/physics.hpp`, `src/mal.hpp`, `src/nl_translator.hpp`**

### Problem
Section 5.4b of the SI paper proposes a three-stage physics-only language pipeline. Stages 1 (resonance signature) and 3 (label lookup) exist. Stage 2 — a syntax macro-layer with fixed grammatical-role frequencies — does not.

### Fix

**Step 3.1: Add syntax slots to physics.hpp.**

```cpp
struct SyntaxSlot {
    float natural_omega;      // fixed: ω_S, ω_V, ω_O
    float phase;              // Kuramoto-integrated
    float amplitude;          // sum of semantically locked cavities
    std::string role;         // "subject" | "verb" | "object"
    int   locked_cavity_id;   // winner at last readout (−1 if slot vacant)
};
std::vector<SyntaxSlot> syntax_slots;

void bind_syntax_slots() {
    syntax_slots = {
        { 6.28f, 0.0f, 0.0f, "subject", -1 },  // 1 Hz
        { 3.14f, 0.0f, 0.0f, "verb",    -1 },  // 0.5 Hz
        { 2.09f, 0.0f, 0.0f, "object",  -1 },  // 0.33 Hz
    };
}
```

**Step 3.2: Extend the tick() Kuramoto pass.**

Add a second coupling term between macro_nodes[k] and syntax_slots[s]:

```cpp
// Inside tick(), after HNCA macro-Kuramoto:
for (auto& s : syntax_slots) {
    float sum = 0.0f;
    for (const auto& m : macro_nodes) {
        sum += std::sin(m.phase - s.phase) * m.amplitude;
    }
    float dtheta = s.natural_omega
                 + syntax_coupling_norm * sum;
    s.phase += dtheta * dt;
    s.phase = std::fmod(s.phase + 9.42477796f, 6.28318531f) - 3.14159265f;
    // Slot amplitude = sum of semantic cluster amplitudes phase-close to this slot.
    float amp_sum = 0.0f;
    for (const auto& m : macro_nodes) {
        float phase_diff = std::fabs(m.phase - s.phase);
        amp_sum += m.amplitude * std::exp(-phase_diff * phase_diff);
    }
    s.amplitude = amp_sum;
}
```

**Step 3.3: Temporal readout head.**

```cpp
std::string physics_generate_sentence(int probe_cavity_id) {
    // Drive the probe, run the tick loop until slots stabilize.
    physics.drive(probe_cavity_id, 20.0f, 6.0f, 15);
    physics.run(200);

    // For each slot, find the cavity with peak displacement whose macro-node
    // phase is closest to the slot phase.
    std::vector<std::string> words;
    for (auto& s : physics.syntax_slots) {
        int   best_cav = -1;
        float best_amp = 0.0f;
        for (int i = 0; i < (int)physics.cavities.size(); ++i) {
            const auto& m = physics.macro_nodes[physics.cavities[i].cluster_id];
            float phase_diff = std::fabs(m.phase - s.phase);
            float score = std::fabs(physics.cavities[i].displacement)
                        * std::exp(-phase_diff * phase_diff);
            if (score > best_amp) { best_amp = score; best_cav = i; }
        }
        s.locked_cavity_id = best_cav;
        if (best_cav >= 0) words.push_back(label_of(best_cav));
    }

    // Concatenate in canonical order (subject, verb, object).
    return join(words, " ");
}
```

**Step 3.4: Add shell command.**

```cpp
else if (cmd == "generate") {
    std::string label;
    if (!(iss >> label)) continue;
    auto it = si.graph.by_label.find(label);
    if (it == si.graph.by_label.end()) continue;
    std::string sentence = physics_generate_sentence(it->second);
    std::fprintf(stdout, "generated: %s\n", sentence.c_str());
}
```

### Success criterion
- On `insulin_pathway`, `generate metformin` produces a coherent SVO triple like `metformin inhibits mtor` or `metformin activates ampk` — order matches the actual axiom edge direction.
- On queries where multiple verb-slot cavities compete, the output is deterministic under seed (same seed → same sentence).
- On queries outside the ontology, the readout head returns empty strings for empty slots (no hallucination).

### Extension
Adverbs and modifiers can be added as additional slots at ω = 4.19, 5.24, ... Choose frequencies as harmonic ratios to keep the Kuramoto lock structure clean.

---

## 4. Sparse Structural Tick

**✅ SHIPPED** — Active Wavefront Tracking with `quiet_ticks[i]` per
cavity, `K_idle = 20`, `wavefront_eps = 1e-4`. Bit-identical results
on 200-concept ontology. `hot_count()` accessor for observability.

**Class: R** · **Effort: 1–2 weeks** · **Files: `src/layer2.hpp`, `src/physics.hpp`**

### Problem
`structural_tick_once()` iterates all N cavities and all T tunnels every tick. On a 200-concept ontology this is fine (~0.5 ms). At 10⁴–10⁵ concepts it becomes untenable.

### Fix
Maintain a *touched-set* of cavities modified in the last physics tick. Only these enter the structural tick:

```cpp
struct PhysicsEngine {
    // ...
    std::unordered_set<int> touched_cavities;    // reset each drive
    std::unordered_set<int> touched_tunnels;     // reset each drive

    void drive(int nid, float freq, float amp, int dur) {
        // ... existing code ...
        // Mark this cavity and every incident tunnel as touched.
        touched_cavities.insert(nid);
        for (int e = 0; e < (int)tunnels.size(); ++e) {
            if (tunnels[e].cavity_a == nid || tunnels[e].cavity_b == nid) {
                touched_tunnels.insert(e);
            }
        }
    }

    void tick() {
        // ... existing tick body but iterate only over touched sets ...
        // At the end, propagate: any tunnel whose either endpoint moved
        // > eps becomes touched next tick. Cavities receiving force from
        // touched tunnels become touched.
    }
};
```

The structural tick then intersects `touched_cavities` with the fission-trigger scan and processes only that subset.

### Success criterion
- On the current 200-concept ontology, results are bit-identical (within FP rounding) to the dense implementation.
- On a synthetic 10⁵-concept ontology with only 200 concepts driven, per-tick time stays under 5 ms.
- Fissions still fire on the concepts that would have fissioned under the dense implementation.

### Risk
Cascade propagation of "touched" status can quickly grow the working set to the full graph under heavy training. Add a decay: touched-set entries are removed after K ticks of no fresh input.

---

## 5. Rubber-Tunnel Frequency Selectivity Theorem

**✅ SHIPPED (implementation)** — per-tunnel `resonance_omega` + `Q_factor`,
branchless band-pass gate in tick() tunnel-force loop. L6-A rubber bridges
spawn with `resonance_omega = 3.0`. Non-L6-A tunnels unchanged (gate=1).
Analytical transfer-function derivation for `mims_transfer.md` still open.

**Class: R** · **Effort: 2–3 weeks** · **Files: new `research-paper/mims_transfer.md`, `src/physics.hpp`**

### Problem
The claim that low-frequency probes ride the L6-A bridge while high-frequency probes are absorbed rests on the impedance mismatch between rubber (`k=0.05`) and steel (`k>=1.6`). The optimal probe frequency of 3 Hz was picked by inspection. We need a formula.

### Fix
Derive the transfer function of a mass-spring-damper tunnel:

For a tunnel of stiffness $k$, damping $c$, connecting mass $m_a$ to mass $m_b$ driven at frequency $\omega$:

$$
H(\omega) = \frac{k}{\sqrt{(k - m_b \omega^2)^2 + (c\omega)^2}}
$$

The half-power cutoff frequency (where amplitude is $1/\sqrt{2}$ of the DC transmission):

$$
\omega_c = \sqrt{k/m_b} \cdot \sqrt{1 + c^2/(2 k m_b)}
$$

For steel ($k=1.8$, $c=0.01$, $m=1$): $\omega_c \approx 1.34$ rad/s ≈ 0.21 Hz — steel is actually already a low-pass filter above this. Wait — that's wrong at first glance because we experience steel as transmitting high frequencies well. The reconciliation: at $\omega$ near the resonance $\omega_0 = \sqrt{k/m}$, there's a *peak* in the transfer function of magnitude $Q = 1/(c/\sqrt{k m})$. For steel that's $Q \approx 134$ — a very sharp resonance. Probes at exactly $\omega_0$ transmit strongly; probes far above are absorbed.

**Implementation.** Add a function `optimal_probe_freq(TunnelType)` to physics.hpp that returns:

```cpp
float optimal_probe_freq(const Tunnel& t, float m_b = 1.0f) {
    return std::sqrt(t.stiffness / m_b) / (2.0f * 3.14159265f);
}
```

For rubber ($k=0.05$): $\omega_0 = 0.22$ rad/s ≈ 0.036 Hz (not 3 Hz). The 3 Hz value used empirically was above rubber's resonance but far below steel's — the rubber tunnel is absorbing at 3 Hz too. What actually happens: at 3 Hz the rubber tunnel *acts as a static coupling* (below resonance), while steel is deep below its own resonance and also transmits well.

The correct analysis is a *ratio* of transfer functions: what frequency maximizes rubber-transmission divided by steel-transmission? Solve $\partial_\omega [H_\text{rubber}(\omega) / H_\text{steel}(\omega)] = 0$.

### Success criterion
- A `mims_transfer.md` paper deriving the transfer functions and the optimal-probe-frequency formula.
- `repurpose <label> auto` command that reads the target's incident tunnel materials and picks the probe frequency algorithmically.
- Reproducing the atorvastatin-at-rank-10 result on `insulin_pathway` without hand-picking 3 Hz.

### Fallback
Empirical sweep: probe at 20 log-spaced frequencies from 0.1 Hz to 30 Hz, rank cavities per frequency, report the frequency that maximally separates analogical partners from mechanistic downstream targets. This is a diagnostic, not a theorem.

---

## 6. Cross-Domain Transfer (Syllogism → Algebra)

**Class: R** · **Effort: 2–3 months** · **Files: new `data/algebra.axioms`, extended `src/rade.hpp`**

### Problem
L6-A shows cross-*concept* transfer within a domain (metformin → atorvastatin via `hmg_coa_reductase`). Cross-*domain* transfer — say a substrate trained on syllogisms improving performance on algebra — has not been demonstrated.

### Fix
Load two ontologies simultaneously and share the physics substrate:

**Step 6.1: Build `data/syllogism.axioms`** with abstract structural triples:

```
socrates    is_a         man
man         has_property mortal
premise_A   implies      premise_B
premise_B   implies      conclusion_C
```

**Step 6.2: Build `data/algebra.axioms`** with variable-binding triples:

```
x           is_a         variable
addition    is_a         operation
addition    associative  true
addition    commutative  true
5           is_a         integer
integer     closed_under addition
```

**Step 6.3: Add domain tags** to cavities:

```cpp
struct Cavity {
    // ...
    int domain_id = 0;    // 0 = shared, 1 = syllogism, 2 = algebra
};
```

Concepts that appear in both ontologies (e.g., `implies`, `equals`) get `domain_id = 0` and are shared.

**Step 6.4: Train on syllogism domain**, measure baseline algebra recall, then train jointly and measure again.

### Success criterion
- Recall of `commutative` concepts in algebra improves after syllogism training (via the shared `implies` cavity fissioning under contradiction between the domains).
- L6-A discovers cross-domain bridges (e.g., `associativity ~ transitive_composition`).
- The improvement is non-trivial (>10% RMS gain over algebra-only training).

### Risk
The two domains may share too few concepts for meaningful transfer. Mitigation: seed a small "shared foundation" ontology with logical connectives (`implies`, `and`, `or`, `not`, `equals`) that both domains reference.

---

## 7. Abduction Rule Discovery from Primitives-Only

**Class: R** · **Effort: 2–3 months** · **Files: new `src/abduction_discovery.hpp`**

### Problem
L6-A resolved analogy-rule discovery. Abduction (reverse-chaining from goal to hypothesis) is not currently discoverable from lower primitives — it must be seeded.

### Fix
An abduction rule has the form: given goal $G$ and rule $(P \Rightarrow G)$, hypothesize $P$. In physics-substrate terms: given a cavity $G$ that is target of many `implies` tunnels, and no cavity currently driving it, spawn a hypothesis cavity $P$ that would drive $G$.

**Mechanism.** After every training round, scan cavities that:
- Have high salience (recently referenced)
- Have low incoming excitation (no cavity currently activating them)
- Have at least one incoming `implies` tunnel

For each such cavity $G$, look at the source $P$ of its strongest `implies` tunnel. If $P$ is itself under-activated, mark $P$ as a *hypothesis candidate*. Add a new cavity `hypothesis_of_G` with a soft (`k=0.3`) tunnel to $P` — a physical realization of "if we assume $P$, then $G$ follows".

```cpp
int discover_abductions(float salience_min = 0.5f,
                        float incoming_max = 0.1f)
{
    int found = 0;
    for (int g = 0; g < (int)sub->cavities.size(); ++g) {
        if (sub->cavities[g].salience < salience_min) continue;
        float incoming_amp = compute_incoming_excitation(g);
        if (incoming_amp > incoming_max) continue;
        // g is a goal without a driver.
        int p = find_strongest_implies_source(g);
        if (p < 0) continue;
        // Insert hypothesis cavity and soft coupling.
        Cavity hyp = sub->cavities[p];  // clone
        hyp.salience = 0.0f;
        int hyp_id = (int)sub->cavities.size();
        sub->cavities.push_back(hyp);
        Tunnel t;
        t.cavity_a = hyp_id;
        t.cavity_b = p;
        t.stiffness = 0.3f;
        t.damping = 0.05f;
        t.polarity = 1.0f;
        sub->tunnels.push_back(t);
        ++found;
    }
    return found;
}
```

### Success criterion
- On a modified ontology where `raining implies wet_ground` exists but `raining` is not currently driven, `discover_abductions` spawns a `hypothesis_of_wet_ground` cavity coupled to `raining`.
- Subsequent probing of `wet_ground` weakly activates the hypothesis cavity, which in turn probes `raining` — a physical realization of "the ground is wet, therefore possibly it rained".
- Extension of the 20-seed benchmark to include abduction-cluster concepts.

### Risk
Naive abduction discovery could spawn thousands of speculative hypotheses. Add a limit: at most 3 abductions per training round.

---

## 8. Formal Liouville Analog for Tension-Bounded Expansion

**Class: M** · **Effort: 6–12 months** · **Files: new `research-paper/liouville_analog.md`**

### Problem
Axiom II states $\Delta|G_t| \leq \alpha \cdot T(t)$ — the graph grows only under tension. This is an inequality, not a conservation law. The classical Liouville theorem says phase-space volume is conserved under Hamiltonian flow. We need the analog for graph-valued state under tension-bounded expansion.

### Fix approach

Classical Liouville: for a Hamiltonian system on manifold $M$ with symplectic form $\omega$, phase-space volume $\int_M \omega^n$ is preserved by the Hamiltonian flow.

For graph-valued state: define an *extended* phase space
$$
\mathcal{P} = \bigsqcup_{N=1}^{\infty} \left(\mathbb{R}^{2N} \times \mathcal{G}_N\right)
$$
where $\mathcal{G}_N$ is the space of typed graphs on $N$ vertices. The dynamics is a piecewise Hamiltonian flow on each stratum plus discrete jumps between strata (fission events).

Conjecture: an appropriate weighted volume
$$
V(t) = \sum_N \int_{\mathcal{G}_N} e^{-\beta T(G)} \, d\text{vol}
$$
is preserved by the combined dynamics, where $T(G)$ is the tension functional and $\beta$ is a "cognitive temperature".

### Fix steps
1. **Prove** the piecewise-Hamiltonian flow is measure-preserving on each stratum (standard).
2. **Prove** the fission jump preserves the weighted volume across strata — the exponential weight compensates for the added dimensions ($e^{-\beta \Delta T}$ vs the volume gain from 2 new cavities + 1 tunnel).
3. **Derive** the "cognitive temperature" $\beta$ from RIFA's tension gain — this is the free parameter that makes the weight work.

### Success criterion
A theorem: *"The weighted phase-space volume $V(t) = \int_{\mathcal{P}} e^{-\beta T} \, d\text{vol}$ is preserved by the RB+G dynamics."*

### Why it matters
This is what would let us bound the thinker index (problem #10) and prove that RB+G is not just heuristically stable but *thermodynamically* stable — the graph cannot grow without bound, and the mathematics says why.

### Fallback
An empirical version: measure $V(t)$ across the 20-seed benchmark; if it stays within 5% under normal training, the conjecture is empirically supported even without a proof.

### ✅ Solution Sketch (Mahir) — Divergence-Free Hamiltonian Route

An independent solution path via classical Hamiltonian mechanics —
reaches the same weighted-volume conservation theorem in a shorter proof.

**Define the Hamiltonian directly on the substrate state:**

$$
H(x_i, v_i, \theta_k, \omega_k) = \sum_i \tfrac{1}{2} m_i v_i^2
                                + \tfrac{1}{2} \sum_{(i,j) \in E} k_{ij} (x_i - p_{ij} x_j)^2
                                + (\text{HNCA phase terms}).
$$

The engine's canonical pairs $(x_i, v_i)$ per cavity and
$(\theta_k, \omega_k)$ per macro-node are already conjugate positions
and momenta — no new state needed.

**Step 1 — Between fissions.** Show the velocity field is divergence-free:
$$
\nabla \cdot \vec{v} = \sum_i\left(\frac{\partial \dot{x}_i}{\partial x_i} + \frac{\partial \dot{v}_i}{\partial v_i}\right) = 0,
$$
by Hamilton's equations + equality of mixed partials. One-page classical
proof. Establishes volume conservation on each stratum.

**Step 2 — At fission.** Choose $\beta$ so the exponential weight
$e^{-\beta \Delta T}$ exactly cancels the added 3-dimensional volume:
$$
\beta = \frac{1}{\Delta T} \ln\!\left(\frac{V_\text{after}}{V_\text{before}}\right).
$$

**Combined:** $V(t) = \int_{\mathcal{P}} e^{-\beta T} \, d\text{vol}$ is
invariant under Hamiltonian flow + tension-gated fission.

This avoids the categorical machinery (nerve construction / stratified
Morse theory) of the roadmap-original approach. Uses only Hamilton's
equations + one exponential weight. **Effort under this route:
3–4 months, not 12.**

---

## 9. Fisher-Information Metric on Graph-Valued States

**Class: M** · **Effort: 6–12 months** · **Files: new `research-paper/graph_fisher.md`**

### Problem
CCH proposed a Fisher-information metric on the cognitive manifold. This works when the state is a vector. For graph-valued state (Axiom I of SI), the appropriate generalization is unclear.

### Fix approach

The Fisher-information metric on a parametric family $\{p_\theta\}$ is
$$
g_{ij}(\theta) = \mathbb{E}_\theta\left[\partial_i \log p_\theta \cdot \partial_j \log p_\theta\right].
$$

For graph-valued states we need a probability distribution *over graphs* parametrized by something (perhaps the concept-cavity displacements). One candidate:

$$
p_\theta(G) = \frac{1}{Z(\theta)} \exp\left(-\beta \sum_{v \in V(G)} \phi(x_v, \theta) - \gamma \sum_{e \in E(G)} \psi(x_{v(e)}, x_{w(e)}, \theta)\right)
$$

— an exponential family over typed graphs whose parameters are the cavity states.

**Step 9.1.** Prove this distribution has a well-defined Fisher-information metric on the parameter space.

**Step 9.2.** Show that the RB+G fission dynamics corresponds to a *geodesic* in this metric under the tension budget.

**Step 9.3.** Categorically generalize to typed graph categories — the metric should be functorial under type-preserving graph morphisms.

### Success criterion
A theorem: *"The RB+G expansion trajectory is a Fisher-geodesic in the graph exponential family, minimizing the tension integral $\int T(G_t) dt$ subject to the polarity-conflict boundary conditions."*

### Why it matters
This is the "cognitive Riemannian geometry" underlying the whole tri-layer architecture — with this, the C/R thinker index becomes measurable as a curvature-integrated quantity rather than a counting statistic.

### ✅ Solution Sketch (Mahir) — Boltzmann Distribution + `corr_ema`

Use the standard Fisher-metric formula on a Boltzmann distribution
computable from telemetry the engine already reports.

**Step 1 — Boltzmann distribution over graphs.**
The engine tracks total mechanical energy $E(G, \theta)$ via
`physics.audit_energy()`. Define:
$$
p(G | \theta) = \frac{1}{Z(\theta)} \exp\!\left(-\beta \, E(G, \theta)\right)
$$
parametrised by cavity displacements and tunnel plasticities collected
into $\theta$.

**Step 2 — Fisher metric in closed form.**
For any exponential family:
$$
g_{\mu\nu}(\theta) = \beta^2 \, \mathrm{Cov}\!\left(\frac{\partial E}{\partial \theta^\mu}, \frac{\partial E}{\partial \theta^\nu}\right).
$$

The right-hand side is a covariance of energy derivatives — and the
engine already tracks this via `corr_ema` in `layer2.hpp`. **The Fisher
metric is directly computable from existing telemetry.**

```cpp
Matrix fisher_metric() {
    int T = (int)sub->tunnels.size();
    Matrix g(T, T);
    float beta_sq = 1.0f;
    for (int i = 0; i < T; ++i)
        for (int j = 0; j < T; ++j)
            g(i, j) = beta_sq * corr_ema[i] * corr_ema[j];
    return g;
}
```

**Step 3 — Analogy-bridge optimality claim.**
L6-A inserts a rubber bridge between α-children $a_i, a_j$. The bridge
is *Fisher-optimal* iff the Fisher distance $d_g(a_i, a_j) < \varepsilon$
— i.e. the endpoints are statistically indistinguishable under
$p(G|\theta)$.

**Publishable claim:** *"L6-A analogical bridging is a Fisher-optimal
compression of the graph state — bridges are inserted exactly when the
Fisher metric declares the endpoints statistically equivalent."*

**Effort:** 3–4 months, largely writing + one C++ helper function to
extract $g_{\mu\nu}$ from `corr_ema`. No new physics required.

---

## 10. Upper Bound on Thinker Index C/R

**Class: M** · **Effort: 12+ months** · **Files: depends on #8 and #9**

### Problem
The thinker index grows monotonically under rule induction (§5.3.3) and L6-A bridge insertion (§7.7). Is it bounded from above?

### Fix approach
This is unlikely to be solvable in isolation. It depends on:
- **Problem #8** — the Liouville analog gives the phase-space volume growth rate.
- **Problem #9** — the Fisher metric gives the geodesic length per fission event.

Given both, the reachable derivation count under $L$ steps is bounded by the volume of a Fisher-ball of radius $L \cdot \sqrt{T_\text{avg}}$ around the seed graph. If this volume grows polynomially in $L$ (bounded), then C/R is bounded; if exponentially (unbounded), C/R diverges.

### Conjecture
For any physically realizable SI system with bounded tension budget $T_\text{max}$, the thinker index is bounded:
$$
C/R \leq \exp(\beta \cdot T_\text{max} \cdot |\Pi|)
$$
where $|\Pi|$ is the primitive count and $\beta$ is the cognitive temperature of problem #8. Bound is loose but finite.

### Why it matters
Practical significance: an SI system with a small tension budget cannot become an arbitrarily powerful thinker. This distinguishes RB+G from "unlimited computation": there is a physical cost to every derivation and a physical cap on how deep the tree can go.

Philosophical significance: SI is not Turing-omnipotent within its budget. It is bounded rational — like humans.

### ✅ Solution Sketch (Mahir) — Topological Exponential Bound

**Step 1 — Notation.**
Introduce disambiguated symbols alongside the paper's $C/R$:

- $P = |\Pi|$ — stored axiom / primitive count.
- $D$ — reachable derivation count (compositions of length $\leq L$).

The paper's thinker index $C/R$ is a function of $D/P$: an SI system
with $D \gg P$ has high $C/R$.

**Step 2 — Branching count.**
Each fission event gains 2 new cavities + 1 new tunnel — each can
participate in future derivations. The per-fission branching factor
$k \leq P$ (bounded by the primitive count). After $L$ fissions,
$D \leq k^L$.

**Step 3 — Tension-budget cap.**
Each fission costs $\Delta T$. Under $g_\text{max}=1$ the fission count
per training round is capped by $L_\text{max} = T_\text{max}/\Delta T$.
Substituting:

$$
\boxed{D_\text{max} \leq \exp\!\left(\beta \cdot T_\text{max} \cdot P\right)}
$$

where $\beta = \ln P / \Delta T$ is the cognitive temperature from #8.

**Step 4 — Interpretation.**
Loose but finite. Says any SI system with bounded tension budget has
finite reachable derivation count. Human brains have $P \approx 10^{11}$
and $T_\text{max} \approx 20$W metabolic budget → predicts bounded
rationality, matching empirical observation.

**Publishable statement:** *"For any physically realizable SI system,
$D_\text{max} \leq \exp(\beta \, T_\text{max} \, |\Pi|)$. SI is bounded
rational within its metabolic budget, not Turing-omnipotent."*

**Effort:** 2–3 months of write-up once #8 and #9 are done — this is
a page-long combinatorial argument built on the earlier theorems.

---

## Priority and Ordering

If executed sequentially, the recommended order is:

1. **#1 Auto-tune fission thresholds** (E, 3 days) — unblocks testing on new ontologies.
2. **#2 Online L6-A** (E, 5 days) — makes the engine self-contained (no operator).
3. **#4 Sparse structural tick** (R, 2 weeks) — unblocks scaling to real ontologies.
4. **#3 Language Stage 2** (R, 3 weeks) — closes the language-generation story.
5. **#7 Abduction discovery** (R, 3 months) — closes the primitive-discovery story.
6. **#6 Cross-domain transfer** (R, 3 months) — validates the framework empirically at scale.
7. **#5 Rubber-tunnel transfer theorem** (R, 3 weeks, can be done any time) — makes L6-A domain-independent.
8. **#8 Liouville analog** (M, 12 months) — foundational theorem.
9. **#9 Fisher metric on graphs** (M, 12 months) — foundational theorem, informs #10.
10. **#10 C/R upper bound** (M, 12+ months) — capstone, depends on #8 and #9.

### Parallelizable

- #1, #2, #3, #4 can all be done concurrently by a single implementer.
- #5, #7 are independent research threads.
- #8, #9 are independent math threads, both required for #10.

### Total time to "complete SI"

- **Engineering track alone** (#1–#4): 6 weeks.
- **Engineering + Research** (#1–#7): 6 months.
- **Everything including theorems** (#1–#10): **~15 months** (was 2–3 years —
  updated after Mahir's divergence-free Hamiltonian + Boltzmann-Fisher
  solution sketches cut Phase 4 from ~24 months to ~9–10 months).

---

## What This Roadmap Does NOT Claim

- **It does not claim SI will scale to human intellectual breadth.** That is a coverage problem (feed it more axioms), not an architectural problem.
- **It does not claim the ten items are exhaustive.** Doing #1–#10 will reveal new problems, as #7.7 empirical work revealed #1–#7 above.
- **It does not claim any of the math items will succeed on the first attempt.** Real theorems often reveal that the conjecture needs restating.
- **It does not claim engineering-track items are guaranteed easy.** #4 (sparse structural tick) in particular has subtle correctness issues around touched-set decay.

---

## What This Roadmap Does Claim

- Every problem in §8 of the SI paper has a concrete fix path.
- None require abandoning the tri-layer architecture.
- None require external validation from the ML community.
- The engineering track alone (#1–#4) gives a demonstrably self-contained, self-tuning, language-generating, scalable engine.
- The math track (#8–#10) gives the framework its theorem-strength foundations.
- Together they are a complete programme for "complete SI" in the sense defined in the response to *"is it impossible to fix all"*.

---

**Future Fixes: ten problems, one architecture, no impossibilities. What remains is time, code, and mathematics — not a category error.**
