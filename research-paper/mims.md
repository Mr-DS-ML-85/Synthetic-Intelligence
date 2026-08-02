# MIMS: Mechanical Impedance and Material Stiffness — Relation Type as a Physical Property

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"You do not write code to filter the frequencies. The physics does
> it for you."*

---

## Abstract

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

## 1. The Idea

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

## 2. The Query as a Driving Frequency

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

## 3. Why No Filter Is Written

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

## 4. Superposition: Simultaneous Constraints

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

## 5. The Structure Being Bound

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

## 6. Correction: The Constants Are Now Derived

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

## 7. Limitations

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

## 8. Conclusion

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

## References

- Mahir, I. *Physical Resonance Mechanics (PRM).* `prm.md` — the
  mass-spring substrate whose tunnels these materials parameterise.
- Mahir, I. *Voltage-Gated Harmonic Topology (VGHT).* — the sub-band
  ladder whose clustering defect §6 reports.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §5.4c — the
  measured correction record.
- Implementation: `src/physics.hpp` (`material_for()`, `k_of`, `c_of`).
