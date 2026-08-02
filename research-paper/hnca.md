# HNCA: Hierarchical Neural Constraint Architecture — Cognition as Constraint Satisfaction across Two Coupled Scales

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"The engine is not searching for an answer. It is integrating
> differential equations, and the answer is where the energy stops."*

---

## Abstract

We specify a two-scale substrate for cognition in which computation is
constraint satisfaction rather than search or prediction. The
**micro-layer** is a continuous-state cellular grid whose cells carry
physical variables and whose edges are differential equations rather
than learned weights. The **macro-layer** wraps clusters of those cells
into regions with distinct natural frequencies, coupled by the Kuramoto
model, reflecting the observation that brain regions operate at
different characteristic rhythms rather than as one homogeneous field.

Processing has four stages: a perturbation is injected as a continuous
stimulus; the injected energy breaks the grid's equilibrium; the engine
integrates the constraint equations until the field settles; and the
resulting topological pattern **is** the thought, not a code for one.
No token is involved at any stage.

The macro-layer of this proposal is implemented and load-bearing in the
current engine. The micro-layer is not, and §6 states precisely which
part survived and why.

---

## 1. Position

Two architectural options are commonly assumed to exhaust the space: a
flat cellular grid, or a Transformer over lexical tokens. Neither is
adequate for constraint-based cognition.

A **flat grid** has no notion of region. Every cell obeys the same
dynamics at the same characteristic rate, so there is no mechanism by
which one part of the system can operate on a different timescale from
another — and differential timescale is what allows slow context to
modulate fast processing.

A **token Transformer** operates on discrete lexical units and computes
a distribution over continuations. Its geometry is a fully-connected
attention matrix, which is to say it has no geometry: every unit is
equidistant from every other, and spatial locality carries no meaning.

What is required instead is a **physics engine for cognition** — a
system with local interaction rules, spatial structure, and multiple
characteristic frequencies.

---

## 2. The Micro-Layer: Continuous-State Cellular Dynamics

The foundational layer is a multi-dimensional continuous grid. A cell
does not contain a string or an embedding; it represents a micro-column
and carries a state vector of physical quantities: membrane potential
$V_m$, a recovery variable $u$, and a local field potential.

The edges are **differential equations**, not learned parameters. The
cell update implements a spiking model natively — for instance the
Izhikevich formulation:

$$
\frac{dV}{dt} = 0.04V^2 + 5V + 140 - u + I
$$

$$
\frac{du}{dt} = a(bV - u)
$$

The significance is what is *absent*. There is no embedding table, no
attention, and no parameter fitted by gradient descent. The constraints
are stated, not learned, and the dynamics they induce are the
computation.

---

## 3. The Macro-Layer: Hierarchical Oscillators

A brain is not a homogeneous grid. Cortex, thalamus and amygdala operate
at different characteristic frequencies, and that heterogeneity is
functional rather than incidental.

Clusters of micro-cells are therefore wrapped into **macro-nodes**, each
with a natural frequency $\omega_i$, and coupled through the Kuramoto
model. The engine continuously solves for phase across the network:

$$
\frac{d\theta_i}{dt} \;=\; \omega_i \;+\; \frac{K}{N}\sum_{j=1}^{N} \sin(\theta_j - \theta_i)
$$

Here $\omega_i$ is the natural frequency of a region and $K$ is the
coupling strength — the analogue of the white-matter tracts connecting
regions.

Two properties follow. Regions with similar $\omega$ **entrain** easily
and exchange influence readily. Regions with dissimilar $\omega$ resist
entrainment, so a frequency separation acts as a *soft domain boundary*
requiring no gating logic and no explicit routing table. Domain
isolation becomes a physical property rather than a control structure.

---

## 4. The Constraint Engine

Input and output are both physical events.

**Stage 1 — Inject perturbation.** Rather than passing text to an
encoder, a continuous stimulus (a current $I$) is injected into specific
sensory cells.

**Stage 2 — Disequilibrium.** The injected energy breaks the grid's
equilibrium. Kuramoto oscillators fall out of phase and cell states
spike. The system is *forced* to react; nothing decides to respond.

**Stage 3 — Continuous constraint solving.** The integration loop runs.
The engine is not searching a space of candidate answers — it is
integrating differential equations across the edges. Energy propagates
bound strictly by the local rules.

**Stage 4 — Attractor convergence.** The field settles into a new stable
configuration. **This topological pattern is the thought.** A downstream
decoder translates the settled state into text or action, and that
translation is a separate, explicitly-named cost rather than part of the
cognition.

---

## 5. Comparison

| Property | Token Transformer | Constraint engine |
| --- | --- | --- |
| Fundamental unit | lexical tokens | continuous state variables |
| Mechanics | probabilistic next-symbol prediction | deterministic differential equations |
| Network geometry | fully-connected attention | spatially bounded topological grid |
| Output generation | sampling from a distribution | relaxation into an attractor |
| Timescales | one | many, set by $\omega_i$ per region |

The intended consequence is a change in what is being programmed. One
does not specify the logic; one specifies the **laws** the substrate
obeys, and the logic is whatever those laws force.

---

## 6. What Was Implemented, and What Was Not

This section is the paper's accounting against the running system, and
it corrects an earlier presentation in which the whole proposal was
described as live.

**The macro-layer is implemented and load-bearing.** Macro-nodes with
per-region natural frequencies, coupled by the Kuramoto update, are in
the current substrate (`struct MacroNode`, `physics.hpp`), and the
frequency-separation-as-domain-boundary property of §3 carries through
to the sub-band ladder of the VGHT layer. The claim that dissimilar
$\omega$ resists entrainment and thereby isolates domains was tested and
holds.

**The micro-layer is not implemented.** The current substrate does not
run Izhikevich cells on a continuous grid. Its Layer-1 unit is a
**mass-spring cavity** — a damped oscillator coupled through tunnels
whose stiffness and damping are fixed by relation type. This is a
different micro-model presenting the same macro-interface, which is
precisely why the macro-layer transferred without modification.

**Why the substitution happened.** The spiking micro-layer was specified
for biological fidelity, and biological fidelity turned out not to be
load-bearing for the structural reasoning the engine is asked to do. The
mass-spring cavity is simpler, has an analytic impedance, and admits a
material table letting relation *type* set transmission properties.
Nothing in §3 or §4 depended on the micro-model being spiking.

The general lesson is worth stating: **the layered specification
survived a total replacement of its bottom layer.** That is evidence the
interface between the two scales was drawn in the right place, and it is
the most useful thing this paper produced.

---

## 7. Limitations

**Attractor convergence is assumed, not guaranteed.** Section 4 asserts
that the field settles. A driven nonlinear system need not converge; it
may oscillate indefinitely, or lock into a ground state and refuse to
leave. Ground-state locking was subsequently observed as a real failure
mode and required a dedicated escape mechanism.

**The decoder is unspecified.** Stage 4 defers the conversion of a
settled topology into text to "a downstream decoder", which is exactly
where the irreducible translation cost of the architecture lives.
Deferring it is not solving it, and naming it as deferred is the minimum
honest treatment.

**No account of acquisition.** Constraints are stated. How a system
acquires new constraints from experience lies outside this proposal and
was addressed separately by local Hebbian consolidation.

---

## 8. Conclusion

HNCA's contribution is the two-scale decomposition, and the specific
claim that differential natural frequency suffices for domain separation
without any routing logic. That claim survived into a substrate whose
micro-layer bears no resemblance to the one proposed here — which is the
strongest available evidence that the decomposition, rather than either
individual layer, was the durable part.

---

## References

- Izhikevich, E. M. *Simple Model of Spiking Neurons*, IEEE Transactions
  on Neural Networks, 2003.
- Kuramoto, Y. *Chemical Oscillations, Waves, and Turbulence*, 1984.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c — the
  substrate in which the macro-layer runs.
- Mahir, I. *Voltage-Gated Harmonic Topology (VGHT).* — develops §3's
  frequency-separation argument into an explicit sub-band ladder.
- Implementation: `src/physics.hpp` (`struct MacroNode`, Kuramoto pass).
