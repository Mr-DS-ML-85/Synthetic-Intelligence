# Voltage-Gated Harmonic Topology (VGHT)

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

*A three-mechanism physical substrate for domain-orthogonal cognition
with sparse cross-domain analogical bridging and avalanche
suppression, built entirely from scalar field dynamics — no rules, no
templates, no domain metadata.*

---

## 0. Motivation

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

## 1. Architecture

Let the cavity population be indexed by `i ∈ [0, N)`, partitioned into
macro-clusters `C_1, C_2, ...` by the connected components of the
tunnel graph (union-find at bind time; the same clustering used by
HNCA). Let a slot layer of `S` phase oscillators sit above the macro
layer as in CMD phase-locked emission.

### 1.1 Domain Sub-Bands

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

### 1.2 Voltage-Gated L6-A Tunnels

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

### 1.3 Global Viscosity (GABA analog)

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

## 2. Interaction of the Three Mechanisms

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

## 3. Physical Interpretation

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

## 4. Implementation Contract

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

## 5. Parameters

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

## 6. Expected Empirical Signature

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

## 7. Relation to Prior CMD Work

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

## 8. Failure Modes

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

## 9. Summary

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
