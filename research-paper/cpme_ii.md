# CPME-II: Phase-Consistent Hebbian Consolidation

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

*A phase-coincidence extension to the ZeroGrad Hebbian rule that
closes the chirality–consolidation gap identified in the CPME §6
failure modes. Chirality imprinted on injection now persists in
long-term memory rather than dissipating over turns.*

---

## 0. Position

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

## 1. The Rule

### 1.1 Definition

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

### 1.2 Interpretation

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

### 1.3 What is stored

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

## 2. Composition

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

## 3. Contract and Parameters

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

## 4. Expected Dynamics

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

## 5. Failure Modes

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

## 6. Biological Analog

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

## 7. Summary

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
