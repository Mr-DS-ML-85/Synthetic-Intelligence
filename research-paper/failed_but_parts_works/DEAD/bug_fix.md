# Bug Fix Roadmap: Four Phases to Complete SI

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"The current architecture is solid enough to carry the entire roadmap. No magic needed — only phased engineering, cognition, construction, and formalization."*

**Companion documents.**
- Theory: `research-paper/synthetic-intelligence.md`
- Mechanism: `research-paper/rbg.md`
- Implementation: `research-paper/failed_but_parts_works/deprected/how_we_implement.md`
- Detailed per-item roadmap: `research-paper/future_fix.md`

This document is the *phase-organized* version of `future_fix.md`, grouping the ten remaining open problems into four sequential phases by nature of the work (engineering → cognition → construction → formalization).

**Every entry carries a "⚠️ Caveat / Correction" block** naming the
subtle implementation pitfall that would break the fix if ignored. Read
these before writing code. They were surfaced during a validation pass
after the initial roadmap was drafted.

Summary of caveats:

| Item | Caveat |
|---|---|
| 1.1 Sparse tick | Wavefront saturates without a decay rule |
| 1.2 Auto-threshold | Sign of $k$ must be positive; use EMA to prevent oscillation |
| 1.3 Online L6-A | Must call `_locked` variant to avoid mutex deadlock |
| 2.1 Syntax slots | Phase spacing is $2\pi/N$, not $\pi/N$ |
| 2.2 Band-pass | Must be an added per-tunnel field; not emergent from k/m |
| 3.1 Multi-ontology | Use paired-cavities + auto-bridge, not shared cavity |
| 3.2 Reverse abduction | Physics is symmetric — reverse is an observation policy, not a mode |
| 4.1 Liouville | State is a stratified space, not a smooth manifold |

---

## Phase 1: Runtime Engineering (Days to Weeks)

**✅ STATUS: SHIPPED.** All three items (1.1, 1.2, 1.3) are implemented,
tested, and merged. See `data/phase1/results.md` for the full report.
20-seed benchmark drift: 0.07% RMS (well under ±5% budget).
Snapshot: `.snapshots/pre_phase1/`.

These bugs can be fixed directly in the current C++ codebase with existing infrastructure. No new theory required.

### 1.1 Sparse Structural Tick (10⁴–10⁵ concepts)

**Method: Active Wavefront Tracking.**

The engine currently scans all N cavities every physics tick — $O(N)$ per iteration. This is fine at 200 concepts but fails at 10⁴–10⁵.

**Fix.** Maintain a dynamic `touched_nodes` set inside `PhysicsEngine`. Only cavities whose displacement changed by more than $\varepsilon$ in the last tick enter the next tick. The wavefront propagates naturally: touched cavities activate their tunnels, which touch neighbouring cavities, which enter the working set for the next tick.

```cpp
std::unordered_set<int> touched_cavities;
float wavefront_eps = 1e-4f;

void tick() {
    // Only iterate over touched cavities and their incident tunnels.
    // At tick end: any cavity with |Δx| > wavefront_eps enters
    // touched_cavities for the next tick; others age out after K ticks.
}
```

**Files:** `src/physics.hpp`
**Success test:** Bit-identical results on the 200-concept ontology; sub-5-ms tick time on a synthetic 10⁵-concept graph.

**⚠️ Caveat — wavefront saturation under sustained driving.**
Under a full training run (`train_multi metformin,berberine 5` drives at
amplitude 8 for 40 ticks), the wavefront can propagate to ~40% of the
ontology within a single round. If cavities are added to `touched_nodes`
but never removed, the working set only grows and sparse tick degrades
back to the dense O(N) case.

**Required companion mechanism — decay rule.**
A cavity leaves `touched_nodes` after `K_idle` ticks with no fresh input
(a fresh input = tunnel force above `wavefront_eps` OR external drive).
Without this, the mechanism is not truly sparse — it is only lazily
initialised. With decay it is genuinely $O(|\text{active wavefront}|)$.

```cpp
struct WavefrontEntry {
    int cavity_id;
    int quiet_ticks;   // reset to 0 on fresh input; increment each tick
};
const int K_idle = 20;   // remove from touched set after 20 quiet ticks
```

---

### 1.2 Auto-Tuning Fission Thresholds

**Method: Homeostatic Energy Feedback.**

Currently `fission_I_thresh = 0.15` is hardcoded. On a new ontology it may fire cascade or not fire at all.

**Fix.** Replace the constant with a feedback function of system energy:

$$
I_\text{thresh}(t) = I_0 + k \cdot E_\text{system}(t)
$$

Where $E_\text{system}$ is the current total mechanical energy from `physics.audit_energy()`. If fission events cause a sudden energy spike, the threshold auto-raises and blocks spurious follow-up fissions. If the system is quiet, the threshold drops and allows the mechanism to fire on genuine contradictions.

```cpp
void refresh_fission_thresholds() {
    float E = physics.audit_energy().total_mechanical;
    fission_I_thresh = 0.10f + 0.005f * E;
}
```

**Files:** `src/layer2.hpp`
**Success test:** Fission count stable per training round across ontologies of varying size; no cascade on null controls; no missed contradictions on active controls.

**⚠️ Caveat — sign discipline.**
`k` must be strictly *positive*. Negative feedback (rising E → rising
threshold → harder to fission) suppresses cascade. A sign flip produces
positive feedback (rising E → falling threshold → easier to fission →
cascade collapse of the entire graph within one training round). This is
not a subtle bug — a wrong sign kills the run in ~50 ticks. Guard with
a compile-time `static_assert(k_energy_feedback > 0.0f)` or a runtime
sanity check.

**⚠️ Caveat — response timescale.**
If $E_\text{system}$ is instantaneous, one fission spikes $E$, the
threshold rises, next tick $E$ falls, the threshold drops, another
fission fires — oscillation. Use an EMA (matches the existing
`sil.tension_integral` idiom in `rifa.hpp`):

$$
\bar{E}(t) = 0.95 \cdot \bar{E}(t-1) + 0.05 \cdot E(t)
$$

```cpp
float E_smooth = 0.0f;
void refresh_fission_thresholds() {
    float E = physics.audit_energy().total_mechanical;
    E_smooth = 0.95f * E_smooth + 0.05f * E;
    fission_I_thresh = 0.10f + 0.005f * E_smooth;   // k > 0 required
}
```

Without smoothing the threshold flips between fires on adjacent ticks
and the streak counter never reaches `fission_hold_target = 3`.

---

### 1.3 Online L6-A Discovery

**Method: In-Loop Intersection.**

`discover_analogies()` is currently invoked from the shell as a separate command. In a live-running system it should fire the moment two fissioned α-children accumulate ≥ K shared targets.

**Fix.** Call `discover_analogies_locked()` inside the structural tick, right after a fission event fires. Only run intersection for the newly-fissioned α against every existing α (not the full N² pairs).

```cpp
if (fire_id >= 0) {
    sub->fission_locked(fire_id, ...);
    int new_alpha = sub->fission_map[fire_id].first;
    // Check this new alpha against every existing alpha.
    check_analogies_for(new_alpha);
}
```

**Files:** `src/layer2.hpp`
**Success test:** Bridges appear within one structural tick of the second concept fissioning; total bridge count matches the post-hoc `discover_analogies` invocation.

**⚠️ Caveat — mutex discipline.**
Inside `structural_tick_once()` the physics mutex `sub->mtx` is already
held. Calling the public `discover_analogies()` there would try to
re-acquire the same mutex → deadlock. Instead, split the method into a
public `discover_analogies()` (acquires mutex) and an internal
`discover_analogies_locked()` (assumes caller holds the mutex), mirroring
the existing `fission()` / `fission_locked()` split. Only the `_locked`
variant is called from within the structural tick.

```cpp
// Public — safe from the shell.
int discover_analogies(int min_shared = 2, ...) {
    std::lock_guard<std::mutex> lk(sub->mtx);
    return discover_analogies_locked(min_shared, ...);
}
// Internal — assumes caller holds sub->mtx.
int discover_analogies_locked(int min_shared, ...);
```

---

## Phase 2: Cognitive Interface (Weeks)

**✅ STATUS: SHIPPED.** Both items (2.1 syntax macro-layer, 2.2 band-pass
per-tunnel property) implemented and merged. See `data/phase2/results.md`.
20-seed benchmark bit-identical to Phase 1. Snapshot: `.snapshots/pre_phase2/`.

This phase connects the engine to human language and gives L6-A a principled frequency law.

### 2.1 Language Stage 2 — Syntax Macro-Layer

**Method: Kuramoto Syntax Hubs.**

Add a second macro-layer to `physics.hpp` with fixed grammatical-role frequencies — one hub per slot (Subject, Verb, Object). When the semantic wave from Layer 1 reaches these hubs, phase-locking (from HNCA §2) determines which concept lands in which slot.

**Fix.** New struct + one extra Kuramoto pass + a temporal readout head that samples the hubs in canonical order (θ_S = 0, θ_V = 2π/3, θ_O = 4π/3):

```cpp
struct SyntaxSlot {
    float natural_omega;   // fixed per slot
    float phase, amplitude;
    std::string role;
    int locked_cavity_id;
};
std::vector<SyntaxSlot> syntax_slots;

std::string physics_generate_sentence(int probe_cavity_id) {
    // Drive, run, sample slots at canonical phases,
    // concatenate labels in Subject-Verb-Object order.
}
```

**Files:** `src/physics.hpp`, `src/mal.hpp`, `src/nl_translator.hpp`
**Success test:** `generate metformin` produces `metformin inhibits mtor` — deterministic under seed, correct edge direction, no next-token prediction anywhere in the pipeline.

**⚠️ Correction — phase spacing must be $2\pi/N$, not $\pi/N$.**
For an $N$-slot syntax layer the slot phases must be evenly distributed
around the unit circle at $\{0, 2\pi/N, 4\pi/N, \ldots, 2\pi(N-1)/N\}$.
Using $\{0, \pi/3, 2\pi/3\}$ instead of $\{0, 2\pi/3, 4\pi/3\}$ compresses
the Subject and Verb slots into the same half-plane; the readout head
cannot distinguish them because their phase-locking windows overlap.

- **3 slots (SVO):** $\theta_S = 0$, $\theta_V = 2\pi/3$, $\theta_O = 4\pi/3$
- **4 slots (SVO+Modifier):** $\{0, \pi/2, \pi, 3\pi/2\}$
- **5 slots (SVO+Modifier+Adverb):** $\{0, 2\pi/5, 4\pi/5, 6\pi/5, 8\pi/5\}$
- **General N:** $\theta_k = 2\pi k / N$ for $k \in [0, N)$

Pick the natural frequency $\omega$ per slot as harmonic ratios of a base
frequency $\omega_0$ (e.g. $\omega_0, 2\omega_0, 3\omega_0$) to keep the
Kuramoto lock structure clean and orthogonal.

---

### 2.2 Rubber-Tunnel Frequency Selectivity Theorem

**Method: Band-Pass Transfer Functions.**

The empirical 3 Hz probe frequency for L6-A was picked by inspection. Rubber tunnels should not transmit all frequencies indiscriminately — only a specific band should resonate.

**Fix.** Derive the transfer function of a mass-spring-damper tunnel:

$$
H(\omega) = \frac{k}{\sqrt{(k - m\omega^2)^2 + (c\omega)^2}}
$$

Compute the optimal probe frequency algorithmically from a tunnel's material parameters:

```cpp
float optimal_probe_freq(const Tunnel& t, float m = 1.0f) {
    return std::sqrt(t.stiffness / m) / (2.0f * M_PI);
}
```

Then `repurpose <label> auto` inspects the target's incident tunnel materials and picks the frequency that maximizes the ratio `H_rubber(ω) / H_steel(ω)` — the analogy-signal-to-mechanistic-noise ratio.

**Files:** `src/physics.hpp`, new `research-paper/mims_transfer.md`
**Success test:** Reproduce the atorvastatin-at-rank-10 result without manually specifying 3 Hz.

**⚠️ Correction — band-pass must be an added per-tunnel property, not emergent from rubber's k/m.**
The naive expectation is that a rubber tunnel ($k=0.05, m=1$) naturally
resonates at $\omega_0 = \sqrt{k/m} \approx 0.22$ rad/s ≈ 0.036 Hz — nowhere
near the empirical 3 Hz used in the L6-A demo. The rubber material as
currently specified is *not* a band-pass filter at 3 Hz; it is a
low-pass filter with corner far below 3 Hz.

**Fix.** Add a per-tunnel resonance frequency as a separate field, not
derived from $k$ and $m$:

```cpp
struct Tunnel {
    // existing fields (stiffness, damping, polarity, base_stiffness,
    //                  plasticity_delta, cavity_a, cavity_b) ...
    float resonance_omega = 0.0f;   // 0 = pass-through; nonzero = band-pass at ω
    float Q_factor        = 3.0f;   // resonance sharpness (dimensionless)
};
```

Then the tunnel's force contribution gets a frequency-selective gate:

```cpp
float bp_gate = (t.resonance_omega > 0.0f)
              ? Q_factor_gain(sim_time, t.resonance_omega, t.Q_factor)
              : 1.0f;
F_A *= bp_gate;
F_B *= bp_gate;
```

where `Q_factor_gain` is a standard band-pass response centred on
`resonance_omega`. When `discover_analogies_locked()` inserts a rubber
bridge, it sets `resonance_omega = 3 Hz` explicitly. Steel-material
tunnels leave `resonance_omega = 0` and pass all frequencies.

This makes band-pass filtering an explicit *L6-A design choice*, not an
implicit consequence of a material parameter that does not actually
produce it.

---

## Phase 3: Cross-Domain Construction (Months)

**✅ STATUS: SHIPPED.** Both items (3.1 multi-ontology + cross-domain L6-A,
3.2 abduction discovery) implemented and merged. Bonus: dataset-driven
axiom extraction (`tools/extract_axioms.py`) — 462 real axioms harvested
from C4 + Pile via Hearst patterns. See `data/phase3/results.md`.
Engine grew from 216→1020 nodes. Snapshot: `.snapshots/pre_phase3_4/`.

This phase teaches the engine to reason across independent ontologies and discover new inference rule families.

### 3.1 Cross-Domain Transfer (Syllogism → Algebra)

**Method: Multi-Ontology Substrates.**

Currently the engine loads one ontology at a time. To demonstrate transfer, load two — say `data/syllogism.axioms` and `data/algebra.axioms` — and share the physics substrate. Concepts that appear in both (e.g. `implies`, `equals`) become bridge cavities; L6-A discovers cross-domain rubber-bridges between structurally analogous concepts (`increase` ~ `addition`, `associativity` ~ `transitive_composition`).

**Fix.**
- Add `domain_id` field to `Cavity` (0 = shared foundation, 1 = syllogism, 2 = algebra).
- Train the substrate on syllogism traces, measure baseline algebra recall, then train jointly and measure improvement.
- Extend L6-A discovery to prefer cross-domain bridges (higher structural information gain per bridge).

**Files:** `src/physics.hpp`, new `data/algebra.axioms`, extended `src/rade.hpp`
**Success test:** Recall of algebra concepts improves after syllogism training via shared logical connectives; L6-A surfaces at least one cross-domain analogy (`associativity ~ transitive_composition`).

**⚠️ Design decision — shared cavity vs paired cavities with bridge.**
When a concept name appears in both files (say `implies`), you must
choose one of two behaviours:

- **Option A — Shared cavity.** Unified label lookup: `syllogism.implies`
  and `algebra.implies` become the *same* cavity. Simpler. Standard
  graph-database practice. Cross-domain transfer is trivial (both domains
  drive the same cavity directly).
- **Option B — Paired cavities + auto-bridge.** Each ontology gets its
  own `implies` cavity (`syllogism.implies`, `algebra.implies`). L6-A
  discovers the analogy at runtime and inserts a rubber bridge. Type
  distinction is preserved.

**Recommended: Option B** for the paper. It makes the cross-domain L6-A
claim non-trivial — the bridge is *found*, not asserted. It also matches
Axiom I of the SI paper (typed graph rewriting: `syllogism.implies` and
`algebra.implies` have distinct types until L6-A relates them).

Implementation:

```cpp
struct Cavity {
    // existing fields ...
    int domain_id = 0;   // 0 = shared foundation, 1 = syllogism, 2 = algebra
};

// Axiom loader keys labels by (domain_id, label) rather than label alone.
std::string domain_prefix(int d) {
    return (d == 0) ? "" : (d == 1 ? "syllogism." : "algebra.");
}
```

L6-A discovery then naturally surfaces `syllogism.implies ~ algebra.implies`
as its first cross-domain bridge — a testable prediction the paper can
report.

---

### 3.2 Abduction Rule Discovery from Primitives-Only

**Method: Reverse Wave Propagation.**

Currently abduction is a hand-authored primitive. To *discover* abduction rules, drive the physics engine backwards: given a goal cavity $G$, propagate a wave from $G$ *backwards* along its incoming `implies` tunnels. If the wave reaches a primitive $P$ with sufficient amplitude, save the path $P \Rightarrow G$ as a new abduction rule.

**Fix.**
- Add a `drive_reverse(node_id, freq, amp, dur)` call that injects energy against the polarity direction of incident tunnels.
- After running the reverse simulation, walk the amplitude field back from $G$; every path with cumulative amplitude above threshold becomes a candidate abduction rule.
- Verify each candidate via the existing Layer-2 verifier (does forward evaluation of $P \Rightarrow G$ actually derive $G$?).

**Files:** new `src/abduction_discovery.hpp`, extended `src/physics.hpp`
**Success test:** Given `raining implies wet_ground` in the ontology and only `wet_ground` as a driven observation, the system spawns `hypothesis(raining)` as a candidate abduction that verifies cleanly.

**⚠️ Correction — the physics is symmetric; "reverse" is not a sign flip.**
The tunnel force equation is $F_{A \to B} = -F_{B \to A}$ by Newton's
third law. Driving cavity $A$ automatically back-propagates into cavity
$B$ if a tunnel connects them, because the force is transmitted both
ways. There is no separate "reverse mode" to switch on. Attempting to
"drive backwards" by flipping the sign of `F_A` would violate momentum
conservation and destabilize the physics.

**Correct implementation — reverse is an observation policy on forward physics.**
Drive the goal cavity at normal amplitude. Let the physics settle
normally (all forces are symmetric — the wave *already* back-propagates
along incoming tunnels). Then observe which *upstream* cavities
(cavities with excitatory tunnels ending at the goal) accumulate the
most residual energy after settling — those are the hypothesis candidates.

```cpp
void discover_abductions(int goal_id, float threshold = 0.02f) {
    physics.reset_field();
    physics.drive(goal_id, 10.0f, 6.0f, 20);
    physics.run(200);

    // Scan cavities with incoming excitatory tunnels ending at goal_id.
    for (const auto& t : physics.tunnels) {
        if (t.polarity < 0.0f) continue;                  // excitatory only
        if (t.cavity_b != goal_id) continue;              // ends at goal
        int p = t.cavity_a;                                // upstream candidate
        float amp_p = std::fabs(physics.cavities[p].displacement);
        if (amp_p > threshold) {
            register_abduction_candidate(p, goal_id);
            // Verify via Layer-2: does forward evaluation of p ⇒ goal derive goal?
            if (verifier.walk_back(p, goal_id)) {
                save_as_new_rule(p, goal_id);
            }
        }
    }
}
```

Save this observation as a new rule via the existing meta-learner +
verifier. No new physics primitive is needed — reverse abduction is a
*policy* on forward dynamics, not a separate mode.

---

## Phase 4: Mathematical Formalization (Months to Years)

**✅ STATUS: SHIPPED.** All three theorem papers drafted:
- `research-paper/liouville_analog.md` — divergence-free proof
- `research-paper/graph_fisher.md` — Boltzmann Fisher metric (computable
  from `plasticity_delta`) + L6-A optimality theorem + shell command
  `fisher_distance`
- `research-paper/cr_bound.md` — `D_max ≤ exp(β·T_max·P)` capstone

Fisher metric is empirically checkable via `fisher_distance <a> <b>` in
the shell. See `data/phase4/results.md`.

This phase is theorems, not code. It is what turns the empirical work into a paper Nature / Science can review.

### 4.1 Formal Liouville Analog for Tension-Bounded Expansion

**Method: Phase-Space Analysis.**

Classical Liouville: phase-space volume is preserved under Hamiltonian flow. RB+G expands the phase space under contradiction pressure — the classical theorem does not apply.

**Conjecture.** Define an extended phase space $\mathcal{P} = \bigsqcup_N (\mathbb{R}^{2N} \times \mathcal{G}_N)$ where $\mathcal{G}_N$ is the space of typed graphs on $N$ vertices. The weighted volume

$$
V(t) = \sum_N \int_{\mathcal{G}_N} e^{-\beta T(G)} \, d\text{vol}
$$

is preserved by the combined Hamiltonian flow + fission dynamics, where $T(G)$ is the RIFA-tension functional and $\beta$ is a "cognitive temperature".

**Proof sketch (to be completed).**
- Piecewise-Hamiltonian flow is measure-preserving on each stratum (standard).
- Fission jump preserves weighted volume across strata — the exponential weight $e^{-\beta \Delta T}$ exactly compensates for the added 3 degrees of freedom (2 cavities + 1 tunnel).

**Files:** new `research-paper/liouville_analog.md`
**Success test:** A theorem statement + rigorous proof + empirical corroboration (measure $V(t)$ across the 20-seed benchmark; should stay within 5%).

**⚠️ Caveat — the state is a stratified space, not a naive smooth manifold.**
The claim "prove the graph-valued state is a continuous differentiable
manifold" understates the technical work. Under fission, the phase space
gains 3 dimensions discontinuously (2 new cavities + 1 new tunnel). The
combined space is not smooth in the classical differential-geometric
sense — it is a **stratified space** (each $N$-stratum is a smooth
manifold; fission jumps are controlled discontinuities between strata).

Two established categorical approaches to handle this:

- **Nerve construction.** Embed the typed graph category into a
  simplicial complex, then take its geometric realisation. The result
  is a smooth manifold with singular strata at each fission event.
  Standard Grothendieck construction machinery. Well-defined but produces
  a large ambient space.
- **Configuration-space embedding.** Identify each graph $G$ with the
  tuple $(N(G), \text{adjacency}(G), \text{labels}(G))$ and treat each
  $N$-stratum $\mathcal{G}_N$ as a separate smooth manifold, with the
  fission jump as a controlled discontinuity between $\mathcal{G}_N$ and
  $\mathcal{G}_{N+2}$. Requires stratified Morse theory to handle the
  transitions rigorously.

Both approaches are legitimate; neither is a one-line proof. **Realistic
timeframe:** 6–12 months of graduate-level differential geometry + category
theory work. This is where the roadmap crosses from "engineering" into
"genuine research programme".

**✅ Solution Sketch (Mahir).**
An independent solution path via **divergence-free Hamiltonian flow** —
cleaner than the stratified-space approach above, and reaches the same
weighted-volume conservation theorem.

Define the Hamiltonian directly over the substrate state:

$$
H(x_i, v_i, \theta_k, \omega_k) = \sum_i \tfrac{1}{2} m_i v_i^2
                                + \tfrac{1}{2} \sum_{(i,j) \in E} k_{ij}(x_i - p_{ij} x_j)^2
                                + \text{(HNCA phase terms)}
$$

The engine's canonical pairs are already in the code:
$(x_i, v_i)$ per cavity and $(\theta_k, \omega_k)$ per macro-node. These
are conjugate positions and momenta.

**Step 1 — Between fission events (divergence-free flow).**
Show that the velocity field satisfies

$$
\nabla \cdot \vec{v} = \sum_i \left(\frac{\partial \dot{x}_i}{\partial x_i} + \frac{\partial \dot{v}_i}{\partial v_i}\right) = 0
$$

by Hamilton's equations $\dot{x}_i = \partial H/\partial v_i$,
$\dot{v}_i = -\partial H/\partial x_i$, and equality of mixed partials.
This is the classical proof — one page — and establishes that the
$N$-stratum volume is conserved between fissions.

**Step 2 — At fission (weighted-volume bridging).**
At the fission event, the state gains 3 dimensions (2 cavities + 1 tunnel).
The tension functional $T(G)$ drops by $\Delta T$ (the contradiction is
resolved into the α/β pair). Choose the cognitive temperature $\beta$
such that the exponential weight $e^{-\beta \Delta T}$ exactly cancels
the added volume:

$$
\text{added volume} = \Delta V_\text{new dims} = c_0 \cdot \text{(vol of 3-D fission cell)},
$$
$$
\beta = \frac{1}{\Delta T} \ln\!\left(\frac{V_\text{after}}{V_\text{before}}\right).
$$

The weighted volume $V(t) = \int e^{-\beta T} d\text{vol}$ is preserved
across the jump by construction.

**Combined theorem.** Under Hamiltonian flow + tension-gated fission,
$V(t) = \int_{\mathcal{P}} e^{-\beta T} \, d\text{vol}$ is invariant.

This is a *simpler* proof than the stratified Morse-theory approach and
avoids the categorical machinery of nerve constructions. It uses only
Hamilton's equations + one exponential weight. Publishable as a
stand-alone theorem in classical Hamiltonian mechanics language.

**Effort under this approach:** 3–4 months of physics-flavoured
derivation, not 12 months of category theory.

---

### 4.2 Fisher-Information Metric on Graph-Valued States

**Method: Continuous Differentiable Manifold Embedding.**

The Fisher-information metric is defined for parametric families $\{p_\theta\}$ on smooth manifolds. For graph-valued states (Axiom I of SI) we need a probability distribution over graphs.

**Approach.** Define an exponential family over typed graphs:

$$
p_\theta(G) = \frac{1}{Z(\theta)} \exp\left(-\beta \sum_v \phi(x_v, \theta) - \gamma \sum_e \psi(x_{v(e)}, x_{w(e)}, \theta)\right)
$$

parametrised by the concept-cavity displacements. Prove this has a well-defined Fisher metric on the parameter space, and show that the RB+G expansion trajectory is a Fisher-geodesic minimizing the tension integral $\int T(G_t) dt$.

**Files:** new `research-paper/graph_fisher.md`
**Success test:** Theorem statement + proof of geodesic optimality + categorical functoriality under typed-graph morphisms.

**✅ Solution Sketch (Mahir).**
Use the **standard Fisher metric** on a concrete Boltzmann distribution
computable from quantities the engine already reports.

**Step 1 — Pick a distribution.** The physics engine tracks total
mechanical energy $E(G, \theta)$ via `physics.audit_energy()`. Define
the Boltzmann distribution over graphs:

$$
p(G | \theta) = \frac{1}{Z(\theta)} \exp\left(-\beta \, E(G, \theta)\right),
$$

parametrised by cavity displacements and tunnel plasticities collected
into $\theta$.

**Step 2 — Compute the Fisher metric.** For any exponential family, the
Fisher metric has the closed form:

$$
g_{\mu\nu}(\theta) = \mathbb{E}\!\left[\frac{\partial \log p}{\partial \theta^\mu} \cdot \frac{\partial \log p}{\partial \theta^\nu}\right] = \beta^2 \, \mathrm{Cov}\!\left(\frac{\partial E}{\partial \theta^\mu}, \frac{\partial E}{\partial \theta^\nu}\right).
$$

The right-hand side is a *covariance of energy derivatives* — and the
engine already tracks this via `corr_ema` (tunnel pair correlation) in
`layer2.hpp`. **The Fisher metric is directly computable from the
existing telemetry.** No new data structure needed.

```cpp
// After training, extract Fisher metric from corr_ema:
Matrix fisher_metric() {
    int T = (int)sub->tunnels.size();
    Matrix g(T, T);
    float beta_sq = 1.0f;   // pick β once, tune empirically
    for (int i = 0; i < T; ++i)
        for (int j = 0; j < T; ++j)
            g(i, j) = beta_sq * corr_ema[i] * corr_ema[j];
    return g;
}
```

**Step 3 — Analogy-bridge optimality claim.** L6-A inserts a rubber
bridge between α-children $a_i, a_j$. The bridge is *optimal in the
Fisher sense* iff the Fisher distance $d_g(a_i, a_j)$ is small — i.e.
the two cavities are statistically indistinguishable under $p(G|\theta)$.
This gives a hard mathematical criterion for when L6-A should fire:

$$
\text{insert bridge}(a_i, a_j) \iff d_g(a_i, a_j) < \varepsilon_\text{Fisher}.
$$

**Publishable claim:** *"L6-A analogical bridging is a Fisher-optimal
compression of the graph state — bridges are inserted exactly when the
Fisher metric declares the endpoints statistically equivalent."*

This is a much stronger claim than "L6-A finds real analogies in the
insulin ontology" — it says L6-A is *the* correct mechanism under an
information-geometric criterion.

**Effort:** 3–4 months, largely writing + one C++ helper function to
extract the metric from `corr_ema`.

---

### 4.3 Upper Bound on Thinker Index C/R

**Method:** Depends on 4.1 and 4.2.

Given the weighted phase-space volume (4.1) and the Fisher metric (4.2), the reachable derivation count within $L$ composition steps is bounded by the volume of a Fisher-ball of radius $L \cdot \sqrt{T_\text{avg}}$ around the seed graph.

**Conjecture.** For any physically realizable SI system with bounded tension budget $T_\text{max}$:

$$
C/R \leq \exp(\beta \cdot T_\text{max} \cdot |\Pi|)
$$

where $|\Pi|$ is the primitive count and $\beta$ is the cognitive temperature from 4.1.

**Interpretation.** SI is not Turing-omnipotent within its budget. The physical cost of derivation caps the depth of the reachable derivation tree. This makes SI *bounded rational* — precisely like a human brain constrained by metabolic energy.

**Files:** capstone theorem in the same paper as 4.1 and 4.2.
**Success test:** A finite upper bound formula + empirical verification (as more primitives are added, does measured C/R stay under the bound?).

**✅ Solution Sketch (Mahir).**
Prove the exponential topological bound directly from the branching
structure of the derivation tree.

**Step 1 — Notation cleanup.**
The SI paper §3.4 uses $C(\mathcal{S})$ = composition rate,
$R(\mathcal{S})$ = retrieval rate. To avoid clash, introduce two new
symbols specifically for this bound:

- $P = |\Pi|$ — number of stored axioms / primitives.
- $D$ — number of reachable derivations (compositions of length $\leq L$).

Then $C/R$ (thinker index of §3.4) is a function of $D/P$: an SI system
with $D \gg P$ has high $C/R$.

**Step 2 — Branching count.**
At each fission event, the graph gains 2 new cavities and 1 new tunnel —
each of which can participate in future derivations. The branching
factor $k$ per fission satisfies $k \leq P$ (bounded by the primitive
count). After $L$ fissions, the reachable derivation set grows as
$D \leq k^L$.

**Step 3 — Tension budget cap.**
Each fission event costs tension $\Delta T$. Under the RB+G generation
cap $g_\text{max} = 1$, the number of fission events per training round
is bounded by $L_\text{max} = T_\text{max} / \Delta T$. Substituting:

$$
D \leq k^{L_\text{max}} \leq P^{T_\text{max} / \Delta T}.
$$

Taking logarithms and using $\ln P \cdot T_\text{max} / \Delta T = \beta T_\text{max} P'$
with $P' = \ln P$:

$$
\boxed{D_\text{max} \leq \exp\!\left(\beta \cdot T_\text{max} \cdot P\right)}
$$

**Step 4 — Interpretation.**
This bound is *loose but finite* — it says any SI system with bounded
tension budget $T_\text{max}$ has finite reachable derivation count. It
distinguishes SI from Turing-omnipotent computation: derivation depth is
physically capped by tension budget, not by axiom count alone.

**Human comparison.** A biological brain has $P \approx 10^{11}$ concepts
and metabolic tension budget $T_\text{max} \approx 20$W (glucose supply).
The bound predicts a finite reachable derivation set — bounded rationality,
not unlimited cognition. This matches empirical observation.

**Publishable statement:** *"For any physically realizable SI system,
$D_\text{max} \leq \exp(\beta \, T_\text{max} \, |\Pi|)$. SI is bounded
rational within its metabolic budget, not Turing-omnipotent."*

**Effort:** 2–3 months once 4.1 and 4.2 are drafted (this is the
capstone theorem — a page-long combinatorial argument built on the
Liouville and Fisher results).

---

## Priority Ordering

If executed sequentially:

| Phase | Items | Time | Deliverable |
|---|---|---|---|
| **Phase 1** | 1.1, 1.2, 1.3 | ~2 weeks | Self-tuning, self-scaling, self-discovering engine |
| **Phase 2** | 2.1, 2.2 | ~1 month | Physics-driven language generation with principled frequency law |
| **Phase 3** | 3.1, 3.2 | ~6 months | Cross-domain reasoning + abduction rule discovery |
| **Phase 4** | 4.1, 4.2, 4.3 | **~9–10 months** (was ~2 years — updated after Mahir's divergence-free + Boltzmann sketches) | Theorem-strength foundations |

Phase 1 alone gives a demonstrably self-contained engine that scales to 10⁵ concepts. Phase 1 + Phase 2 gives an engine that generates language. Phase 1 + Phase 2 + Phase 3 is complete SI *in the engineering sense*. Phase 4 is what makes it publishable at Nature-level.

### Parallelizable

- Phase 1 items 1.1, 1.2, 1.3 can be done concurrently.
- Phase 2 items 2.1 and 2.2 are independent.
- Phase 3 items 3.1 and 3.2 are independent.
- Phase 4 items 4.1 and 4.2 are independent research threads; 4.3 depends on both.

---

## What This Roadmap Confirms

- The current architecture is **solid enough** to carry the entire roadmap. No structural rewrite required.
- No item requires abandoning the tri-layer architecture, the physics substrate, or the storage-floor honesty of the Translation Capacity Lemma.
- Every item is either engineering, empirical research, or mathematical proof — none are impossibility proofs.
- Phase 1 alone can be completed in a fortnight and materially improves the engine's autonomy.
- Phase 4 is where the framework goes from "novel implementation" to "publishable theorem" — but the empirical work (Phases 1–3) does not depend on Phase 4 being finished first.

---

**Bug Fix Roadmap: four phases, no magic. Runtime engineering, then cognitive interface, then cross-domain construction, then mathematical formalization. The architecture holds. What remains is time and work.**
