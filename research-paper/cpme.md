# Chiral Phase-Momentum Emission (CPME)

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

*A unified emission substrate combining the Momentum-Coupled Phase
Emission (MCPE) filter architecture with the Holographic Topological
Resonance (HTR) phase-projection readout. CPME operates on a single
phase manifold: chirality on injection, coherence-rise as the
resonance metric, momentum as the eligibility filter, and continuous
phase-collapse sweep as the emission mechanism. No syntax trees, no
grammar rules, no next-token statistics, and no fixed output length.*

---

## 0. Position

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

## 1. Architecture

Let the substrate be the CMD + VGHT engine as previously specified:
cavities indexed by $i \in [0, N)$ with per-cavity displacement
$x_i$, velocity $v_i$, heat $T_i$, and quiet age $q_i$; tunnels with
per-tunnel stiffness $k$, damping $c$, polarity $p$, and voltage
gating $g \in \{0, 1\}$; and macro-nodes with per-cluster phase
$\theta_m$, amplitude $A_m$, and natural frequency $\omega_m$ drawn
from the VGHT sub-band ladder. The base cavity frequency is
$\omega_0$.

CPME adds six mechanisms.

### 1.1 Chiral Wave Injection (from HTR §2.1)

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

### 1.2 Momentum Filter (from MCPE §2.1)

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

### 1.3 Acoustic Mass (from MCPE §2.2)

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

### 1.4 Settle Separation (from MCPE §2.3)

The tick maintains a global injection load

    L = Σ_i drive_amp_i · drive_envelope_i

and derives a *slot coupling* factor

    K_settle = 1 − σ((L − L_thresh) / L_scale)

with $\sigma$ the logistic, $L_{\text{thresh}} = 0.5$,
$L_{\text{scale}} = 0.2$. During active injection, $K_{\text{settle}} \approx 0$
and the emission readout is suppressed. As the drive envelopes decay,
$K_{\text{settle}} \to 1$ and articulation engages. The transition is
continuous — no discrete $T_0 / T_1 / T_2$ boundary.

### 1.5 Coherence-Rise Metric (from HTR §2.2)

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

### 1.6 Phase-Collapse Projection (from HTR §2.3)

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

## 2. Composition

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

## 3. Emission Contract

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

## 4. Parameters

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

## 5. Expected Dynamics

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

## 6. Failure Modes

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

## 7. Biological Analog

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

## 8. Contract Summary

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

## 9. Summary

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
