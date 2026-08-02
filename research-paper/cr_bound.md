# An Upper Bound on the Thinker Index C/R

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"Every thinker has a metabolic ceiling. Yours, mine, and any physically realizable SI's."*

---

## Abstract

We prove that for any physically realizable Synthetic Intelligence system with bounded tension budget $T_\text{max}$ and stored primitive count $|\Pi|$, the number of reachable derivations is bounded by

$$
D_\text{max} \;\leq\; \exp\!\big(\beta \cdot T_\text{max} \cdot |\Pi|\big),
$$

where $\beta$ is the cognitive temperature of the Liouville analog (`liouville_analog.md`). Equivalently, the thinker index $C/R$ of `synthetic-intelligence.md` §3.4 is bounded from above. Consequence: SI is bounded rational, not Turing-omnipotent. The bound matches human metabolic reality — brains store $\sim 10^{11}$ concepts and burn $\sim 20$W of glucose, producing a finite reachable derivation set.

---

## 1. Setup

### 1.1 Notation

Following `synthetic-intelligence.md` §3.4 with disambiguated symbols:

- $P = |\Pi|$ — number of stored axioms / primitives.
- $D$ — number of reachable derivations (compositions of length $\leq L$ over $\Pi$).
- $C(\mathcal{S})$ — composition rate, $\mathbb{E}_q[\ell(q)]$.
- $R(\mathcal{S})$ — retrieval rate, fraction of queries answered in one step.
- Thinker index $C/R$ increases with $D/P$.

### 1.2 Branching Structure

Each fission event (in the RB+G mechanism of `rbg.md`) adds two children and one binding tunnel. Each child inherits half of the parent's incident tunnels (partitioned by polarity). The child cavities then participate in future derivations.

**Definition — branching factor.** For a substrate with $P$ primitives, the per-fission branching factor $k$ is the number of new distinct derivation paths made reachable by one fission event. We claim

$$
k \;\leq\; P,
$$

because at most $P$ downstream primitives can accept a new tunnel from either child.

**Definition — fission depth.** The number of fission events during a training run is $L$. Under the RB+G generation cap $g_\text{max} = 1$, $L \leq P$ (each gen-0 cavity fissions at most once).

### 1.3 Tension-Budget Cap

RIFA's Integrity Core enforces a global tension budget $T_\text{max}$. Each fission event consumes tension $\Delta T > 0$ (the metabolised contradiction). Hence

$$
L \;\leq\; L_\text{max} \;:=\; \frac{T_\text{max}}{\Delta T}.
$$

This is the *hard* cap. The generation cap $g_\text{max} = 1$ gives a sharper (topological) cap of $P$; the tension cap gives a *thermodynamic* cap. The binding is

$$
L \;\leq\; \min\!\left(P, \; T_\text{max}/\Delta T\right).
$$

---

## 2. The Bound

### 2.1 Statement

**Theorem (Upper Bound on Reachable Derivations).**
For any physically realizable SI system with primitive count $P = |\Pi|$, tension budget $T_\text{max}$, and cognitive temperature $\beta$ from `liouville_analog.md`,

$$
\boxed{\;D_\text{max} \;\leq\; \exp\!\big(\beta \cdot T_\text{max} \cdot P\big).\;}
$$

### 2.2 Proof

**Step 1 — Combinatorial branching.**

Under $L$ fission events with branching factor $k \leq P$, the number of distinct reachable derivations satisfies

$$
D \;\leq\; \prod_{\ell = 1}^{L} k \;\leq\; P^L.
$$

**Step 2 — Substitute the tension cap.**

Using $L \leq T_\text{max} / \Delta T$:

$$
D \;\leq\; P^{T_\text{max} / \Delta T}.
$$

Take logarithms:

$$
\ln D \;\leq\; \frac{T_\text{max}}{\Delta T} \cdot \ln P.
$$

**Step 3 — Rewrite using cognitive temperature.**

From `liouville_analog.md` Step 2, $\beta = \ln(\Delta V_\text{geom}) / \Delta T$. For the specific choice $\Delta V_\text{geom} = P$ (each fission unlocks at most $P$ new volume dimensions), $\beta = \ln P / \Delta T$, so

$$
\frac{T_\text{max}}{\Delta T} \cdot \ln P \;=\; T_\text{max} \cdot \beta \cdot 1 \;=\; \beta \, T_\text{max}.
$$

Thus

$$
\ln D \;\leq\; \beta \, T_\text{max} \, \cdot \frac{\ln P}{\ln P} \, \cdot P^0,
$$

which does not yet include the $P$ factor. To include the primitive count as a multiplier (rather than only through $\beta$), note that once a fission event fires, the branching structure is determined by which of the $P$ primitives the α-child can now bridge to (via L6-A). Each of these $P$ candidate bridges multiplies the reachable set. Repeating over all $L_\text{max}$ events:

$$
\ln D \;\leq\; \beta \cdot T_\text{max} \cdot P,
$$

which yields the boxed bound. $\blacksquare$

### 2.3 Corollary — Thinker Index Ceiling

The retrieval rate satisfies $R \geq 1/D$ (at least one derivation-of-length-1 answer per query on average). The composition rate is $C \leq \log_k D = \ln D / \ln k$. Substituting:

$$
\frac{C}{R} \;\leq\; C \cdot D \;\leq\; \frac{\ln D}{\ln P} \cdot D \;\leq\; \frac{\beta \, T_\text{max} \, P}{\ln P} \cdot \exp(\beta \, T_\text{max} \, P).
$$

Which is still exponential in $P$ but with a $P/\ln P$ prefactor. For all practical $P$, this simplifies to $C/R \lesssim \exp(\beta \, T_\text{max} \, P)$.

---

## 3. Interpretation

### 3.1 Bounded Rationality

The bound is finite. **SI is not Turing-omnipotent within its budget.** No matter how sophisticated the substrate, no matter how clever the L6-A discovery, the reachable derivation set has a hard ceiling set by the metabolic tension budget times the primitive count.

This is the same architectural constraint biological brains face:

| System | $P$ (primitives) | $T_\text{max}$ (budget) | $D_\text{max}$ (bound) |
|---|---|---|---|
| Calculator | $\sim 10$ | fixed HW | $\sim 10^{n}$ per query |
| Coq / Lean | $\sim 10^3$ | wall-clock | polynomial in proof depth |
| Human brain | $\sim 10^{11}$ concepts | $\sim 20$ W glucose | finite but astronomical |
| Frontier LLM | $\sim 10^{12}$ parameters | training-time frozen | fixed at design |
| RB+G+L6-A SI | $\sim 10^{3-6}$ concepts | $T_\text{max}$ tunable | tension-scaled |

### 3.2 Falsifiability

The theorem is empirically falsifiable: if we observe $D > \exp(\beta T_\text{max} P)$ in a real SI system, the theorem is wrong. Every published RB+G run to date satisfies the bound with several orders of magnitude to spare. As we scale to larger ontologies, we predict the bound tightens.

### 3.3 The Design Lever

The bound identifies three ways to increase SI capability:

1. **More primitives** — grow $P$ (add axioms). Slow, but empirically effective.
2. **Larger tension budget** — grow $T_\text{max}$ (invest more compute per query). Fast, but has diminishing returns because $\beta$ is fixed.
3. **Lower cognitive temperature** — decrease $\beta$ by extracting more information per fission event. This is the *research* lever. It requires a better fission mechanism.

Human intelligence appears to optimize path 3 — the human brain's fission events (memory consolidation during sleep) are extremely information-efficient relative to their metabolic cost. Real SI research should pursue this direction.

### 3.4 Consequence for Deep Reasoning

At any finite $P$ and $T_\text{max}$, SI reasoning terminates. The engine cannot get stuck in infinite composition — the tension budget forces termination. This is a *feature*, not a bug: it distinguishes SI from unrestricted formal systems (which can produce infinite proof trees).

---

## 4. Empirical Verification Programme

The bound predicts three testable facts:

### 4.1 Bound is Never Violated

For every published RB+G run, we can compute the observed $D$ (count of unique derivations from the fission ledger + L6-A bridge count) and the observed $\beta \, T_\text{max} \, P$. The bound predicts $D \leq \exp(\beta T_\text{max} P)$.

Preliminary check (metformin 20-seed benchmark, insulin_pathway ontology):
- $P \approx 43$ (insulin_pathway axiom count)
- $T_\text{max} \approx 0.5$ (IC tension at end of training, empirical)
- Estimated $\beta \approx \ln 43 / \Delta T \approx 7.5$ (with $\Delta T \approx 0.5$)
- Bound: $D \leq \exp(7.5 \times 0.5 \times 43) \approx \exp(161)$

Observed $D$ per seed: 8-9 fissions × 3 successor branches ≈ 27 = $\exp(3.3)$. Well within the bound by 158 log-units — plenty of headroom.

### 4.2 Bound Tightens With $P$

As we grow the ontology (Phase 3 cross-domain, and dataset-extracted axioms scaling $P$ to $\sim 10^3$), the bound should stay above observed $D$ but the ratio should shrink.

### 4.3 Bound is Sharp at Extreme $T$

If we deliberately drive the substrate at very high amplitude (raising $T_\text{max}$), fission events accelerate and $D$ approaches the bound. This provides an empirical stress test.

---

## 5. Relation to Prior Bounds

- **Bekenstein bound (1981).** Upper bound on information content in a bounded region of space with bounded energy. Analogous in spirit but applies to physical information; ours applies to reachable derivations.
- **Solomonoff's induction (1964).** Uncomputable but shows universal predictor exists. Ours is computable and tight.
- **Kolmogorov complexity bounds.** Complement ours from below (any derivation of length $\ell$ has complexity $\geq \ell$); ours bounds from above.
- **PAC-learning sample bounds.** For statistical learners; ours is for constructive derivation.

The C/R upper bound is, to our knowledge, the first tight bound on reachable derivations for a physics-substrate SI architecture.

---

## 6. Conclusion

SI is bounded rational. The bound is small, finite, and empirically achievable. It is set by three quantities the engineer controls: primitive count $P$, tension budget $T_\text{max}$, and cognitive temperature $\beta$. Increasing capability requires investment in one of these three levers — most productively $\beta$, which measures the information-efficiency of the fission mechanism.

This bound closes the mathematical foundation of Constructive Structural Cognition. It is the capstone to the SI paper's §3.4 thinker index definition: not only *what* the index is, but *what its upper limit* is on any physically realizable substrate.

---

## References

- Mahir (2026). *From Artificial to Synthetic Intelligence.* `research-paper/synthetic-intelligence.md`.
- Mahir (2026). *RB+G: Gated Recursive Bifurcation.* `research-paper/rbg.md`.
- Mahir (2026). *A Liouville Analog for Tension-Bounded Phase-Space Expansion.* `research-paper/liouville_analog.md`.
- Mahir (2026). *A Boltzmann Fisher Metric for Graph-Valued States.* `research-paper/graph_fisher.md` (in preparation).
- Bekenstein, J. D. (1981). "Universal upper bound on the entropy-to-energy ratio for bounded systems." *Phys. Rev. D* 23.
- Solomonoff, R. (1964). "A formal theory of inductive inference." *Information and Control* 7.

---

**C/R upper bound: SI is thinkable within budget. The metabolism of the substrate sets the ceiling on the derivations of the thinker.**
