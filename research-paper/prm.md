# PRM: The Phonic Resonance Matrix — Concepts as Resonant Cavities and Inference as Wave Propagation

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"Inhibition is not an `if` statement. It is two waves arriving out of
> phase."*

---

## Abstract

We specify a substrate in which a concept is not an identifier, a token
or an embedding, but a **physical resonant cavity** with mass,
displacement and velocity. Relations are not pointers but **tunnels**
that transmit mechanical force between cavities. Inference is not search
but **wave propagation**, and the answer to a query is whichever cavity
is vibrating hardest once the field has settled.

The architecture's distinguishing property is that several capabilities
usually requiring explicit algorithms become consequences of the medium.
Inhibition is destructive interference rather than a conditional.
Multi-constraint search is two wavefronts colliding rather than a set
intersection. Structural analogy is sympathetic resonance between
cavities that share a harmonic ratio and no edge at all.

Section 7 reports what was measured and what remains an argument from
mechanism, including one claim in the original formulation that does not
survive scrutiny.

---

## 1. The Substrate

Concepts are **resonant volumes**. Each cavity carries a mass, a
displacement $x$, a velocity $v$, and a natural frequency determined by
its physical parameters. In the acoustic reading, a large cavity has a
low resonant frequency and a small one a high frequency.

Relations are **physical tunnels** — channels connecting cavities, not
references. Two polarities exist:

- An **excitatory** tunnel transmits displacement directly, producing
  sympathetic resonance in the target cavity.
- An **inhibitory** tunnel inverts phase by $180^\circ$, so its
  contribution actively cancels vibration at the target.

The design commitment is that the state of a concept is its *actual
mechanical state*, not a value standing for one.

---

## 2. Dynamics

The tick integrates Hooke's law and Newton's second law across every
tunnel:

$$
F \;=\; k\,(x_a \cdot p - x_b)\;-\;c\,(v_a - v_b)
$$

$$
v \;\mathrel{+}=\; \frac{F}{m}\,\Delta t,
\qquad
x \;\mathrel{+}=\; v\,\Delta t
$$

where $k$ is tunnel stiffness, $c$ damping, $p \in \{+1,-1\}$ polarity,
and forces are applied equal and opposite to the two endpoints.

A query is an injected driving force rather than a string. Reading the
result means observing which cavities have the largest displacement
after the field settles — the physical analogue of placing pressure
sensors in the pipes and seeing which are shaking.

---

## 3. Why Inhibition Requires No Logic

In a conventional implementation, negation is a branch: *if the relation
is inhibitory, subtract instead of add.* Here the polarity term $p$ sits
inside the force expression, so an inhibitory tunnel produces a
$180^\circ$-shifted contribution by arithmetic alone.

The consequence is architectural rather than aesthetic. Because
inhibition is multiplication by $-1$ rather than a conditional, the
update path contains no branch on relation semantics — which is what
permits the whole tick to be expressed as scalar arithmetic and
saturating nonlinearities, with no control flow that depends on what a
relation *means*.

---

## 4. Emergent Capabilities

These are the paper's substantive claims: behaviours obtained from
propagation rather than from an algorithm written to produce them.

### 4.1 Constructive interference as multi-constraint search

Drive two cavities simultaneously — say `type_2_diabetes` and
`autophagy`.

A graph search attempts to trace a literal edge path between them and
fails if none exists. The matrix instead radiates waves outward from
both. The wavefronts split, reflect, and meet. At a distant cavity —
`mtor` — the two arrive **in phase**, amplitudes add, and that cavity
spikes.

The system did not know `mtor` was the bridge and did not search for it.
It vibrated because it sits at the geometric intersection where two
independent wavefronts collided. The conjunction was computed by the
medium.

### 4.2 Sympathetic resonance as structural analogy

Strike a 440 Hz tuning fork and a nearby string tuned to 880 Hz begins
to sound, with no mechanical connection between them.

If cavity parameters reflect structural properties, then energy injected
into one structure can excite another that shares a harmonic ratio **and
has no connecting edge**. That is a structural analogy — a repurposing
candidate — found through harmonics rather than traversal.

### 4.3 Standing waves as attractor discovery

Run the tick for a few thousand iterations and kinetic energy settles
into a standing wave: some cavities become nodes of near-zero motion
through destructive interference, others antinodes of maximum
vibration.

Standing wave patterns are **computationally irreducible**: one cannot
predict where the pattern forms by inspecting the rules; the simulation
must be run. When the field settles, it has resolved the equilibrium of
a large number of competing constraints simultaneously, and the
high-amplitude cavities are the result.

### 4.4 Detecting this without a classifier

Novelty is observed rather than classified. After the field settles,
scan for cavities with high amplitude that were **not** directly driven:

```cpp
// After the wave has propagated, find the novelty
for (const auto& cavity : cavities) {
    if (cavity.displacement > THRESHOLD) {
        if (!was_injected(cavity.id)) {
            // A cavity is vibrating that we never excited directly:
            // an emergent resonance point.
            report(cavity.physical_label, cavity.displacement);
        }
    }
}
```

There is no `if`/`else` classifier deciding what counts as interesting.
The condition is *energy arrived here and we did not put it here*, which
is a statement about the field rather than about a category.

---

## 5. Position Against Pattern Matching

| Property | Neural / graph search | PRM |
| --- | --- | --- |
| Concept state | activation value or embedding | physical displacement and velocity |
| Inhibition | a rule or conditional | mechanical cancellation of opposed waves |
| Reachability | programmed paths or learned weights | radiated energy; waves collide unprogrammed |
| Multi-constraint | explicit set intersection | interference of two wavefronts |
| Analogy | similarity metric over representations | sympathetic resonance at harmonic ratio |
| Output | argmax over scores | whichever cavity is physically shaking |

The claim is that energy in a physical medium **radiates** rather than
following single logical lines, and that novelty arises from the
unprogrammed collisions of those radiating waves.

---

## 6. Implementation Note

The substrate is a one-dimensional wave simulator of the kind used in
game audio and fluid engines — tension, mass and displacement integrated
directly. No tensor library, no autodiff, and no linear-algebra
framework is involved, because nothing in §2 is a matrix operation.

---

## 7. What Was Measured, and What Was Overstated

**Implemented and load-bearing.** Cavities with mass, displacement and
velocity; polarity-carrying tunnels; the force integration of §2; and
amplitude readout are the Layer-1 substrate of the current engine
(`struct Cavity`, `physics.hpp`). The branchless property of §3 holds:
the tick contains one guard branch and no conditional on relation type.

**Measured.** Attractor settling (§4.3) and the undriven-cavity novelty
probe (§4.4) run and produce results. Multi-source interference (§4.1)
occurs and drives the analogical bridging mechanism reported separately
with 20-seed reproducibility data.

**Overstated in the original formulation.** Section 4.2's sympathetic
resonance was presented as though cavity volume already encoded real
structural properties. It does not. Cavity parameters are derived from
graph position and relation fan-out, **not** from molecular structure, so
the harmonic ratio between two cavities carries no independent
structural information about the entities they denote. The mechanism is
sound; the specific claim that it discovers *chemical* analogies without
edges is unsupported by anything implemented, and analogical bridging in
the current engine works through shared-target intersection rather than
harmonics.

**Not implemented.** The literal acoustic framing — transducers,
piezoelectric sensors, physical pipes — is expository. The substrate is
a numerical simulation, and no claim is made that a physical acoustic
device was built.

---

## 8. Limitations

**Settling is assumed.** Section 4.3 assumes the field reaches a
standing wave. A driven system may instead lock into a ground state and
refuse to leave, which was later observed as a real failure mode
requiring a dedicated escape mechanism.

**Amplitude is not relevance.** High displacement means energy arrived,
which is not the same as being an answer. A highly-connected hub
accumulates energy from every direction; this required a separate
thermodynamic mechanism to suppress ubiquitous participants.

**Cost scales with the field, not the query.** Every tick integrates
every tunnel, so a query costs the same whether it touches three
concepts or three thousand. At large ontology sizes this dominates, and
sparse iteration over only the active wavefront is the necessary and
currently incomplete remedy.

---

## 9. Conclusion

PRM's contribution is the identification of a representation in which
several operations normally requiring algorithms are instead consequences
of propagation: inhibition as phase inversion, conjunction as
interference, and constraint resolution as settling.

The parts that survived are those making no assumption about what the
cavities *denote* — the dynamics, the polarity treatment, the readout.
The part that did not survive assumed cavity parameters encoded external
structure they were never given, which is a useful reminder that a
physical metaphor stops being explanatory exactly where the physics
stops being connected to the domain.

---

## References

- Mahir, I. *MIMS: Mechanical Impedance and Material Stiffness.* — the
  material table parameterising these tunnels.
- Mahir, I. *HNCA: Hierarchical Neural Constraint Architecture.* — the
  macro-layer wrapping these cavities into frequency-separated regions.
- Mahir, I. *RB+G: Gated Recursive Bifurcation with Analogical
  Bridging.* — the mechanism that actually performs analogical discovery.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c.
- Implementation: `src/physics.hpp` (`struct Cavity`, `struct Tunnel`,
  `tick()`).
