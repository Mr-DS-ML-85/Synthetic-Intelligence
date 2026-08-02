# CMD: Continuous Morphological Dynamics — Sensory Grounding and Thermodynamic Habituation

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"A stopword is not a word on a list. It is a cavity that has heated
> itself past its Curie point."*

---

## Abstract

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

## 1. Why a Sensory Layer Is Necessary

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

## 2. Byte-Lane Grounding

### 2.1 Structure

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

### 2.2 Binding by rarity

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

### 2.3 Consolidation and starvation

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

### 2.4 Why recognition is graded

Because a concept accumulates energy from *all* the lanes its label
touches, a query that shares most of its bytes with a label still
delivers most of the energy. `metfromin` drives nearly the same lane set
as `metformin` and resonates the same concept at slightly lower
amplitude. A hash lookup returns nothing for the same input.

**This is the falsifiable consequence of the design**, and it is the
right thing to test the layer against: if recognition were not graded,
the sensory layer would be an expensive hash table.

### 2.5 A representational limit, and the escape

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

## 3. Thermodynamic Habituation

### 3.1 The stopword problem, without a stopword list

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

### 3.2 The Curie gate

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

### 3.3 What this buys

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

## 4. Wavefront-Gated Emission

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

## 5. Integration

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

## 6. The Failure This Layer Fixed

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

## 7. What CMD Does Not Solve

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

## 8. Conclusion

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

## References

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
