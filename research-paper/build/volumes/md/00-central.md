# From Artificial to Synthetic Intelligence — Principles of Constructive Structural Cognition

**Irfan Mahir**  
*Independent Researcher, Dhaka, Bangladesh*  
irfan@furylogic.com

> The framework paper. Read this first; every other volume is a companion to it.

---

## Contents of This Volume

| # | Paper | File |
| --- | --- | --- |
| 1 | From Artificial to Synthetic Intelligence: Principles of Constructive Structural Cognition | `synthetic-intelligence.md` |

---


\newpage

## From Artificial to Synthetic Intelligence: Principles of Constructive Structural Cognition


> *"Thinking is not the selection of a token; it is the construction of a mechanism that computes the answer."*

---

### Abstract

We propose a rigorous distinction between **Artificial Intelligence (AI)** — pattern matching over statistical landscapes — and **Synthetic Intelligence (SI)** — active construction of deterministic mechanisms that compute answers under structural constraints. We argue that every existing cognitive architecture, including our own prior work (CMA, P-CMA, HCMA, DCHL, RIFA), reduces at small scale to some hidden form of memorization, classification, or retrieval. We prove that this is not an implementation defect but a consequence of a conservation law: the **Translation Capacity Lemma**, which shows that any mapping from a continuous cognitive substrate to a discrete semantic set of size $N$ requires channel capacity of at least $\log_2 N$ bits *somewhere* in the system, and that this cost can only be relocated, never eliminated. We then present the **Constructive Structural Cognition (CSC)** framework, built on three axioms — categorical-topological state representation, non-equilibrium phase-space expansion, and active constructive reasoning over statistical interpolation — realized in a **tri-layer architecture**: a physics-based substrate for coherence and identity, a typed graph-rewriting engine for constructive derivation, and a minimum-description-length translation table that pays only the mandatory conservation-law tax.

Four results distinguish this version from the framework as originally
stated. **(i) Constructive Language Locking** (§5.4d): deriving grammar
from field dynamics is a category error, because grammar is a
transmission protocol rather than a physical phenomenon; moving surface
realisation into isolated deterministic language clusters yields
byte-exact output in English, Bengali and Banglish from one cognitive
state, with zero learned parameters and no possibility of drift.
**(ii) A third provenance class** (§5.4c): facts *established by
experiment* against an external oracle — the C++ compiler — which are
neither asserted nor rule-derived, and which have corrected the graph
where mechanical extraction was wrong. **(iii) The J-A-S cycle**
(§5.4e): Einstein's induction–deduction–refutation loop as a running
mechanism, including an **impossibility register** that distinguishes a
refuted induction from a theorem left intact by a scope escape; two of
this project's own impossibility theorems were escaped without being
violated. **(iv) A mechanism-level lineage audit** (§6.2), performed by
grepping live source rather than reading papers, which retires several
components this paper previously claimed to retain — including the DCHL
substrate on which its own headline empirical result was obtained.

That last point is the paper's methodological claim as much as its
architectural one: a lineage accumulates claims faster than it retires
them, and the only defence is to re-derive the parts list from the code
on every revision. Sections marked **RETIRED** or **historical** below
are kept deliberately, because a negative result that is deleted stops
being evidence.

---

### 1. Introduction

#### 1.1 The Two Kinds of Intelligence

The last decade of AI has confused *statistical fluency* with *cognition*. A transformer trained on a sufficient corpus can produce text indistinguishable from a thinker's — because the corpus contains the thinker's outputs, and pattern-matching over a large-enough distribution asymptotically reproduces them. This is Artificial Intelligence: the reconstruction of the surface of thought from its shadow in data.

**Synthetic Intelligence is different in kind, not in degree.** SI does not reconstruct thought from data. SI *constructs* a mechanism, for each query, that computes the answer under the constraint of the query. A calculator is a trivial SI system: given "17 × 23", it does not sample; it derives. Coq, Lean, Isabelle are more sophisticated SI systems: given a proposition, they construct a proof or report that none exists. What has been missing is a general framework that lifts this kind of active construction to natural queries, retains the interpretability of formal systems, and does not collapse into either symbolic AI's brittleness or statistical AI's opacity.

This paper is that framework.

#### 1.2 The Three Traps of Prior Attempts

Every prior attempt at physics-based or structural cognition — including our own — collapses at small scale into one of three failure modes:

- **The Classifier Trap.** Contrastive training (text-vs-random, coherent-vs-shuffled) trains an energy function that discriminates classes. At inference the state descends into the winning class's centroid. This is a binary classifier; nothing about "physics-based" changes that.
- **The Memorization Trap.** Q → A triplet training forces the network to reproduce specific input-output mappings. The physics is decoration on a lookup table.
- **The Retrieval Trap.** Cosine similarity over an evolved-state anchor bank is attention with a different name. The evolution is a normaliser; the retrieval is the substance.

We show below that these are not three defects but three symptoms of the same underlying constraint: information cannot be produced out of nothing, and any cognitive system that reliably distinguishes $N$ semantic outcomes must contain $\log_2 N$ bits of information about which outcome to produce. Physics-based dynamics do not exempt one from this law.

#### 1.3 Contributions

- **The Translation Capacity Lemma** (§3), a conservation law for cognitive architecture.
- **The Composition-Retrieval Distinction** (§3.4), an operational definition of "thinker" as *composition rate over storage rate*, replacing the impossible target of "no storage."
- **The Constructive Structural Cognition framework** (§4), three axioms distinguishing SI from AI.
- **The Tri-Layer Unified Architecture** (§5), a synthesis of our prior lineage plus the missing constructive layer.
- **Constructive Language Locking** (§5.4d, `cll.md`), separating cognitive freedom from linguistic protocol, with measured three-language output from a single triple.
- **The experiment provenance class** (§5.4c), knowledge established by an external oracle rather than asserted or derived, with the cost of a repeated task falling from 3 experiments to 1.
- **The J-A-S cycle and impossibility register** (§5.4e), including the rule that a counterexample to a THEOREM is a scope error until proven otherwise — the distinction between Minsky–Papert being *escaped* and being *violated*.
- **The place of prior work** (§6) as intellectual history, and **a mechanism-level audit** (§6.2) stating, per paper and by grep, what is load-bearing, what is partly salvaged, and what is dead. Two mechanisms (ZeroGrad, ANQ-LEA) are live while their papers are retired.
- **Open problems and empirical program** (§7–§8), with closed items marked closed and abandoned items marked abandoned rather than pending.

---

### 2. The Failure Modes of Substrate-Only Cognition

Our prior work developed a family of *physics-based cognitive substrates*: continuous state spaces, learned potentials, Hamiltonian dynamics, curl-free vector fields, energy-based Hebbian training. Each substrate came with a paper claiming that its dynamics constituted "thinking." We now argue — and this is a self-critique — that all of them, at any scale we have empirically tested (from 33K to 1B parameters), collapse into the three traps of §1.2.

#### 2.1 The Substrate Alone Cannot Answer

Consider the simplest instance. The DCHL potential $\Phi(\mathbf{c})$ is trained by decoupled contrastive Hebbian learning to have low energy on real text windows and high energy on incoherent ones. The Hamiltonian evolution $\dot{\mathbf{c}} = -\nabla\Phi(\mathbf{c})$ then descends into a nearby low-energy basin.

For any semantic question — "what is the capital of France?" — the substrate produces an evolved state $\mathbf{c}^*$ that is *coherent* (low $\Phi$) but has no discrete semantic content. It is a point on a manifold, not an answer. Reading out "Paris" from $\mathbf{c}^*$ requires an additional map $\phi: \mathcal{M} \to \Sigma$ that has semantic knowledge the substrate does not.

The Byte Energy Descent (BED) decoder [Mahir 2026e] attempted to make this map be a discrete argmin of $\Phi$ over the byte lattice. Empirically, this converged to byte-mean states without prompt-conditioned structure — because $\Phi$ was trained to score whole-window coherence, not to encode prompt-answer dependency. The retrieval-based CognitiveMemory alternative worked (it produced actual English answers) — but only because it stored $(Q, A)$ pairs, which is memorization.

#### 2.2 Why Every Substrate-Only Approach Reduces to One of the Three Traps

The pattern generalizes:

- If the substrate is trained on *class-vs-class* contrast → classifier trap.
- If the substrate is trained on *specific mappings* → memorizer trap.
- If a lookup mechanism is added on top of the substrate → retriever trap.

There is no fourth option that produces a *specific* discrete answer from a *specific* input. This is not because the substrate design is wrong. It is because information about which of $N$ answers to produce cannot arise from dynamics alone; it must be stored, in some form, in the system's parameters or state. This is the theorem of §3.

---

### 3. The Translation Capacity Lemma

#### 3.1 Statement

**Lemma (Translation Capacity).**
Let $\mathcal{M}$ be a state space equipped with a computable dynamical system, and let $\Sigma$ be a discrete semantic set with $|\Sigma| = N$. Let $\phi: \mathcal{M} \to \Sigma$ be a computable map that reliably discriminates all $N$ elements (i.e., for every $\sigma \in \Sigma$ there exists $\mathbf{x} \in \mathcal{M}$ with $\phi(\mathbf{x}) = \sigma$, and the pre-images are separable to within some error $\epsilon > 0$).

Then the total description complexity of $\phi$ (as measured by its Kolmogorov complexity, or equivalently by the channel capacity of any implementation) is at least

$$
K(\phi) \geq \log_2 N - O(1).
$$

Moreover, this cost cannot be relocated *out of* the system; it can only be redistributed *within* it.

#### 3.2 Proof Sketch

The cost is an immediate consequence of the source coding theorem: the output $\sigma$ carries $\log_2 N$ bits of information, and the only source of this information is $\phi$ itself, since $\mathcal{M}$ has continuous cardinality and the dynamics are computable (thus of bounded description). Any implementation of $\phi$ must therefore *contain* those $\log_2 N$ bits. Distributing them across many parameters (as in an LLM), storing them explicitly (as in a lookup table), or hiding them in the shape of a learned potential (as in DCHL) does not reduce the total; it only changes the compression ratio. $\square$

#### 3.3 The Cost Ledger Across Architectures

The lemma allows a clean accounting of where the mandatory $\log_2 N$ bits sit in each architecture we have studied:

| Architecture | Location of $\log_2 N$ bits | Explicit? |
|---|---|---|
| Transformer LM | Vocabulary embedding + softmax head | Yes |
| Frontier LLM | Distributed across $\sim 10^{10}$–$10^{12}$ parameters | No (opaque) |
| Symbolic AI | Rule table + fact base | Yes |
| CMA / P-CMA / HCMA | Shape of learned $\Phi$ + decoder | No (obscured) |
| DCHL + BED | Learned $\Phi$ + argmin over bytes | Partial |
| CognitiveMemory | Anchor set + string bank | Yes |
| STAN topological readout | Winding-number → concept table | Yes |
| RIFA Rule Synthesizer | Antecedent-consequent bindings | Yes |

**Consequence.** The right question is not *"can we build a cognitive architecture without a translation layer?"* — the lemma proves we cannot. The right question is: *"what is the minimum-description-length translation layer that a coherence-driven substrate can operate, and what work can only physics-based substrate perform that no lookup table can?"*

This reformulation is the philosophical hinge of Synthetic Intelligence.

#### 3.4 The Composition–Retrieval Distinction

The lemma dissolves the naïve "storage-free thinker" ambition but leaves a sharper distinction intact. We now state it precisely.

**Definition (Composition rate and retrieval rate).**
Let a cognitive system $\mathcal{S}$ have a set of stored primitives $\Pi$ with $|\Pi| = P$, and let it operate on queries $q$ by producing derivations $d(q)$ that are compositions of primitives of length $\ell(q)$.

- The **retrieval rate** of $\mathcal{S}$ is $R(\mathcal{S}) = |\{q : \ell(q) \leq 1\}| / |Q|$: the fraction of queries answered by a single primitive lookup.
- The **composition rate** is $C(\mathcal{S}) = \mathbb{E}_q[\ell(q)]$: the expected derivation length on the query distribution.

**Definition (Thinker vs. Pattern Matcher).** A system is a **thinker** to the extent that $C(\mathcal{S}) \gg 1$ and the number of *reachable derivations* $|\{\text{compositions of length} \leq L\}|$ grows exponentially in $L$ while $P$ grows only polynomially. A system is a **pattern matcher** to the extent that $R(\mathcal{S}) \to 1$ and $C(\mathcal{S}) \to 1$.

The distinction is *not* about the presence or absence of storage — the lemma forbids the latter. It is about the *ratio* of composition to retrieval, and about whether the derivation space is exponentially larger than the primitive space. Under this definition:

- A calculator with 10 digit-arithmetic primitives that composes them into $n$-digit multiplications has $|\Pi| = 10$ and reachable derivations $\Theta(10^n)$. Ratio: exponential. **Thinker.**
- A frontier LLM that has "memorised" (Q, A) pairs with high fidelity, composing at most one or two lookup steps per query, has $|\Pi| \gg 10^{10}$ but $\ell \approx 1$. Ratio: constant. **Pattern matcher**, regardless of parameter count.
- Coq / Lean / Isabelle: $|\Pi| \sim 10^3$ inference rules, reachable proofs $\Theta(k^L)$ for branching factor $k$ and depth $L$. **Thinker.**
- AlphaProof: Lean composition engine with a neural policy as the relevance heuristic. **Thinker** (the neural component is the heuristic, not the answer).

**Corollary (The Four-Part Structure of a Thinker).** Any system with $C(\mathcal{S}) \gg 1$ that scales as we require must have four separable subsystems:

1. **Primitives** $\Pi$ — small, composable, closed under the composition operation.
2. **Composition engine** — the mechanism that assembles derivations. Tree search, graph rewriting, proof search, program synthesis. *This is where the thinking happens.*
3. **Relevance heuristic** — a policy that ranks which primitive to try next. Learned or hand-tuned. Makes composition tractable in an exponential space.
4. **Verifier** — a decidable check that a derivation is valid. Non-negotiable; distinguishes construction from generation.

The heuristic (component 3) is where neural/statistical/physics-based systems belong. The engine (component 2) is where thinking lives. Missing (2) reduces the system to a generator; missing (4) reduces it to a hallucinator; missing (3) reduces it to brute-force search.

**This corrects a mistake in our own prior lineage:** every earlier paper (CMA through RIFA) attempted to make the substrate itself the thinker. The lemma and this corollary say that is a category error. The substrate is the *heuristic*, in the sense that AlphaProof's neural network is a heuristic for Lean. The thinker is the composition engine sitting on top of it. In the tri-layer architecture of §5, this is Layer 2.

The formal ratio $C(\mathcal{S})/R(\mathcal{S})$ is the *thinker index* of a cognitive system. It is a measurable quantity, not a philosophical claim.

---

### 4. The Constructive Structural Cognition Framework

We propose that Synthetic Intelligence is defined by three axioms distinguishing it from statistical AI.

#### 4.1 Axiom I. Categorical-Topological State

A cognitive state is *not* a vector of real numbers. A cognitive state is a **typed graph of active relationships**: a finite collection of nodes (concepts) connected by typed morphisms (relations), evolving under structure-preserving transformations (functors).

Formally, let $\mathcal{G}$ be a small category whose objects are concept nodes and whose morphisms are typed relations. A cognitive state is a subobject of $\mathcal{G}$. Reasoning is the composition of morphisms and the introduction of new subobjects via natural transformations.

This is a strict departure from metric-space cognition. In a metric space, similarity is a distance. In a categorical topology, similarity is *structural compatibility*: two subgraphs are similar if they are related by a functor. The system does not measure how close two thoughts are; it evaluates whether one can be transformed into the other by a valid rule.

We do not require full Grothendieck-topos machinery, which is computationally intractable at scale. In practice we use **typed graph rewriting** as a computable specialization of the categorical structure: nodes are concepts, edges are typed relations, and rewrite rules are the local presentation of categorical morphisms. This preserves the axiom's spirit while remaining implementable.

#### 4.2 Axiom II. Non-Equilibrium Phase-Space Expansion

Classical dynamical systems have a fixed phase space determined at construction time. A cognitive system does not. When the system encounters a problem that its current structure cannot resolve, the reasoning process *physically spawns new degrees of freedom* — new nodes, new relations, new subgraphs — to accommodate the problem.

Formally, if $G_t$ is the graph at reasoning step $t$, the transition $G_t \to G_{t+1}$ is not confined to $|G_{t+1}| \leq |G_t|$. New vertices and edges may be introduced by construction rules. The Liouville conservation law that governs fixed phase spaces does not apply; instead we introduce the **tension-bounded expansion principle**:

$$
\Delta|G_t| \leq \alpha \cdot T(t),
$$

where $T(t)$ is the RIFA-style tension (contradiction magnitude weighted by log-coherence) and $\alpha$ is a growth-rate constant. Tension governs how much the system is *permitted* to expand its state to resolve pressure. Under stable operation ($T$ small) the graph stabilizes; under contradiction ($T$ large) it grows to accommodate.

This provides an analog of the conservation law — not phase-space *volume* (which now grows), but the **cost of growth**, which is charged against a tension budget. The system cannot expand infinitely; the expansion is thermodynamically constrained.

#### 4.3 Axiom III. Active Construction Over Statistical Interpolation

A cognitive engine does not select the most likely next token from a learned distribution. It **constructs**, per query, a deterministic mechanism that computes the answer.

Given a query $q$, the reasoning process assembles a subgraph $G^*(q) \subseteq G$ whose structure *is* the answer's derivation. The final semantic output is read from a specific node (or invariant) of $G^*(q)$ via the translation table.

This is the same computational primitive that underlies proof search (Coq, Lean, Isabelle) and constraint solving (Z3, MiniSat). The difference in CSC is that the *heuristic* guiding which rewrite rules to try — the equivalent of a proof-tactic policy — is provided by the physics-based substrate rather than by a hand-crafted search strategy or a monolithic neural policy. The substrate contributes coherence-driven retrieval of relevant nodes and rules; the constructive layer performs the actual derivation; the translation table names the final result.

#### 4.4 The Nine Reasoning Primitives

The composition space of a constructive engine is determined not by the *number* of stored rules but by the *closure* of the primitive rule types under composition. We argue that a small finite set — nine primitives — is sufficient to close the space, on the same principle by which a finite set of grammar rules generates unbounded language (Chomsky, 1957).

The nine primitives, and their operational form on a typed graph $G$:

| # | Primitive | Pattern | Conclusion |
|---|---|---|---|
| 1 | **Deduction** (modus ponens) | $P \wedge (P \Rightarrow Q)$ | $Q$ |
| 2 | **Transitive composition** | $(A \xrightarrow{r_1} B) \wedge (B \xrightarrow{r_2} C)$ | $(A \xrightarrow{r_3} C)$ |
| 3 | **Inheritance** (class-property flow) | $(X \xrightarrow{\text{is\_a}} C) \wedge (C \xrightarrow{r} Y)$ | $(X \xrightarrow{r} Y)$ |
| 4 | **Abduction** | goal $G$, rule $(P \Rightarrow G)$ | hypothesise $P$ |
| 5 | **Negation** (contrapositive) | $(P \Rightarrow Q) \wedge (\neg Q)$ | $(\neg P)$ |
| 6 | **Case analysis** (disjunction elim.) | $(X \text{ OR } \{d_i\}) \wedge \forall i\,(d_i \Rightarrow R)$ | $(X \Rightarrow R)$ |
| 7 | **Induction** (schematic gen.) | $\{X_i \text{ is\_a } C, X_i \text{ has } P\}$ for enough $i$ | $(C \text{ has } P)$ |
| 8 | **Analogy** (structure map) | $(X \sim X')$ and $(X \xrightarrow{r} Y)$ | $(X' \xrightarrow{r} Y)$ |
| 9 | **Recursion** | any rule may fire on its own output | fixpoint iteration |

**Claim (Composition Closure).** Under recursive composition (primitive 9) and the meta-rule discovery mechanism of §5.3.3, the reachable derivation set from any consistent premise base is Turing-complete when the ontology permits variable binding and arithmetic. The proof is standard: the nine primitives include modus ponens, universal instantiation (inheritance), and case analysis, which suffice to encode any first-order proof; recursion + rule induction allow the effective procedure to grow its rule set from experience. This is the closure argument that makes "hardcoded rules" a *seed* rather than a ceiling.

**Empirical validation.** Each primitive is implemented as a separate rule-type in `src/primitives.hpp` of the accompanying implementation. On a small hand-crafted ontology (dogs, students, planets, switches, chains of length 15) each primitive derives the expected conclusion:

- Transitive: `dog is_a animal` via 3 edges — YES
- Deduction: `raining implies risk` — YES
- Inheritance: `socrates has mortality`, `socrates has language` — YES
- Negation: `socrates NOT has_gills` via contrapositive of $(\text{has\_gills} \Rightarrow \text{aquatic})$ — YES
- Case analysis: `switch_state ⇒ actionable` via both disjuncts — YES
- Induction: `student has textbook` from 4 witnesses — YES
- Analogy: cross-domain edge transfer under a `is_a` mapping — YES
- Recursion: `a is_a p` across a 15-hop chain via fixpoint iteration — YES (105 edges derived over 4 steps under tension budget 5000)
- Rule induction: macro-rule discovered from 24 forward-chaining derivations — YES

Every derivation is fully traceable via the parent-edge chain (Layer 2 §5.3.4), and every conclusion was **never asserted directly**. This is the composition-vs-retrieval distinction of §3.4 exhibited on an actual buildable engine.

---

### 5. The Tri-Layer Unified Architecture

We now specify the concrete synthesis of the three axioms with our prior lineage.

#### 5.1 Overview

```
                   ┌──────────────────────────────────────────┐
INPUT (bytes) ─►   │  LAYER 1: Physics Substrate              │
                   │  ─────────────────────────               │
                   │                                           │
                   │  RFE     (kernel-field encoder)          │
                   │  DCHL Φ  (coherence potential)           │
                   │  Hamiltonian evolution                    │
                   │  RIFA IC + SIL + Crown Lattice           │
                   │                                           │
                   │  Trained via ZeroGrad Hebbian.           │
                   │  Output: (a) coherent state c*           │
                   │          (b) tension T(t)                │
                   │          (c) retrieved nodes & rules     │
                   └──────────────────┬───────────────────────┘
                                      │
                   ┌──────────────────▼───────────────────────┐
                   │  LAYER 2: Constructive Substrate         │
                   │  ─────────────────────────               │
                   │                                           │
                   │  Typed graph G of active concepts        │
                   │  Rewrite rules → morphisms G → G'        │
                   │  Growth budget bounded by T(t)           │
                   │                                           │
                   │  Reasoning = fixpoint of rule-application │
                   │  Terminates on:                          │
                   │   – answer node reached                  │
                   │   – no rule applies                      │
                   │   – tension budget exhausted             │
                   │                                           │
                   │  Realises Axioms I, II, III.             │
                   │  Output: final graph G*                  │
                   └──────────────────┬───────────────────────┘
                                      │
                   ┌──────────────────▼───────────────────────┐
                   │  LAYER 3: MDL Translation                │
                   │  ─────────────────────────               │
                   │                                           │
                   │  Table T of ≤ N rows:                    │
                   │    (graph_invariant_i, string_i)         │
                   │                                           │
                   │  Lookup: match(G*) → string              │
                   │  If no match above threshold →           │
                   │    "unknown", supported by IC's         │
                   │    reported low coherence                │
                   │                                           │
                   │  Cost: exactly log₂N bits (Lemma).       │
                   └──────────────────┬───────────────────────┘
                                      │
                                      ▼
                              SEMANTIC ANSWER
```

#### 5.2 Layer 1: Physics Substrate — the Relevance Heuristic

In the four-part structure of §3.4, Layer 1 is the **relevance heuristic**, not the thinker. Its job is the same as AlphaProof's neural policy: given the current query and derivation state, propose *which* primitives (nodes, rules) Layer 2 should try next, in what order. It does not answer; it ranks.

Its four responsibilities:

- **Semantic normalization.** Paraphrases of a query evolve to nearby states under the Hamiltonian, so downstream retrieval and rewriting see a canonical form. This is work no lookup table can do.
- **Contradiction metabolism.** RIFA's Integrity Core [Mahir 2026g] converts contradiction pressure into a scalar tension $T(t)$ that regulates Layer 2's growth budget.
- **Identity persistence.** RIFA's Silence-Integration Layer sustains the operator across long input gaps without external prompting.
- **Rule-relevance ranking.** Given the current $(q, G_t)$, output a probability distribution over Layer 2's rule set — a *tactic policy* in the theorem-proving sense. This is where the substrate replaces a brute-force forward-chaining search.

The substrate is trained by the **ZeroGrad** Hebbian paradigm [Mahir 2026d]: no backprop, no loss functions, no optimizer state, no gradient descent on parameters. All updates are local and physics-derived. Weight compression uses **Arch-Native Quantization** at 4-bit precision [Mahir 2026f] for GPU-scale substrates. **Crucially, the substrate is trained on derivation traces from Layer 2**, not on raw text: it learns to predict which rule was actually applied at each step of past successful derivations. This is precisely the training signal used by AlphaZero's policy network — but implemented via ZeroGrad Hebbian rather than backprop.

**Anti-memorization guarantee.** The substrate never sees a `(query, answer)` pair. Its training signal is `(graph state, rule applied)` triples, drawn from the derivations Layer 2 has itself generated. There is therefore no channel through which the substrate could memorize the mapping from queries to answers — the mapping is not in the training data. It could, at worst, memorize `(state, rule)` correlations, which is exactly what a heuristic policy is *supposed* to do (see §5.2's list of responsibilities). The engine's answers come from Layer 2's construction plus Layer 3's translation table; neither of these paths involves the substrate as anything other than a proposer of what to try next. This is what closes the "hidden memorization" trap that pattern-matcher-in-thinker's-clothing systems fall into.

#### 5.3 Layer 2: Constructive Substrate — the Thinker

Layer 2 is the composition engine of §3.4. This is where thinking lives. It is a **typed graph rewriter** operating on a graph $G_t = (V_t, E_t, \tau)$ where $\tau: E_t \to \mathcal{R}$ assigns each edge a relation type from a finite ontology $\mathcal{R}$. Its four subcomponents mirror the corollary:

**5.3.1 Rule Family.** Rewrite rules are of the form

$$
\text{if } (P \subseteq G_t) \text{ then } G_{t+1} := G_t \cup Q,
$$

where $P$ is a pattern subgraph and $Q$ is a construction subgraph. New nodes in $Q \setminus P$ realize Axiom II (phase-space expansion). The initial rule set is the **nine reasoning primitives of §4.4** (deduction, transitive composition, inheritance, abduction, negation, case analysis, induction, analogy, recursion). Each is implemented as a distinct rule type with its own pattern matcher; the accompanying implementation `src/primitives.hpp` fires all nine on the empirical suite of §4.4.

By the composition-closure claim of §4.4, this seed set is sufficient to reach a Turing-complete derivation space under recursion + rule induction. The set is fixed at design time; the *macros* discovered from usage (§5.3.3) multiply the reachable derivation space monotonically over the system's lifetime, without new hand-authored rules.

**5.3.2 Composition Engine.** Both forward and backward chaining. Forward derives everything currently reachable; backward pursues a specific query goal by working from the goal to the premises. The engine is a bounded-depth iterative deepening: forward for exploration, backward for goal-directed proof. Termination conditions: (a) answer node reached, (b) no rule applies (fixpoint), (c) tension-bounded budget $\alpha \cdot T(t)$ exhausted.

**5.3.3 Rule Induction (Meta-Rule Discovery).** After every successful derivation, sub-chains of length $\geq 2$ are recorded with their frequency. When a sub-chain crosses a MDL threshold (its compressed representation is shorter than the sum of its parts), it is registered as a new **macro-rule**. Subsequent derivations that would have replayed the sub-chain now apply the macro-rule instead. This is how the system grows its own primitive set from experience — the primitive count $P$ grows polynomially, but each new macro-rule multiplies the reachable derivation space. The result is a monotone increase in the thinker index $C/R$ over the system's lifetime, purely from usage, without any additional training data.

**5.3.4 Verifier.** Every derived edge carries the rule that produced it and the parent edges. A verifier walks any candidate answer edge back to premises and re-checks each application. This is the mechanism that prevents Layer 2 from being a generator: no output is emitted unless its derivation type-checks against the current rule set. Failed verification returns "unknown" via Layer 3, not a hallucinated answer.

This layer is what none of our prior work possessed. It is what turns coherent-state normalisation into constructive derivation, and — through §5.3.3 — enables the system to *increase its thinker index over time* rather than being frozen at design.

#### 5.4 Layer 3: Minimum-Description-Length Translation

The translation table pays the conservation-law tax explicitly. Each row is a pair $(I_i, s_i)$ where $I_i$ is a graph invariant (e.g., the label of a designated answer node, or a signature over $G^*$) and $s_i$ is its human-readable string. Lookup is exact (invariants are discrete). The total cost is $\Theta(N)$ table rows for $N$ answers, meeting the $\log_2 N$ lower bound within a constant factor.

Because the translation is explicit, it is **interpretable** in a way no distributed LLM head is: every row can be inspected, every answer traced to a specific graph structure. Because the invariants are discrete, translation **does not hallucinate**: an answer is either a table hit or "unknown" (reported with IC-supported low coherence).

#### 5.4b Physics-Driven Language Generation via Resonance Signatures — **RETIRED**

> **STATUS: RETIRED. Superseded by §5.4d (Constructive Language Locking).**
>
> This section is preserved because it states a hypothesis clearly and
> that hypothesis was **wrong in kind, not in degree** — which is a
> result worth keeping. Do not cite it as describing the current
> system.
>
> The proposal below asks a resonance field to *discover* English word
> order. §5.4d argues that this is a category error: grammar is a
> transmission protocol two speakers agreed on, not a physical
> phenomenon awaiting measurement. There is no gradient to follow
> because there is no fact of the matter. The concept-level ordering
> described in Stage 3 partially worked and survives in CPME/CPSB; the
> *grammatical* claims of Stage 2 did not, and the byte-level emission
> babbled (`mrs.md` §4b).
>
> **What replaced it:** surface realisation moved out of the physics
> entirely, into isolated deterministic language clusters (§5.4d,
> `cll.md`). What was retained: phase-collapse ordering across
> *concepts*, which is a physical claim and holds.
>
> The comparison table at the end of this section should be read as
> *what was hoped for in 2026*, not as a measured result. Two of its
> rows ("Grammar: fixed macro-node ω per grammatical role", "Sequencing:
> phase-locked slot sampling") describe a mechanism that was
> implemented and did not work.

The MDL translation table of §5.4 answers a single query with a single string
via a static lookup on a graph invariant. Natural-language *generation* —
producing ordered sentences — requires an extension: the wave dynamics of
Layer 1 already encode meaning, but the *sequence* of words must emerge from
a mechanism that does not sample from a next-token distribution. We proposed
a three-stage physics-only pipeline that produces grammatically ordered
output without any statistical prediction.

**Stage 1 — Resonance Signature (semantics).** A query drives the cognitive
substrate; after a settling window the standing-wave field
$\{x_i, v_i, \theta_k\}$ across cavities and macro-nodes IS the meaning.
This is consistent with the fixed-point account of phenomenal consciousness
in BSMA [Mahir 2026m, `bmsa_new.md`]: the wave field is not a proxy for a
thought — it is the thought. No decoding is required at this stage — the
meaning lives in the physical state, and the primitives it references live
in the graph.

**Stage 2 — Phase-Locking with a Syntax Macro-Layer.** In addition to the
semantic macro-nodes of HNCA [Mahir 2026n, `hnca.md`] we introduce a
second macro-layer whose regions have *grammatical roles*: one macro-node
per slot (Subject, Verb, Object, and optional Adverb / Modifier) with a
fixed natural frequency $\omega_S, \omega_V, \omega_O$. The semantic
macro-nodes couple into the syntax layer via the same Kuramoto rule
already used for HNCA:

$$
\frac{d\theta_i}{dt} = \omega_i + \frac{K}{M}\sum_j \sin(\theta_j - \theta_i)\,A_j
$$

When a semantic cluster's amplitude peaks near the Subject slot's phase,
that cluster IS the subject of the emitted sentence. This is the same
mechanism a human uses when a "verb slot" in Broca's area recruits
whichever motor-action concept currently has the highest neural gain —
grammatical role is a *physical slot*, not a statistical association.

**Stage 3 — Temporal Readout Head with Tunnel Decoding.** A readout head
samples the cavities at each slot phase in canonical order:
$\theta_S = 0$, $\theta_V = 2\pi/3$, $\theta_O = 4\pi/3$. For each slot
the head reports the cavity whose displacement is peak *at that phase*.
Only then are the raw label strings concatenated. The result is not a
bag of concepts — it is an ordered sentence whose sequence is determined
by physics, not by probability. Concretely, for a probe on `metformin`:

- The semantic wave excites `metformin.α`, which phase-locks into the
  Subject slot; the tunnel `metformin --inhibits--> mtor` transmits into
  the Verb slot at phase $2\pi/3$; the target cavity `mtor.β` phase-locks
  into the Object slot at $4\pi/3$.
- The readout head samples these three slots in the fixed canonical order.
- Layer-3 table maps each cavity id to its label string.
- Output: `metformin inhibits mtor` — one sentence, deterministic,
  physics-derived, no next-token prediction anywhere in the pipeline.

**Storage-floor honesty.** The three raw label strings (`metformin`,
`inhibits`, `mtor`) ARE stored — they must be, by the Translation
Capacity Lemma of §3.1. The *sequencing* is NOT stored. No pair
`(query, "metformin inhibits mtor")` appears anywhere in the training
data. The order emerges from the physical geometry of the syntax
macro-layer coupled into the semantic substrate. This is the same
storage floor a human brain requires: a child who has never heard
`metformin` cannot dream about it. Facts must be stored; sentences
need not be.

**Comparison.**

| Mechanism | LLM | This engine |
|---|---|---|
| Semantics | Distributed in $\sim 10^{12}$ weights | Standing-wave field on the substrate |
| Sequencing | $\arg\max$ over next-token distribution | Phase-locked slot sampling |
| Grammar | Learned from token co-occurrence | Fixed macro-node $\omega$ per grammatical role |
| Sampling | Temperature $\times$ softmax | None — deterministic slot readout |
| Novelty | Statistical interpolation | Physics-computed constructions (fission, bridge insertion, resonance) |
| Storage | $(Q,A)$ pairs baked into weights | Only primitive fact strings; $O(N)$ rows |
| Failure mode | Hallucination | Explicit "unknown" via slot-vacancy |

This mechanism is the natural extension of Layer 3 — the same MDL
table supplies the label strings; the syntax macro-layer supplies the
ordering that a single-slot lookup cannot. Neither statistical sampling
nor an autoregressive language model appears anywhere in the pipeline.
The extension is a small addition: one macro-layer with three-to-five
fixed frequencies, one readout head, no new weights, no new training.

#### 5.4c The Unified Working Substrate — What Is Built, Measured, and Load-Bearing

The papers in this lineage each propose one mechanism. Read separately
they are a pile of proposals; read as a stack they are one substrate in
which each mechanism gates a different physical quantity. This section
arranges the mechanisms that are **implemented and measured** — not
proposed — in the order the field encounters them. Everything below is
in `src/physics.hpp` unless noted.

**Input path.**

| Mechanism | Paper | Gates | Status |
| --- | --- | --- | --- |
| Byte-lane sensory layer | CMD | 256 lanes; concepts reached only via learned bindings | built |
| Bigram lanes | MRS §4b.1 | ordered pairs; recognition survives misspelling | **7/7 measured** |
| Chirality | CPME §1.1 | arrival time → phase gradient | built |

**Field dynamics.** Each row multiplies a different term; none is a
special case of another.

| Mechanism | Paper | Gates | Physical quantity |
| --- | --- | --- | --- |
| MIMS materials | `mims.md` | tunnel stiffness / damping | **now derived from fan-out, not hand-set** |
| Curie habituation | CMD | tunnel coupling | per-cavity heat $T_i$ |
| VGHT sub-bands | `vght.md` | domain isolation | macro-node $\omega$ |
| VGHT voltage gates | `vght.md` | cross-domain bridges | $\sigma(\kappa(|x_a||x_b| - \theta))$ |
| VGHT viscosity | `vght.md` | avalanche suppression | $E_{\text{total}}$ |
| CPSB gauge $\Psi$ | `cpsb.md` | directed phase order | per-tunnel offset |
| CSAT chaos | `CSAT.md` | metastability | Lorenz $\hat x \cdot \tanh(T_i/T_c)$ |
| Energy homeostasis | this §5.4c | inter-turn forgetting | global gain $\sqrt{B/E}$ |

**Structural mechanisms.**

| Mechanism | Paper | Effect |
| --- | --- | --- |
| RB+G fission | `rbg.md` | contradiction → new vertices (Axiom II) |
| L6-A bridging | `rbg.md` §5 | shared-target intersection → rubber tunnel |
| ZeroGrad Hebbian | `zerograd.md` | local plasticity, no backprop |
| CPME-II phase Hebbian | `cpme_ii.md` | consolidates only phase-consistent co-activity |

**Two corrections this arrangement forced.** Assembling the stack
revealed defects invisible when each paper was read alone:

1. *MIMS constants were guesses.* Stiffness is now **derived** from a
   relation's fan-out: $k_r = k_{hi}/(1 + \log_2(1 + \text{fanout}_r))$.
   A relation whose subject almost determines its object is a hard
   constraint and transmits cleanly; a promiscuous one is diffuse and
   should not. Twenty hand-set numbers became one formula, and the
   physics measurably improved.

2. *VGHT sub-bands were assigned per connected component, which
   silently failed.* Union-find over **all** tunnels merged 69% of the
   graph (1877 of 2712 cavities) into one component, which then
   received a single frequency band — so domain isolation was absent
   exactly where it was needed, and biology, taxonomy and computing
   resonated in the same band. The cause was generic `is_a` / `has`
   edges bridging every domain through shared abstract hubs. The fix
   follows from correction (1): those relations now carry *low*
   stiffness by construction, so clustering unions only over tunnels
   above median stiffness. **Two masses joined by a very weak spring
   are not one rigid body.** The giant component fell to 621 cavities
   and cross-domain contamination disappeared:

   ```
   before   mammal -> biology open source license kernel posix compliance
   after    mammal -> warm blood hair spine
   ```

**Verification path (Phase 2).** The substrate now forms hypotheses
from the graph, emits real source expressing them, and submits them to
an external oracle:

| Fact | Provenance |
| --- | --- |
| `human is_a great_ape` | derived — forward chaining, premises cited |
| `bind_byte has arity_2` | **established by experiment** — contradicted extraction; compiler settled it |
| `drive has sig_int_float_float_int` | **established by experiment** — absent from the graph entirely |

This adds a third provenance class to the substrate. Neither asserted
nor derived: *established by experiment*. Cost on a repeated task falls
from 3 experiments to 1, because the hypothesis it starts from is the
one it earned.

#### 5.4d Language Governance — Isolated Syntax Clusters

*Developed at length in `research-paper/cll.md` (Constructive Language
Locking). This section states the principle and the measured result;
the companion paper gives the formal definition, cost accounting,
comparison table and failure modes.*

§5.4b proposed deriving word order from field physics, and §8.5 asked
for "grammatical constraint expressed as a field property." **That was
a category error, and this section corrects it.**

**Cognition and language are different kinds of thing.**

| | Cognition | Language |
| --- | --- | --- |
| Purpose | discover | transmit |
| Needs | novelty, experiment, resonance, oracle validation | a fixed, agreed protocol |
| Bound by rules? | never — that would cripple it | entirely — that is what it *is* |

English SVO is not a physical law awaiting discovery by resonance. It
is a human communication convention. Bengali SOV encodes identical
cognitive content under a different convention. Attempting to make the
substrate *discover* grammar confuses a protocol with a phenomenon.

**Architecture.** The substrate splits, and the core is untouched:

```
[ Core SI Physics Substrate ]  ->  raw resonant concept triples
                |                  (subject, relation, object)
                v
[ Isolated Language Cluster ]  ->  deterministic surface realization
     |- English  (SVO grid)
     |- Bengali  (SOV grid)
     |- Banglish (SOV grid, Latin script)
                |
                v
        human-readable stream
```

Not one grammar rule enters `physics.hpp`. Fission, L6-A bridging,
thermodynamic discovery and novelty are unaffected — the cognitive
flexibility the rest of this paper argues for is preserved *because*
the protocol lives outside the physics.

**Measured** (`src/lang.hpp`), one cognitive state under three protocols:

```
(biology, is_a, science)     en: Biology is a science.
                             bn: Biology science একটি।
                             bl: Biology science ekta.

(metformin, inhibits, mtor)  en: Metformin inhibits mtor.
                             bn: Metformin mtor বাধা দেয়।
                             bl: Metformin mtor badha dey.
```

**Cost accounting.** This adds no hidden cost and hides nothing. Layer 3
already stores label strings — the Translation Capacity Lemma (§3.1)
forces it. A word-order constant is ~2 bits; a relation lexicalisation
table is one row per relation per language; a transliteration map is one
row per word pair. All are Layer-3 *protocol* cost, explicitly named,
and none is intelligence.

**A defect testing disproved immediately.** The first implementation
treated article insertion as a global language property and emitted
*"Biology is a the science."* Whether an object takes a determiner
belongs to the **construction**, not the language: `is_a` supplies its
own article, proper nouns take none. Article placement is now a
per-relation flag. This is the shape of the remaining work — real
grammar exceeds word order, but every additional rule is deterministic
data, never a learned parameter.

**Two properties this buys that a language model cannot.**

1. **Zero-hallucination surface form.** The transliteration relation is
   strict and static: `tumi` is *always* `তুমি`. A model
   code-switching Banglish drifts between spellings because its output
   is sampled; this cannot, because nothing is sampled.
2. **O(1) multilingual swap.** Adding Japanese means adding a table.
   The same cognitive state emits Japanese with the substrate
   unchanged. A language model requires retraining or far greater
   capacity for the same capability.

**Still open in this layer** — all deterministic, none requiring ML:
morphological agreement (*studies*, not *study*), Bengali verb
conjugation by person and honorific (তুমি / আপনি / তুই), free word
order under case marking, and multi-triple discourse with pronoun
resolution.

#### 5.4e The J-A-S Cycle — Einstein's Epistemology as Substrate Architecture

Einstein described the structure of physical theory as a cycle rather
than a ladder:

```
                  +---------------------------+
                  |        Axioms (A)         |
                  +---------------------------+
                     /                     \
                    v                       v
                +-----+                   +-----+
                |  S  |                   | S'  |   deduced theorems
                +-----+                   +-----+
                   |                         |
                   | experiment              | experiment
                   v                         v
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                Sense Experience (E)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        \                                     /
         +------------- Jump (J) ------------+
```

The crucial claim is that **J is not logical**. No inference rule takes
you from observations to axioms; the leap is inductive, unjustified,
and revisable. Deduction runs *downward* from axioms to theorems;
experiment tests theorems back against experience; refutation
propagates *upward* to revise the axioms that produced them.

This is not an analogy for the architecture — it is a description of
it. Every element is already implemented:

| Einstein | Substrate | Implementation |
| --- | --- | --- |
| **E** — sense experience | raw source, byte input | `extract_ast_axioms.py`, byte-lane layer |
| **J** — the jump | induction: universal generalisation over observations | §5.3.3 rule induction, L6-A bridging, RB+G fission |
| **A** — axioms | the typed concept graph | `data/*.axioms` |
| **S, S'** — deduced theorems | forward chaining | 100 derived edges, premises cited |
| **experiment** | compile-verify against an external oracle | `synth`, §5.4c |
| **refutation → revise A** | corrected axiom written back | `data/verified.axioms` |

**Why this matters for the SI claim.** §3.4 defines a thinker by
composition rate. The J-A-S cycle sharpens that: a system that only
deduces (S from A) is a theorem prover, and a system that only
observes (E) is a sensor. What makes something a *scientist* is the
closed loop — jumping to axioms that are not entailed, deducing their
consequences, and letting an external oracle refute them.

The substrate closes that loop. It is the strongest available answer to
the charge that this is an indexing engine: an index has no J, no
experiment, and no mechanism by which reality can overturn what it
holds.

**The essential asymmetry.** J is *not* the inverse of deduction and
must never be implemented as such. An induced axiom is a guess whose
only warrant is survival under experiment. The substrate therefore
tracks three provenance classes — *asserted*, *derived*, *established
by experiment* — and a fourth is added here: *induced*, which carries
the weakest warrant and is the first thing retracted under refutation.

#### 5.5 What Each Prior Component Contributes, and What Was Dropped

> **CORRECTED.** An earlier version of this table listed DCHL, BED, ANQ
> and CMC as *retained components of the current architecture*. They are
> not, and were not at the time the claim was written. The table below
> is reconciled against the mechanism-level audit in §6.2, which was
> produced by grepping the live source rather than by reading the
> papers. Where the two disagreed, the grep won.
>
> The error is worth naming because it is the characteristic failure of
> a long lineage: a synthesis section written once and never re-checked
> silently becomes a description of a system that no longer exists. That
> is the failure §6.2 exists to prevent, and it had already happened
> here.

The substrate this paper describes is the **oscillator substrate**
(`src/physics.hpp`). It replaced the **DCHL substrate** — a continuous-state
Hamiltonian system with a learned potential $\Phi$ — which is retired.
Everything below is stated relative to that replacement.

| Component | Status in the CURRENT engine | Where, or why not |
|---|---|---|
| CMA fixed potential | **historical only** | supplied the manifold vocabulary of §4; no code |
| P-CMA predictive transition | **dropped** | subsumed by graph-rewriter transitions |
| HCMA hierarchical states | **dropped with DCHL** | its contrastive training is the negative result of §2.1 |
| CCH curl-free constraint | **not implemented** | grep finds no `curl` in the substrate; the constraint was never enforced in code, so it guarantees nothing here |
| DCHL learned $\Phi$ | **RETIRED** | the substrate that collapsed into the §1.2 traps; trained artifacts archived out of the tree |
| **ZeroGrad learning rule** | **LIVE — foundational** | `plasticity_delta = pd·hebbian_decay + plastic_rate·v_a·v_b`; every Hebbian update, including CPME-II, is this rule. *The paper is dead; the rule is load-bearing* (§6.2.4) |
| **RIFA IC/SIL** | **LIVE** | `IntegrityCore`, `SilenceIntegration` — contradiction and identity metabolism |
| RIFA Rule Synthesizer | **live but bypassed on bulk load** | streaming-only; feeding it a sorted axiom file manufactures rules from alphabetical order, so ingestion disables it |
| BED (Byte Energy Descent) | **DEAD** | `DEAD/deprected/bed.md`; converged to byte-mean with no prompt structure |
| CognitiveMemory retrieval | **dropped** | subsumed by the Layer-3 table, which names the cost honestly |
| STAN topological readout | **dropped** | bifurcation drift; replaced by graph invariants |
| ANQ 4-bit quantisation | **DEAD as a scheme** | retired with the GPU trainer it served |
| **ANQ LEA integrator** | **LIVE** | continuous integrator replacing threshold-commit; used by every saturating accumulator in the tick (§6.2.4) |
| CMC memory reduction | **not in live code** | no current trainer needs it — there is no gradient training left to compress |
| **LTO** | **LIVE** | runtime axiom acquisition and persistence |

Two entries above are the point of the whole audit: **ZeroGrad and ANQ-LEA
are running in production while their papers sit in `DEAD/`.** A dead
paper is not a retracted one. Cite those two as *sources of mechanisms*,
never as *descriptions of the system*.

The architecture is minimal in the sense that removing any **LIVE**
component costs a specific capability. It is *not* minimal in the sense
that every historical component found a home — most did not, and saying
otherwise was the error corrected above.

---

### 6. Relationship to Prior Work in the Lineage

> **Read this section as intellectual history, not as a parts list.**
> Every item below states what a paper *contributed to the thinking*.
> Whether the mechanism survives into running code is a separate
> question, answered — per paper, by grep — in §6.2. Several entries
> here are historically important and currently dead (CCH, CMC, ANQ,
> SCER, BED). Conflating "this shaped the design" with "this is in the
> system" is precisely the error corrected in §5.5.

The synthesis makes explicit what each earlier paper contributed, and what none of them could have done alone:

- **CMA** (2026a) introduced the manifold and Hamiltonian formalism.
- **P-CMA** (2026b) added learned transitions and the planning idea.
- **CCH** (2026c) established the curl-free constraint as the mathematical guarantor of transitive reasoning.
- **HCMA** (2026h) introduced decoupled contrastive Hebbian learning (DCHL) and the two-level state.
- **ZeroGrad** (2026d) formalized the local, backprop-free training paradigm.
- **CMC** (2026i) reduced training memory to $O(1)$ via cognitive state compression.
- **ANQ** (2026f) achieved 4-bit weight compression that preserves DCHL physics.
- **CME** (2026j) extended the substrate to multimodal inputs via unified state.
- **LTO** (2026k) introduced online substrate adaptation without stopping inference.
- **SCER** (2026l) proposed multi-mode routing at the reasoning layer.
- **RIFA** (2026g) added structural pressure, contradiction metabolism, silence endurance, and self-assembling control.
- **BED** (2026e) introduced the discrete argmin decoder for byte-level output.

Each was locally correct within its scope. The synthesis reveals that none was sufficient alone to escape the three traps of §1.2, and that the missing piece — the constructive graph-rewriting layer with explicit MDL translation — is precisely what turns *substrate* into *thinker*.

**Two corrections to the list above, both established by §6.2.** CCH's
curl-free constraint is described here as having "established" a
guarantor of transitive reasoning; it was never implemented, so it
guarantees nothing about this engine and the claim is historical only.
And the framing "each was locally correct" is too generous to HCMA and
BED: HCMA's contrastive objective was *wrong* in the specific way §2.1
diagnoses (it trains a classifier), and BED's decoder demonstrably
converged to the byte-mean. Those are negative results, and they are
more useful stated as such.

---

#### 6.2 Lineage Audit — What Still Carries Weight, and What Is Dead

A lineage of thirty-plus papers accumulates claims faster than it
retires them. This section states, for every prior paper, whether it is
load-bearing in the current engine. The audit was performed by grepping
the live source for each mechanism, not by reading the papers.

**The distinction that matters most: a paper can be dead while its
mechanism survives.** Research lineages do not retire ideas cleanly;
they cannibalise them. Marking a paper dead says the *proposal as
stated* is retired. It does not say the idea was worthless, and in two
cases below the idea is running in production while its paper sits in
`DEAD/`.

##### 6.2.1 Load-bearing today (`research-paper/`)

Mechanisms verified present in `src/physics.hpp`, `src/layer2.hpp`, or
the live binaries:

| Paper | Mechanism | Where |
| --- | --- | --- |
| `prm.md` | mass-spring cavities | `struct Cavity` |
| `mims.md` | material impedance | `material_for()` — **now derived, not hand-set** |
| `hnca.md` | macro Kuramoto | `struct MacroNode` |
| `ntda.md` | neuromodulators | `struct Neurotransmitters` |
| `tsda.md` | salience integrator | `Cavity::salience` |
| `rifa.md` | contradiction + identity | `IntegrityCore`, `SilenceIntegration` |
| `srg.md` | dual-port routing | `x_E/x_I` (opt-in) |
| `rbg.md` | gated fission | `fission_locked()` |
| `vght.md` | sub-bands, voltage gates | band ladder, `voltage_gated` |
| `cpme.md`, `cpme_ii.md` | momentum emission, phase Hebbian | `readout_phase_collapse()` |
| `cpsb.md` | gauge Ψ | `psi_for()` |
| `CSAT.md` | chaotic metastability | Lorenz, opt-in |
| `mrs.md` | bigram recognition | `bigram_bind_w` (emission half retired) |
| `cll.md` | language locking | `src/lang.hpp` |
| `graph_fisher.md` | Fisher metric | `fisher_distance()` |
| `RADE.md`, `maa.md`, `l9.md` | discovery, dispatch, L9 fixes | `rade.hpp`, `mal.hpp` |
| `bmsa_new.md`, `phenomenal-consciousness.md` | bounded self-model | `bsma_topo.hpp` |
| `liouville_analog.md`, `cr_bound.md` | proofs | no code required |

##### 6.2.2 Partly works — retired as architectures, mined for parts
(`research-paper/failed_but_parts_works/`)

These described the **DCHL substrate**, a continuous-state Hamiltonian
system that preceded the oscillator substrate. That architecture is
retired: it collapsed into the three traps of §1.2, exactly as §2
predicts. What survived was extracted.

| Paper | Status | What survived |
| --- | --- | --- |
| `cma.md` | superseded | manifold + Hamiltonian framing; the vocabulary of §4 |
| `hcma.md` | superseded | DCHL contrastive training — the *negative* result of §2.1 |
| `cch.md` | **not in live code** | curl-free constraint; grep finds no `curl` in the substrate |
| `cmc.md`, `cmc_extra.md` | **not in live code** | training-memory compression; no current trainer needs it |
| `cme.md` | **never implemented** | multimodal unified state; aspirational |
| `rifa.md` | **LIVE** (duplicated to root) | IC/SIL are load-bearing; this copy is redundant |
| `README.md` | historical | describes the retired DCHL chatbot |

The honest summary: this folder is the record of a substrate that did
not work and the reasons why. §2 of this paper is a self-critique built
on it. Keeping it is correct — the negative result is a contribution —
but nothing in it except RIFA is load-bearing.

##### 6.2.3 DEAD — formally retired
(`research-paper/failed_but_parts_works/DEAD/`)

**These papers are dead. They are retained for provenance only and
should not be cited as describing the current system.**

| File | Why dead |
| --- | --- |
| `bug_fix.md`, `future_fix.md` | roadmaps fully superseded by `ROADMAP.md` |
| `deprected/bed.md` | Byte Energy Descent — converged to byte-mean, no prompt structure (§2.1) |
| `deprected/scer.md` | multi-mode routing — subsumed by the field-observable dispatcher |
| `deprected/hmhra.md`, `hrm.md`, `omt.md`, `cb.md`, `cg.md` | superseded before implementation |
| `deprected/fixes.md` | ad-hoc patch log, no longer navigable |

##### 6.2.4 Two mechanisms that outlived their papers

This is the finding the audit exists to surface.

**ZeroGrad** (`DEAD/deprected/zerograd.md`). The paper is dead — it
described backprop-free training of the DCHL potential, and that
substrate is retired. **The learning rule is live.** `physics.hpp`
line 20 carries it verbatim:

```
ZeroGrad : plasticity_delta = pd·hebbian_decay + plastic_rate·v_a·v_b
```

Every Hebbian update in the current engine, including the
phase-consistent extension of `cpme_ii.md`, is this rule. The paper
died; the rule became foundational.

**ANQ** (`DEAD/deprected/anq.md`). The 4-bit quantisation scheme is
retired with the GPU trainer it served. **One idea survived**: the
continuous-integrator-instead-of-threshold-commit pattern, cited in
`physics.hpp` line 21 as `ANQ LEA`, and now used by every saturating
accumulator in the tick.

**The lesson for citation practice.** A dead paper is not a retracted
one. These two should be cited as *sources of mechanisms*, never as
*descriptions of the system*. Conflating the two is how a lineage
accumulates claims it can no longer defend — which is the failure this
audit exists to prevent.

---

### 7. Empirical Program

The architecture makes testable predictions. We enumerate them so the empirical validation is unambiguous.

#### 7.1 Substrate-Only Baselines (Falsification of §1.2 Traps)

At all tested scales ($10^4$ – $10^9$ parameters), substrate-only variants (DCHL alone, BED alone, RIFA alone) should reduce to classifier, memorizer, or retriever behaviour under any measurable generalization test. **This is a falsification prediction of the Translation Capacity Lemma.** Empirical evidence to date (this paper's supporting implementation and the accompanying limitations documented in [Mahir 2026e]) is consistent with this prediction.

#### 7.2 Substrate-Level Paraphrase Invariance

Under Hamiltonian evolution, semantically equivalent queries should converge to states $\mathbf{c}^*$ closer to one another than the raw query encodings. Concretely, for a set of paraphrase clusters $\{P_k\}$, define

$$
r_k \equiv \frac{\text{mean intra-cluster distance after evolution}}{\text{mean intra-cluster distance before evolution}}.
$$

The claim is $r_k < 1$ for well-trained substrates. This is *the* work only physics-based dynamics can do; if it fails, the substrate has no non-trivial role.

#### 7.3 Layer-2 Generalization to Unseen Combinations

The graph rewriter is symbolic and deterministic; it should achieve near-100% accuracy on syllogisms whose *rules* have been supplied but whose *specific instances* have never been trained. This is the sharpest possible test of constructive reasoning versus memorization: an LLM at any scale below full-corpus coverage cannot achieve this, because it has no explicit derivation mechanism.

#### 7.4 OOD Honesty

When queries lie outside the ontology of concepts and rules, the pipeline must refuse — not confabulate. The mechanism is coherence-thresholded lookup at Layer 3, backed by IC's contradiction reporting. The claim is that hallucination rates should be near zero on strictly-OOD test sets, with all non-answers being explicit refusals.

#### 7.5 Scale Behavior — **applies to the retired DCHL substrate only**

> This prediction is stated in units the current architecture does not
> have. The oscillator substrate has **no parameters to scale**: cavities
> and tunnels are created from the ontology, and their materials are
> *derived* from relation fan-out rather than fitted. "1 B parameters in
> 4-bit ANQ on 8 GB VRAM" describes the retired system.
>
> The corresponding question for the current substrate is not parameter
> count but **graph scale**, and there the honest answer is a measured
> limitation rather than a prediction: at 157 k concepts / 237 k
> relations the per-query cost is dominated by a dense tick over all
> cavities and tunnels, because Layer 1 has no sparse wavefront
> iteration. Scaling behaviour is therefore currently an *engineering*
> bound, not an architectural claim, and should not be reported as one.

*(historical)* At GPU-scale substrates (~1B parameters, achievable in 4-bit ANQ on 8GB VRAM), the paraphrase-invariance ratio $r_k$ should improve monotonically with substrate capacity; the constructive layer's accuracy should be independent of substrate capacity (a symbolic engine's correctness is not a scaling law); and the translation table's necessary size $N$ is a property of the domain, not the substrate.

#### 7.5b Empirical: The Nine-Primitive Layer 2 and a 0.5B Layer 1

Two empirical facts from the accompanying implementation:

**Layer 2 (constructive substrate).** All nine primitives from §4.4 fire correctly on hand-crafted premise bases (transitive, deduction, inheritance, negation, case-analysis, induction, analogy, recursion, and macro-rule induction). Every derived conclusion was never asserted directly. Every derivation is traceable to its premises via the parent-edge chain. The macro-rule induction fires on repeated derivation streams. This validates the §3.4 composition-over-retrieval definition on an actual buildable engine, at zero cost of "training data" — the primitives are hand-authored (a seed of ~1 KB of code) and their compositions are unbounded.

**Layer 1 — HISTORICAL, on the RETIRED DCHL substrate.** The paragraph
that follows describes the *previous* Layer 1, not the current one. It
is retained because the measurement was real and the diagnosis was
correct; it must not be read as evidence for the architecture this paper
now describes. The current Layer 1 is the oscillator substrate of
§5.4c, which has **no gradient training, no parameters, and no GPU
path** — so none of these numbers apply to it, and the "pending fix"
below was never propagated because the substrate it belonged to was
retired instead.

> *(historical)* The denoising DCHL trainer, ANQ 4-bit weights, and pinned-memory host / device pipeline complete a 4-epoch training pass on 8 000 samples from a C4 + Pile-8 corpus in $4$ min $43$ s wall clock on an RTX 4060 (8 GB VRAM), using 285 MB of that VRAM for a 539 M parameter model (D = 512, H = 1 048 576, packed in 4-bit ANQ). Denoising $\Delta\Phi$ is correctly signed on every epoch on which the substrate has not yet saturated the tanh activation. **A saturation limit is empirically visible at |h| $\approx 0.008$** — the substrate's hidden activations sit deep in tanh's linear regime — traceable to an aug-norm scaling of $\sqrt{\lVert c\rVert^2 + 1}$ that compresses pre-activations by $\Theta(1/\sqrt{D})$ for byte-scale inputs. The proposed fix — replace the divisor by $\sqrt{(\lVert c\rVert^2 + 1)/D}$ — was implemented in reference code as `pre_inv_scale` and never propagated to the CUDA kernel.

**Why this is reported rather than deleted.** The DCHL result is the
empirical content of §2: a physics-based substrate, trained
contrastively at 0.5 B parameters, that reduced to the classifier trap
exactly as the Translation Capacity Lemma predicts. Deleting it would
remove the evidence for this paper's central self-critique. Reporting it
as though it were current would be a fabrication. It is therefore kept,
dated, and fenced.

**What the current claims actually rest on.** Layer 2's correctness is
independent of any substrate numerics: the nine primitives are exact
symbolic operations. The paper's core claims — composition-over-retrieval,
thinker-index growth under rule induction, verifier soundness, and the
provenance chain from a derived edge back to its premises — rest on
Layer 2 and on the oscillator Layer 1, neither of which involves
training. Every number in §7.6–§7.7 was produced on that stack.

#### 7.6 The Thinker Index and Its Growth Under Rule Induction

The thinker index $C(\mathcal{S})/R(\mathcal{S})$ of §3.4 is directly measurable. For any query set $Q$, run the pipeline, count $\ell(q)$ for each $q$, and compute the ratio. The claim is:

1. **Baseline.** With only the initial hand-authored rules (§5.3.1), $C/R > 1$ on out-of-training queries — the value is bounded by the maximum derivation length allowed by the tension budget.
2. **Rule induction growth.** After $M$ queries, with rule induction active (§5.3.3), the primitive set $|\Pi|$ grows by $O(\log M)$ (new macro-rules registered at MDL thresholds), while the reachable derivation space grows exponentially in $|\Pi|$. **The thinker index therefore grows monotonically over the system's lifetime**, with no additional training data.
3. **Comparison to LLMs.** An LLM's thinker index is fixed at training time and does not grow with usage. This is *the* separable empirical claim distinguishing SI from AI: SI systems become more of a thinker over time; AI systems do not.

These are separable empirical claims, testable in a modest research programme.

#### 7.7 Recursive Bifurcation and Analogical Bridging (RB+G / L6-A)

The physics substrate of Layer 1 supports two structural mechanisms that
Layer 2 uses as *primitives beyond the nine of §4.4*: **Recursive Bifurcation
with Gating (RB+G)** for splitting contradictory concepts, and **L6
Analogical Bridging** for inserting cross-domain resonance links between
fissioned siblings.

**RB+G — the mechanism.** When a concept-cavity's incoherence metric
$I = (\Sigma^+ \cdot \Sigma^-) / (\Sigma^+ + \Sigma^-)^2$ sustains above
0.15 with local tension $\tau > 0.005$ for three structural ticks, the
cavity is split into two children (α excitatory, β inhibitory) bound by
a learned low-stiffness coordination spring. Four documented failure
modes of the original RB proposal [Mahir 2026o, `rb.md`] are addressed by
four mechanisms:

1. **generation cap** ($g_{\max}=1$) — children of a fission are
   generation-1 and ineligible for further fission, preventing cascade;
2. **alias ledger** — a parallel to `cavities[]` that never touches
   `si.graph.by_label`; child labels are synthetic `<parent>.α` / `.β`;
3. **energy-conserving fan-out** ($1/\sqrt{2}$) — a drive on a fissioned
   parent routes to both children at $\text{amp}/\sqrt{2}$, so total
   injected energy matches a single-cavity drive at amp;
4. **learned binding tunnel** — $k_\text{init}=1.0$, $p_\text{init}=0.0$,
   converging under Hebbian to a coordination stiffness rather than RB's
   fiat $k=2.5$;
5. **audit trail** — every fission appended to `data/fission_ledger.jsonl`.

**20-seed reproducibility** (rounds=5, seed formula
$s = \mathtt{0xCAF17E00} \oplus (\mathtt{0x9E3779B9}\cdot(i+1))$):

| Concept | Cluster type | Baseline gain | RB+G gain (RMS) | Fissions/seed | Target fissioned |
|---|---|---|---|---|---|
| metformin | dual-tension (act ampk, inh mtor) | 0.887 ± 0.045 | 1.517 ± 0.201 | 8.85 ± 0.96 | 20/20 |
| berberine | tri-hub (act ampk, inh mtor, inh hmg) | 0.900 ± 0.030 | 1.515 ± 0.139 | 8.60 ± 1.16 | 20/20 |
| atorvastatin | mono-directional (inh hmg, treats hcy) | 1.007 ± 0.004 | 0.957 ± 0.034 | 9.75 ± 1.84 | 20/20 |
| luna | mutual + death_spiral inhibition | 1.244 ± 0.064 | 1.865 ± 0.162 | 7.00 ± 0.95 | 20/20 |
| linux | null (is_a, has, no polarity conflict) | 1.032 ± 0.007 | 1.039 ± 0.057 | 5.00 ± 0.00 | 20/20 |
| windows_os | pure null (only is_a) | 0.969 ± 0.008 | 0.955 ± 0.010 | 2.00 ± 0.00 | 0/20 |

Concepts with polarity conflict show a large positive RB+G effect. The
mono-directional concept (atorvastatin) shows an honest null effect —
fission fires on nearby concepts but the target's own recall does not
improve, because there was no contradiction inside it to metabolize.
The pure-null concept (windows_os) is never fissioned in 20/20 seeds —
the mechanism refuses to fire when the substrate has no contradiction
to resolve. This is the falsifiability guarantee: RB+G is not a
free-lunch amplifier.

**Four-lever ablation (5 seeds, metformin).** Full RB+G reaches
1.35 ± 0.05 concept RMS gain against a fission-off baseline of 0.92.

| Configuration | RMS gain | Fissions | Reading |
|---|---|---|---|
| BASELINE (fission off) | 0.922 ± 0.011 | 0.0 | negative control |
| ABLATE_CAP ($g_{\max}=999$) | 1.536 ± 0.239 | 25.6 ± 3.3 | cascade — 3× fissions, no accuracy win |
| ABLATE_ALIAS | 2.495 ± 0.144 | 3.4 ± 0.8 | confound: probe hits ghost cavity |
| ABLATE_FANOUT (factor=1.0) | 1.856 ± 0.069 | 13.6 ± 1.4 | inflated peak — energy doubling |
| ABLATE_BIND | 1.470 ± 0.057 | 8.8 ± 0.8 | minor loss vs FULL |
| FULL (all four mechanisms) | 1.345 ± 0.054 | 8.4 ± 1.0 | RB+G |

The ablation is honest, not marketing: two of the ablations produce
*higher* raw RMS than FULL, and we do not hide that. `ABLATE_ALIAS`
produces higher raw numbers because the probe measurement itself is
altered when alias routing is off — the probe hits the zeroed ghost
cavity and the recorded post/pre ratio reflects neighbour drift, not
target recall. `ABLATE_FANOUT` inflates the peak because $\sqrt{2}\times$
more energy is injected. These are documented confounds. What FULL
buys is honest energy accounting and cascade-free convergence, not the
biggest number.

**L6 Analogical Bridging.** After RB+G fissions two related concepts,
`discover_analogies()` computes the intersection of α-child excitatory
targets and inserts a rubber tunnel ($k=0.05$, $c=0.15$) between
α-cavities when $\geq K$ targets are shared. This is the physics
realization of L6 Analogical Mapping from `l9.md` and the structural-
analogy prescription of `mims.md`. The rubber material means MIMS
impedance filtering: high-frequency mechanistic probes are absorbed by
the low-stiffness edge; low-frequency structural probes ride the bridge.

On the insulin-pathway ontology, training `metformin + berberine`
surfaces five real analogies including:

- **`akt ~ ampk`** — both kinases activating `glut4` (glucose uptake);
- **`akt ~ mtor`** — both kinases in the insulin cascade;
- **`berberine ~ resveratrol`** — both AMPK activators (a known
  drug-repurposing pair);
- **`type2_diabetes ~ drug`** (six shared targets) — cross-category
  structural analogy;
- **`molecule ~ ampk`** (empty shared target list) — noise / weak analogy
  reported honestly.

Probing `metformin` at 3 Hz after bridge insertion surfaces
`atorvastatin` at rank 10 — a concept **unreachable by naive graph
propagation** from metformin (no direct or 2-hop path in the axioms),
reachable only through the physics-inserted bridge
`metformin.α ↔ berberine.α ↔ hmg_coa_reductase ↔ atorvastatin`. In a
negative-control run that fissions both concepts but skips
`discover_analogies`, `atorvastatin` disappears from the top-10.
That single row is the L6-A signal.

**No line in any axiom file says `metformin ~ berberine`**, no cavity
`metformin.α` exists at bind time, and no rubber tunnel exists until
`discover_analogies` inserts it. This is construction over retrieval —
the composition-over-retrieval definition of §3.4 exhibited in physics,
not in symbolic proof.

**Storage-floor honesty.** The primitive facts (`metformin activates
ampk`, `berberine activates ampk`) ARE stored in `data/insulin_pathway.axioms`.
The lemma of §3.1 requires this. What is *not* stored: the analogy
`metformin ~ berberine`, the child cavity `metformin.α`, the rubber
bridge, or the answer that a 3 Hz probe at metformin surfaces
atorvastatin. Every one of these is physics-constructed from the
axiom base at query time. This is what makes it a thinker under the
§3.4 definition — a constant primitive count, an exponentially larger
reachable derivation space.

---

### 8. Open Problems — Status After RB+G / L6-A

The five open problems posed in earlier drafts of this paper have been
partially resolved by the RB+G and L6-A work of §7.7. We reproduce each
open problem verbatim below and mark its current status.

#### 8.1 Partially Resolved

- **Domain-independent rule induction.** *(Original: "§5.3.3 registers
  macro-rules from frequent derivation sub-chains. Whether this suffices
  to discover genuinely new rule families (not just macros of existing
  families) is open. Can rule induction find analogy or abduction rules
  if the initial set had only transitive composition?")*
  **Status: RESOLVED for analogy.** L6-A (§7.7, `rbg.md` §5) demonstrates
  that the physics substrate discovers analogy relations *without* an
  analogy primitive being seeded — the mechanism is: RB+G fissions two
  concepts under contradiction; `discover_analogies()` computes the
  intersection of their α-child excitatory targets; a rubber tunnel is
  inserted when the intersection is nontrivial. On the insulin-pathway
  ontology this produces `akt ~ ampk`, `berberine ~ resveratrol`, and
  three other real analogies — none supplied as premises. Abduction rule
  discovery from primitives-only remains open.

- **Cross-domain heuristic transfer.** *(Original: "A substrate trained
  on derivation traces from one domain should provide some relevance
  signal in a related domain via shared type structure. Quantifying the
  transfer is open.")*
  **Status: PARTIALLY RESOLVED.** L6-A demonstrates *cross-concept*
  transfer *within* a domain: a 3 Hz probe on `metformin` surfaces
  `atorvastatin` at rank 10 via the physics-inserted bridge
  `metformin.α ↔ berberine.α ↔ hmg_coa_reductase ↔ atorvastatin`.
  Atorvastatin is unreachable from metformin by any 2-hop path in the
  raw axiom graph. This is transfer through shared *structural* targets
  (both drugs act on `hmg_coa_reductase`), which is the exact
  "shared type structure" the original problem referenced. Full
  cross-*domain* transfer (syllogisms → algebra) remains open and
  requires a substrate spanning multiple ontologies.

- **The tension-bounded conservation law.** *(Original: "The expansion
  principle $\Delta|G_t| \leq \alpha T(t)$ is proposed but not proved to
  be strictly conservative. What is the analog of Liouville's theorem
  for tension-bounded phase-space expansion?")*
  **Status: EMPIRICALLY REALIZED, THEORETICALLY OPEN.** RB+G is a concrete
  mechanism whose *implementation* preserves energy exactly: the
  $1/\sqrt{2}$ fan-out (M3 of §7.7) guarantees that a drive on a fissioned
  parent injects the same total kinetic energy as a drive on a single
  cavity — proven algebraically by
  $2 \cdot \frac{1}{2} m (\text{amp}/\sqrt{2})^2 = \frac{1}{2} m \, \text{amp}^2$
  and verified in the ABLATE_FANOUT ablation (§7.7): setting fan-out to
  1.0 injects $\sqrt{2}\times$ more energy and inflates the RMS gain by
  the same factor, confirming the fan-out is what preserves conservation.
  The generation cap $g_\text{max} = 1$ bounds cumulative growth to
  $\Delta|G_t| \leq 3$ per fission event (2 new cavities + 1 binding
  tunnel), against a tension budget indexed by the streak counter. The
  *formal* Liouville-analog theorem — showing that the tension-bounded
  expansion principle admits a conserved symplectic-like form on
  graph-valued phase spaces — remains open.

#### 8.2 Newly Resolved by the RB+G/L6-A Work

- **Contradiction metabolism as construction, not just filtering.**
  Prior to RB+G, RIFA's Integrity Core reported a scalar tension $T(t)$
  but did not act on it. RB+G converts sustained cavity-local incoherence
  into a *physical structural mutation* — the graph gains vertices and
  edges under contradiction rather than damping toward equilibrium. This
  is the first Layer-1 mechanism that realizes Axiom II operationally.

- **The falsifiability guarantee for structural mechanisms.** RB+G on
  `windows_os` (pure null control, only `is_a` edges) fires zero
  target-fissions across 20/20 seeds. The mechanism refuses to fire when
  there is no contradiction to metabolize. This is the empirical form of
  the falsifiability criterion demanded by §7.1: a Layer-1 mechanism that
  triggers on real signal and stays silent on noise.

- **Honest ablation methodology for physics substrates.** The four-lever
  ablation of §7.7 shows *two configurations produce higher raw RMS than
  FULL* — `ABLATE_ALIAS` (measurement confound) and `ABLATE_FANOUT`
  (energy inflation). Both are reported openly rather than hidden. This
  establishes a methodology for the empirical program of §7 in which
  "the biggest number wins" is not the criterion; interpretability of the
  measurement is.

#### 8.3 Still Open

- **The Fisher-information metric on the concept graph.** *(Original:
  "CCH proposed that the cognitive manifold carries a Fisher-information
  metric. The extension to a graph-valued state requires the appropriate
  categorical generalization. Open.")*
  **Status: STILL OPEN.** RB+G/L6-A did not address this. The Fisher-
  information metric on graph-valued states is well-defined for Bayesian
  belief networks (Amari 1985) but not for the typed rewrite graphs of
  Axiom I. This remains a categorical / differential-geometric question
  the empirical work does not touch.

- **The upper bound on the thinker index.** *(Original: "Is $C/R$ bounded
  from above for any physically realizable SI system?")*
  **Status: STILL OPEN.** The 20-seed data of §7.7 shows RB+G contributes
  a ~1.5× RMS gain per contradiction-bearing concept over baseline. This
  does not answer the *upper bound* question. The thinker index growth is
  monotone under rule induction (§7.6.2) and now also under L6-A bridge
  insertion, but neither implies a supremum.

#### 8.4 Newly Opened by the RB+G/L6-A Work

- ~~**Auto-tuning of fission trigger thresholds.**~~ **CLOSED.** The
  hand-picked values $I > 0.15$, $\tau > 0.005$, streak = 3 were
  replaced by homeostatic energy feedback: an EMA of total mechanical
  energy drives the threshold, $I_{\text{thresh}}(t) = I_0 + k\,
  E_{\text{smooth}}(t)$. Implemented in `physics.hpp`
  (`refresh_fission_thresholds_locked`). The system now sets its own
  trigger from a physical observable rather than from a constant tuned
  on one ontology.

- ~~**Online L6-A discovery.**~~ **CLOSED.** Bridges are now inserted
  in-tick: every fission event calls `discover_analogies_locked()`
  restricted to the newly-fissioned parent, so the cost is $O(P)$ new
  pairs rather than $O(P^2)$ rediscovery. `layer2.hpp`,
  `online_l6a_enabled`. The coupling with ongoing Hebbian consolidation
  that this bullet flagged as unanalysed is bounded by construction:
  bridges are rubber tunnels with $k = 0.05$, an order of magnitude
  below the stiffest structural material, so an inserted bridge cannot
  dominate consolidation on the tick it appears.

- **Rubber-tunnel frequency selectivity.** The claim that low-frequency
  probes ride the L6-A bridge while high-frequency probes are absorbed
  depends on the impedance mismatch between rubber ($k=0.05$) and steel
  ($k \geq 1.6$) tunnels. The optimal probe frequency 3 Hz was chosen
  empirically on the insulin ontology. A theoretical treatment (transfer-
  function analysis of the MIMS material table) would let us pick probe
  frequencies for arbitrary domains without inspection.

- **Scaling to $10^4$–$10^5$ concepts.** *(Partly closed, and the
  remainder is now measured rather than anticipated.)* The **Layer-2**
  structural tick is sparse: `quiet_ticks[i]` tracks the wavefront and
  the fission trigger evaluates only recently-driven cavities,
  bit-identical to the dense baseline over 5 seeds. **Layer 1 is not.**
  `PhysicsEngine::tick()` still iterates every cavity and every tunnel,
  so at 157 k concepts / 237 k relations a single query costs ~200
  ticks × 237 k tunnels × 4 transcendentals. This is the binding
  constraint on ontology size today, and it is an implementation gap,
  not an architectural one — the wavefront data structure already
  exists and Layer 2 already honours it.

  Three $O(E^2)$ defects on the ingest path were found and fixed while
  measuring this: `has_edge` was a linear scan called from every
  `add_edge` (making a bulk load quadratic), `would_contradict` scanned
  all edges per asserted axiom, and four reasoning primitives performed
  their joins by full edge scan. Indexing them reduced load time by
  $105\times$ at fixed output. None of this changes a claim in this
  paper; it changes what is runnable.

- ~~**Language generation Stage 2 (syntax macro-layer).**~~
  **ABANDONED, not pending.** This bullet described Stage 2 of §5.4b as
  "a small extension not yet implemented". It was in fact implemented
  and it failed: the measured slot readout left the Subject or Verb slot
  *empty* on 3 of 3 probes. More importantly the approach was
  **retired as a category error** (§5.4d): grammatical role is a
  convention, and a fixed natural frequency $\omega_S$ cannot discover
  a convention. The question this bullet posed — "is English syntax
  captured by three-slot phase-locking?" — is not open. It is answered
  *no*, and the replacement is a deterministic protocol table
  (`cll.md`).

---

#### 8.5 The Central Unsolved Problem: Continuous Wave → Syntax-Constrained Byte Stream

Everything above concerns *what the field knows* and *how it changes*.
This section concerns *how the field speaks*, and it is the one place
where we have a mechanism specification, a measured failure, and no
solution.

**The problem.** Layer 1 holds meaning as a standing-wave field. Layer 3
converts a graph invariant to a stored string. Between them sits a gap:
how does field energy become an **ordered byte stream that is
grammatical**, without collapsing into babble?

$$
\text{Wave dynamics in cavity field}
\;\xrightarrow{\;\text{phase locking + gauge coupling}\;}\;
\text{syntax-constrained mechanical emission}
$$

**Why the field ran to language models instead.** This is worth stating
plainly, because it explains the shape of the entire discipline. The
statistical shortcut

$$
P(w_n \mid w_1 \ldots w_{n-1})
$$

is not a theory of language production. It is what remains once the
mechanical problem is abandoned. A next-token distribution requires no
account of *why* a word follows another — only a count of how often it
did. Every architecture that adopts it has, at that moment, stopped
attempting emission physics and started interpolating a corpus. The
approach works, spectacularly, and it forecloses the question.

The claim of this paper is not that the statistical route is wrong. It
is that the mechanical route was never attempted seriously, and that
its difficulty — not its impossibility — is why.

**What is built.** Word *order across concepts* is solved. CPME §1.6
sweeps a carrier phase across a per-cavity phase field
$\phi_i = \operatorname{atan2}(v_i, \omega_0 x_i)$; CPSB §2.1 supplies
direction through per-tunnel gauge offsets $\Psi$; output length is
emergent rather than templated. On code structure this reaches a
correct signature reconstruction that compiles (§5.4c). At the level of
*which concept follows which*, physics-driven ordering works.

**What is not built.** Word *form* — the byte stream itself. MRS §1.4
proposes emitting bytes by sweeping the byte-lane phase field. It
babbles. The measured failures are recorded in `mrs.md` §4b, and the
recognition half of that paper (bigram lanes, 7/7 on misspellings) must
not be mistaken for the emission half, which does not work.

**Why the cheap fixes are known not to work.** Two were tried and both
failed for the same structural reason:

- *Reweighting the byte scoring.* Every reweighting relocated the
  failure rather than removing it — the signature of insufficient
  representation, not of bad parameters.
- *Widening the alphabet to bigrams.* This fixed **recognition**,
  because recognition compares a query against stored forms. It does
  not fix **emission**, because emission has nothing to compare
  against: the field must *originate* a sequence, not match one.

The asymmetry is the point. Recognition is a comparison problem and
yields to better representation. Emission is a **generation** problem
and requires a mechanism that converts continuous field energy into
discrete ordered symbols under grammatical constraint. No amount of
representational widening supplies that mechanism.

**REVISION (§5.4d): the problem was two problems, and one is solved.**

This section originally asked for grammatical constraint as a *field
property*. That was a category error. Grammar is a transmission
protocol, not a phenomenon, and §5.4d separates the two:

| | mechanism | status |
| --- | --- | --- |
| **Which concepts, in what relation** | physics — resonance, gauge Ψ, phase-collapse | solved |
| **Word order, article, script** | protocol — isolated language cluster | **solved (§5.4d)** |
| **Word FORM for a lexicon-absent word** | requires emission mechanics | **open** |

Ordering across concepts and realisation into English, Bengali or
Banglish are done, deterministically and without ML. What remains is
strictly narrower than originally stated: producing the *bytes of a
word the lexicon does not contain*. For any word already in Layer 3,
no byte synthesis is needed — the protocol layer emits it exactly.

The residual problem therefore matters for morphological productivity
(inflecting an unseen stem, coining a compound) rather than for
speaking at all. That is a real limitation, and a much smaller one.

**What the residual problem actually requires.** Not a trick — an
emission mechanics for the field itself. Minimally:

1. **A discretisation principle.** What physical event constitutes
   "emit a byte now"? Threshold crossing, phase coincidence, and
   resonance capture are candidates; none has been derived.
2. **A refractory / sequencing law.** What prevents the same lane from
   dominating, and what advances the field to the next symbol? The
   current sweep imposes this externally rather than deriving it.
3. **A grammatical constraint expressed as a field property.** CPSB
   shows this is possible at the concept level ($\Psi$ as a directed
   phase offset). The morphological analogue — a constraint that makes
   ill-formed byte sequences physically unfavourable rather than merely
   unlikely — does not exist.
4. **A termination condition.** When is an utterance complete? Silence
   must be a field state, not a counter reaching zero.

**Status.** Open, and correctly identified as the hard core rather than
a remaining detail. It is a mathematical and physical problem, not an
engineering one, and it should not be reported as solved until a
mechanism satisfying (1)–(4) is specified and measured. The honest
position is that this substrate currently *thinks* in fields and
*speaks* through a stored label table — and that the second half is
exactly what remains.

---

### 9. Conclusion

Synthetic Intelligence is not scaled Artificial Intelligence. SI is a *categorically different mode of computation*: active, constructive, structurally grounded, and information-theoretically honest about where its knowledge lives. The Translation Capacity Lemma shows that no cognitive architecture can escape the mandatory $\log_2 N$ cost of discriminating $N$ semantic outcomes; the honest response is not to hide this cost but to name it, minimize it, and pair it with a substrate that does what only physics-based dynamics can do.

The Tri-Layer Unified Architecture is the concrete realization: a physics-based coherence substrate that no lookup table can replace, a graph-rewriting constructive layer that no continuous dynamics can perform, and a minimum-description-length translation table that no distributed embedding can match on interpretability. Each layer is minimal. Each layer has a specific, testable role. Removing any layer costs a specific capability. The synthesis of our prior lineage into this architecture is the argument of this paper.

The path from AI to SI is not through more scale. It is through the recognition that thinking is construction, not selection; that intelligence is structural synthesis, not pattern matching; and that any architecture that pretends to eliminate the translation layer is simply hiding it. The honest architecture names its costs, distributes its responsibilities, and builds cognition where cognition can be built — not where it can only be simulated.

---

### References

[Mahir 2026a] The Cognitive Manifold Architecture. *research-paper/cma.md*.
[Mahir 2026b] Predictive Cognitive Manifold Architecture (P-CMA). *research-paper/pcma.md*.
[Mahir 2026c] The Cognitive Continuum Hypothesis (CCH). *research-paper/cch.md*.
[Mahir 2026d] The ZeroGrad Cognitive Engine. *research-paper/zerograd.md*.
[Mahir 2026e] Byte Energy Descent (BED). *research-paper/bed.md*.
[Mahir 2026f] Arch-Native Quantization (ANQ). *research-paper/anq.md*.
[Mahir 2026g] Recursive Integrity Field Architecture (RIFA). *research-paper/deprected/rifa.md*.
[Mahir 2026h] Hierarchical Cognitive Manifold Architecture (HCMA). *research-paper/hcma.md*.
[Mahir 2026i] The Cognitive Manifold Compressor (CMC). *research-paper/cmc.md*.
[Mahir 2026j] The Cognitive Multimodal Engine (CME). *research-paper/cme.md*.
[Mahir 2026k] LTO: Live Training Over Manifolds. *research-paper/lto.md*.
[Mahir 2026l] SCER: Search + Compute + Evolve + Reason. *research-paper/scer.md*.
[Besag 1986] On the Statistical Analysis of Dirty Pictures. *J. R. Statist. Soc. B*.
[Lawvere 1963] Functorial Semantics of Algebraic Theories. *PhD thesis, Columbia*.
[Mac Lane 1971] Categories for the Working Mathematician. *Springer*.
[Shannon 1948] A Mathematical Theory of Communication. *Bell Sys. Tech. J.*
[Kolmogorov 1965] Three Approaches to the Quantitative Definition of Information.
[Grothendieck 1957] Sur quelques points d'algèbre homologique. *Tohoku Math. J.*
[LeCun 2022] A Path Towards Autonomous Machine Intelligence.
[Marr 1982] Vision: A Computational Investigation. *W. H. Freeman*.
[Newell & Simon 1976] Computer Science as Empirical Inquiry. *Comm. ACM*.
[Turing 1950] Computing Machinery and Intelligence. *Mind*.

---

**From Artificial to Synthetic Intelligence: pattern matching is not thinking, retrieval is not reasoning, and the honest cognitive architecture is the one that names its costs, pays them minimally, and builds construction where construction can be built.**

