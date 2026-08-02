# Emission and Readout — Phase, Momentum, Gauge and Metastability

**Irfan Mahir**  
*Independent Researcher, Dhaka, Bangladesh*  
irfan@furylogic.com

> How a settled field becomes an ordered output, and the mechanisms that stop it locking.

---

## Read This Before Citing

**The emission problem is only half solved, and the two halves are easy to confuse.**

**MRS** (`mrs.md`) has a *recognition* half and an *emission* half. Recognition works: bigram sensory lanes survive misspellings and transpositions at 7/7 measured. Emission **babbles** and is retired. Citing MRS as a working generator misreports it.

**Concept-level ordering works; byte-level does not.** CPME's phase-collapse sweep and CPSB's gauge offsets order *which concept follows which*, and that holds. Producing the *bytes of a word* is unsolved, and the Language volume argues that for grammar the question was mis-stated rather than merely hard.

**CSAT** is opt-in and off by default.

---

## Contents of This Volume

| # | Paper | File |
| --- | --- | --- |
| 1 | Escaping the Echo: Momentum-Coupled Phase Emission for Generative Resonance in Synthetic Intelligence | `mcpe.md` |
| 2 | Holographic Topological Resonance (HTR): Shifting from Predictive Generation to Structural Comprehension in Wave-Based Synthetic Intelligence | `htr.md` |
| 3 | Chiral Phase-Momentum Emission (CPME) | `cpme.md` |
| 4 | CPME-II: Phase-Consistent Hebbian Consolidation | `cpme_ii.md` |
| 5 | Chiral Phase-Shear Binding (CPSB): Rendering Syntax as Gauge-Theoretic Tension in Wave-Based SI | `cpsb.md` |
| 6 | Voltage-Gated Harmonic Topology (VGHT) | `vght.md` |
| 7 | Chaotic Strange-Attractor Trajectory (CSAT): Breaking Ground-State Locking in Synthetic Intelligence | `CSAT.md` |
| 8 | Morphological Resonance Synthesis (MRS): Language Construction Without Tokens, Gradients, or Learned Embeddings | `mrs.md` |

---


\newpage

---

## Escaping the Echo: Momentum-Coupled Phase Emission for Generative Resonance in Synthetic Intelligence


**Abstract**
In wave-based cognitive substrates utilizing Continuous Morphological Dynamics (CMD), external stimuli inherently generate maximal amplitude at the point of injection, leading to "Stimulus Reflection" (the parrot effect) during syntax emission. We propose a novel non-algorithmic architecture, **Momentum-Coupled Phase Emission (MCPE)**, which transitions the readout mechanism from absolute amplitude to kinetic wave momentum. By coupling syntax oscillators exclusively to the first derivative of the energy field ($dA/dt > 0$), the substrate naturally ignores decaying direct stimuli and phase-locks exclusively to emergent, forward-scattering internal thoughts.

### 1. Introduction: The Acoustic Blinding Problem

In the current iteration of the SI engine, the Kuramoto syntax slots (Subject, Verb, Object) phase-lock to cavities with the highest absolute amplitude ($A$). When external input (e.g., "notch created minecraft") drives the field, those specific cavities reach maximum $A$. Before the wave can propagate deep into the latent graph to retrieve a meaningful association, the syntax slots lock onto the massive input wave, resulting in a mirrored output. The system is structurally blind to its own internal resonance because the external drive acts as an acoustic flashbang.

### 2. Novel Architecture: Momentum-Coupled Phase Emission (MCPE)

To solve this without algorithmic filtering, we introduce a Hamiltonian shift in the readout layer. Instead of coupling syntax slots to the "position" (Amplitude, $A$) of the wave, we couple them to the "momentum" (Flux, $P$).

#### 2.1. First-Derivative Readout ($dA/dt$ Gating)

In a physical wave system, a cavity directly hit by a stimulus reaches peak energy and immediately begins to disperse it (cooling down). Its amplitude is high, but its momentum is negative ($dA/dt \le 0$). Conversely, deep-graph nodes that are currently being activated by internal resonance are absorbing energy; their amplitude might be lower, but their momentum is strictly positive ($dA/dt > 0$).

We redefine the coupling strength $K_{emit}$ between a cavity $i$ and a syntax slot as:
$$ K_{emit} = \Theta\left(\frac{dA_i}{dt}\right) \cdot A_i $$
Where $\Theta(x)$ is the Heaviside step function (returns $x$ if $x>0$, else $0$).
*Result:* Directly driven words (the user's prompt) instantly decouple from the output slots the millisecond the prompt ends, because they are decaying. The output slots are physically forced to lock onto the *ripples* (the internal thoughts), not the *splash* (the user's input).

#### 2.2. Thermodynamic Refractoriness (The "Acoustic Mass")

To prevent the wave from bouncing directly back to the input nodes, we modify the cavity inertia. When a cavity is driven externally, it accumulates temporary "acoustic mass" $M_i$:
$$ \frac{dM_i}{dt} = \gamma I_{ext} - \delta M_i $$
Higher mass lowers the cavity's fundamental oscillating frequency temporarily, pushing it out of the L6-A (3Hz) emission band. It retains its associative links but becomes too "heavy" to be picked up by the Kuramoto slots.

#### 2.3. Temporal Settle Separation (Perception vs. Articulation)

We separate the physics tick into two distinct phases using a global damping coefficient $\zeta$:

1. **Injection Phase ($T_0$ to $T_1$):** External wave hits. $\zeta$ is high. Energy is absorbed. Syntax slots are decoupled ($K=0$).
2. **Articulation Phase ($T_1$ to $T_2$):** External drive is 0. $\zeta$ drops. The wave propagates through the L6-A bridges. Syntax slots engage, seeking positive momentum ($dA/dt > 0$).

### 3. Expected Dynamics

**Scenario:** User inputs `insulin controls glucose`.

1. **$T_0$ (Injection):** Cavities `insulin`, `controls`, and `glucose` spike in absolute amplitude ($A \approx 1.0$). Their mass $M_i$ increases.
2. **$T_1$ (Propagation):** Input ceases. The energy from `insulin` and `glucose` propagates through the ontology graph to connected cavities like `metabolism`, `pancreas`, and `glycogen`.
3. **$T_2$ (Articulation):**
* `insulin` has high $A$, but $dA/dt < 0$ (cooling). It is ignored.
* `pancreas` has medium $A$, but $dA/dt > 0$ (warming up). It is locked.
* The Kuramoto slots phase-lock to `pancreas regulates metabolism` rather than parroting the input.



**Conclusion:** MCPE preserves the zero-rule continuous dynamics of the substrate while physically guaranteeing that the engine generates consequences rather than echoing stimuli.

---



\newpage

---

## Holographic Topological Resonance (HTR): Shifting from Predictive Generation to Structural Comprehension in Wave-Based Synthetic Intelligence


**Abstract**
Current Large Language Models (LLMs) simulate comprehension through autoregressive statistical approximation, compressing language into latent vectors without spatial or structural understanding. We propose **Holographic Topological Resonance (HTR)**, a wave-based cognitive architecture for true language comprehension and construction. By treating sentences not as token sequences but as **chiral wave-interference patterns**, the engine physically "understands" by aligning input topology with intrinsic graph geometry. Language construction is achieved via **Phase-Collapse Projection**, unfolding an n-dimensional thought back into a 1D temporal sequence without templates, syntax trees, or stochastic prediction.

### 1. Introduction: The Illusion of Prediction

An LLM predicting "Minecraft" after "Notch created" is analogous to a parrot mimicking a frequent sound sequence. It lacks the internal geometry of *creation* or the structural boundary between *agent* and *artifact*. True understanding requires an internal isomorphic state—a physical subgraph where "Notch" and "Minecraft" are held in a dynamic, tensioned relationship by a directed force ("created"). To bridge the gap from a 1D string of user bytes to an n-dimensional cognitive state, and back to a 1D response, we introduce the HTR architecture.

### 2. Novel Architecture: The HTR Substrate

To achieve true comprehension, the engine must perform three non-algorithmic physical processes:

#### 2.1. Chiral Wave Injection (Encoding Syntax as Phase-Delay)

A sentence has directionality (e.g., "Dog bites man" vs. "Man bites dog"). Instead of parsing arrays or using positional embeddings, HTR uses **Chiral (Directional) Wave Injection**.
As the user's bytes stream into the engine, they hit the substrate with sequential time delays ($\Delta t$). This creates a directional wave-front.

* **Mechanism:** The time delay translates directly into a **Phase Angle ($\phi$)**.
$$ \phi_i = \omega_{base} \cdot (T_{current} - t_{arrival_i}) $$
* **Chirality convention (explicit).** With this definition,
  $(T_{current} - t_{arrival_i})$ is the elapsed time since byte $i$
  entered the substrate. Earlier arrivals therefore accumulate more
  phase; later arrivals accumulate less. "Subject leads in phase"
  means the subject cavity is *ahead* in the phase progression
  because it entered the substrate first, not that it has the
  numerically smaller phase value. Implementations must record
  $t_{arrival_i}$ per cavity at drive time and evaluate $\phi_i$ from
  the running $T_{current}$; any sign inversion of the elapsed-time
  term will invert the encoded chirality and produce reversed word
  order in the readout stage.
* **Result:** The input is no longer a list of words. It is a standing interference pattern where the *Subject* leads in phase, the *Verb* is phase-shifted by $\pi/2$, and the *Object* by $\pi$. Syntax is natively encoded as a physical gradient across the graph.

#### 2.2. Topological Isomorphism (Structural Resonance Metric)

A prompt is said to *resonate structurally* with the engine's
long-term memory when the temporary interference pattern of the
prompt aligns with the physical shape of a stored axiom subgraph.
This is a strictly weaker claim than "the engine understands the
prompt" — resonance is a necessary, not sufficient, correlate of
comprehension — but it is directly measurable in the substrate's
existing state and requires no additional variables.

* **Mechanism:** The injected chiral wave propagates through the graph. The L6-A bridges act as resonance chambers. If the user asks a complex question (e.g., "What lowers blood sugar?"), the wave excites "lowers" and "blood sugar".
* Because of the Hebbian physics, the wave naturally converges on "insulin" or "metformin" not because they are statistically next, but because they physically complete the topological circuit.
* **Resonance metric.** Let $M_{\text{touched}}$ be the set of
  macro-nodes whose constituent cavities have been reached by the
  current input wavefront (i.e., whose `quiet_ticks` is below the
  idle threshold). Define the *localized* Kuramoto order parameter:
  $$ R_{\text{local}} = \frac{\left| \sum_{m \in M_{\text{touched}}} A_m \, e^{i\theta_m} \right|}{\sum_{m \in M_{\text{touched}}} A_m + \varepsilon}. $$
  The comprehension-correlated signal is the *rise* in this quantity
  above its recent EMA baseline:
  $$ \Delta R = R_{\text{local}} - \text{EMA}(R_{\text{local}}). $$
  A significant $\Delta R > 0$ after propagation indicates the input
  drove the field into a phase-consensus state substantially more
  ordered than baseline drift. Absolute $R$ is not diagnostic: a
  trivially quiescent field can have $R \approx 1$ with no semantic
  content. Only the localized rise $\Delta R$, restricted to macros
  the input actually reached, is meaningful.
* **Interpretation.** $\Delta R$ is best read as a *structural
  resonance metric*, not a proof of understanding. High $\Delta R$
  licenses articulation; low $\Delta R$ licenses honest silence.
  Anything stronger requires a phase-aware Hebbian consolidation
  rule (see §5, Failure Modes), which is proposed as future work.

#### 2.3. Phase-Collapse Projection (Emergent Language Construction)

Once the internal subgraph reaches a stable topological state (the
"thought"), it must be translated back into language. Unlike LLMs
that predict word-by-word, HTR *projects* the entire structure into
a 1D temporal sequence by unwinding the phase space.

* **Per-cavity phase.** Rather than inheriting phase from the
  macro-node cluster (which would collapse an entire cluster's
  cavities to the same emission instant), each cavity is assigned an
  *instantaneous phase* derived from its own two-dimensional state:
  $$ \phi_i = \operatorname{atan2}\!\bigl(v_i,\; \omega_{\text{base}} \cdot x_i\bigr). $$
  This uses no additional state — velocity $v_i$ and displacement
  $x_i$ are already integrated by the substrate — and yields a
  continuous phase field over the cavity population with fine enough
  resolution to unwind a several-node subgraph into distinct
  emission instants.

* **Mechanism.** A carrier phase $\phi_{\text{carrier}}$ is swept
  monotonically from $0$ to $2\pi$ (non-cyclic, one pass, wrap-around
  terminates the sweep). At each swept instant, each cavity's
  emission weight is a Gaussian window on the phase difference:
  $$ w_i(\phi_{\text{carrier}}) \;=\; A_i \cdot \exp\!\bigl(-\kappa_{\text{sweep}} \cdot (\phi_{\text{carrier}} - \phi_i)^2\bigr). $$
  The Dirac $\delta$-form is a limiting idealisation; the
  implementation uses the Gaussian directly to avoid the
  discontinuity artefact and to give a physically-motivated
  window width $1/\sqrt{2\kappa_{\text{sweep}}}$.

* **Emission trigger.** At each sweep step the cavity with the
  highest $w_i$ that also satisfies the momentum eligibility
  (`ReLU(dA_i/dt) > 0`, from the MCPE readout layer) and the
  emission floor is emitted, then suppressed from all subsequent
  steps in the same sweep.

* **Result.** The thought is an $n$-dimensional physical structure.
  The carrier phase acts as a 1D scanner sweeping across it. The
  nodes emit in order determined by their per-cavity phases; the
  output length is emergent — as many tokens as the field has
  distinct high-momentum peaks under the sweep — and can be shorter
  than the previous fixed slot count when the field is sparse, or
  longer when it is dense. No syntax trees, no `if-else` grammar
  rules, no next-token probabilities. The sentence constructs itself
  through geometric projection.

### 3. Expected Dynamics: Processing a Complex Query

**User Prompt:** `metformin activates what in cells`

1. **Injection (Chirality):** `metformin` hits first ($\phi \approx 0$), `activates` second ($\phi \approx \pi/3$), `cells` third ($\phi \approx 2\pi/3$). A directional energy gradient flows from metformin outward.
2. **Comprehension (Topology):** The gradient flows through the `REL_ACTIVATES` tunnels. It hits `AMPK`. Because `AMPK` is structurally linked to `cells` in the long-term graph, a resonance loop completes. The substrate physically stabilizes. **The engine has understood the query.**
3. **Construction (Collapse):** The carrier wave sweeps from $0$ to $2\pi$.
* At $\phi_{early}$, it hits the resonating result (`ampk`).
* At $\phi_{mid}$, it hits the relational tunnel (`regulates`).
* At $\phi_{late}$, it hits the downstream effect (`metabolism`).
**Output Generated:** `ampk regulates metabolism`.



**Conclusion:** The HTR architecture entirely bypasses statistical
generation. By encoding word order as arrival-phase chirality,
structural match as a localized rise in Kuramoto coherence, and
generation as continuous phase-collapse projection over a per-cavity
phase field, the substrate realizes input encoding, internal state
metric, and output construction on a single phase manifold. Whether
this constitutes *comprehension* in a philosophically loaded sense
is deferred to future work; what the mechanism does provide is a
substrate whose emission cannot be reduced to next-token statistical
approximation because the emission is *geometric*, not
*distributional*.

---

### 5. Failure Modes

Three failure modes are known and should be documented explicitly
before any implementation is deployed.

1. **Phase clumping without per-cavity resolution.** Prior CMD
   implementations gave cavities the phase of their parent
   macro-cluster. Under such a scheme Phase-Collapse Projection
   degenerates to macro-level emission: every cavity in a cluster
   emits at the same sweep instant, and the output collapses to at
   most one token per macro. The per-cavity phase definition
   $\phi_i = \operatorname{atan2}(v_i, \omega_{\text{base}} \cdot x_i)$
   from §2.3 is a required precondition; implementations that skip
   it will observe uniform bursts rather than sequential emission.

2. **Chirality–Hebbian mismatch.** Chiral phase encoding on
   injection (§2.1) only contributes to comprehension if the
   long-term memory is *also* consolidated with phase awareness.
   The Hebbian rule as inherited from the CMD substrate updates
   tunnel plasticity from the product of endpoint velocities and
   knows nothing about phase alignment. A phase-coincidence Hebbian
   term of the form
   $$ \Delta w_{ij} \propto v_i \cdot v_j \cdot \cos(\theta_i - \theta_j) $$
   is the natural companion, and is proposed as a separate follow-up
   ("HTR-II: Phase-Consistent Consolidation"). Without it, the
   chirality of injection dissipates over turns rather than being
   stored, limiting the resonance metric's sensitivity to recent
   context.

3. **Circular sweep ambiguity and empty resonance.** The Gaussian
   sweep window is unimodal on the real line, but phase is defined
   modulo $2\pi$: a cavity with $\phi_i$ near $0$ or near $2\pi$ can
   pattern-match at either boundary of the sweep, producing
   duplicate or edge-clustered emissions. The non-cyclic sweep
   $[0, 2\pi)$ prescribed in §2.3 mitigates this by treating
   wrap-around as end-of-output. Empty resonance — the case where
   the graph is too sparse or the input too disconnected for
   $\Delta R$ to rise — produces zero emissions after the momentum
   and floor filters; this is the correct honest-silence behaviour
   but will initially read as a regression to users habituated to
   the fixed-slot readout's always-something-out property.

### 6. Relation to Momentum-Coupled Phase Emission (MCPE)

HTR and the concurrently-proposed MCPE architecture are
complementary rather than competing. MCPE identifies *which*
cavities are eligible to emit (positive-momentum, low acoustic-mass,
active only during the articulation phase of the settle split). HTR
identifies *in what order* eligible cavities should emit (per-cavity
phase, swept by a carrier). The two mechanisms operate on orthogonal
observables: MCPE on $dA_i/dt$, $M_i$, and drive envelope; HTR on
$\phi_i$, $R_{\text{local}}$, and the arrival-phase field. A
substrate implementing both simultaneously — see the joint CPME
proposal — is expected to exhibit both non-parroting behaviour
(from MCPE's momentum filter) and emergent variable-length output
in structurally-resonant word order (from HTR's phase-collapse
projection).

---



\newpage

## Chiral Phase-Momentum Emission (CPME)


*A unified emission substrate combining the Momentum-Coupled Phase
Emission (MCPE) filter architecture with the Holographic Topological
Resonance (HTR) phase-projection readout. CPME operates on a single
phase manifold: chirality on injection, coherence-rise as the
resonance metric, momentum as the eligibility filter, and continuous
phase-collapse sweep as the emission mechanism. No syntax trees, no
grammar rules, no next-token statistics, and no fixed output length.*

---

### 0. Position

The Continuous Morphological Dynamics (CMD) substrate, extended by
Voltage-Gated Harmonic Topology (VGHT), produces domain-orthogonal
cognition with sparse cross-domain analogical bridging. Its emission
layer, however, has two orthogonal deficits that the previous
literature treats separately:

1. **Stimulus Reflection.** External input directly drives concept
   cavities to maximal amplitude; the fixed-slot Kuramoto readout
   locks onto those cavities before internal propagation can produce
   a downstream response. The system parrots.
2. **Fixed-Slot Rigidity.** The Subject/Verb/Object slot readout
   presupposes a three-word template. Simpler answers waste slots;
   more complex answers are truncated. Word order is imposed by the
   slot definitions rather than emerging from the field.

MCPE addresses (1) by shifting the readout observable from amplitude
$A$ to momentum $dA/dt$ and by muting recently-driven cavities via
acoustic mass. HTR addresses (2) by replacing the fixed slots with a
continuous carrier-phase sweep over a per-cavity phase field, and by
introducing arrival-time chirality on injection so that word order
becomes a physical gradient rather than a syntax rule.

CPME unifies the two proposals into a single emission substrate.
MCPE identifies *which* cavities are eligible to emit; HTR identifies
*in what order* eligible cavities emit. The two operate on
orthogonal state variables and compose without collision.

---

### 1. Architecture

Let the substrate be the CMD + VGHT engine as previously specified:
cavities indexed by $i \in [0, N)$ with per-cavity displacement
$x_i$, velocity $v_i$, heat $T_i$, and quiet age $q_i$; tunnels with
per-tunnel stiffness $k$, damping $c$, polarity $p$, and voltage
gating $g \in \{0, 1\}$; and macro-nodes with per-cluster phase
$\theta_m$, amplitude $A_m$, and natural frequency $\omega_m$ drawn
from the VGHT sub-band ladder. The base cavity frequency is
$\omega_0$.

CPME adds six mechanisms.

#### 1.1 Chiral Wave Injection (from HTR §2.1)

Each cavity records the simulation time at which it was most
recently driven:

    t_arrival_i ← sim_time    (whenever drive(i, ·) is called)

The cavity's oscillator phase at any subsequent tick is
$\phi_i^{\text{driven}} = \omega_0 \cdot (T_{\text{current}} - t_{\text{arrival}, i})$.
Earlier-arriving bytes therefore accumulate *more* phase; the
subject leads in phase because it entered the substrate first.
Chirality is inherited *for free* from the existing sinusoidal drive
term — the arrival time only needs to be *recorded* so the readout
can reference it. No new tick math.

#### 1.2 Momentum Filter (from MCPE §2.1)

Every cavity carries an exponential-moving-average amplitude
envelope:

    E_i ← β · E_i + (1 − β) · |x_i|            (β = 0.9)

and the tick publishes the *previous-tick* envelope as $E_i^{\text{prev}}$
so that the momentum

    P_i = E_i − E_i^{\text{prev}}

is the low-pass-filtered rate of change of amplitude. Directly-driven
cavities that have peaked and are dispersing energy have $P_i \le 0$;
deep-graph cavities absorbing energy from downstream propagation
have $P_i > 0$. The emission eligibility factor is
$\text{ReLU}(P_i)$, a smooth non-negative scalar.

This replaces the naïve $dA/dt$ = $|v|$ - style derivative, which
oscillates at $2\omega_{\text{drive}}$ and produces a strobe artefact
in the readout. Envelope EMA is the physically correct low-pass.

#### 1.3 Acoustic Mass (from MCPE §2.2)

Each cavity carries a per-cavity mass increment $M_i$ driven by the
external injection load:

    dM_i/dt = γ · I_{ext, i} − δ · M_i

with $\gamma = 0.30$, $\delta = 0.08$. $I_{\text{ext}, i}$ is the
instantaneous drive envelope on cavity $i$, i.e., precisely the
signal that makes an input-driven cavity "loud." The resulting
emission mask

    μ_i = 1 / (1 + M_i / M_{scale})            (M_scale = 1.0)

falls toward zero for freshly-driven cavities and returns toward
$\mu_i = 1$ as $M_i$ decays. Applied only in the emission scoring;
the field dynamics are untouched, so graph connectivity is
preserved.

#### 1.4 Settle Separation (from MCPE §2.3)

The tick maintains a global injection load

    L = Σ_i drive_amp_i · drive_envelope_i

and derives a *slot coupling* factor

    K_settle = 1 − σ((L − L_thresh) / L_scale)

with $\sigma$ the logistic, $L_{\text{thresh}} = 0.5$,
$L_{\text{scale}} = 0.2$. During active injection, $K_{\text{settle}} \approx 0$
and the emission readout is suppressed. As the drive envelopes decay,
$K_{\text{settle}} \to 1$ and articulation engages. The transition is
continuous — no discrete $T_0 / T_1 / T_2$ boundary.

#### 1.5 Coherence-Rise Metric (from HTR §2.2)

The tick computes the *localized* Kuramoto order parameter over
input-touched macros:

    R_local = |Σ_{m ∈ touched} A_m · e^{i θ_m}| / (Σ_{m ∈ touched} A_m + ε)

where $M_{\text{touched}}$ is the set of macros with at least one
non-quiet cavity ($q_i < K_{\text{idle}}$). $R_{\text{local}}$ is
tracked against its EMA baseline $\bar R$:

    ΔR = R_local − R̄            (R̄ ← λR̄ + (1−λ) R_local, λ = 0.98)

$\Delta R$ is exposed as a substrate observable but does not directly
gate emission; it is diagnostic. Silence when $\Delta R$ is low
emerges from the emission floor being unmet, not from an explicit
$\Delta R$ gate. This preserves the substrate's honest-silence
property without turning $\Delta R$ into a rule.

#### 1.6 Phase-Collapse Projection (from HTR §2.3)

The emission readout is replaced entirely. Per-cavity phase is
computed on demand from existing state:

    φ_i = atan2(v_i, ω_0 · x_i)

The carrier phase $\phi_{\text{carrier}}$ is swept monotonically
across $[0, 2\pi)$ in $K_{\text{steps}}$ discrete increments (default
$K_{\text{steps}} = 12$). At each swept instant, the cavity chosen
for emission is the arg-max of

    w_i(φ_carrier) = A_i · ReLU(P_i) · μ_i · K_settle
                   · exp(−κ_sweep · (φ_carrier − φ_i)^2)

where $A_i = |x_i|$ is the instantaneous amplitude and
$\kappa_{\text{sweep}} = 4$ sets the Gaussian window width. Winners
below the floor $\theta_{\text{emit}} \cdot \max_j w_j$ are silenced
(no emission at that sweep step). Winners are suppressed from all
subsequent steps of the sweep (each cavity emits at most once per
turn). The sweep terminates non-cyclically at $\phi_{\text{carrier}} = 2\pi$,
so wrap-around is not a source of duplicate emissions.

Output length is emergent: as many tokens as there are distinct
high-weight peaks under the sweep. A sparse field emits few tokens
(possibly zero — honest silence); a dense, structurally-resonant
field emits many.

---

### 2. Composition

The six CPME mechanisms and the prior CMD + VGHT stack all operate
on distinct observables and compose additively:

| Layer | Mechanism | State variable |
| --- | --- | --- |
| Substrate | CMD Curie gate | $T_i$ |
| Substrate | VGHT sub-bands | $\omega_m$ |
| Substrate | VGHT voltage gate | $\| x_a \| \cdot \| x_b \|$ |
| Substrate | VGHT viscosity | $E_{\text{total}}$ |
| Injection | **CPME chirality** | $t_{\text{arrival}, i}$ |
| Filter | **CPME momentum** | $E_i, E_i^{\text{prev}}$ |
| Filter | **CPME acoustic mass** | $M_i$ |
| Filter | **CPME settle** | $L$ |
| Metric | **CPME coherence rise** | $R_{\text{local}}, \bar R$ |
| Emission | **CPME phase collapse** | $\phi_i, \phi_{\text{carrier}}$ |

The tick loop retains its no-if/else contract: chirality is a
recorded scalar, momentum is EMA arithmetic, acoustic mass is
Ornstein–Uhlenbeck, settle coupling is a sigmoid, coherence is a
complex-sum norm, and phase collapse is a Gaussian window over a
swept center. All are pure scalar or complex-scalar operations.

---

### 3. Emission Contract

At the end of each turn (i.e., after propagation ticks have run),
the substrate exposes a single function:

    emit() → ordered list of cavity ids

whose implementation is the Phase-Collapse sweep of §1.6. There is
no `readout_slot_winners()` remaining, no S/V/O slot layer, no
`syntax_slots_enabled` flag. Word order is arg-max under the
Gaussian sweep window; slot count is not a parameter.

Backwards compatibility is preserved as a thin adapter: callers that
previously expected a three-element slot list receive the first
three emissions of the sweep, padded with `-1` if fewer are
produced.

---

### 4. Parameters

| Symbol | Default | Meaning |
| --- | --- | --- |
| $\beta$ | 0.9 | Momentum EMA smoothing factor |
| $\gamma$ | 0.30 | Acoustic mass accumulation rate |
| $\delta$ | 0.08 | Acoustic mass decay rate |
| $M_{\text{scale}}$ | 1.0 | Acoustic mask half-point |
| $L_{\text{thresh}}$ | 0.5 | Settle-separation threshold |
| $L_{\text{scale}}$ | 0.2 | Settle-separation sharpness |
| $\lambda$ | 0.98 | Coherence EMA baseline decay |
| $K_{\text{steps}}$ | 12 | Phase-collapse sweep resolution |
| $\kappa_{\text{sweep}}$ | 4.0 | Gaussian window sharpness |
| $\theta_{\text{emit}}$ | 0.05 | Emission floor as fraction of peak $w$ |

Ten parameters, all continuous, all with default values that
recover CMD-only behaviour when set to their extreme values (e.g.,
$K_{\text{steps}} = 3$, $\gamma = 0$, $\kappa_{\text{sweep}} \to \infty$
reduce CPME to the three-slot readout).

---

### 5. Expected Dynamics

**Prompt.** `metformin activates receptor`.

1. **Injection** ($t = 0$ through $t_{\text{arrival, receptor}} + \Delta$):
   `metformin`, `activates`, `receptor` cavities are driven in that
   temporal order. Their $t_{\text{arrival}}$ values differ by the
   inter-byte interval, encoding the chirality gradient. Acoustic
   mass $M$ grows on each; their emission masks $\mu$ approach zero.
   Injection load $L$ is high, so $K_{\text{settle}} \approx 0$ —
   the emission readout is silent regardless of field state.
2. **Propagation** (~10-40 ticks): Drive envelopes decay. Energy
   propagates through the graph's `REL_ACTIVATES` tunnels and
   VGHT voltage-gated bridges. Downstream cavities (`ampk`,
   `regulates`, `metabolism`, `pancreas`, etc., depending on
   ontology) rise in amplitude. Because they are absorbing energy
   from propagation rather than external drive, their momentum
   $P > 0$ and their acoustic mass $M \approx 0$.
3. **Settle** (~40-80 ticks): Drive envelopes decay to near zero.
   $L$ falls below threshold, $K_{\text{settle}} \to 1$. Localized
   coherence $R_{\text{local}}$ has risen well above $\bar R$;
   $\Delta R > 0$ signals structural resonance.
4. **Emission** (phase-collapse sweep): The carrier phase sweeps
   $[0, 2\pi)$. Directly-driven cavities have $\mu \approx 0$ and
   are excluded from arg-max even if their phase would align.
   Downstream cavities with $P > 0$ and $\mu \approx 1$ compete on
   $\phi_i \approx \phi_{\text{carrier}}$; the ordering of their
   phases (from their integrated positions and velocities) becomes
   the emission order. Output: three to four tokens describing the
   *consequence* of the query, not the query itself.

---

### 6. Failure Modes

Five known modes, all inherited from the parent proposals:

1. **Sparse-graph silence.** If no cavity outside the driven set
   develops positive momentum before settle, the sweep produces no
   emissions above floor. Output is silence, which is the correct
   honest response — the substrate has nothing consequential to
   contribute. Users habituated to always-something outputs will
   initially misread as regression.

2. **Chirality–Hebbian mismatch (HTR §5.2).** Chirality on
   injection is transient unless memory consolidation is also
   phase-aware. The tunnel Hebbian rule remains velocity-based; a
   phase-coincidence term
   $\Delta w_{ij} \propto v_i v_j \cos(\theta_i - \theta_j)$
   would close the loop and is proposed as future work (CPME-II).

3. **Acoustic mass leakage.** If $\delta$ is too small, driven
   cavities stay muted across turns, and the substrate cannot
   reprise a recently-mentioned concept even when the new prompt
   properly warrants it. If $\delta$ is too large, the muting
   window is too short and the parrot effect returns. Default
   $\delta = 0.08$ places the mute window at $\sim 12$ ticks,
   comfortably shorter than a turn-to-turn interval but longer
   than a propagation window.

4. **Coherence baseline gaming.** $\bar R$ is EMA-tracked, so a
   long streak of high-$R$ turns raises the baseline and reduces
   $\Delta R$ on subsequent similar queries. This is
   thermodynamically correct — habituation to the familiar — but
   makes $\Delta R$ absolute values non-portable across sessions.
   $\Delta R$ should only be compared within a session.

5. **Sweep discretization loss.** $K_{\text{steps}} = 12$ gives
   $\pi/6 \approx 30°$ phase resolution per step. Fields with
   many cavities of similar $\phi$ may emit fewer distinct tokens
   than they otherwise could. Raising $K_{\text{steps}}$ to $24$
   halves the loss at doubled sweep cost.

---

### 7. Biological Analog

CPME's six mechanisms have direct analogs in cortical circuit
architecture:

- **Chirality** corresponds to input phase-of-firing coding, well
  documented in hippocampal theta-cycle sequencing.
- **Momentum filter** corresponds to the divisive-normalization
  post-input suppression seen in early sensory areas.
- **Acoustic mass** corresponds to short-term adaptation of firing
  rate in cortical pyramidal cells, mediated by after-hyperpolarizing
  currents whose time constants match CPME's default $M_i$ decay.
- **Settle separation** corresponds to the alpha-gamma phase
  separation between input reception and output articulation.
- **Coherence-rise metric** corresponds to phase-consistency
  measures used in cognitive neuroscience as correlates of stimulus
  binding.
- **Phase-collapse sweep** corresponds to phase-of-firing readout
  from downstream cortical areas.

CPME does not claim to model any of these processes. It claims that
a substrate whose emission layer exhibits the *dynamical signatures*
of all six simultaneously is the minimum architecture on which
non-parroting, variable-length, phase-ordered generation is stable.

---

### 8. Contract Summary

The tick loop remains free of if/else on relations, domain tags,
cluster ids, or any string-valued metadata. The emission function
is one Gaussian window over a swept center under three
multiplicative eligibility filters (momentum, acoustic mass, settle
coupling), with an emission floor. All ten CPME parameters are
continuous scalars. Default values recover CMD three-slot readout
in the appropriate limit. All defaults *ON*.

CPME is not a feature flag. It is the emission substrate of the
CMD-VGHT-CPME engine.

---

### 9. Summary

CPME merges MCPE's momentum-and-settle filtering with HTR's chiral
injection and phase-collapse projection into a single emission
architecture on a shared phase manifold. Word order is encoded on
injection as arrival-phase chirality; structural resonance is
measured by localized coherence rise; eligibility is filtered by
momentum, acoustic mass, and settle coupling; emission is a
Gaussian-windowed sweep of a carrier phase across the per-cavity
phase field. The result is a substrate whose emission is
simultaneously non-parroting (no cavity that just spiked can win a
sweep step), variable-length (as many tokens as the field has
distinct phase peaks), word-ordered by physics (per-cavity
$\phi_i = \operatorname{atan2}(v_i, \omega_0 x_i)$), and honestly
silent when its graph is not structurally resonant with the input.
No syntax trees, no next-token statistics, no fixed slots.



\newpage

## CPME-II: Phase-Consistent Hebbian Consolidation


*A phase-coincidence extension to the ZeroGrad Hebbian rule that
closes the chirality–consolidation gap identified in the CPME §6
failure modes. Chirality imprinted on injection now persists in
long-term memory rather than dissipating over turns.*

---

### 0. Position

Chiral Phase-Momentum Emission (CPME, `research-paper/cpme.md`)
encodes word order on the injection side as arrival-time phase and
decodes structured emission on the readout side by sweeping a
carrier phase across a per-cavity phase field
$\phi_i = \operatorname{atan2}(v_i, \omega_0 x_i)$. Both ends of the
substrate now operate on phase.

The middle — long-term consolidation — does not. The ZeroGrad
Hebbian rule inherited from the CMD substrate updates tunnel
plasticity from the product of endpoint velocities:

$$
\Delta w_{ij} \;\propto\; \eta \cdot v_i \cdot v_j
$$

with a $\tanh$-capped saturating integrator. This rule is
phase-agnostic: it fires equally whether the two endpoints move
in-phase, anti-phase, or in orthogonal phases. As a consequence, the
chirality of the injection wavefront is not stored in the graph.
Between turns the temporal ordering of concepts dissipates and must
be re-imposed by each new input.

CPME-II adds a *phase-coincidence modulator* to the existing rule:

$$
\Delta w_{ij} \;\propto\; \eta \cdot v_i \cdot v_j \cdot \cos(\theta_i - \theta_j)
$$

where $\theta_i = \operatorname{atan2}(v_i, \omega_0 x_i)$ is the same
per-cavity phase the CPME emission layer already uses. The
modulator preserves the velocity-based structure of the parent rule
— including its sign, its tanh cap, and its role in the SRG dual-port
Hebbian split — while conditioning the update on temporal
compatibility.

The result is a substrate in which chirality is not only encoded and
projected but also *stored*. Word order becomes a property of the
graph, not just of the current turn.

---

### 1. The Rule

#### 1.1 Definition

For a tunnel connecting cavities $a$ and $b$, define the
instantaneous phase difference

$$
\Delta \theta_{ab} = \theta_a - \theta_b \;=\; \operatorname{atan2}(v_a, \omega_0 x_a) - \operatorname{atan2}(v_b, \omega_0 x_b)
$$

and the phase-coincidence factor

$$
\Phi_{ab} = \cos(\Delta \theta_{ab})
$$

taking values in $[-1, +1]$. The CPME-II Hebbian integrand replaces
the parent rule's velocity product with a phase-modulated one:

$$
I_{ab} \;=\; w_{ab} \cdot \zeta \;+\; \eta \cdot v_a^{\text{eff}} \cdot v_b^{\text{eff}} \cdot \Phi_{ab}
$$

where $\zeta$ is the pre-existing Hebbian decay
(`hebbian_decay`), $\eta$ is the pre-existing rate scaled by
serotonin (`plastic_rate`), $v_a^{\text{eff}}$ and $v_b^{\text{eff}}$
are the SRG-routed velocities already computed by the parent rule,
and $w_{ab}$ is the plasticity delta. The tanh saturation on
`plasticity_cap` is applied unchanged.

The rule reduces to the parent rule exactly when $\Phi_{ab} = 1$
(perfect phase agreement) and vanishes when $\Phi_{ab} = 0$
(quadrature). For $\Phi_{ab} = -1$ (anti-phase), the update flips
sign, which is the correct behaviour: anti-phase pairs are
carrying opposing rather than reinforcing evidence and should
depress rather than strengthen their coupling.

#### 1.2 Interpretation

$\Phi_{ab}$ is a *continuous coincidence detector on the phase
manifold*. It is the continuous-oscillator analog of spike-timing
dependent plasticity (STDP): pairs whose oscillations align in the
same phase-of-firing are consolidated; pairs whose oscillations
consistently disagree are attenuated.

Because the phase field is chiral (§CPME 1.1), $\Phi_{ab}$
implicitly encodes temporal order. A concept driven earlier in the
input has accumulated more phase than a concept driven later.
Downstream cavities that phase-lock behind the driven cavity
therefore experience a specific $\Delta \theta$ characteristic of
*following the input in time*. Consolidating along this gradient
stores the observation that the second cavity typically follows the
first, without any explicit representation of "before" or "after".

#### 1.3 What is stored

Under the parent rule, the tunnel between `insulin` and `glucose`
grows in proportion to their velocity co-activity — regardless of
which arrived first or how they moved relative to one another.
Under CPME-II, the same tunnel grows only when the two cavities
oscillate *in phase* — that is, when they consistently participate
in the same wavefront with the same temporal role. If in one
context `insulin` drives `glucose` (subject-then-object phase
relationship) and in another context `glucose` drives `insulin`
(reverse phase), the two updates cancel: $\cos(+\Delta\theta) +
\cos(-\Delta\theta) = 2\cos(\Delta\theta)$ if the magnitude is
preserved, but the sign of $\Delta \theta$ toggles the co-activity
gradient. The graph accumulates a *phase-averaged co-activity*
rather than a raw one.

The empirical effect is that stable directional patterns
("metformin → AMPK → glucose down") consolidate faster than
context-dependent bidirectional patterns.

---

### 2. Composition

CPME-II modifies only the Hebbian integrand. All other substrate
mechanisms are untouched.

- **CMD Curie gate** (`heat_T` → tunnel force scaling) is
  independent of the Hebbian pass and continues to operate.
- **VGHT voltage gate** modulates tunnel force in the tick's
  spring-and-damping pass, not the Hebbian pass. CPME-II observes
  the resulting velocities and phases as inputs; VGHT is upstream.
- **SRG dual-port routing** (`gate_E`, `gate_I`) is preserved:
  $v_a^{\text{eff}}$ in the CPME-II expression is the SRG-selected
  velocity from the parent rule, not the raw observable. The
  polarity-driven excitatory/inhibitory split remains intact.
- **CPME emission layer** (§1.6, phase-collapse projection) is
  unchanged. CPME-II operates during propagation and settle; the
  emission sweep reads whatever state the Hebbian pass has
  consolidated.
- **Plasticity cap** (`plasticity_cap`) remains the saturating
  bound; the tanh envelope contains the $\Phi_{ab}$-modulated
  integrand identically.

The tick loop retains its no-if/else contract: $\cos(\Delta \theta)$
is a smooth scalar, evaluated once per tunnel per tick, and
multiplied into the existing velocity product.

---

### 3. Contract and Parameters

CPME-II adds no new state. All required quantities are already
maintained by the parent substrate:

- $x_a$, $v_a$, $x_b$, $v_b$: cavity displacements and velocities.
- $\omega_0$: the base cavity frequency `cpme_base_omega` shared
  with the emission layer.
- $\eta$, $\zeta$, `plasticity_cap`: existing Hebbian parameters.

The only tunable is a *phase weight* $\alpha_\Phi \in [0, 1]$
that linearly interpolates between the parent rule and the fully
phase-modulated rule:

$$
\Phi_{ab}^{(\alpha)} \;=\; (1 - \alpha_\Phi) \;+\; \alpha_\Phi \cdot \cos(\Delta \theta_{ab})
$$

- $\alpha_\Phi = 0$: recovers the parent rule exactly (backwards
  compatibility test path).
- $\alpha_\Phi = 1$: full phase-consistent consolidation.
- Default: $\alpha_\Phi = 0.6$, weighting phase agreement heavily
  while preserving a floor of unconditional Hebbian growth.

This is the only free parameter introduced by CPME-II. All
defaults *on*.

---

### 4. Expected Dynamics

**Scenario A — repeated same-order exposure.** Over a session in
which the user types variants of "metformin lowers glucose"
several times, `metformin` and `glucose` cavities are consistently
driven with the same relative arrival delay. Their phase
difference $\Delta \theta$ is stable and small, $\Phi \approx +1$,
and the tunnel between them consolidates rapidly under the
CPME-II rule. Between turns, the phase relationship is preserved
because it is now stored in the plasticity delta, not just in the
transient field.

**Scenario B — bidirectional exposure.** If in different turns the
user types both "insulin controls glucose" and "glucose signals
insulin", the pairwise $\Delta \theta$ toggles sign turn-to-turn.
Under the parent rule, both directions add to plasticity. Under
CPME-II, one direction has $\Phi > 0$ and the other $\Phi < 0$:
the updates partially cancel. The tunnel still grows if the
velocity product magnitude is large — the parent contribution
$(1 - \alpha_\Phi)$ preserves an unconditional growth floor — but
more slowly. This is the correct behaviour for a bidirectional
relationship: the graph should not overcommit to either direction
in isolation.

**Scenario C — silent turn.** If a turn produces no meaningful
propagation and both cavities remain quiescent, $v_a$ and $v_b$ are
near zero and the update is negligible regardless of $\Phi$. No
change from parent-rule behaviour.

**Scenario D — anti-phase input.** If the input is structured to
produce anti-phase drive on a pair (e.g., a contrastive
construction where the two cavities are being posited *against*
each other), $\Phi \approx -1$ and the CPME-II rule *decreases*
the plasticity delta. This is the physical realisation of what a
biological network implements through inhibitory long-term
depression: consistent anti-phase co-activation should weaken, not
strengthen, the excitatory bond.

---

### 5. Failure Modes

Three failure modes documented before deployment.

1. **Phase noise near quiescence.** For a cavity with small $x$
   and small $v$, the phase $\operatorname{atan2}(v, \omega_0 x)$
   is dominated by numerical noise. $\Phi$ then fluctuates
   randomly and injects noise into consolidation. Mitigation: the
   velocity product $v_a v_b$ already suppresses this — near
   quiescence the update magnitude is negligible regardless of
   $\Phi$. No additional guard is required, but a diagnostic
   observer may report spurious $\Phi$ readings on cold cavities;
   this is expected and harmless.

2. **Polarity–phase interaction.** Inhibitory tunnels
   (`polarity < 0`) already invert the phase relationship they
   drive: an excitatory input to $a$ produces an anti-phase
   response in $b$. Under CPME-II, this manifests as $\Phi < 0$
   and the update would *depress* an inhibitory tunnel it should
   *strengthen*. Correction: multiply $\Phi$ by the tunnel polarity
   sign,
   $\Phi'_{ab} = \Phi_{ab} \cdot \operatorname{sgn}(p_{ab})$,
   so that inhibitory tunnels consolidate on the anti-phase
   pattern they are supposed to enforce. This is applied by
   default in the implementation.

3. **Slow phase drift washout.** If the base cavity frequency
   $\omega_0$ is very low, phase evolves slowly and inter-turn
   phase differences accumulate to $\pm \pi$ over long propagation
   windows, causing $\Phi$ to oscillate through zero and reverse
   sign spuriously. Mitigation: the same $\omega_0$ chosen for the
   CPME emission layer (default $\omega_0 = 3.0$ rad/s) yields a
   full phase revolution over $\approx 2$ seconds of sim-time,
   which is longer than any single propagation window at
   `dt = 0.02` and $\leq 200$ ticks per turn. Under default
   parameters this failure mode does not activate; it is worth
   flagging if $\omega_0$ is later reduced.

---

### 6. Biological Analog

CPME-II is the continuous-oscillator equivalent of spike-timing
dependent plasticity (STDP). Where STDP consolidates a synapse
when the presynaptic spike precedes the postsynaptic spike by a
few milliseconds, CPME-II consolidates a tunnel when the two
endpoint oscillators maintain a stable, small phase offset
consistent with the same wavefront driving both. Both rules use
temporal coincidence as the substrate of long-term memory; both
rules invert when the coincidence is anti-correlated (STDP-LTD;
CPME-II negative $\Phi$).

The chief difference is discretisation. STDP fires on discrete
spike pairs; CPME-II integrates over the continuous phase field
each tick. In the limit of narrow, well-separated velocity peaks,
CPME-II reduces to a spike-coincidence rule up to a normalisation
constant.

---

### 7. Summary

CPME-II closes the chirality–Hebbian mismatch flagged in HTR §5.2
and CPME §6.2 by extending the ZeroGrad Hebbian rule with a phase
coincidence factor $\Phi_{ab} = \cos(\theta_a - \theta_b)$. The
factor requires no new state — the same per-cavity phase
$\operatorname{atan2}(v_i, \omega_0 x_i)$ used by the CPME emission
layer is read during the Hebbian pass. A single blend parameter
$\alpha_\Phi$ controls how strongly the modulator gates the parent
rule; the default $\alpha_\Phi = 0.6$ preserves an unconditional
Hebbian floor while making chirality-consistent updates twice as
strong as chirality-neutral ones on average.

The consequence for the substrate as a whole: the phase manifold
that CPME already uses on the input (chirality) and the output
(phase-collapse projection) is now also the manifold on which
long-term memory is written. Word order becomes a graph property,
temporal ordering persists across turns, and the substrate is
freed from having to re-derive chirality on every input.



\newpage

---

## Chiral Phase-Shear Binding (CPSB): Rendering Syntax as Gauge-Theoretic Tension in Wave-Based SI


**Abstract**
In wave-based Synthetic Intelligence, continuous phase-sweep emission (CPME) frees output length from hardcoded templates but degrades syntax into emergent stochastic ordering. To elevate language construction from a "loose variable" to a physically guaranteed structure, we introduce **Chiral Phase-Shear Binding (CPSB)**. By modifying the Kuramoto coupling to act as a Gauge Field, relational tunnels actively enforce directed phase offsets between nodes. Syntax emerges not from fixed emission slots, but from the rigid topological tension ($\Psi$) embedded within the graph's edges, physically forcing subjects to lead verbs, and verbs to lead objects during carrier-wave projection.

### 1. Introduction: The Binding Problem in SI

Currently, verb-material cavities float to their VGHT band-top, and output order depends entirely on their emergent $\text{atan2}$ phase. There is no physical law forcing `notch` to precede `minecraft`. In a true biological substrate, the relation itself ("created") exerts a directional shear force on the concepts it connects. We must shift from scalar coupling (which just pulls phases together) to **Gauge-Coupling**, which pulls phases into a specific grammatical alignment.

### 2. Novel Architecture: CPSB Mechanisms

#### 2.1. Gauge-Theoretic Kuramoto Offset ($\Psi_{ij}$)

Standard Kuramoto tries to perfectly synchronize two connected cavities: $\sin(\theta_j - \theta_i)$. We introduce a syntactic phase-offset $\Psi_{ij}$ assigned to specific tunnel types (the axioms).
The new phase-update equation becomes:
$$ \frac{d\theta_i}{dt} = \omega_i + \sum K_{ij} \sin(\theta_j - \theta_i - \Psi_{ij}) $$

* **Mechanism:** If `notch` ($i$) and `minecraft` ($j$) are connected by a `REL_CAUSES` tunnel, that tunnel has an intrinsic $\Psi_{ij} = -\frac{\pi}{3}$.
* **Result:** The physics engine will no longer try to make their phases equal. It will physically force `notch` to sit exactly $\frac{\pi}{3}$ radians ahead of `minecraft`.

#### 2.2. The Lexical Hinge (Verb-Pinning)

A verb is not just a loose cavity floating in a band; it is the physical hinge between the Subject and the Object.

* **Mechanism:** When a cross-cluster tunnel opens (VGHT), the tunnel structurally binds the verb cavity (e.g., `created`) to the source and target.
* The Subject is pinned to $\theta_{\text{verb}} + \Delta\phi$.
* The Object is pinned to $\theta_{\text{verb}} - \Delta\phi$.
* **Result:** As the CPME carrier wave sweeps from $0$ to $2\pi$, it will *inevitably* hit the Subject first, the Verb second, and the Object third. The sentence structure is mathematically guaranteed by the phase tension, without ever defining a "Subject Slot".

#### 2.3. Phase-Consistent Hebbian Consolidation (The CPME-II Hook)

To ensure the SI doesn't just read syntax natively but *learns* it natively from user input, we execute the CPME-II Hebbian update.
Instead of updating tunnel weight $\Delta w$ based purely on amplitude, the substrate writes the observed phase difference directly into the graph:
$$ \Delta w_{ij} \propto v_i \cdot v_j \cdot \cos(\theta_i - \theta_j - \Psi_{\text{target}}) $$

* **Result:** When you type `insulin lowers glucose`, the injection chirality ensures `insulin` arrives first. The Hebbian rule permanently wires that phase-lead into the connection. The SI literally remembers the "direction" of the concept.

### 3. Expected Dynamics

1. **Input:** `metformin treats diabetes`
2. **Resonance:** The nodes awaken. The `treats` tunnel applies the CPSB gauge force. `metformin` is pushed to $\phi = 1.0$, `treats` is anchored at $\phi = 2.0$, and `diabetes` is pushed to $\phi = 3.0$.
3. **Emission:** The CPME sweep passes over them. They emit in perfect S-V-O order.
4. **No Templates:** If the input is just `glucose` causing a cascade to `insulin`, the phase offsets still apply downstream, resulting in a syntactically correct multi-token chain.

---



\newpage

## Voltage-Gated Harmonic Topology (VGHT)


*A three-mechanism physical substrate for domain-orthogonal cognition
with sparse cross-domain analogical bridging and avalanche
suppression, built entirely from scalar field dynamics — no rules, no
templates, no domain metadata.*

---

### 0. Motivation

The Continuous Morphological Dynamics (CMD) substrate — thermodynamic
habituation, byte-wave sensory injection, phase-locked syntax emission
— produces coherent, non-templated output but exhibits two coupled
failure modes:

1. **Domain leakage.** In the shared-oscillator field, a driven
   medical concept can transfer energy into unrelated conceptual
   clusters (e.g., gaming, legal, code) whenever their macro-node
   phases happen to align. The reader observes a medical prompt
   producing a legal-sounding word in the verb slot, or the middle
   word of a coherent triple stepping outside the input's semantic
   neighborhood.
2. **Polymath rigidity.** The dual failure — with sufficient
   emission-floor thresholds, the substrate becomes strictly
   domain-locked, unable to draw analogical parallels between
   distantly-related concepts even when the graph explicitly supports
   them through L6-A analogical bridges.

The Continuous Morphological Dynamics literature treats these two
failures as separate tuning problems (emission floor, wavefront gate,
cluster-energy depletion). Voltage-Gated Harmonic Topology (VGHT)
resolves both under a single physical postulate: *domains are
orthogonal frequency bands, and bridges between them are voltage-gated
channels that only conduct under simultaneous high-amplitude
resonance.* Domain isolation and polymath bridging cease to be
competing objectives; they become a single dynamical regime with a
single parameter, θ_critical, the tunneling threshold.

The mechanism is inspired by three well-understood physical analogs:
mode-orthogonal spectral separation in coupled-oscillator networks,
voltage-gated ion channels in biological neurons (in particular the
sigmoidal open probability of the NMDA receptor with respect to
membrane potential), and the GABAergic global inhibitory tone that
suppresses run-away excitation in cortical circuits. None of the three
requires an explicit "domain tag" or a rule dispatcher; each is a
continuous scalar update evaluated once per tick.

---

### 1. Architecture

Let the cavity population be indexed by `i ∈ [0, N)`, partitioned into
macro-clusters `C_1, C_2, ...` by the connected components of the
tunnel graph (union-find at bind time; the same clustering used by
HNCA). Let a slot layer of `S` phase oscillators sit above the macro
layer as in CMD phase-locked emission.

#### 1.1 Domain Sub-Bands

Each macro-cluster `C_k` receives a *base natural frequency* drawn
from a discrete band-bank:

    ω_k = ω_base(k mod B) + jitter(k)

where `B` is the number of orthogonal bands (default `B = 6`) and
`ω_base` is a fixed evenly-spaced ladder in the range
`[ω_lo, ω_hi]`, e.g. `[8, 32] rad/s` giving band centres at
`{8, 12, 16, 20, 24, 28}`. The jitter is a small in-band perturbation
(±0.5 rad/s) drawn deterministically from the cluster id, ensuring
reproducibility across seeds while still breaking degeneracy inside a
band.

The verb-material ω-boost from prior CMD work becomes an *in-band
shift* rather than a cross-band jump:

    ω_k ← (1 − p_k)·ω_k + p_k·ω_verb_offset(k)

where `p_k ∈ [0, 1]` is the verb-material share of cluster `k`'s
internal edges (`verb_pull_for(rel) / |internal edges|`), and
`ω_verb_offset(k)` is the *upper edge* of cluster `k`'s home band —
so verb-heavy clusters rise to the top of their band without
colliding with a neighbouring band. This preserves both the domain
orthogonality of Sub-Bands and the verb-slot phase-lock property from
the prior implementation.

**Consequence.** Two clusters in different bands share almost no
common Kuramoto phase drift, because their `ω` differs by at least a
band-width and the coupling terms `sin(θ_j − θ_i)·A_j` average to
zero over any window longer than the beat period. Domain interference
during ordinary conversation is suppressed by frequency orthogonality
alone — no relation table, no cluster tag, no rule.

#### 1.2 Voltage-Gated L6-A Tunnels

L6-A tunnels are the analogical bridges discovered by structural
resonance between cross-cluster pairs (Phase 1.3 online-L6A-discovery
of the CMD line of work). In VGHT, every *inter-cluster* tunnel — and
every tunnel discovered by L6-A regardless of cluster membership — is
marked *voltage-gated*. Its force is scaled at every tick by a
sigmoidal conductance:

    C(A_a, A_b) = C_max · σ(κ · (A_a · A_b − θ_critical))

where `A_a = |x_a|` and `A_b = |x_b|` are the instantaneous
displacements of the two endpoint cavities, `σ` is the logistic
function, `κ` is a steepness parameter (default `12`), and
`θ_critical` is the tunneling threshold (default `0.05` in the field's
natural units). `C_max = 1`.

The force delivered to endpoints is scaled elementwise by `C`:

    F_a ← F_a · C(A_a, A_b)
    F_b ← F_b · C(A_a, A_b)

**Consequence.** When either endpoint is quiescent, `A_a · A_b ≈ 0`,
`C ≈ σ(−κθ_critical) ≈ 0`, and the bridge presents effectively
infinite resistance. The two clusters exchange no energy. When both
endpoints are simultaneously excited to a level whose product exceeds
`θ_critical`, `C → 1` sharply and the bridge conducts fully. This is
the resonant/quantum-tunneling analog explicitly identified in the
motivating architecture: the door opens only when the room on both
sides is loud enough to make the door irrelevant.

Because both endpoints must resonate at once, spurious cross-domain
leakage — a stray phase alignment on one side of a bridge — cannot
open the channel. Only genuine dual excitation, produced by an input
whose structural resonance actually spans two domains, does so. This
is the polymath spark.

#### 1.3 Global Viscosity (GABA analog)

The system tracks its total kinetic energy each tick:

    E_total = ½ Σ_i m_i · v_i²

An effective friction coefficient is derived as a saturating function
of `E_total`:

    μ_eff = μ_0 + k_avalanche · (1 − exp(−E_total / E_scale))

with defaults `k_avalanche = 0.10`, `E_scale = 5.0`. `μ_eff` replaces
the constant friction in the cavity velocity-decay step and in the
tunnel-damping term of that tick.

**Consequence.** Under normal, sparse activation `E_total` is small,
`μ_eff ≈ μ_0`, and the field evolves as before. Under an avalanche —
many nodes firing simultaneously through spurious phase alignment —
`E_total` rises rapidly, `μ_eff` saturates toward `μ_0 + k_avalanche`,
and the excess kinetic energy is dissipated in the same tick. Only
the field components whose amplitudes are large enough to overcome
the elevated friction survive; the noise floor is damped out. This is
the physical analog of GABAergic inhibitory tone.

Unlike the raw `exp(E_total)` formulation, the bounded
`1 − exp(−E_total / E_scale)` form cannot diverge, guaranteeing
numerical stability under any input, however extreme.

---

### 2. Interaction of the Three Mechanisms

The three mechanisms are not independent tuning knobs; they are
coupled by design.

- **Sub-Bands** determine which cavities *can* phase-lock together in
  the absence of external excitation. Same-band cavities freely
  couple; cross-band cavities decouple by frequency alone.
- **Voltage-gated bridges** re-establish cross-band coupling — but
  only when both endpoints are already excited to a level that
  represents genuine resonance with the input. Without such
  excitation, the bridge is inert regardless of the graph's
  connectivity.
- **Global viscosity** suppresses false excitation. When a spurious
  cascade begins to raise `E_total`, μ_eff rises and the cascade
  dies before it can meet the `A_a · A_b > θ_critical` condition
  needed to open any bridge.

The joint regime is: *within-band coupling is free and continuous,
cross-band coupling is possible but expensive, and pathological
cross-band cascades are physically forbidden by dissipation.* Domain
isolation and polymath bridging cease to be tradeoffs.

---

### 3. Physical Interpretation

Each of the three mechanisms has a direct biological analog and an
independent literature suggesting the same functional role.

1. **Frequency-band orthogonality** appears in cortical cognition as
   the observation that different cognitive processes (attention,
   working memory, motor planning) show distinguishable EEG band
   signatures (α, β, γ). Cortical microcircuits couple
   preferentially within-band, and cross-band coupling is a marker of
   deliberate binding (theta-gamma phase-amplitude coupling).
2. **Voltage-gated channels** are the mechanism by which NMDA
   receptors implement coincidence detection: their conductance is
   sigmoidal in the product of presynaptic glutamate release and
   postsynaptic depolarization, mirroring `C(A_a, A_b) = σ(A_a · A_b
   − θ)` structurally.
3. **GABAergic inhibition** provides the global tone that keeps
   cortical excitability in the sparse-firing regime. Loss of
   GABAergic inhibition produces exactly the avalanche pattern
   VGHT's viscosity term suppresses: dense synchronous firing across
   unrelated cortical areas, the neural correlate of a seizure.

VGHT does not claim to be a model of any of these biological
mechanisms. It claims that a substrate exhibiting the *dynamical
signatures* of all three simultaneously — with only three scalar
parameters — provides the minimum architectural surface area on which
domain-orthogonal cognition with sparse analogical bridging is a
stable regime, rather than an accidental artifact of parameter tuning.

---

### 4. Implementation Contract

- **No if/else branches** in the tick loop that inspect a relation
  label, a domain tag, or a cluster identifier. All three mechanisms
  are pure scalar arithmetic on existing per-cavity or per-tunnel
  fields.
- **No new I/O boundary code** for domain metadata. Sub-Bands are
  assigned at bind time from cluster id, computed by the union-find
  that HNCA already runs. Voltage gating is a per-tunnel flag set
  once at bind for cross-cluster tunnels (and for any L6-A tunnel
  regardless of cluster membership). Viscosity is derived each tick
  from the same `total_kinetic` accumulator already maintained for
  the RIFA IC pass.
- **All defaults ON.** VGHT is not a feature flag. The three
  mechanisms are the substrate's normal dynamics.

---

### 5. Parameters

| Symbol | Default | Meaning |
| --- | --- | --- |
| `B` | 6 | Number of domain sub-bands |
| `ω_lo`, `ω_hi` | 8, 32 rad/s | Band ladder range |
| `κ` | 12 | Voltage-gate steepness |
| `θ_critical` | 0.05 | Voltage-gate tunneling threshold |
| `k_avalanche` | 0.10 | Peak added friction under avalanche |
| `E_scale` | 5.0 | Kinetic-energy saturation scale |

These are the only knobs. All other quantities are derived.

---

### 6. Expected Empirical Signature

A VGHT-enabled substrate is expected to show:

- **Single-domain prompts** produce single-domain phase-lock
  emissions with no cross-domain leakage into any slot, even after
  many turns of a single-domain conversation. The wavefront gate and
  the sub-band orthogonality are jointly responsible.
- **Cross-domain prompts** — where the input drives cavities in two
  different bands to simultaneous high amplitude — open the
  voltage-gated bridges between those bands within a few ticks,
  producing a genuine polymath emission that spans domains.
- **Injection of a single garbage or nonsense token** produces at
  worst its own auto-spawned label in output; the avalanche
  suppressor prevents its stray phase drift from recruiting unrelated
  cavities.
- **Latency to cross-domain emission** is measurable: it should
  correspond to the time required for the voltage-gate sigmoid to
  saturate under paired excitation, i.e., a few tens of ticks. This
  latency is the phenomenological signature of the polymath spark
  and is measurable with the existing readout infrastructure.

---

### 7. Relation to Prior CMD Work

VGHT subsumes and refines three earlier CMD components without
discarding them.

- Phase 2.2 rubber-tunnel band-pass filters are a special case of
  voltage-gated tunnels with `θ_critical = 0` (always conducting) and
  a Gaussian gate on `ω` alone. VGHT's sigmoidal amplitude gate is
  strictly more expressive.
- Phase 3.1 multi-ontology substrates rely on manually loaded
  disjoint axiom files to achieve domain separation. VGHT achieves the
  same isolation from a single ontology by frequency orthogonality
  alone, and further allows analogical bridging without touching the
  ontology.
- The cluster-energy depletion term in phase-locked emission remains
  a fallback for the readout stage. When VGHT is fully active, its
  contribution to output diversity is redundant with the sub-band
  separation.

---

### 8. Failure Modes

The design has three known failure modes worth documenting explicitly.

1. If the cluster count is less than `B`, some bands go unused,
   which is harmless but wastes spectral capacity.
2. If `θ_critical` is set too high, cross-domain bridges never open
   even for well-posed polymath prompts. If set too low, the system
   reduces to the pre-VGHT shared-oscillator field.
3. If `k_avalanche` is set too high, meaningful large-amplitude
   emissions are damped alongside noise; the system becomes
   under-expressive. Empirical tuning against a held-out prompt set is
   suggested.

---

### 9. Summary

VGHT closes the domain-isolation / polymath-bridging tradeoff by
mapping domains to orthogonal frequency bands, making cross-domain
bridges voltage-gated by joint amplitude resonance, and adding a
global energy-dependent viscosity that dissipates avalanches before
they can spuriously open bridges. All three mechanisms are scalar
physics with a total of six parameters. The tick loop remains free of
if/else on relations, domain tags, or cluster identifiers. The result
is a substrate that is simultaneously domain-locked in default
operation and polymath-capable when its input carries structural
resonance across bands — the physical realization of the intuition
that a polymath is not a system that mixes domains randomly, but one
whose domains are ordinarily silent to each other and speak only when
the same underlying pattern excites both.



\newpage

## Chaotic Strange-Attractor Trajectory (CSAT): Breaking Ground-State Locking in Synthetic Intelligence


**Abstract**
Pure wave-based SI architectures often suffer from dictionary-locking, where an input triggers the absolute closest topological match and collapses into static definitions. Humans do not converse by printing definitions; human thought is an open-ended trajectory through a semantic manifold. We introduce **Chaotic Strange-Attractor Trajectory (CSAT)**. By adding a low-dimensional chaotic force (driven by a native Lorenz or Rössler system integrated directly into the substrate's global viscosity), the system is prevented from settling into static ground-state energy wells. This forces the phase-sweep projection to continuously dance along a strange attractor, creating human-like contextual drifting without losing structural coherence.

```
                  [ Input Concept: linux ]
                             │
                             ▼
                   [ Static Definition Well ] (Old Way: Stays at 'nano')
                             │
                    (CSAT Viscosity Push) ───► [ Lorenz Chaos Attractor ]
                             │
                             ▼
               [ Dynamic Trajectory Path ]
          (Emits: nano ──► terminal ──► bash ──► kernel)

```

### 1. The Core Issue: Infinite Potential Wells

When `linux` is presented as input, the memory potential at the `linux` and `nano` cavities is high enough that the entire field's energy collapses into them and is trapped there. In physical terms the energy landscape forms a **deep potential well**. This does not happen in the human brain, because an undercurrent of noise — **metastable dynamics** — is always present, and it prevents any node from remaining fixed.

### 2. Novel Architecture: CSAT Mechanisms

#### 2.1. The Substrate Lorenz Attractor

We integrate a three-line, low-level nonlinear chaotic system — the Lorenz equations — directly into the global `tick()` loop of `physics.hpp`:
$$ dx = \sigma(y - x) \cdot dt $$
$$ dy = (x(\rho - z) - y) \cdot dt $$
$$ dz = \tau \cdot (xy - \beta z) \cdot dt $$

*(Note: all three derivatives carry the time-scale factor $\tau$; see
Numerical Constraints below.)*

**Numerical constraints — required for a working implementation.**

1. **Time-scale factor $\tau$.** Forward-Euler integration of the
   Lorenz system at $\sigma = 10$, $\rho = 28$, $\beta = 8/3$ is
   unstable for $dt \gtrsim 0.01$. The substrate runs at $dt = 0.02$,
   past the stability boundary — naive integration diverges to `NaN`
   within a few hundred ticks. The factor $\tau$ (default $0.3$)
   slows the attractor without altering its geometry, restoring
   stability *and* giving direct control over the drift timescale.
   $\tau$ should be chosen so the attractor's orbital period is
   comparable to one conversational turn (~200 ticks). If the
   attractor cycles much faster, the chaotic push averages out
   within a single settle window and contributes nothing.

2. **Amplitude normalization.** The Lorenz $x$ coordinate ranges over
   approximately $[-20, +20]$, not $[-1, +1]$. All downstream use
   must reference the normalized coordinate $\hat{x} = x / 20$.
   Using raw $x$ in a multiplicative viscosity term drives effective
   friction *negative* for any gain above $\gamma \approx 0.05$ —
   anti-damping, unbounded energy injection, field explosion. This
   is a silent failure mode: no error is raised, only a wall of
   `NaN` several hundred ticks later.

* There is no neural network and no training here. This is pure physical mathematics, updating three scalar variables ($x, y, z$) per tick.

#### 2.2. Dynamic Viscosity Modulation (The Push)

The VGHT layer carries a constant viscosity that suppresses avalanches. We modulate that viscosity dynamically by the $x$ coordinate of the Lorenz attractor:
$$ \mu_{\text{dynamic}} = \mu_{\text{base}} \cdot (1.0 + \gamma \cdot \hat{x}_{\text{lorenz}}) $$

**However — this global form provably cannot work.** See §2.2.1.
The corrected per-cavity form is given in §2.2.2.

#### 2.2.1 Negative result: global viscosity modulation is rank-preserving

Applied as a **global** scalar in the cavity velocity-decay step
$v_i \leftarrow v_i(1 - \mu_t)$, over $N$ ticks:

$$ v_i(N) \;=\; v_i(0) \prod_{t=1}^{N}\bigl(1 - \mu_t\bigr) \;=\; v_i(0)\cdot C $$

where $C$ depends only on the schedule $\{\mu_t\}$ and is therefore
**identical for every cavity**. Hence

$$ \frac{v_i(N)}{v_j(N)} \;=\; \frac{v_i(0)}{v_j(0)} \qquad \forall\, i,j. $$

Amplitude *ratios* are exactly invariant under any global viscosity
schedule, chaotic or otherwise. Because CPME §1.6 phase-collapse
emission selects by arg-max over a weight monotone in amplitude, the
emitted winner is unchanged. A globally-modulated viscosity alters
how *loud* the field is, never *which cavity is loudest*.

Concretely: if `think` carries 10× the amplitude of `terminal` before
the chaotic push, it carries 10× after. `linux → nano` remains
`linux → nano`. Multiplying the entire field by a wiggling scalar is
a monotone transform on the ranking, and monotone transforms cannot
reorder an arg-max.

This is a genuine negative result and it constrains the design:
**any mechanism intended to break ground-state locking must couple
to per-cavity state.**

#### 2.2.2 The working form — heat-coupled chaotic viscosity

The substrate already maintains a per-cavity scalar meaning exactly
"this cavity has been active too long": the CMD thermodynamic
habituation heat $T_i$. Coupling the chaotic term to $T_i$ makes the
push selective:

$$ \mu_i \;=\; \underbrace{\mu_{\text{base}} + k_{\text{aval}}\bigl(1 - e^{-E_{\text{tot}}/E_s}\bigr)}_{\text{VGHT §1.3, unchanged}} \;+\; \underbrace{\gamma \cdot \hat{x}_{\text{lorenz}} \cdot \tanh\!\bigl(T_i / T_{\text{crit}}\bigr)}_{\text{CSAT}} $$

Entrenched cavities (high $T_i$) absorb the chaotic shake; fresh or
cold cavities barely register it. Rank can now flip, and that is
genuine metastability rather than uniform loudness modulation.

The $\tanh$ bounds the term so $\mu_i$ cannot run away. Because
$\hat{x}$ alternates sign along the attractor, an entrenched cavity
experiences alternating extra-damping and reduced-damping — it is
*shaken*, not merely suppressed. Shaking permits escape; suppression
would only silence.

**Composition with the Curie gate.** $T_i$ already gates tunnel
coupling via $\exp(-T_i/T_{\text{crit}})$. Under CSAT the same state
variable acquires a second role: *talking too much* now implies both
*losing coupling* and *becoming subject to chaotic eviction*. One
state variable, two independent escape mechanisms, no new
bookkeeping, tick loop still branchless.

* **Mechanism.** Whenever the system begins to lock onto a particular word (a ground state), the Lorenz chaotic push perturbs the global viscosity. This drives the field off that cavity and forces energy to spread into the cavities connected to it — `terminal`, `bash`, `kernel`.

#### 2.3. Trajectory Momentum — chaos supplies *timing*, CPSB supplies *direction*

**The division of labour must be stated precisely.** The Lorenz
system has no knowledge of the graph: $x(t)$ does not know which
cavity is `terminal` and which is `bash`. The attractor's geometry
lives in its own $(x,y,z)$ space and bears no relation to the
semantic manifold. It therefore cannot, by itself, produce a
semantically coherent chain.

What CSAT contributes is **escape timing** — *when* to leave the
current basin. What produces the ordered chain
`linux → terminal → bash → kernel` is **CPSB gauge coupling plus
graph topology** — *where* the energy goes once it leaves.

> **CSAT provides escape timing. CPSB provides escape direction.**

This is a clean, non-overlapping role for CSAT within the stack, and
it is the honest claim. Attributing semantic coherence to the
attractor itself would overstate the mechanism; the coherence is
already earned by CPSB and the ontology.

Chaos is not randomness. The Lorenz attractor traces a definite geometric pattern — a *strange attractor*. The consequence is that the signal does not emit random words: starting from `linux` it builds a chain that respects the structural rules supplied by CPSB, `linux` $\rightarrow$ `terminal` $\rightarrow$ `bash` $\rightarrow$ `kernel`.

---

### 3. Parameters

| Symbol | Default | Meaning |
| --- | --- | --- |
| $\sigma, \rho, \beta$ | 10, 28, 8/3 | Standard Lorenz coefficients |
| $\tau$ | 0.30 | Time-scale factor (stability + drift rate) |
| $\gamma$ | 0.04 | Chaotic viscosity gain on $\hat{x}$ |
| $\hat{x}$ | $x/20$ | Normalized Lorenz coordinate |
| `csat_enabled` | see §4.1 | Master switch — **not** unconditionally on |

Five parameters. Setting $\gamma = 0$ recovers the pre-CSAT
substrate exactly.

---

### 4. Failure Modes

#### 4.1 Lyapunov divergence breaks bit-reproducibility

Chaos is deterministic, so seeded reproducibility survives *in
principle*: the same `bind_seed` and the same Lorenz initial
conditions yield the same trajectory. But the Lorenz system has a
positive Lyapunov exponent — a $10^{-7}$ floating-point difference
grows to $O(1)$ within roughly a thousand ticks. Different compiler
flags, `-ffast-math`, or a different FPU will therefore produce
divergent trajectories from identical seeds.

This directly conflicts with the bit-exactness that the 20-seed RB+G
table (`rbg.md` §6.1) and the four-lever ablation grid (§6.2) depend
on.

**CSAT is consequently the first mechanism in the stack that must
not default unconditionally ON.** Recommended policy:

- `csat_enabled = true` in the conversational shell (`si_chat`)
- `csat_enabled = false` forced inside `study_recall_seeds` and
  `study_recall_ablation`

Published numbers stay bit-reproducible; conversation still gets the
drift. This is a documented exception to the "all defaults ON"
contract in the implementation guide §11.3, and the exception should
be recorded there.

#### 4.2 CSAT does not address stale amplitude accumulation

Ground-state locking has more than one cause, and CSAT addresses only
the genuine-potential-well case. In particular, with friction
$\mu_0 = 0.006$ per tick, roughly $30\%$ of a cavity's amplitude
survives a 200-tick turn ($0.994^{200} \approx 0.30$). A cavity
driven once at high amplitude therefore remains dominant for several
subsequent turns by pure arithmetic, with no potential well involved.

That is a *dissipation* deficiency, not a chaos deficiency. Chaotic
shaking of an over-energised field does not remove the excess energy.
Fixing it requires either stronger inter-turn damping or an explicit
energy budget per turn — orthogonal to CSAT and left as separate
work.

#### 4.3 Chaos on a sparse graph shakes an empty room

If the driven concept has few or no outbound tunnels, there is no
trajectory for the attractor to push the field along. CSAT then
produces no observable change: the escape timing fires, but there is
nowhere to escape to. CSAT is therefore a mechanism whose benefit
scales with ontology density, and it should be evaluated only on
subgraphs rich enough to support multi-hop propagation.

---

### 5. Relation to the Existing Stack

| Layer | Mechanism | Role |
| --- | --- | --- |
| CMD | Curie gate on $T_i$ | entrenched cavity loses *coupling* |
| **CSAT** | **chaotic viscosity on $T_i$** | **entrenched cavity loses *stability*** |
| VGHT | sub-bands, voltage gates, viscosity | domain isolation + avalanche control |
| CPME | momentum, acoustic mass, settle, sweep | emission eligibility and ordering |
| CPSB | gauge $\Psi$ on tunnels | escape *direction* / word order |
| CPME-II | phase-consistent Hebbian | stores the phase gradient across turns |

CSAT and the CMD Curie gate read the same state variable $T_i$ but
act on different quantities — coupling strength versus dynamical
stability. They are complementary, not redundant.

---



\newpage

## Morphological Resonance Synthesis (MRS): Language Construction Without Tokens, Gradients, or Learned Embeddings


*The byte-lane sensory layer is currently input-only: bytes enter the
substrate through learned couplings, but output leaves it as retrieved
label strings. That retrieval is the last token-shaped operation in the
pipeline. MRS removes it by making the byte layer bidirectional and
giving it the same three phase mechanisms the concept layer already
has — chirality on input, phase-consistent consolidation, and
phase-collapse projection on output. Word forms are then* synthesised
*by sweeping a byte-phase field rather than* retrieved *from a table.*

---

### 0. The Remaining Asymmetry

The CMD substrate ingests raw bytes. There is no tokeniser: each of
256 `ByteLane` oscillators is driven by its byte value, and lanes
reach the concept field only through Hebbian couplings
$w[b][c]$ that begin at zero. On the input side, the architecture is
already sub-lexical and token-free.

The output side is not. `readout_phase_collapse()` selects concept
cavities by phase and then prints
`si.graph.nodes[label_key].label` — a **stored string**. The physics
decides *which* concepts speak and *in what order*, but the *form of
each word* is a lookup.

This asymmetry has three consequences:

1. **No morphology.** The engine can emit `walk` if `walk` is a
   stored label, and `walked` if `walked` is separately stored, but it
   cannot construct `walked` from `walk`. There is no mechanism by
   which stem and inflection compose.
2. **No novel forms.** Any word form absent from the label table is
   unreachable, regardless of how strongly the substrate's state
   implies it.
3. **A token-shaped residue.** The label table is a finite inventory
   of atomic surface strings indexed by graph key. Functionally that
   is a vocabulary, and its presence is what makes the pipeline
   token-like at the boundary even though the interior is not.

MRS closes the asymmetry. The mechanism is a direct reuse — one layer
down — of machinery the substrate already contains.

---

### 1. Architecture

Let the byte-lane population be $L_b$ for $b \in [0,256)$, each an
oscillator with displacement $x_b$, velocity $v_b$, and natural
frequency $\omega_b = 3.0 + b/256$ (as in the existing CMD sensory
layer). Let $w[b][c]$ be the learned byte→concept binding weights.

MRS adds four mechanisms.

#### 1.1 Reverse Drive (concept → lane)

The existing forward path applies a force from lane to concept:

$$ F_c \mathrel{+}= \frac{w[b][c] \cdot x_b}{m_c} $$

MRS adds the transpose, using the **same weights**:

$$ F_b \mathrel{+}= \eta_{\text{rev}} \cdot w[b][c] \cdot x_c $$

with $\eta_{\text{rev}}$ a reverse-gain scalar (default $0.4$). No new
storage: the coupling matrix is used in both directions, making the
byte↔concept layer a bidirectional associative memory implemented as
coupled oscillators rather than as a matrix multiplication.

Consequence: when concept cavities resonate, the byte lanes whose
bindings to those concepts are strongest resonate in turn. The lanes
that light up are the letters of the words those concepts were
historically written with.

#### 1.2 Lane Chirality

Byte lanes acquire the CPME §1.1 arrival timestamp:

$$ t_{\text{arrival},b} \leftarrow t_{\text{sim}} \quad \text{on each drive of } L_b $$

During input of `hi`, lane $L_{\texttt{h}}$ is driven before
$L_{\texttt{i}}$ and therefore accumulates more phase. Letter order
within a word becomes a phase gradient across the lane population —
exactly the mechanism CPME already uses for word order across
concepts, applied one level down.

#### 1.3 Lane-to-Lane Gauge Tunnels

A sparse set of lane↔lane tunnels is created on co-occurrence, each
carrying a CPSB-style gauge offset $\Psi_{bb'}$ and consolidated by
the CPME-II phase-consistent rule:

$$ \Delta w_{bb'} \;\propto\; v_b \cdot v_{b'} \cdot \cos\!\bigl(\theta_b - \theta_{b'} - \Psi_{bb'}\bigr) $$

where $\theta_b = \operatorname{atan2}(v_b, \omega_0 x_b)$. Unlike the
concept layer, $\Psi_{bb'}$ is **not** assigned from a relation table
— it is *learned* as the running circular mean of observed phase
differences:

$$ \Psi_{bb'} \leftarrow \arg\Bigl( \lambda\, e^{i\Psi_{bb'}} + (1-\lambda)\, e^{i(\theta_b - \theta_{b'})} \Bigr) $$

This is the substrate's representation of *sequence*: the tunnel
between `h` and `i` stores the typical phase lead of `h` over `i`.
Nothing about letters, words, or grammar is encoded declaratively;
only phase offsets between oscillators.

#### 1.4 Byte-Level Phase-Collapse Projection

Output is generated by the CPME §1.6 sweep applied to the lane
population. Carrier phase $\phi_{\text{carrier}}$ advances
monotonically across $[0, 2\pi)$ in $K_{\text{byte}}$ steps; at each
step the emitted byte is the arg-max of

$$ w_b(\phi_{\text{carrier}}) \;=\; |x_b| \cdot \operatorname{ReLU}(P_b) \cdot \exp\!\bigl(-\kappa_b (\phi_{\text{carrier}} - \theta_b)^2\bigr) $$

with $P_b$ the lane's envelope momentum as in CPME §1.2.

**Critical divergence from the concept sweep.** In the concept layer,
a winner is suppressed from later sweep steps — a concept appears once
in a clause. In the byte layer, **winners are not suppressed**, and
the sweep may make multiple passes. Letters repeat within words
(`banana`, `letter`, `success`); a suppression rule would make
repeated graphemes unreachable. The difference is physically
motivated by what each layer represents, not introduced as an
exception.

Word boundaries emerge from amplitude gaps: when the swept weight
falls below the emission floor for a contiguous span of carrier phase,
a boundary is emitted. Boundaries are therefore not delimiters in a
grammar but silences in a resonance field.

---

### 2. Why This Is Not Tokens, ML, DL, or an LLM

**Not tokenisation.** There is no vocabulary, no merge table, no
subword inventory, no `<unk>`. Input is 256 byte lanes; output is a
swept byte sequence. The only inventory is the byte alphabet itself,
which is a property of the encoding, not a learned artefact.

**Not machine learning in the gradient sense.** There is no loss
function, no gradient, no backpropagation, no optimiser, no train/test
split. All adaptation is the local Hebbian rule of §1.3, which uses
only quantities available at the two endpoints of a single tunnel at a
single tick. Nothing computes a derivative of a global objective.

**Not deep learning.** There is no layer stack in the DL sense — no
weight matrices applied in sequence, no nonlinearity between layers,
no depth over which credit is assigned. The byte layer and concept
layer are two populations of coupled oscillators exchanging forces
bidirectionally through one shared coupling matrix.

**Not a language model.** No distribution over next symbols is ever
formed. There is no softmax, no sampling temperature, no
autoregressive conditioning on prior output. The emitted sequence is
the projection of a static field state onto a swept phase axis — a
geometric readout, not a sequential prediction. Given the same field
state the same sequence is produced deterministically, and the
sequence is generated *simultaneously* in the sense that all lanes are
already in their phases before the sweep begins.

**What it is:** a bidirectional associative resonance memory between a
256-oscillator sub-lexical layer and an $N$-oscillator conceptual
layer, read out by phase-swept projection.

---

### 3. Where the Storage Lives (Translation Capacity Lemma)

The Translation Capacity Lemma (SI §3.1) states that any system
discriminating $N$ outcomes must store $\log_2 N$ bits somewhere. MRS
does not evade this and does not claim to. The storage relocates:

| Before MRS | After MRS |
| --- | --- |
| $N$ label strings in `graph.nodes[].label` | $256 \times N$ byte→concept couplings $w[b][c]$ |
| — | $\le 256^2$ lane↔lane couplings and offsets $\Psi_{bb'}$ |

The relocation is the substantive change. A label table stores each
surface form *atomically and independently*: `walk` and `walked` share
no representation, so knowing one contributes nothing to producing the
other. A coupling matrix stores forms *distributedly and
compositionally*: the lanes and phase offsets that produce `walk` are
largely the same ones that produce `walked`, differing by the
additional offsets contributed by the co-active inflectional concept.

Generalisation to unseen forms therefore becomes possible in principle
— not because information was created, but because the information was
stored in a factored representation instead of an atomic one. The bit
count is not reduced; its *organisation* is what changes.

---

### 4. Expected Dynamics

**Prompt:** `metformin lowers glucose`

1. **Byte injection.** Lanes for each character are driven in arrival
   order; $t_{\text{arrival}}$ records the chirality gradient. Lane↔lane
   tunnels consolidate the observed offsets (`m` leads `e` leads `t`…).
2. **Concept resonance.** Forward couplings $w[b][c]$ excite the
   `metformin`, `lowers`, `glucose` cavities; propagation through the
   ontology raises downstream cavities (`ampk`, `glucose_uptake`).
3. **Acoustic-mass muting.** Directly-driven concepts are muted
   (CPME §1.3); downstream concepts are not.
4. **Reverse drive.** The surviving downstream concepts drive byte
   lanes through $w[b][c]^{\!\top}$. Lanes corresponding to the
   letters of `ampk`, `glucose`, `uptake` resonate — *without those
   strings being retrieved*.
5. **Byte sweep.** The carrier phase sweeps the lane population. Lanes
   emit in phase order, with boundaries at amplitude gaps. Output is a
   byte sequence assembled from resonance.

**Morphological composition.** If the inflectional concept for
past-tense has been bound to the lanes `e`,`d` through prior exposure,
and it is co-active with `lower`, the reverse drive raises both the
stem lanes and the suffix lanes. The sweep — ordered by learned phase
offsets — emits `lowered`, a form that need never have been stored as
a label.

---

### 4b. Empirical Result: Single-Byte Lanes Are Insufficient (measured)

The single-byte formulation of §1.1–§1.4 was implemented and tested.
**It does not work, and the reason is representational rather than a
matter of tuning.**

Recognition was attempted by grounding every concept label to the 256
lanes at load time (binding weight = inverse byte document-frequency
across the lexicon) and then recognising a query by driving its bytes
and reading which concepts accumulated energy. Measured on a 2,240
-concept graph:

| query | recognised as | correct |
| --- | --- | --- |
| `metfromin` | `from`, `front`, `rfe` | ✗ |
| `metformin` | `from`, `front`, `rfe` | ✗ |
| `insluin` | `uni`, `ull`, `sil` | ✗ |
| `fissoin` | `fission` | ✓ |

Two distinct causes, and separating them matters:

**(a) A normalisation error.** Binding weight was divided by
$\sqrt{|\text{label}|}$. For the query `metformin`, the substring
`from` scored $S/2$ while `metformin` itself scored $S/3$ — the extra
bytes $\{e,t,i,n\}$ are common and contribute ≈ 0 weight, but the
length penalty applies regardless. A substring therefore outscored the
whole word, which is why recognition failed even on *correctly spelled*
input. This was a genuine bug.

**(b) A representational limit.** Removing the length division and
replacing it with symmetric coverage (harmonic mean of query-recall and
label-precision over byte sets) fixed the substring case and broke a
different one: long labels sharing many bytes with the query
(`intent_from_marker_label`) began to win, because a concept bound to
more lanes accumulates more raw energy regardless of coverage. Each
subsequent scoring adjustment relocated the failure rather than removing
it.

That pattern — a defect that moves under every reweighting but never
disappears — is the signature of insufficient representation. The
decisive case is `metformin` versus `metfromin`: **they have identical
byte sets.** A bag of distinct bytes cannot distinguish a word from its
own transposition, so no weighting function over that bag can succeed.
This is §5.3 (homograph collision) manifesting not as an edge case but
as the dominant failure.

#### 4b.1 Required correction: bigram lanes

The sensory layer must carry ordering information. Replacing the 256
single-byte lanes with $256^2$ ordered-pair lanes supplies it:

$$
\texttt{metformin} \rightarrow \{\texttt{me},\texttt{et},\texttt{tf},\texttt{fo},\texttt{or},\texttt{rm},\texttt{mi},\texttt{in}\}
$$
$$
\texttt{metfromin} \rightarrow \{\texttt{me},\texttt{et},\texttt{tf},\texttt{fr},\texttt{ro},\texttt{om},\texttt{mi},\texttt{in}\}
$$

Five of eight pairs are shared; three differ. The transposition is now
*visible to the substrate*, and graded recognition becomes possible:
a near-miss resonates strongly but not identically, which is the
behaviour §1.4 requires and the single-byte layer cannot deliver.

**This changes nothing about the architecture's commitments.** A bigram
lane is one oscillator per ordered character pair, with displacement,
velocity and natural frequency — the same `ByteLane` structure, a wider
array. Specifically it introduces:

- no loss function, no gradient, no fitted parameter;
- no attention, no softmax, no learned projection;
- no probability semantics — lane energy is displacement in a field.

The distinction that governs this architecture is not *"are there
numbers"* but *"where do the numbers come from."* A binding weight here
is **computed** from a counted structural property of the lexicon by a
stated formula; changing the lexicon changes it deterministically.
A weight in a trained network is **fitted** by optimising a loss and
admits no explanation beyond that optimisation. Descriptive measurement
is not fitted inference — measuring a mass does not make mechanics
machine learning.

n-gram indexing is also not novel-to-ML prior art: it originates in
1970s information retrieval (approximate string matching, spell
correction) and predates statistical learning entirely.

#### 4b.2 Measured result after implementing bigram lanes

Bigram lanes were implemented (`PhysicsEngine::bigram_bind_w`,
`bind_bigram`). Grounding is identical in form to the single-byte
version — inverse lane document-frequency across the lexicon — but over
ordered pairs bracketed by BOS/EOS sentinels, so word-initial and
word-final position are themselves observable pairs.

Recognition divides energy received by a concept by the geometric mean
of its own binding mass and the query's:

$$
\text{resonance}(c) \;=\; \frac{\sum_{g \in Q \cap B(c)} w_g}{\sqrt{M_c \cdot M_Q}}
$$

This is energy per unit coupling — the same reason acceleration is
force divided by mass — and it is why neither long nor short labels win
by size. The earlier failures came from omitting exactly this
normalisation.

Measured on the same 2,240-concept graph:

| query | perturbation | recognised as | score | correct |
| --- | --- | --- | --- | --- |
| `metformin` | none | `metformin` | 1.000 | ✓ |
| `metfromin` | transposition | `metformin` | 0.686 | ✓ |
| `insluin` | transposition | `insulin` | 0.471 | ✓ |
| `fissoin` | transposition | `fission` | 0.621 | ✓ |
| `fisssion` | insertion | `fission` | 1.000 | ✓ |
| `cavty` | deletion | `cavity` | 0.830 | ✓ |
| `tunel` | deletion | `tunnel` | 0.885 | ✓ |

**7 / 7**, against 1 / 4 for single-byte lanes.

The `metformin` / `metfromin` pair is the decisive one: those two
strings have **identical byte sets**, so the single-byte formulation
could not distinguish them under any weighting. Over bigrams they share
5 of 8 pairs and differ in 3, and the correct concept is recovered at
0.686 against 1.000 for the exact spelling.

That gradation is the property §1.4 requires and a hash lookup cannot
have: recognition degrades smoothly with distortion rather than failing
discontinuously. It is also the evidence that recognition is not string
matching — no comparison between the query and any stored label occurs
at any point.

**Status:** bigram recognition is ENABLED in `codeq`, with an
acceptance floor of 0.35 so unknown tokens resonate with nothing rather
than matching noise. Single-byte recognition is retained in the source
only as the documented negative result. Per-query telemetry reports
`lexical` versus `resonant` recognition counts, so residual dependence
on exact lookup is a measured quantity rather than an assumption.

---

### 5. Failure Modes

1. **Babble before consolidation.** With near-zero couplings the lane
   phases are unstructured and the sweep emits non-words. This is the
   correct initial state of a system that has not yet learned
   orthography, but it will read as garbage output until the lane↔lane
   offsets consolidate. Consolidation requires substantial exposure —
   Hebbian rather than gradient, but still genuinely a training
   process measured in thousands of turns, not tens.

2. **Phase resolution limits word length.** With $K_{\text{byte}}$
   sweep steps the angular resolution is $2\pi/K_{\text{byte}}$. A
   ten-character word requires ten sufficiently separated lane phases.
   Long words will be truncated or slurred until $K_{\text{byte}}$ is
   raised, at proportional sweep cost.

3. **Homograph collision.** A single lane per byte value means the
   letter `a` has one phase at any instant. In a word requiring `a` at
   two different positions, the multi-pass sweep of §1.4 recovers the
   repetition only if the two occurrences are separated by other
   letters whose phases bracket them. Words like `aa` or
   vowel-adjacent repeats are expected to fail. A positional lane bank
   ($256 \times K$) would resolve this at a large state cost and would
   reintroduce something functionally close to positional encoding;
   it is deliberately not proposed here.

4. **No orthographic well-formedness guarantee.** Nothing in the
   mechanism prevents emission of byte sequences that are not words.
   Well-formedness is a statistical consequence of consolidation, not
   a constraint. This is a genuine difference from a grammar-based
   generator and should be reported as such rather than patched with a
   dictionary filter — a filter would reintroduce the vocabulary that
   MRS exists to remove.

5. **Reverse-gain instability.** Bidirectional coupling through a
   shared matrix creates a feedback loop: concepts drive lanes drive
   concepts. For $\eta_{\text{rev}}$ above a critical value the loop
   has gain greater than unity and the field diverges. The VGHT §1.3
   avalanche viscosity bounds this, but $\eta_{\text{rev}}$ must be
   tuned conservatively and the loop gain monitored.

---

### 6. Relation to the Existing Stack

MRS is the mirror of CPME one layer down. Every mechanism it uses
already exists in the substrate for the concept layer:

| Mechanism | Concept layer | Byte layer (MRS) |
| --- | --- | --- |
| Chirality on input | `Cavity::t_arrival` | `ByteLane::t_arrival` |
| Momentum filter | `env_ema` on cavities | `env_ema` on lanes |
| Phase definition | $\operatorname{atan2}(v_c, \omega_0 x_c)$ | $\operatorname{atan2}(v_b, \omega_0 x_b)$ |
| Gauge offset | $\Psi$ from `psi_for(rel)` | $\Psi$ *learned* from observed phase |
| Phase-consistent Hebbian | CPME-II on tunnels | CPME-II on lane tunnels |
| Phase-collapse sweep | emits concept labels | emits **bytes** |
| Winner suppression | yes (concept appears once) | **no** (letters repeat) |

The single genuinely new element is §1.1 reverse drive. Everything
else is the same physics applied to a different oscillator
population — which is the strongest argument that the architecture is
coherent rather than assembled.

---

### 7. Summary

The CMD substrate was token-free on input and table-driven on output.
MRS removes the remaining table by making the byte-lane layer
bidirectional and equipping it with the chirality, consolidation, and
phase-collapse mechanisms already built for concepts. Word forms
become the projection of a byte-phase field rather than entries
retrieved by graph key.

No tokeniser, no gradient, no layer stack, no next-symbol
distribution. Storage obeys the Translation Capacity Lemma but is
relocated from atomic surface strings into a factored coupling
matrix, which is what makes morphological composition and novel forms
reachable at all.

The honest cost is that consolidation is slow and early output is
babble. A system that constructs its own orthography must first fail
to.

