# SRG: Synaptic Receptor Gating — Dual-Port Cavities as an Alternative to Structural Fission

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"A neuron does not split when it receives conflicting input. It
> receives it through different receptors."*

---

## Abstract

When excitatory and inhibitory waves arrive at a single cavity in a
wave-propagation substrate, they sum. Summation of opposed signals is
destructive interference: the contradictory structure carried by the two
waves cancels, and the plasticity updates they would have produced
cancel with it. A concept holding genuine contradictions therefore
learns nothing, because its updates annihilate.

**Resonant Bifurcation** solves this by splitting the cavity into two.
This paper specifies the alternative: keep one cavity and **split its
input ports**, exactly as a biological neuron uses distinct receptor
types — AMPA for excitation, GABA for inhibition — on a single cell
body.

Each cavity is given an **E-port** admitting only positive-polarity
tunnels and an **I-port** admitting only negative-polarity ones. The two
channels are integrated separately and combined only at readout, so
signed cancellation cannot occur during accumulation. Graph topology
stays fixed, no dynamic allocation is required, and the energy delivered
by a query is not divided between child nodes.

The mechanism is implemented and opt-in, and §6 states honestly what
that costs and when the alternative is preferable.

---

## 1. The Destructive Interference Problem

Consider a cavity receiving two tunnels, one of positive polarity and
one of negative. In a scalar cavity, both contribute to the same
displacement variable:

$$
x \;\leftarrow\; x + \Delta_{+} + \Delta_{-}
$$

When $\Delta_{+} \approx -\Delta_{-}$, the cavity is left at rest and
the field concludes nothing happened. But something *did* happen: two
structurally significant events arrived. The information was not
processed and discarded — it was destroyed at the point of summation.

The consequence for learning is worse than the consequence for readout.
Hebbian consolidation is driven by the product of endpoint velocities.
If the velocity is cancelled, the product is near zero, and the tunnels
that delivered the conflicting evidence are **not** strengthened. A
concept that holds contradictions is therefore the one concept that
cannot learn about them.

This is the phenomenon that motivated structural fission.

---

## 2. Dual-Port Cavities

Rather than one intake, each cavity is given two:

- **E-port** — accepts waves from positive-polarity tunnels only.
- **I-port** — accepts waves from negative-polarity tunnels only.

The cavity therefore carries four state variables instead of two:
$(x_E, v_E)$ and $(x_I, v_I)$.

The biological reading is direct. A cortical neuron does not maintain a
single undifferentiated input. Excitatory and inhibitory afferents
terminate on different receptor populations with different reversal
potentials, and the cell integrates them through separate conductances
before they jointly determine whether it fires.

---

## 3. Mechanics

Waves entering the E-port and I-port do not sum into a shared
displacement field during accumulation. They are integrated in separate
channels, and the observable displacement is derived only at
end-of-tick:

$$
x \;=\; x_E - x_I, \qquad v \;=\; v_E - v_I
$$

Equivalently, the I-port contribution is phase-inverted before it
reaches the observable — a $180^\circ$ shift — so an inhibitory signal
carries an explicit negative functional sign rather than silently
erasing an excitatory one.

**The property that matters is where the subtraction happens.** Both
formulations produce the same observable $x$. They differ in what is
available to plasticity: because Hebbian updates are computed from the
*port* velocities rather than the observable, an excitatory update lands
in $v_E$ products and an inhibitory one in $v_I$ products, and they
cannot cancel. Structure accumulates in both channels simultaneously.

With gating disabled, $x_I$ and $v_I$ remain zero and the observable
equals the E-port exactly, so the baseline dynamics are recovered
bit-for-bit. The mechanism is a strict extension.

---

## 4. Comparison with Structural Fission

Both mechanisms address the same failure. They differ in cost.

| Property | Fission (RB+G) | Receptor gating (SRG) |
| --- | --- | --- |
| Graph topology | mutates — new vertices spawned | **static** |
| Allocation | dynamic; parent/alias ledger required | none |
| Query energy | divided among children | **delivered whole to one cavity** |
| State per concept | grows | fixed, 2× scalar |
| Expressive gain | two independently-addressable concepts | one concept, two insulated pathways |
| Audit trail | explicit fission event with provenance | none — no structural event occurs |

**Static topology is the practical advantage.** Fission requires
allocating vertices at runtime and maintaining a ledger mapping a parent
label to its children, so that later queries resolve correctly. Gating
requires neither: the graph after a thousand ticks is the graph that was
loaded.

**Undivided energy is the functional advantage.** Under fission, driving
a concept excites a parent whose energy then distributes across
children, so the amplitude reaching any single child is reduced. This
was a live concern for recall gain. Under gating, the drive reaches one
cavity at full amplitude and the tension is processed through two
insulated pathways inside it.

---

## 5. What Gating Does Not Provide

The comparison above would be dishonest without this section.

**No new concepts.** Fission produces two *addressable* entities:
`metformin_alpha` can acquire its own edges, participate in its own
analogies, and be reported separately. Gating produces one entity with
better internal bookkeeping. If the contradiction is genuinely two
distinct senses of a word, gating does not separate them — it only stops
them destroying each other.

**No structural audit trail.** A fission event is a recorded structural
mutation with provenance, and downstream analysis can ask *when* and
*why* a concept split. Gating changes nothing structurally, so there is
nothing to record and nothing to explain.

**Doubled state.** Every cavity pays for two ports whether or not it
ever receives conflicting input. Fission pays only for concepts that
actually contradict.

The mechanisms are therefore **complementary rather than competing**:
gating is the cheap always-on defence against cancellation, fission is
the expensive response to a contradiction severe enough to warrant two
concepts.

---

## 6. Implementation Status

Dual-port integration is implemented in the current substrate as the
`x_E` / `v_E` and `x_I` / `v_I` channels on each cavity, with the
observable derived at end-of-tick per §3.

It is **opt-in and off by default.** With SRG disabled the inhibitory
channel stays at zero and the substrate reproduces baseline behaviour
exactly, which is what makes the extension safe to carry. It is not
enabled by default because the fission path is the mechanism for which
reproducible multi-seed measurements exist; enabling gating by default
would change the baseline against which those results were obtained.

An honest statement of the evidence: the cancellation problem of §1 is
real and observed, the fix in §3 is sound by construction, and a
head-to-head measurement of gating against fission on the same ontology
**has not been run**. Until it is, the comparison table in §4 is an
argument from mechanism, not a report of a result.

---

## 7. Conclusion

Receptor gating is the conservative answer to destructive interference:
it changes how a cavity integrates its inputs rather than how many
cavities exist. It costs one extra pair of state variables per cavity,
requires no dynamic allocation, and preserves the amplitude of a driven
query.

It does not replace fission, because it cannot create a concept. It
removes the reason a concept needed to be created in the *common* case —
where the conflict is signed input to one coherent idea rather than two
ideas sharing a name.

---

## References

- Mahir, I. *RB+G: Gated Recursive Bifurcation with Analogical
  Bridging.* — the fission mechanism this paper offers an alternative to.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c — the
  substrate carrying both mechanisms.
- Mahir, I. *Recursive Integrity Field Architecture (RIFA).* — the
  contradiction metabolism that detects the tension in the first place.
- Implementation: `src/physics.hpp` (`x_E`, `v_E`, `x_I`, `v_I`;
  end-of-tick observable derivation).
