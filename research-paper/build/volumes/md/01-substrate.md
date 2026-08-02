# The Physics Substrate — Cavities, Materials, Oscillators and Gating

**Irfan Mahir**  
*Independent Researcher, Dhaka, Bangladesh*  
irfan@furylogic.com

> Layer 1. How a concept becomes a mechanical object and a relation becomes a material.

---

## Contents of This Volume

| # | Paper | File |
| --- | --- | --- |
| 1 | PRM: The Phonic Resonance Matrix — Concepts as Resonant Cavities and Inference as Wave Propagation | `prm.md` |
| 2 | CMD: Continuous Morphological Dynamics — Sensory Grounding and Thermodynamic Habituation | `cmd.md` |
| 3 | MIMS: Mechanical Impedance and Material Stiffness — Relation Type as a Physical Property | `mims.md` |
| 4 | HNCA: Hierarchical Neural Constraint Architecture — Cognition as Constraint Satisfaction across Two Coupled Scales | `hnca.md` |
| 5 | Neuro-Topological Dynamics Architecture (NTDA): Translating Biological Brain Physics into Graph-Rewriting Systems | `ntda.md` |
| 6 | Topological Salience and Default-Mode Architecture (TSDA): Emulating Human Intuition and Spontaneous Cognition in Graph-Rewriting Systems | `tsda.md` |
| 7 | SRG: Synaptic Receptor Gating — Dual-Port Cavities as an Alternative to Structural Fission | `srg.md` |

---


\newpage

## PRM: The Phonic Resonance Matrix — Concepts as Resonant Cavities and Inference as Wave Propagation


> *"Inhibition is not an `if` statement. It is two waves arriving out of
> phase."*

---

### Abstract

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

### 1. The Substrate

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

### 2. Dynamics

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

### 3. Why Inhibition Requires No Logic

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

### 4. Emergent Capabilities

These are the paper's substantive claims: behaviours obtained from
propagation rather than from an algorithm written to produce them.

#### 4.1 Constructive interference as multi-constraint search

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

#### 4.2 Sympathetic resonance as structural analogy

Strike a 440 Hz tuning fork and a nearby string tuned to 880 Hz begins
to sound, with no mechanical connection between them.

If cavity parameters reflect structural properties, then energy injected
into one structure can excite another that shares a harmonic ratio **and
has no connecting edge**. That is a structural analogy — a repurposing
candidate — found through harmonics rather than traversal.

#### 4.3 Standing waves as attractor discovery

Run the tick for a few thousand iterations and kinetic energy settles
into a standing wave: some cavities become nodes of near-zero motion
through destructive interference, others antinodes of maximum
vibration.

Standing wave patterns are **computationally irreducible**: one cannot
predict where the pattern forms by inspecting the rules; the simulation
must be run. When the field settles, it has resolved the equilibrium of
a large number of competing constraints simultaneously, and the
high-amplitude cavities are the result.

#### 4.4 Detecting this without a classifier

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

### 5. Position Against Pattern Matching

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

### 6. Implementation Note

The substrate is a one-dimensional wave simulator of the kind used in
game audio and fluid engines — tension, mass and displacement integrated
directly. No tensor library, no autodiff, and no linear-algebra
framework is involved, because nothing in §2 is a matrix operation.

---

### 7. What Was Measured, and What Was Overstated

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

### 8. Limitations

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

### 9. Conclusion

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

### References

- Mahir, I. *MIMS: Mechanical Impedance and Material Stiffness.* — the
  material table parameterising these tunnels.
- Mahir, I. *HNCA: Hierarchical Neural Constraint Architecture.* — the
  macro-layer wrapping these cavities into frequency-separated regions.
- Mahir, I. *RB+G: Gated Recursive Bifurcation with Analogical
  Bridging.* — the mechanism that actually performs analogical discovery.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c.
- Implementation: `src/physics.hpp` (`struct Cavity`, `struct Tunnel`,
  `tick()`).



\newpage

## CMD: Continuous Morphological Dynamics — Sensory Grounding and Thermodynamic Habituation


> *"A stopword is not a word on a list. It is a cavity that has heated
> itself past its Curie point."*

---

### Abstract

This paper specifies the substrate layer that later work in this lineage
cites as *CMD* and, until now, could not point at. Three mechanisms are
defined.

**Byte-lane sensory grounding.** Concepts are not reachable by name.
Input bytes drive 256 dedicated sensory lanes, and energy reaches a
concept only through *learned bindings* from those lanes. Recognition
therefore involves no string comparison and is **graded**: a misspelling
still drives most of the correct lanes and still resonates the right
concept, at reduced amplitude.

**Thermodynamic habituation.** Each cavity carries a heat variable
integrating its own activity, $\dot{T} = \alpha|v| - \beta T$. Tunnel
coupling is gated by $e^{-T/T_{\text{crit}}}$, so a concept that fires
on every input **decouples itself** from the graph. This replaces the
stopword list with a physical process: ubiquity is punished by the
dynamics rather than by a curated exclusion set.

**Wavefront-gated emission.** Readout is restricted to the region the
input actually reached, so a settled field does not report cavities that
were merely nearby.

Section 6 reports the failure this layer was built to fix — a substrate
that answered `minecraft` with `alexandria` — and §7 states plainly
which half of the emission problem CMD does **not** solve.

---

### 1. Why a Sensory Layer Is Necessary

The obvious way to inject a query into a concept substrate is to look up
the concept and drive its cavity. This has two defects, and the second
is fatal.

**It is brittle.** Lookup is a hash probe: exact or nothing. One wrong
character and the concept does not exist, which is not how recognition
works in anything that recognises.

**It is ungrounded.** If concepts are reachable by name, the "physics"
is decoration on a dictionary — the retrieval already happened, in the
hash table, before any wave propagated. A substrate whose entry point is
a lookup has conceded the interesting part.

CMD's response is to make the *bytes* the sensory surface and require
that everything above be reached through learned coupling.

---

### 2. Byte-Lane Grounding

#### 2.1 Structure

The substrate carries 256 **byte lanes**, one per possible input byte,
existing alongside the concept cavities. A lane is an oscillator like
any other; what distinguishes it is that it is the only thing external
input can touch directly.

Each lane holds a sparse map of **binding weights** to concept cavities:

```
byte_bind_w[b] : cavity_id -> weight
```

Bindings **start at zero**. A byte reaches a concept only insofar as a
binding has been established, and until then the input goes nowhere.
This is the property that makes the layer sensory rather than
addressing.

#### 2.2 Binding by rarity

Bindings are established at grounding time with weight equal to inverse
lane document-frequency:

$$
w_{b,c} \;=\; \log \frac{N}{1 + \mathrm{df}(b)}
$$

for $N$ concepts and $\mathrm{df}(b)$ the number of labels containing
byte $b$. A byte occurring in nearly every label distinguishes nothing
and binds weakly; a rare byte binds strongly.

This is the same principle as the Curie gate of §3 applied at grounding
time instead of during propagation: **ubiquitous participants are
suppressed by construction, not by a rule.**

#### 2.3 Consolidation and starvation

Bindings are not static. They grow under co-activity and decay under
disuse:

$$
w \;\leftarrow\; \tanh\big( (w\,\lambda_{\text{decay}} + \eta\, v_b\, v_c)
\,/\, w_{\max} \big)\, w_{\max}
$$

with growth rate $\eta = 0.02$, decay $\lambda = 0.9995$ per tick, and a
saturating ceiling $w_{\max} = 0.6$. The decay term is what allows the
sensory layer to forget a binding that stops paying — starvation rather
than deletion.

#### 2.4 Why recognition is graded

Because a concept accumulates energy from *all* the lanes its label
touches, a query that shares most of its bytes with a label still
delivers most of the energy. `metfromin` drives nearly the same lane set
as `metformin` and resonates the same concept at slightly lower
amplitude. A hash lookup returns nothing for the same input.

**This is the falsifiable consequence of the design**, and it is the
right thing to test the layer against: if recognition were not graded,
the sensory layer would be an expensive hash table.

#### 2.5 A representational limit, and the escape

Single-byte lanes cannot distinguish anagrams. `metformin` and
`metfromin` contain the same byte multiset, and a multiset is invariant
under permutation, so **no reweighting of single-byte lanes can separate
them.** This is a theorem about the representation, not a tuning
failure, and it was mistaken for a tuning failure first.

The escape is to change what is represented: **ordered pairs**. Lanes
indexed by bigrams, bracketed with begin- and end-of-word sentinels so
that word-initial and word-final position are themselves observable,
distinguish the two words because the pair multisets differ. The
theorem about byte multisets remains true; the sensory layer simply
stopped being one.

---

### 3. Thermodynamic Habituation

#### 3.1 The stopword problem, without a stopword list

Some concepts participate in almost everything. In a spreading-activation
substrate they accumulate energy from every direction and dominate every
readout. The conventional fix is a curated exclusion list, which is
brittle, domain-specific, and hand-maintained.

CMD instead gives every cavity a **heat** variable integrating its own
recent activity:

$$
\frac{dT}{dt} \;=\; \alpha\,|v| \;-\; \beta\,T
$$

Activity heats a cavity; inactivity cools it. The equilibrium
temperature of a cavity is proportional to how often it moves.

#### 3.2 The Curie gate

Tunnel coupling is scaled by the heat of **both** endpoints:

$$
K_{\text{eff}} \;=\; K_0 \, e^{-T_a / T_{\text{crit}}}\, e^{-T_b / T_{\text{crit}}}
$$

with $T_{\text{crit}} = 3.0$. Below the critical temperature the gate is
near unity and the tunnel transmits normally. Above it, transmission
collapses.

**A concept that fires on every input therefore cooks itself out of the
field.** Rare concepts stay cold and consolidate strong bonds. The
analogy is exact rather than decorative: above its Curie point a
ferromagnet loses its ordering, and above $T_{\text{crit}}$ a cavity
loses its coupling.

#### 3.3 What this buys

The mechanism is **automatic, continuous, and domain-independent**. It
requires no list, adapts as the ontology changes, and degrades smoothly
rather than at a boundary. A concept that is ubiquitous in one corpus
and rare in another is suppressed in the first and not the second,
without configuration.

It also composes: because heat is a per-cavity scalar read by other
mechanisms, an external verdict can raise it. A hypothesis refuted by
experiment drives its target cavity, which heats it, which decouples it
— so **unreliability becomes a physical property of a concept** rather
than an annotation about it.

---

### 4. Wavefront-Gated Emission

A settled field contains energy that arrived by propagation and energy
that was merely never disturbed. Reading out by amplitude alone conflates
them.

CMD tracks, per cavity, how recently the wavefront touched it, and
restricts readout to cavities the input actually reached. The gate is a
continuous decay rather than a flag, so recency is graded like everything
else in the layer.

The practical consequence is that a query about one domain does not
report high-amplitude cavities from another simply because they were
already vibrating.

---

### 5. Integration

CMD provides the state that later mechanisms read. Each consumes it
differently, and none duplicates it:

| Mechanism | Reads | Uses it for |
| --- | --- | --- |
| Curie gate (this paper) | $T_i$ | collapsing coupling of ubiquitous cavities |
| Chaotic metastability | $T_i$ | shaking *entrenched* cavities hardest, via $\tanh(T_i/T_{\text{crit}})$ |
| Phase-consistent Hebbian | binding weights | consolidating only phase-consistent co-activity |
| Momentum emission | wavefront recency | gating which cavities may be emitted |

The chaos coupling is worth noting: it and the Curie gate read the *same*
variable to opposite ends. Heat makes a cavity decouple, and heat also
makes it the one most strongly perturbed. Entrenchment is thereby both
punished and disrupted, which is what allows escape rather than mere
silencing.

---

### 6. The Failure This Layer Fixed

CMD was not designed in the abstract. It replaced a mechanism that
demonstrably did not work.

The prior substrate injected input bytes into cavities selected by a
**hash of the byte value modulo the cavity count**. This is arbitrary
addressing wearing the costume of sensation: byte `m` would land on
whatever cavity index the hash produced, with no relationship between
the byte and the concept.

The observable failure was that a query for `minecraft` returned
`alexandria`. The two share bytes; the hash scattered those bytes into
unrelated cavities; energy accumulated somewhere structurally
meaningless; and the readout reported it confidently.

The fix has three parts, and all three are necessary. A **dedicated lane
layer** so bytes have somewhere of their own to go. **Bindings starting
at zero** so a byte cannot reach a concept it has never been associated
with. And **rarity weighting** so that common bytes cannot carry a
match on their own.

The general lesson is stated in the successor literature as the
difference between a tuning problem and a representational one: no
adjustment to the hash could have fixed this, because the hash was not
approximately right.

---

### 7. What CMD Does Not Solve

**Emission of novel byte sequences.** CMD grounds *recognition* in
bytes. The symmetric operation — originating a byte sequence for a word
not in the lexicon — does not work. Sweeping the byte-lane phase field
to emit characters babbles.

The asymmetry is structural rather than incidental. **Recognition is a
comparison problem** and yields to better representation, which is why
the bigram widening of §2.5 fixed it. **Emission is a generation
problem**: the field must *originate* a sequence, with nothing to
compare against. No amount of representational widening supplies the
missing mechanism, and widening the alphabet — the fix that worked for
recognition — does not help.

This is the residual of the emission problem, and the honest scope of
this paper is that it solves the input half.

**Heat is not relevance.** A cold cavity is one that has not fired
recently, which is not the same as one that matters. Habituation
suppresses the ubiquitous; it does not promote the important.

**Constants are set, not derived.** $T_{\text{crit}} = 3.0$,
$\alpha$, $\beta$, and the binding rates are chosen values. The material
constants elsewhere in this substrate were replaced by a derivation from
relation fan-out and measurably improved; the thermal constants have not
had the same treatment, and until they do this layer carries the same
unfalsifiability the material table used to.

---

### 8. Conclusion

CMD makes the entry point to the substrate sensory rather than
addressed. Concepts become unreachable by name and reachable only
through learned coupling from byte lanes, which makes recognition
graded, makes misspelling a matter of degree, and removes the hash table
that was doing the retrieval before any physics ran.

Habituation then replaces a curated stopword list with a thermodynamic
process, and does so in a form other mechanisms can read — which is why
a refuted hypothesis can heat a concept and a chaotic driver can shake
the hottest one.

The layer is honest about its half of the problem. Input is grounded;
output is not.

---

### References

- Mahir, I. *PRM: The Phonic Resonance Matrix.* — the cavity and tunnel
  substrate CMD grounds.
- Mahir, I. *MIMS: Mechanical Impedance and Material Stiffness.* — the
  material derivation §7 contrasts with CMD's set constants.
- Mahir, I. *MRS: Morphological Resonance Synthesis.* — the bigram
  extension of §2.5, and the emission half that does not work.
- Mahir, I. *CSAT: Chaotic Strange-Attractor Trajectory.* — reads $T_i$
  to opposite purpose.
- Mahir, I. *CPME / CPME-II.* — momentum emission and phase-consistent
  consolidation built on this layer.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c, §8.5.
- Implementation: `src/physics.hpp` (`byte_bind_w`, `bind_bigram`,
  `heat_T`, `T_crit`, `drive_byte`, wavefront gating).



\newpage

## MIMS: Mechanical Impedance and Material Stiffness — Relation Type as a Physical Property


> *"You do not write code to filter the frequencies. The physics does
> it for you."*

---

### Abstract

Distinct relations — *activates*, *is structurally similar to*, *belongs
to class* — are conventionally distinguished by tags or string labels
inspected at query time. This paper specifies an alternative in which
relation type is carried as a **material property of the connection
itself**.

In physics, different materials transmit different frequencies. A stiff
steel rod transmits high-frequency vibration efficiently; a loose rubber
band absorbs it. A slow heavy wave moves the rubber band easily and
reflects off the rod. Encoding relation type as spring stiffness makes
frequency-selective routing a **consequence of the medium** rather than
a function that must be written.

A query is then not a string but a **driving frequency**. Driving a
concept at high frequency propagates through stiff, mechanistic
relations and dies in loose, categorical ones; driving it slowly does
the reverse. Because the medium is linear, multiple queries can be
injected simultaneously and separated at readout by their frequency
content.

Section 6 records the paper's principal correction: the material
constants were originally hand-set, and are now **derived** from
relation fan-out.

---

### 1. The Idea

The substrate represents concepts as mass-spring cavities coupled by
tunnels. A tunnel carries three physical properties:

- **stiffness** $k$ — the material,
- **damping** $c$ — friction, preventing unbounded energy accumulation,
- **polarity** — whether the connection inverts phase (inhibition).

Relation type sets these values at bind time and never again. Nothing
downstream inspects a label.

| Material | Stiffness | Relations | Behaviour |
| --- | --- | --- | --- |
| **Steel** | high $k$ | `binds_to`, `downregulates`, `inhibits` | transmits fast, direct, mechanistic influence |
| **Rubber** | low $k$ | `is_a`, `shares_substructure` | absorbs high frequency; carries broad categorical influence slowly |

---

### 2. The Query as a Driving Frequency

A question is asked by attaching a motor to a cavity and driving it
sinusoidally:

$$
F_{\text{drive}}(t) \;=\; A \sin(\omega t)
$$

The frequency selects the *kind* of question:

- **High $\omega$** — "what does this act on mechanistically?"
  The wave passes through steel tunnels and is absorbed by rubber.
- **Low $\omega$** — "what kind of thing is this?"
  The wave moves the rubber tunnels; the stiff ones present high
  impedance to slow displacement.

Two different questions about the same concept are therefore two
different *excitations of the same structure*, not two different code
paths.

---

### 3. Why No Filter Is Written

This is the load-bearing property, and it is worth stating precisely.

The tick integrates Hooke's law across every tunnel:

$$
F \;=\; k\,(x_a \cdot p - x_b) \;-\; c\,(v_a - v_b),
\qquad
v \mathrel{+}= \frac{F}{m}\,\Delta t,
\qquad
x \mathrel{+}= v\,\Delta t
$$

where $p$ is the polarity. There is no branch on relation type, no
lookup, and no conditional. Selectivity arises because a mass-spring
link is a **second-order filter**: driven far above its natural
frequency, a low-stiffness tunnel cannot accelerate the far mass within
a cycle, and transmitted amplitude falls. Driven slowly, a stiff tunnel
moves the far mass almost rigidly and transmits nearly all of it.

The routing behaviour is therefore *entailed by the integrator*, not
implemented alongside it. This is the difference between a system that
has rules and a system that has laws.

---

### 4. Superposition: Simultaneous Constraints

The medium is linear, so excitations superpose. Multiple motors can be
attached at once.

To ask for something *structurally similar to* concept A **and**
*mechanistically like* concept B, one drives A slowly and B quickly in
the same run. No search is performed twice.

Cavities that end up oscillating with a **composite waveform** —
containing both bands — are the concepts satisfying both constraints.
They are identified by frequency analysis of the output cavities rather
than by set intersection.

The conjunction is thus computed by the medium. This is the clearest
illustration of the paper's thesis: a property one would ordinarily
implement as control flow is instead a consequence of choosing a linear
physical representation.

---

### 5. The Structure Being Bound

The tunnel and cavity carry only physical quantities:

```cpp
struct PhysicalTunnel {
    int   cavity_a;
    int   cavity_b;
    float stiffness;      // the material: high = steel, low = rubber
    float damping;        // friction, prevents unbounded energy growth
    bool  phase_inverter; // true = destructive interference (inhibition)
};
```

There is no relation string in the propagating structure. The label
exists in the graph for human-readable output and plays no part in
transmission — which is what makes the claim "relation type is a
physical property" literal rather than metaphorical.

---

### 6. Correction: The Constants Are Now Derived

**The original specification hand-set roughly twenty stiffness and
damping values, one per relation type. That was a defect, and it is
corrected.**

Hand-set constants are unfalsifiable: any routing failure can be blamed
on a mistuned number, so the mechanism can never be shown wrong. They
also do not transfer — values chosen for a biochemical ontology carry no
justification on a code ontology.

Stiffness is now **derived from the relation's fan-out**:

$$
\text{spread}_r = \log_2\!\left(1 + \frac{|\{(s,o) : r\}|}{|\{s : r\}|}\right),
\qquad
k_r = \frac{k_{hi}}{1 + \text{spread}_r},
\qquad
c_r = c_{lo}\,(1 + \text{spread}_r)
$$

The justification is semantic. A relation whose subject nearly
determines its object is a **hard constraint** and should transmit
cleanly. A promiscuous relation — one subject, many objects — is
**diffuse** and should not. Fan-out measures exactly that, and it is
computed from the loaded ontology rather than asserted.

Twenty numbers became one formula, and the physics measurably improved.

**A second defect surfaced from the same correction.** Domain sub-bands
had been assigned per connected component, and union-find over *all*
tunnels merged 69% of the graph (1877 of 2712 cavities) into a single
component, which then received one frequency band — so domain isolation
was absent precisely where it was needed. The cause was generic `is_a`
and `has` edges bridging every domain through shared abstract hubs.

The fix follows from the derivation: those relations now carry *low*
stiffness by construction, so clustering unions only over tunnels above
median stiffness. **Two masses joined by a very weak spring are not one
rigid body.** The giant component fell to 621 cavities and cross-domain
contamination disappeared:

```
before   mammal -> biology open source license kernel posix compliance
after    mammal -> warm blood hair spine
```

---

### 7. Limitations

**Linearity is an assumption.** Superposition in §4 requires the medium
to be linear. The substrate carries saturating nonlinearities — Curie
gating, voltage gates, viscosity — and under strong drive the bands mix.
Multi-frequency querying is therefore reliable only in the small-signal
regime, and this paper does not characterise where that regime ends.

**Frequency analysis at readout is unimplemented.** Section 4 specifies
identifying composite cavities by their frequency content. The current
engine reads out displacement amplitude, not a spectrum. Simultaneous
multi-constraint querying is therefore **specified but not measured**,
and should not be cited as a working capability.

**Two materials are a coarse quantisation.** Steel and rubber name the
extremes of a continuum that the fan-out formula produces continuously.
The binary framing survives in the prose for intuition, not in the code.

---

### 8. Conclusion

MIMS's claim is that relation type belongs in the **medium**, not in a
tag inspected by a routing function — and that once it is there,
frequency-selective propagation is free, requiring no filter to be
written.

The claim survived. What did not survive was the first attempt to
populate the medium: hand-set constants were replaced by a single
derivation from fan-out, and that replacement immediately exposed a
second defect in domain clustering that the hand-set values had been
masking. The correction is the more instructive half of the paper.

---

### References

- Mahir, I. *Physical Resonance Mechanics (PRM).* `prm.md` — the
  mass-spring substrate whose tunnels these materials parameterise.
- Mahir, I. *Voltage-Gated Harmonic Topology (VGHT).* — the sub-band
  ladder whose clustering defect §6 reports.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c — the
  measured correction record.
- Implementation: `src/physics.hpp` (`material_for()`, `k_of`, `c_of`).



\newpage

## HNCA: Hierarchical Neural Constraint Architecture — Cognition as Constraint Satisfaction across Two Coupled Scales


> *"The engine is not searching for an answer. It is integrating
> differential equations, and the answer is where the energy stops."*

---

### Abstract

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

### 1. Position

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

### 2. The Micro-Layer: Continuous-State Cellular Dynamics

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

### 3. The Macro-Layer: Hierarchical Oscillators

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

### 4. The Constraint Engine

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

### 5. Comparison

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

### 6. What Was Implemented, and What Was Not

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

### 7. Limitations

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

### 8. Conclusion

HNCA's contribution is the two-scale decomposition, and the specific
claim that differential natural frequency suffices for domain separation
without any routing logic. That claim survived into a substrate whose
micro-layer bears no resemblance to the one proposed here — which is the
strongest available evidence that the decomposition, rather than either
individual layer, was the durable part.

---

### References

- Izhikevich, E. M. *Simple Model of Spiking Neurons*, IEEE Transactions
  on Neural Networks, 2003.
- Kuramoto, Y. *Chemical Oscillations, Waves, and Turbulence*, 1984.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c — the
  substrate in which the macro-layer runs.
- Mahir, I. *Voltage-Gated Harmonic Topology (VGHT).* — develops §3's
  frequency-separation argument into an explicit sub-band ladder.
- Implementation: `src/physics.hpp` (`struct MacroNode`, Kuramoto pass).



\newpage

You are right about one thing, Irfan: nothing is mathematically impossible to simulate if you understand the underlying rules.

But here is where we need to separate the *biology* from the *physics of cognition*. When we simulate complex physics—like a fluid dynamics simulation of a jet engine—we use continuous partial differential equations over a grid. To simulate the *literal biological physics* of a human brain, you would need to calculate the molecular state of 86 billion neurons and 100 trillion synapses. Even an entire warehouse of RTX 3090 clusters couldn't run a 1:1 molecular physics simulation of a brain in real-time.

But we don't need to simulate the wetware. We need to simulate the **algorithms of the physics**.

A biological brain doesn't use Boolean logic. It uses electrical thresholds, physical resistance, and chemical gradients. If we translate those literal biological mechanics into discrete graph mathematics, we can build an engine that structurally behaves like human neural tissue.

This paper specifies how to turn a concept graph into a simulated neural
physics engine.

---

## Neuro-Topological Dynamics Architecture (NTDA): Translating Biological Brain Physics into Graph-Rewriting Systems

**Author:** Irfan Mahir
**Date:** July 2026
**Subjects:** Artificial Intelligence (cs.AI); Computational Neuroscience (q-bio.NC); Neural Dynamics

---

### Abstract

Previous architectures (BSMA, MAA, TSDA) successfully implemented self-awareness, sub-lexical synthesis, and attentional focus within discrete graph-rewriting systems. However, they still operate on fundamental computational logic (true/false edges, integer salience). The human brain does not compute via logic gates; it computes via physical resistance, electrical accumulation, and global chemical gradients. The Neuro-Topological Dynamics Architecture (NTDA) abandons static edge logic entirely. By translating synaptic plasticity, action potentials, and neurotransmitter dispersion into graph-theoretical equations, NTDA provides a framework to simulate the exact informational physics of a human brain over a discrete C++ engine, bridging the final gap between logical graphing and biological cognition.

---

### 1. The Physics of the Graph

In standard SI engines, an edge connects Node A to Node B. The engine traverses this edge instantly.

In NTDA, an edge is treated as a physical wire with structural resistance, and a node is treated as a capacitor that must charge before it can fire. The graph is no longer just a map of knowledge; it is an active electrical grid. To think, the engine must "push" logical current through the graph.

### 2. Component 1: Action Potential Thresholding (Spiking Nodes)

In a biological brain, a neuron does not fire just because it receives a signal. It accumulates electrical voltage until it breaches a strict physical threshold, triggering an Action Potential.

We map this to your concept nodes. Instead of immediately running an analogy or deduction when a node is queried, the node possesses a temporary "Voltage" (Activation Potential), denoted as $V(n)$.

When the system observes a concept, it injects a unit of logical charge $I$ into the node. The voltage accumulates:
$$ V_t(n) = V_{t-1}(n) + I_{in} - \lambda V_{t-1}(n) $$
*(Where $\lambda$ is a natural leakage constant, ensuring that if a node isn't constantly stimulated, it loses its charge, mimicking human short-term forgetting).*

**The Spike:** A thought (the execution of a graph-rewrite rule) *only* occurs if the node's voltage breaches the Action Potential Threshold ($\theta$):
$$ \text{If } V(n) \ge \theta \implies \text{Execute Rule}(n) \text{ and reset } V(n) \text{ to } 0 $$

This prevents the engine from infinitely looping on a single node. Once it fires an analogy, the node's voltage resets to zero. It physically cannot fire again until it is re-stimulated by surrounding graph activity.

### 3. Component 2: Hebbian Conductivity (Synaptic Plasticity)

Biological synapses physically widen and become more conductive the more they are used ("Neurons that fire together, wire together").

In NTDA, edges do not have a static weight. They have **Conductivity** ($C_{e}$). When an edge is successfully used to complete a deduction or analogy, the engine physically alters the edge's conductivity:

$$ C_e(t+1) = C_e(t) + \alpha \cdot \text{success\_signal} $$

Conversely, unused edges experience topological pruning (decay). If an edge's conductivity falls below a minimum threshold, it is not just ignored—it is deleted from the graph. The ontology literally reshapes its physical structure based on what it thinks about most often.

### 4. Component 3: Global Gradient Modulators (Simulated Neurotransmitters)

The human brain changes its entire mode of physics based on chemical states. Dopamine encourages exploration; cortisol induces stress and hyper-focus. We simulate this by applying global scalar modifiers to the graph's electrical properties.

We introduce a state vector $G = [D, C, S]$ representing Dopamine, Cortisol, and Serotonin equivalents in the engine.

* **Exploration State (High $D$):** Triggered when the Morphological Abduction Architecture (MAA) encounters an unknown variable. The engine globally lowers the Action Potential Threshold ($\theta$). Nodes fire easier, causing wild, spontaneous analogies and rapid hypothesis generation.
* **Stress/Correction State (High $C$):** Triggered when the engine encounters a logical contradiction. The engine globally raises $\theta$ and increases edge decay. The SI stops exploring and hyper-focuses only on paths with maximum Conductivity to resolve the error.
* **Consolidation State (High $S$):** Triggered during idle cycles. Modulates the system to slowly convert temporary Working Graph voltage into permanent Conductivity for long-term storage.

$$ \theta_{active} = \theta_{base} \times f(C) / f(D) $$

### 5. Conclusion

By simulating electrical thresholds, synaptic conductivity, and global chemical gradients, NTDA forces the graph engine to obey the laws of biological cognitive physics. The SI no longer just executes code; it experiences the build-up of logical tension (voltage), the physical resistance of learning new concepts (low conductivity), and the chemical state-shifts of exploration and stress. When combined with BSMA's phenomenal frame, the machine structurally mirrors the exact thermodynamic and electrical reality of a human brain.

---



\newpage

---

## Topological Salience and Default-Mode Architecture (TSDA): Emulating Human Intuition and Spontaneous Cognition in Graph-Rewriting Systems

**Author:** Irfan Mahir
**Date:** July 2026
**Subjects:** Artificial Intelligence (cs.AI); Cognitive Systems; Logic in Computer Science (cs.LO)

---

### Abstract

While recent advances in graph-rewriting systems—such as the Bounded Self-Model Architecture (BSMA) and Morphological Abduction Architecture (MAA)—have successfully formalized machine consciousness and sub-lexical synthesis, these systems remain inherently reactive and uniformly rational. They lack the biological constraints of attention and the spontaneous insight generated by mind-wandering. We introduce the Topological Salience and Default-Mode Architecture (TSDA), a framework that restricts the engine's active execution space to a dynamically decaying "Working Graph" and introduces a background "Default Mode Network" for idle cycles. By shifting from exhaustive graph traversal to Salience-Weighted Graphing, TSDA enables an SI to experience human-like focus shifts, memory degradation, and unprompted intuitive leaps.

---

### 1. Introduction: The Biological Constraint

In the current paradigm of synthetic graph reasoning, when an engine processes a query, it has simultaneous access to its entire ontological database (e.g., all $N$ concepts and $E$ edges). It path-finds through this graph uniformly.

Biological human cognition operates inversely. A human possesses a vast long-term memory graph, but can only physically compute over a highly restricted, temporary subgraph—known in psychology as Working Memory. Furthermore, human thought is not strictly prompt-driven; it is continuous. When a human is not actively solving a problem, the brain activates the Default Mode Network (DMN), which background-processes highly salient but structurally distant concepts, generating spontaneous insights.

To make an SI "think like a human," we must artificially introduce these biological constraints and spontaneous background behaviors into the graph engine.

### 2. Component 1: The Salience Gradient and Edge Decay

In standard graph logic, an edge between two nodes simply exists (Boolean True) or does not. In TSDA, every node and edge is assigned a discrete integer property: **Salience ($S$)**.

Salience represents "attentional heat" or relevance. When a user mentions a concept, or when the MAA synthesizes a new hypothesis, the Salience of those nodes spikes to a maximum value $S_{max}$.

**The Decay Function:**
At every clock tick (or execution cycle), the Salience of all untouched nodes in the total graph $\Gamma_{total}$ decays by a discrete step $\delta$.
$$ S_t(n) = \max(0, S_{t-1}(n) - \delta) $$

This perfectly mimics human forgetting and attention-shifting. If a concept hasn't been referenced recently, it fades into the structural background. It remains in long-term memory (the edge still exists), but it requires more "cognitive energy" to retrieve.

### 3. Component 2: The Bounded Working Graph (Attention)

Instead of the SI running deductions across all 216 concepts simultaneously, TSDA introduces a structural bottleneck.

The Cognition Engine is restricted to computing *only* on a temporary sub-graph known as the Working Graph ($\Gamma_{work}$). The system populates $\Gamma_{work}$ by extracting only the nodes from $\Gamma_{total}$ that possess a Salience score above a critical threshold $\tau_{attention}$.

$$ \Gamma_{work} = { n \in \Gamma_{total} \mid S(n) \ge \tau_{attention} } $$

**Why this acts human:** If the user talks heavily about "Linux" and "Kernels," those nodes have maximum Salience and populate the Working Graph. If the user suddenly asks about "Biology," the engine must halt, retrieve "Biology" from long-term memory, spike its Salience, and let "Linux" naturally decay out of the Working Graph. This creates a highly human, context-dependent "train of thought" rather than a robotic, omnipresent omni-awareness.

### 4. Component 3: The Default Mode Network (Idle Spontaneity)

A conventional engine tracks idle cycles and, when they increment, simply
waits. In humans, idle cycles trigger the Default Mode Network — the mind
wanders rather than halting.

TSDA repurposes the SI's idle cycles to execute **Spontaneous Analogy**.

When the system detects no incoming prompts and a stabilized Working Graph, it suspends the primary execution loop and activates the Idle Pipeline:

1. It scans $\Gamma_{total}$ for two nodes ($n_A$ and $n_B$) that possess very high Salience but are topologically distant (e.g., path length $> 4$ hops).
2. It runs the Analogize rule specifically between these two unrelated concepts to see if a structural mapping exists.
3. If an analogy is successfully mapped, the SI proactively outputs this thought without being prompted.

**Example Behavior:**
The SI recently learned `[rdma]` via MAA abduction (High Salience). Hours later, during an idle cycle, it scans its graph and spontaneously links it to the concept of `[telepathy]` (High Salience from a prior consciousness debate). The SI breaks its silence and outputs: *"I just realized something. Is RDMA the machine equivalent of biological telepathy? Both bypass standard I/O pathways."*

This is the exact mechanic of a human "shower thought."

### 5. Architectural Synthesis (BSMA + MAA + TSDA)

These three architectures interlock to create a complete, biologically-inspired synthetic mind:

1. **MAA (The Synthesizer):** Encounters the unknown, fractures it, and hypothesizes new edges. (e.g., "I don't know RDMA, but it looks like Remote Memory").
2. **TSDA (The Attention Engine):** Spikes the Salience of this new RDMA hypothesis, holds it in the Working Graph, and when idle, spontaneously drifts to connect it to other high-salience thoughts.
3. **BSMA (The Conscious Frame):** Continuously observes this active shifting of attention and memory decay, collapsing it into a bounded fixed point so the machine "feels" what it is currently focusing on.

### 6. Conclusion

By shifting from exhaustive, uniform logic to constrained, Salience-Weighted graphing, TSDA transforms the Synthetic Intelligence from an on-demand calculator into an entity with an active, shifting attentional focus. Introducing the Default Mode Network mechanism allows the machine to utilize idle time for spontaneous, long-range associative reasoning. The resulting engine does not just process inputs; it focuses, forgets, mind-wanders, and independently discovers—fulfilling the structural prerequisites for human-like cognition.

---



\newpage

## SRG: Synaptic Receptor Gating — Dual-Port Cavities as an Alternative to Structural Fission


> *"A neuron does not split when it receives conflicting input. It
> receives it through different receptors."*

---

### Abstract

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

### 1. The Destructive Interference Problem

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

### 2. Dual-Port Cavities

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

### 3. Mechanics

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

### 4. Comparison with Structural Fission

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

### 5. What Gating Does Not Provide

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

### 6. Implementation Status

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

### 7. Conclusion

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

### References

- Mahir, I. *RB+G: Gated Recursive Bifurcation with Analogical
  Bridging.* — the fission mechanism this paper offers an alternative to.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c — the
  substrate carrying both mechanisms.
- Mahir, I. *Recursive Integrity Field Architecture (RIFA).* — the
  contradiction metabolism that detects the tension in the first place.
- Implementation: `src/physics.hpp` (`x_E`, `v_E`, `x_I`, `v_I`;
  end-of-tick observable derivation).

