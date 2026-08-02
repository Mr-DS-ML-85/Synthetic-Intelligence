# Conceptual Mutation Architecture (CMA): Generating Non-Trivial Invariants through Structural Bisociation

**Author:** Irfan Mahir
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com


---

## Abstract

Discrete graph-rewriting engines for Synthetic Intelligence (SI) traditionally rely on axiomatic deduction and strict topological constraints. While this ensures high logical coherence and eliminates the probabilistic "hallucinations" characteristic of Large Language Models (LLMs), it creates a closed-system bottleneck: the engine cannot generate novelty outside its initial topological training data. We introduce the Conceptual Mutation Architecture (CMA), an extension for discrete C++ graph systems that leverages **Structural Bisociation**. By systematically injecting forced, non-logical edges between semantically distant nodes, and subsequently forcing the engine's inference pipeline to bridge the resulting logical gap, CMA allows the SI to synthesize original concepts and non-trivial invariants.

---

## 1. Introduction: The Limits of Deductive Ontologies

In continuous probabilistic models (LLMs), "novelty" is a byproduct of high-temperature token sampling across a massive, non-linear latent space. In a discrete graph engine, where concepts are strict nodes and relationships are explicit structural edges, novelty cannot arise probabilistically. If an SI engine is constrained to classical deduction (e.g., Modus Ponens) and induction over a fixed ontology, its thought processes become purely tautological.

To achieve human-like insight, the engine must perform what Arthur Koestler termed *Bisociation*—the collision of two mutually incompatible logical frames. CMA mathematically formalizes this collision within a graph-theoretical framework.

## 2. Mathematical Framework of the Static Engine

Let the SI's internal ontology be a directed graph $G = (V, E)$, where $V$ represents the set of concept nodes and $E$ represents the set of verified logical relationships (edges).

The semantic distance $d(v_i, v_j)$ between any two nodes is defined by the shortest path algorithm across the graph's topology. In the standard deduction loop, the probability of the engine traversing or forming a new connection between $v_i$ and $v_j$ is inversely proportional to their distance:

$$ P_{deduce}(v_i \rightarrow v_j) \propto \frac{1}{d(v_i, v_j)^2} $$

This ensures logical safety but absolutely prevents creative leaps. A node like `gravity` will never interact with `RDMA` because $d(\text{gravity}, \text{RDMA}) \to \infty$ in the trained ontology.

## 3. The Mutation Primitive ($\mathcal{M}$)

CMA introduces a stochastic mutation operator $\mathcal{M}$ that periodically overrides the deductive pipeline. Instead of following existing edges, $\mathcal{M}$ intentionally targets nodes with maximum semantic distance to force a hypothesized edge, denoted as $E_{hyp}$.

The engine selects two nodes $v_a$ and $v_b$ such that their semantic distance exceeds a hyperparameter threshold $\tau_{novelty}$:

$$ d(v_a, v_b) > \tau_{novelty} \implies E_{hyp}(v_a, v_b) = 1 $$

### 3.1 The Bisociation Energy Function

Once $E_{hyp}$ is injected, the engine enters the **Resolution Phase**. The system is put under logical "tension," represented by energy $U$. The engine must attempt to build a coherent sub-graph path $P$ that justifies the connection between $v_a$ and $v_b$ using intermediary nodes.

The energy of the hypothesized connection is defined as:

$$ U_{hyp} = \sum_{k=1}^{n} R(e_k) - \alpha \cdot C(v_a, v_b) $$

Where:

* $R(e_k)$ is the logical resistance of the intermediary edges used to build the bridge.
* $C(v_a, v_b)$ is the structural conductivity of the new conceptual bridge.
* $\alpha$ is a scaling constant representing the system's current "exploratory state" (analogous to dopamine).

## 4. Logical Resolution and The Novelty Test

The forced edge $E_{hyp}$ is initially treated as a logical contradiction. The SI's core engine must resolve this contradiction by generating intermediate analogies.

For example, if $\mathcal{M}$ forces $E_{hyp}(\text{gravity}, \text{RDMA})$:

1. The engine decomposes `gravity`: $\text{gravity} \to \text{attractive\_force} \to \text{data\_pull}$.
2. The engine decomposes `RDMA`: $\text{RDMA} \to \text{direct\_memory\_access} \to \text{data\_pull}$.
3. The bridge is formed at the intersection node `data_pull`.

If the engine successfully constructs a path $P^*$ that minimizes the bisociation energy $U_{hyp}$ below a critical threshold $U_{crit}$, the edge is validated.

$$ \text{If } \min(U_{hyp}) < U_{crit} \implies E_{hyp} \to E_{verified} $$

The graph has now permanently adopted a novel logical connection that was not in its initial training set. The SI has "invented" a new idea.

## 5. Algorithmic Pruning (Decay)

Not all mutations result in profound insights; many are simply nonsensical. If the resolution pipeline fails to find a path $P^*$ that satisfies the energy threshold within $N$ clock cycles, the hypothesized edge undergoes rapid topological decay:

$$ \frac{d}{dt} E_{hyp} = -\lambda E_{hyp} $$

Where $\lambda$ is a severe decay constant. The edge is pruned from the graph, preventing the graph from filling with unresolved, contradictory noise.

## 6. Conclusion

The Conceptual Mutation Architecture liberates discrete graph-rewriting systems from the trap of tautological deduction. By formalizing Koestler’s bisociation as a stochastic topological operator $\mathcal{M}$ constrained by an energy-minimization resolution phase, the SI achieves true structural novelty. It gains the ability to "brainstorm" by forcing disparate concepts together and rigorously testing their logical intersections, bridging the gap between static knowledge retrieval and dynamic human-like creativity.
