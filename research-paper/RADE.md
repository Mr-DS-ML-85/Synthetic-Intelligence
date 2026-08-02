# RADE: The Recursive Axiom-Derivation Engine — Deduction over Prediction in a Persistent Truth Graph

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"Novelty by guessing and novelty by formal expansion look alike from
> the outside. Only one of them can show its work."*

---

## Abstract

A Transformer-style attention mechanism remains a statistical prediction
machine regardless of how its routing is rearranged: what is computed is
a distribution over continuations conditioned on co-occurrence, and
changing the plumbing does not change that. This paper specifies
**RADE**, an architecture organised around **logical synthesis** rather
than probabilistic approximation.

RADE replaces three components at once. Knowledge moves from
floating-point weights into an **Axiom Substrate** — a persistent graph
of verified relations. Inference moves from a forward pass to a **proof
pass** over that graph. And termination moves from a fixed depth to
**homeostatic inhibition**, a resource budget whose exhaustion triggers
a lateral switch to a different region of the graph rather than an
unbounded search.

The consequence that matters is a change of failure mode. A predictive
system fails by producing fluent falsehood, which is undetectable from
the output alone. RADE fails by **logical inconsistency**, which is
detectable, localisable, and attributable to specific premises.

---

## 1. The Problem with Rearranged Plumbing

Architectural variation within the predictive family — sparse attention,
linear attention, state-space substitutions, mixture routing — changes
the cost of computing $P(w_n \mid w_1 \ldots w_{n-1})$ without changing
what that quantity is. A count of how often one symbol followed another
is not an account of *why* it did, and no rearrangement of the machinery
that computes the count converts it into one.

This is not an argument that prediction is useless. It is an argument
about what a system can be asked for. A predictive system can be asked
for a continuation. It cannot be asked for a *derivation*, because it
does not have one to give — the output was not produced by steps that
could be exhibited.

RADE is specified to answer the second kind of request.

---

## 2. The Axiom Substrate

### 2.1 Knowledge as structure, not as parameters

In a Transformer, knowledge is distributed across floating-point weights
and cannot be inspected, edited, or deleted at the granularity of a
fact. In RADE it is held explicitly:

- **Formal objects** $O$ — concepts, as graph vertices.
- **Operators** $\Phi$ — typed relations, as directed edges.

The distinction from embeddings is persistence semantics. An embedding
degrades under continued training: the representation of a concept
drifts as unrelated updates accumulate. A graph edge does not. If the
substrate holds `A is_a B`, it holds it until something explicitly
removes it, and the removal is an event that can be logged.

### 2.2 Why this matters for correction

A predictive model cannot be told that one specific thing it believes is
wrong. The belief is not localised, so correction is retraining, and
retraining perturbs everything else.

In a truth graph, a wrong fact is an edge. Removing it is $O(1)$, and
the consequences of removing it are computable: every derived edge
citing it as a premise becomes unsupported and can be retracted by
following the provenance chain. Correction is therefore *bounded* rather
than global.

---

## 3. The Proof Pass

The forward pass is replaced by a traversal.

**Input.** A query — for example, *why is X a superconductor?*

**Operation.** The engine does not estimate an answer. It searches the
graph for a path connecting the queried concepts through typed
operators. If a path exists, the path *is* the answer, and every edge
along it is either an asserted premise or a derived edge that names the
two premises that produced it.

**When no path exists.** The engine enters a **search-and-constrain**
phase: it proposes a candidate link and tests it against the existing
axioms. A candidate that contradicts a held axiom is rejected. A
candidate that survives is committed as a derived edge, with its
provenance recorded.

This is the distinction the paper turns on:

> **Novelty by guessing** produces a plausible string.
> **Novelty by formal expansion** produces an edge that survived a
> consistency test and can name what it rests on.

Both look like new output. Only the second can be audited, and only the
second can be retracted cleanly when its support is withdrawn.

---

## 4. Homeostatic Inhibition

An unbounded proof search does not terminate on a large graph. The
biological analogue of the fix is inhibitory interneurons, which prevent
excitation from propagating without limit.

**Resource budget.** Every search path is allocated a bounded budget.

**Lateral inhibition.** A path that yields no result within its budget
is abandoned, and the search is displaced to a different region of the
graph rather than deepened.

The behavioural effect is the ability to *stop pursuing an unproductive
line*, which is a capability rather than a limitation: a system that
cannot abandon a path cannot allocate effort. This mechanism is the
direct ancestor of the tension-bounded construction budget in the
successor architecture, where the budget is derived from a physical
observable rather than set as a constant.

---

## 5. Comparison

| Property | Transformer | RADE |
| --- | --- | --- |
| Foundation | statistical probabilities | logical axioms |
| Inference | forward pass | proof pass over a graph |
| Novelty | interpolation, indistinguishable from hallucination | formal expansion, consistency-tested |
| Memory | lossy KV cache | persistent graph of hard relations |
| Correction | retraining, global | edge removal, bounded by provenance |
| Failure mode | fluent falsehood, **undetectable from output** | logical inconsistency, **detectable and localisable** |
| Objective | minimise prediction error | maximise logical consistency |

The failure-mode row carries the argument. A system whose errors are
detectable can be improved by an operator who does not have access to
its internals. A system whose errors are fluent cannot.

---

## 6. Ingestion: The Axiom-ifier

A corpus is not knowledge until it is structured. RADE separates
**reading** from **reasoning**: an ingestion stage converts source
material into structured triples, and the reasoning engine never touches
raw text.

The ingestion stage emits only triples that the substrate can type-check
against its operator set. Anything it cannot structure is dropped rather
than approximated — a coverage gap is preferable to a fabricated
relation, because a gap is visible and a fabrication is not.

The successor architecture implements exactly this split: mechanical AST
extraction produces triples from source, and every fact carries a
provenance class distinguishing what was asserted from what was derived
and what was established by experiment.

---

## 7. Status and Relation to the Implemented System

RADE is an **architectural proposal**, written before the substrate that
now exists. Its components map onto implemented mechanisms as follows:

| RADE component | Implemented as | Status |
| --- | --- | --- |
| Axiom Substrate | typed concept graph, `data/*.axioms` | **live** |
| Proof pass | forward chaining with premise citation | **live** — derived edges name their parents |
| Search-and-constrain | contradiction check before assertion | **live** |
| Resource budget | tension-bounded construction budget | **live**, now derived from field energy |
| Lateral inhibition | budget exhaustion ends a derivation chain | **partial** — no region-switching heuristic |
| Axiom-ifier | mechanical AST extraction | **live**, no ML |

The one component not carried across is the explicit region-switching
behaviour of §4. The current engine exhausts a budget and stops; it does
not redirect to a different subgraph. Whether that redirection is worth
implementing is open, and it should not be reported as present.

---

## 8. Limitations

**Ingestion is the bottleneck, not reasoning.** The proof pass is cheap;
converting unstructured source into typed triples is not. The quality
ceiling of the whole system is the quality of that conversion, and this
paper does not solve it.

**Consistency is not truth.** The search-and-constrain phase rejects
candidates that contradict held axioms. It cannot reject a candidate
that is consistent with a set of axioms that are themselves wrong.
Consistency is a necessary condition and nothing more.

**Coverage cliff.** A query whose concepts are absent from the substrate
returns nothing. This is the correct behaviour and it is also a visible
limitation; the honest presentation is an explicit gap rather than a
degraded guess.

---

## 9. Conclusion

RADE's claim is narrow and, stated carefully, defensible: an
architecture that stores relations rather than parameters, infers by
traversal rather than by prediction, and bounds its search by an
explicit budget can answer for its outputs in a way a predictive system
structurally cannot.

It does not claim to reason better. It claims to reason *auditably* —
and to fail in a way that can be seen.

---

## References

- Mahir, I. *From Artificial to Synthetic Intelligence: Principles of
  Constructive Structural Cognition.* — the tri-layer architecture in
  which RADE's components are implemented.
- Mahir, I. *JAS: Einstein's Epistemology as a Running Architecture.* —
  the provenance classes that make retraction bounded.
- Implementation: `src/rade.hpp`.
