# RB: Resonant Bifurcation — Overcoming Cognitive Interference via Dynamic Topological Fission

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> **STATUS: SUPERSEDED by RB+G** (`rbg.md`), which adds generational
> capping, an alias ledger, energy-conserving fan-out, explicit readout
> and an audit trail, and reports 20-seed reproducibility data. This
> paper is retained as the **precursor record**: it states the original
> mechanism and the reasoning that produced it, without the controls
> that later proved necessary. Cite `rbg.md` for the mechanism as
> implemented; cite this only for priority of the idea.

---

## Abstract

When a concept accumulates contradictory structure, a wave-propagation
substrate representing it as a single cavity suffers destructive
interference: opposed waves cancel inside the cavity and the plasticity
updates they would have produced cancel with them. Conventional systems
respond by re-weighting connections. We propose instead that the engine
**physically splits the node**.

On a sustained high-incoherence, high-tension signal the substrate
executes a structural mutation: it clones the cavity, routes excitatory
tunnels to one child and inhibitory tunnels to the other, and binds the
children with a single high-stiffness tunnel so the system continues to
treat them as one concept while allowing them to oscillate
independently.

The thesis is that intelligence does not only re-weight existing
connections — it **spawns new structure to house concepts its current
structure cannot hold**.

---

## 1. Trigger Condition

The incoherence ratio $I$ is conventionally logged as a failure metric.
Here it is used as a **trigger**.

Fission fires when, over three consecutive structural ticks:

$$
I > 0.15 \quad\text{and}\quad \tau > \tau_{\text{threshold}}
$$

The interpretation is that the concept is **too complex to be
represented by a single point**. It contains contradictions, and its
representation is the wrong shape rather than the wrong value.

Requiring persistence over three ticks distinguishes a structural
condition from a transient: a single noisy tick should not restructure
the graph.

---

## 2. The Mitosis Protocol

When the trigger fires, the engine performs a structural mutation in
three steps.

**Clone.** Create `X_alpha` and `X_beta` from parent concept `X`.

**Polarity routing.** Route every excitatory tunnel (positive
$\Sigma^{+}$) to `X_alpha`, and every inhibitory tunnel (negative
$\Sigma^{-}$) to `X_beta`.

**Concept binding.** Connect `X_alpha` and `X_beta` with a single
ultra-high-stiffness tunnel — a *semantic spring*.

The binding is what prevents fission from being mere deletion of a
concept. The children remain mechanically coupled, so the substrate
still treats them as the same thing, while their displacements are no
longer forced to share one variable.

---

## 3. Why It Works

Before fission, positive and negative waves arrive at one cavity and
interfere destructively. Both the observable displacement and, more
damagingly, the Hebbian velocity product are driven toward zero — so the
tunnels carrying the contradictory evidence are never strengthened.

After fission they enter **two separate resonant cavities** that are
bound together. Taking a worked case:

- `metformin_alpha` accumulates the positive associations — *treats
  diabetes*, *activates AMPK*.
- `metformin_beta` accumulates the negative ones — *causes GI distress*,
  *inhibits complex I*.

Because the semantic spring is stiff, the engine still knows they are
the same entity. Because they are distinct cavities, they oscillate
independently and no longer cancel each other's plasticity.

---

## 4. Why This Is Difficult on Standard Infrastructure

The mechanism requires **dynamic graph topology**, and mainstream deep
learning infrastructure is built to forbid it.

Matrix multiplication requires fixed dimensions. Splitting a node
changes the adjacency matrix from $N \times N$ to
$(N{+}1) \times (N{+}1)$ *during* a run. Frameworks organised around
static tensor shapes and pre-compiled kernels treat that as an error
condition, not a capability.

A localised wave-propagation model written against ordinary memory has
no such constraint: a new cavity is an allocation and a few pointer
updates. **The mechanism is unavailable not because it is a bad idea but
because the dominant substrate cannot express it** — which is a claim
about tooling, not about cognition.

---

## 5. What RB+G Added, and Why It Was Necessary

The mechanism above is incomplete in ways that only appear on contact
with a real ontology. `rbg.md` supplies:

| Addition | Problem in the original |
| --- | --- |
| **Generational cap** | children can themselves fission, without a bound the graph grows without limit |
| **Alias ledger** | after splitting, a query for `metformin` must still resolve; the original specifies no mapping from parent to children |
| **Energy-conserving fan-out** ($1/\sqrt{2}$) | cloning tunnels duplicates energy, injecting it into the field from nothing |
| **Explicit readout** | with children present, what a query *returns* is unspecified above |
| **Audit trail** | a structural mutation with no provenance record cannot be explained or undone |
| **Auto-tuned trigger** | $I > 0.15$ is hand-picked for one ontology; the successor derives it from field energy |
| **20-seed measurement** | this paper reports no measurement at all |

The last row is the substantive one. The mechanism as stated here is a
*proposal*; RB+G is the version that was measured against a
fission-disabled baseline across 20 seeds and four ablation levers.

---

## 6. Conclusion

Resonant Bifurcation's contribution is the reframing: a persistently
incoherent concept indicates that the **representation has the wrong
shape**, and the correct response is to change the structure rather than
the weights. That claim survived into RB+G unchanged.

Everything else in this paper — the specific threshold, the routing
rule, the absence of energy accounting — was superseded within one
iteration. The idea was right and the controls were missing, which is
the ordinary condition of a first draft and the reason this one is
labelled as a precursor rather than presented as a result.

---

## References

- Mahir, I. *RB+G: Gated Recursive Bifurcation with Analogical
  Bridging.* `rbg.md` — **the citable version of this mechanism.**
- Mahir, I. *Synaptic Receptor Gating (SRG).* `srg.md` — the
  static-topology alternative to fission.
- Mahir, I. *Recursive Integrity Field Architecture (RIFA).* — source of
  the incoherence and tension signals used as the trigger.
- Implementation: `src/layer2.hpp` (`fission_locked`).
