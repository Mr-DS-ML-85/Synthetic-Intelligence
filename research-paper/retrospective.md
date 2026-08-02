# A Retrospective on the DCHL Substrate: Which Mechanisms Survived a Retired Architecture, and Why

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"A dead paper is not a retracted one. Four of the mechanisms running
> in production today belong to papers filed as retired."*

---

## Abstract

Between 2025 and 2026 this lineage produced a family of continuous-state
cognitive architectures — CMA, P-CMA, CCH, HCMA/DCHL, CMC, ANQ,
ZeroGrad, BED, CME — organised around a learned potential $\Phi$ on a
cognitive manifold. **That substrate is retired.** It collapsed into the
failure modes its successor formalises as the classifier, memorisation
and retrieval traps.

This paper is the salvage record: what survived, what did not, and why
the two questions have different answers than the folder structure
suggests. Four mechanisms whose papers are filed as retired are
load-bearing today — **ZeroGrad**'s local Hebbian rule, **ANQ**'s
continuous-integrator pattern, **CMA**'s mutation primitive, and
**LTO**'s runtime acquisition. One constraint that gave the lineage its
name — **CCH**'s curl-free field — was never implemented, although the
*geodesic* half of the same paper found a home in a place its author did
not anticipate.

Negative results are reported as results. HCMA's diagnosis of
hidden-layer collapse under a global scalar teaching signal, and its
observation that a static potential cannot traverse a sequence, were
correct, were derived rather than guessed, and motivated the oscillator
substrate that replaced them.

Every status claim was verified by reading the live source, not the
paper that made it.

---

## 1. Why Salvage Deserves a Paper

A research lineage accumulates claims faster than it retires them.
Papers are written when work begins to succeed and are rarely revisited
when it stops. The result is a literature describing a system nobody
runs.

Two failure modes follow. **Over-claiming**: a synthesis section written
once and never re-checked keeps listing components removed long ago.
**Under-claiming**: a mechanism is buried with the architecture it was
introduced in, even though the mechanism was sound and only the
architecture failed.

The second is the more expensive error. The unit of retirement should be
**the proposal**, not **the idea**.

### 1.1 Method, and a warning it produced

Each status claim was produced by searching live source for the
mechanism, then reading the code to confirm the match was not
incidental. Where paper and source disagreed, the source won.

The method immediately surfaced a hazard worth stating on its own:

> **Two distinct papers in this lineage are both called CMA.**
>
> - `CMA.md` — *Conceptual Mutation Architecture*. **Live**, implemented
>   in `src/bisociation.hpp`, which names the paper in its first line.
> - `failed_but_parts_works/cma.md` — *Cognitive Manifold Architecture*.
>   Retired with the continuous-state family.
>
> An audit performed by filename rather than by content will conclude
> that CMA is retired, which is false for one of the two. This is
> recorded because it is exactly the class of error the audit exists to
> catch, and it nearly escaped.

---

## 2. The Substrate That Was Retired

The retired family shared one commitment: **cognition as motion in a
continuous state space under a learned potential.**

$$
F(c) = -\nabla \Phi(c), \qquad
\dot{c} = \frac{\partial H}{\partial p}, \qquad
\dot{p} = -\frac{\partial H}{\partial c}
$$

A concept was a point $c$ on a manifold; reasoning was a trajectory;
logical consistency was to be enforced by making the field conservative.
Training was contrastive: push $\Phi$ down on coherent states, up on
incoherent ones.

The commitment failed. Sections 3 and 4 separate the parts of the
failure that were *diagnosed correctly* from the mechanisms that were
*worth keeping*.

---

## 3. Negative Results Worth Keeping

### 3.1 Hidden-layer collapse is mathematical, not a tuning bug

HCMA identified that the contrastive Hebbian update for $\Phi(c)$ used a
**global scalar** energy signal applied uniformly to every hidden unit.
Every unit received the same gradient direction, so none could
specialise.

The important part is the modality of the diagnosis:

> These issues are not mere engineering bugs — they are **mathematical
> consequences** of using a single scalar potential both to represent
> "goodness" and to drive generation.

This is the reasoning pattern the successor lineage later formalised as
the difference between a tuning problem and a category error, and it was
applied correctly here first. HCMA's own response — per-unit contrastive
errors $h^{\text{pos}}_j - h^{\text{neg}}_j$, i.e. **decoupling** the
teaching signal — was the right shape, which is why the substrate is
named DCHL.

**Status: the diagnosis survives; the fix did not save the substrate.**
Decoupling removed the collapse and left §3.3 untouched.

### 3.2 A static potential cannot traverse a sequence

A single particle in a static potential falls into the nearest local
minimum and stays. Sequence generation requires either a time-dependent
landscape or an energy-injection mechanism.

This is a correct and general statement about gradient flow, and it is
the direct ancestor of two current mechanisms: driven oscillation — a
cavity is *driven*, not released into a basin — and the chaotic
metastability work that attacks ground-state locking.

**Status: fully survives, restated in oscillator terms.**

### 3.3 Contrastive training on coherence is classifier training

A system trained to separate coherent from incoherent states learns an
energy function that discriminates two classes. At inference the state
descends into the winning class's centroid. Calling the descent
"physics" does not change what was learned.

Measured on the retired substrate at 539 M parameters (D = 512,
H = 1 048 576, 4-bit ANQ, RTX 4060 8 GB): a 4-epoch pass over 8 000
C4/Pile samples in 4 min 43 s, with correctly-signed denoising
$\Delta\Phi$ but hidden activations saturating at $|h| \approx 0.008$ —
deep in tanh's linear regime, traceable to an aug-norm scaling of
$\sqrt{\lVert c \rVert^2 + 1}$ compressing pre-activations by
$\Theta(1/\sqrt{D})$.

A fix was identified (divide by $\sqrt{(\lVert c\rVert^2+1)/D}$) and
**never propagated**, because the substrate was retired rather than
repaired. This is recorded so the number is not mistaken for a live
benchmark.

**Status: this result is the evidence base for retiring the family.** It
is the most useful thing the substrate produced.

---

## 4. Mechanisms That Outlived Their Papers

### 4.1 ZeroGrad — the paper is dead, the rule is foundational

ZeroGrad proposed eliminating backpropagation entirely: each component
trained by a **local, forward-only** objective with Hebbian updates,
extending Forward-Forward-style local learning to the P-CMA components.

The architecture it trained is gone. **The rule is live**, carried
verbatim:

$$
\Delta w_{ab} \;=\; w_{ab}\,\gamma_{\text{decay}} \;+\; \eta\, v_a v_b
$$

Every Hebbian update in the current engine is this rule, including the
phase-consistent extension gating consolidation on
$\cos(\theta_a - \theta_b - \Psi)$. It survived a change of substrate
because it never depended on one: it is local, needs only two adjacent
activations, and assumes nothing about what the state space means.

**Status: LIVE, foundational.**

### 4.2 ANQ — one idea survived the quantisation scheme

ANQ was 4-bit weight quantisation preserving DCHL physics, retired with
the GPU trainer it served. There are no weights left to quantise.

One idea survived: **replace threshold-and-commit with a continuous
integrator.** Rather than accumulating evidence until a threshold fires
and then discretely committing, maintain a saturating accumulator that
never makes a hard decision. Every saturating quantity in the current
tick — salience, tension, endurance, identity, thermal habituation —
follows:

$$
x \;\leftarrow\; \tanh\!\big(x\,\lambda + \eta\,u\big)
$$

The payoff is structural rather than numerical: a system with no
thresholds has no branches, and the current tick loop contains exactly
one branch — a null guard — in its entire update path.

**Status: scheme DEAD, pattern LIVE and pervasive.**

### 4.3 CMA (Conceptual Mutation) — live, and nearly lost to a filename

The Mutation primitive forces a hypothesised edge between two
semantically **distant** nodes, $d(a,b) > \tau_{\text{novelty}}$, then
enters a resolution phase searching for a bridge subgraph that would
explain the connection. If a bridge is found within budget and the
hypothesis energy is below $U_{\text{crit}}$, the edge is adopted;
otherwise the hypothesis decays and is discarded.

The discard path is the load-bearing detail. A mutation mechanism that
kept its failures would inject permanent noise into the graph; because
unvalidated hypotheses decay, the substrate can afford to guess
aggressively.

This is distinct from repurposing search, which looks for candidates
that already *share* a mechanism target. Mutation deliberately forces
pairs that share **nothing** and asks whether a bridge can be invented.

**Status: LIVE** (`src/bisociation.hpp`, which cites `CMA.md` in its
first line). See §1.1 for why this was nearly filed as retired.

### 4.4 LTO — dead binary, live principle

The LTO **trainer** is retired with the DCHL substrate it updated, and
its source now sits in `legacy/`.

The LTO **principle** — acquiring knowledge at runtime and persisting it
so the next boot inherits it — is live in the axiom loader, which cites
`lto.md` §2.2 by name for its streaming-update semantics. The mechanism
changed completely: instead of updating a learned potential online, the
system appends a typed triple to a file. The commitment survived; its
implementation did not.

**Status: architecture DEAD, principle LIVE.**

### 4.5 RIFA — never actually retired

RIFA's Integrity Core and Silence Integration were filed with the
partly-working set, but both are load-bearing:

$$
\text{tension} \leftarrow \text{tension}\cdot\gamma + \beta \textstyle\sum \tfrac12 m v^2,
\qquad
\text{coherence} = \sigma(\text{tension}/2)
$$

Contradiction metabolism and identity persistence run on these. The
filing was clerical, and the paper is duplicated byte-identically in two
locations — itself a symptom of the bookkeeping problem this paper
addresses.

**Status: LIVE. Should never have been in the salvage folder.**

---

## 5. The Instructive Case: CCH, Half Dead

CCH gave the lineage its central theorem. For a conservative field
$\mathbf{F} = -\nabla\Phi$:

$$
\nabla \times \mathbf{F} = 0
\quad\Longrightarrow\quad
\int_A^C \mathbf{F}\cdot d\mathbf{s} \ \text{is path-independent}
$$

The intended reading: **logical transitivity as a geometric guarantee**.
The argument is mathematically correct. It is also **not implemented** —
a search of the live substrate finds no `curl`, no enforcement term, and
no test that the field is conservative. The constraint was stated, never
imposed.

### 5.1 What happened to the transitivity goal

The goal was achieved by **leaving the continuous domain**. Transitivity
is now a typed rewrite rule that fires exactly:

```
(X is_a C) ∧ (C is_a D)  ⟹  (X is_a D)      [premises cited]
```

The derived edge records the two premises that produced it, so
transitivity is not merely guaranteed — it is *auditable*, which the
field formulation never offered. One can print the proof.

> A guarantee you can prove about a mechanism you did not build is worth
> less than a weaker property you can print from the mechanism you did.

### 5.2 What happened to the geodesic framing — the part I got wrong

An earlier version of this retrospective recorded CCH as wholly
unimplemented. That was too strong, and the correction is instructive.

CCH's slogan was *reasoning as geodesic flow on a logical manifold*. The
curl-free half died. **The geodesic half is live**, in a form the
original did not anticipate: the substrate computes a Fisher-information
metric over graph states, and `fisher_distance` measures **geodesic
distance in that metric** between two cavities. For the Boltzmann form
$p(G|\theta) = e^{-\beta E(G,\theta)}/Z(\theta)$, the Fisher metric is
$g_{ij} = \beta^2\,\mathrm{Cov}(\partial E/\partial\theta_i,
\partial E/\partial\theta_j)$, estimated from per-tunnel co-firing
correlations.

The manifold changed identity — from a continuous space of cognitive
states to a **statistical manifold over graph configurations** — and the
geodesic notion transferred intact. Two cavities at small Fisher
distance are statistically indistinguishable, which makes them optimal
analogical bridge candidates.

**Status: curl-free constraint never implemented; geodesic framing LIVE
on a different manifold.** A paper can be half-dead, and recording it as
wholly dead loses a real result.

---

## 6. What Did Not Survive

| Mechanism | Claim | Why it is dead |
| --- | --- | --- |
| **CMC** | $O(1)$ training memory via a Cognitive State Buffer; $O(L \times H) \to O(H)$ | Solves activation storage during **backward passes**. The current engine has no gradient training, so there is nothing to compress. Correct solution, vanished problem. |
| **CMC — reversible dynamics** | reconstruct activations by reversing the Hamiltonian flow instead of storing them | Depends on the dynamics being time-reversible. The current tick is explicitly **dissipative** — damping, Curie habituation, viscosity — so the trajectory cannot be run backwards even in principle. |
| **CMC — zero optimizer state** | no momentum or variance buffers, because ZeroGrad needs none | Not dead; **subsumed**. It is a corollary of §4.1 rather than a separate contribution. |
| **BED** | byte-level output by discrete argmin descent | Converged to the byte-mean; produced no prompt-dependent structure. A measured failure, not a deprecation. |
| **CME** | unified multimodal state without embeddings | Never implemented. Aspirational, and should never have been cited as a component. |
| **SCER** | multi-mode routing at the reasoning layer | Subsumed by field-observable dispatch: the readout mode is chosen by arg-max over physical quantities, not by a router. |
| **P-CMA transitions** | learned transition operator | Subsumed by graph-rewriter transitions. |

CMC is the most interesting entry: **the problem disappeared rather than
the solution failing.** That is a distinct retirement reason worth
recording separately — if gradient training ever returns to this
lineage, CMC becomes relevant again unchanged, whereas BED and CME would
not.

The reversible-dynamics row is the one genuine incompatibility. It is
not that the mechanism is unneeded; it is that the successor substrate
**forecloses** it, because dissipation is doing real work (thermal
habituation is how ubiquitous concepts decouple themselves) and
reversibility would have to be traded for it.

---

## 7. Summary

| Paper | Verdict | What survives |
| --- | --- | --- |
| `zerograd.md` | **paper DEAD, rule LIVE** | local forward-only Hebbian update — foundational |
| `anq.md` | **scheme DEAD, pattern LIVE** | continuous integrator replacing threshold-commit |
| `CMA.md` (Conceptual Mutation) | **LIVE** | forced distant-pair hypothesis with decay on failure |
| `lto.md` | **binary DEAD, principle LIVE** | runtime acquisition with persistence |
| `rifa.md` | **LIVE** | Integrity Core, Silence Integration; misfiled |
| `cch.md` | **half dead** | curl-free never built; geodesic framing live on a statistical manifold |
| `hcma.md` | **retired, diagnosis survives** | collapse-from-global-signal; static potentials cannot sequence |
| `cma.md` (Cognitive Manifold) | **historical** | manifold/Hamiltonian vocabulary |
| `cmc.md`, `cmc_extra.md` | **problem vanished / foreclosed** | intact solution, no gradients left; reversibility incompatible with dissipation |
| `cme.md` | **never implemented** | nothing |
| `bed.md` | **measured failure** | negative result only |
| `scer.md` | **subsumed** | replaced by field-observable dispatch |

---

## 8. Recommendations for Citation

1. **Cite ZeroGrad, ANQ, and LTO as sources of mechanisms, never as
   descriptions of a system.** All three describe architectures that no
   longer exist and rules that are still running.
2. **Disambiguate CMA.** *Conceptual Mutation* is live; *Cognitive
   Manifold* is retired. Citing "CMA" unqualified is ambiguous in this
   lineage.
3. **Do not cite CCH's curl-free theorem as a property of any
   implemented engine.** It is a true theorem about a field that was
   never built. Its geodesic framing may be cited, on the statistical
   manifold.
4. **Cite HCMA and BED for their negative results.** They are the
   empirical basis for retiring an entire architectural family; deleting
   them would remove the evidence for that decision.
5. **Do not cite CME.** Nothing was built.
6. **Re-derive any parts list from source before repeating it.**

---

## 9. Conclusion

The retired substrate failed as an architecture and succeeded as an
experiment. It produced one large negative result — contrastive training
on coherence yields a classifier, at every scale tested — and four
mechanisms that outlived it.

The methodological point generalises past this lineage. Retirement
should apply to proposals rather than ideas, and the only reliable way
to tell which is which is to ask the source code rather than the
literature. When the two disagree, as they did here in six places, the
literature is wrong by construction: it was written earlier.

Two of those six were found only on a second pass — a live mechanism
hidden behind a duplicated acronym, and a paper that was half alive.
Neither would have been caught by reading abstracts.

---

## References

- Mahir, I. *The Cognitive Manifold Architecture (CMA).* — retired.
- Mahir, I. *Conceptual Mutation Architecture (CMA).* — **live**;
  see `src/bisociation.hpp`.
- Mahir, I. *The Cognitive Continuum Hypothesis (CCH).*
- Mahir, I. *Hierarchical Cognitive Manifold Architecture (HCMA/DCHL).*
- Mahir, I. *The Cognitive Manifold Compressor (CMC).*
- Mahir, I. *ZeroGrad Cognitive Engine.* — dead paper, live rule.
- Mahir, I. *Adaptive Neural Quantisation (ANQ).* — dead scheme, live pattern.
- Mahir, I. *LTO: Live Training Over Manifolds.* — dead binary, live principle.
- Mahir, I. *Byte Energy Descent (BED).* — measured failure.
- Mahir, I. *Recursive Integrity Field Architecture (RIFA).* — live.
- Mahir, I. *A Boltzmann Fisher-Information Metric for Graph-Valued
  Cognitive States.* — where CCH's geodesic survived.
- Mahir, I. *From Artificial to Synthetic Intelligence.* §2, §5.5, §6.2.
- Hinton, G. *The Forward-Forward Algorithm*, 2022.
