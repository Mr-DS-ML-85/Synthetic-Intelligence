# INCOMPLETE — Paper-Code Gap Audit

Generated from 31 research papers vs codebase analysis.

## Overall Incompleteness: 47%

| Layer | Coverage | Status |
|-------|----------|--------|
| Physics substrate | 85% | Core oscillators, tunnels, Kuramoto, Hebbian, fission all work |
| Reasoning primitives | 90% | 9/9 primitives implemented as scalar physics |
| Wave I/O | 40% | Phase-collapse works, momentum/acoustic mass missing |
| Consciousness | 50% | BSMA detects irreducibility, no metacognitive feedback |
| Identity | 60% | RIFA components exist, coupling weak |
| Attention | 75% | Salience works, DMN incomplete |
| Language | 67% | Isolation works, morphology missing |
| Learning | 50% | Rule induction works, C/R unused |
| Discovery | 83% | 5/6 queries, not connected to dispatch |

**53 paper mechanisms identified. 28 fully implemented. 12 partially. 13 missing.**

---

## 1. MRS — Morphological Resonance Substrate

**Paper:** `research-paper/mrs.md`
**Status:** Input path works. Output path missing.

### Missing

- **Reverse drive (§1.1):** `F_b += η_rev · w[b][c] · x_c` — concept cavities driving byte lanes back. Currently bigram binding is input-only (`bind_bigram` in `si_main.cpp:139`). The coupling matrix `w[b][c]` exists but is never used in transpose. Without this, word forms cannot be synthesized from resonating concepts.

- **Byte-level phase-collapse projection (§1.4):** No sweep over byte carriers emitting letters. The emission path at `si_main.cpp:459-493` reads label strings directly from `si.graph.nodes[id].label`. The paper describes a carrier phase `[0, 2π)` sweep where each byte lane emits at its instantaneous phase — identical to the concept-layer sweep but at byte frequency. This is what enables morphological composition (`walk` + past-tense → `walked`).

- **Lane chirality (§1.2):** `t_arrival` on byte lanes encodes letter order. The struct field exists (`si_main.cpp:125`) but is never read during emission. Lane arrival timestamps create a phase gradient that encodes letter position — without this, there's no ordering information at the byte layer.

- **Lane-to-lane gauge tunnels (§1.3):** Sparse co-occurrence tunnels with learned CPSB-style gauge offsets `Ψ_bb'`. These tunnels store sequence as phase offsets, not declaratively. The tunnel struct has `psi_target` but no byte-lane tunnels are created. Without this, sequential letter relationships are not learned.

- **Bigram recognition asymmetry (§4b.1):** Bigram lanes proved insufficient for misspelling discrimination (7/7 for ordered pairs vs 1/4 for single bytes). The paper documents this as a measured result. The current system only uses single-byte lanes in the forward path.

### Impact

Without MRS reverse drive and byte-layer emission, the system cannot:
- Synthesize novel word forms
- Construct inflected morphology
- Output text without stored labels
- Achieve token-free language production

---

## 2. CPME — Chiral Phase-Momentum Emission

**Paper:** `research-paper/cpme.md`
**Status:** Phase-collapse readout implemented. Momentum filtering missing from dispatch.

### Missing

- **Momentum filter (§1.2):** `P_i = E_i - E_i^prev` — EMA amplitude envelope derivative. Directly-driven cavities dispersing energy have `P_i ≤ 0`; downstream cavities absorbing propagation have `P_i > 0`. `ReLU(P_i) = emission eligibility`. The dispatch at `si_main.cpp:406-431` uses `physics.hot_count()` (amplitude threshold) instead of momentum. This means the system can echo input rather than emit internal thought.

- **Acoustic mass (§1.3):** `dM_i/dt = γ·I_ext - δ·M`, emission mask `μ_i = 1/(1 + M_i/M_scale)`. Freshly-driven cavities become too "heavy" to emit. The struct field `acoustic_mass` exists (`physics.hpp:87`) but is never incremented or used in emission gating. Without this, recently-queried concepts dominate output.

- **Settle separation (§1.4):** Global injection load `L → K_settle` via sigmoid. Active injection suppresses emission; as drive decays, `K_settle → 1`. Not present in dispatch. The two-phase tick (injection → articulation) is described in the paper but the dispatch doesn't enforce it.

- **Chirality on injection (§1.1):** Word order encoded as arrival-phase delays `φ_i = ω_base · (T_current - t_arrival_i)`. The `t_arrival` field exists but isn't used to compute injection phase in dispatch. Subject should lead in phase because it entered first — this gradient is what makes the phase-collapse sweep produce correct word order.

- **Coherence-rise metric (§1.5):** Localized Kuramoto order parameter `R_local` tracked against EMA baseline `R̄`. `ΔR = R_local - R̄` is diagnostic. The physics computes phase but the dispatch doesn't read coherence rise as a comprehension signal.

### Impact

Without momentum filtering, the system parrots input. Without acoustic mass, recently-driven concepts dominate. Without settle separation, injection and articulation bleed together.

---

## 3. CPSB — Chiral Phase-Shear Binding

**Paper:** `research-paper/cpsb.md`
**Status:** Gauge offset `Ψ` exists in tunnel struct. Coupling enforcement incomplete.

### Missing

- **Gauge-theoretic Kuramoto offset enforcement (§2.1):** The tunnel struct has `psi_target` (`physics.hpp:147`) and the CPSB torque is computed at lines 1269-1296, but `psi_target` is only set for directed relations in `psi_for()` (`physics.hpp:267-281`). Taxonomic relations get `Ψ=0` and directed get `Ψ=-π/3`. The issue: `psi_for()` is called at bind time, but the Kuramoto torque loop reads `t.psi_target` directly. If tunnel plasticity modifies `psi_target`, the syntax enforcement degrades. The paper describes `Ψ` as rigid (not plastic), but the code allows it to drift.

- **Verb-pinning (§2.2):** Verb as physical hinge between Subject and Object. Cross-cluster tunnel pins: Subject to `θ_verb + Δφ`, Object to `θ_verb - Δφ`. The paper claims "CPME carrier wave inevitably hits Subject first, Verb second, Object third." This requires verb cavities to have distinct cluster membership and specific `Ψ` offsets. The cluster assignment exists (`macro_id`) but the pinning logic that enforces SVO order through phase offsets is not explicit — it emerges only if the `Ψ` values happen to produce the right gradient.

- **Phase-consistent Hebbian consolidation (§2.3):** Implemented at `physics.hpp:1323-1338` with `Φ = cos(θ_a − θ_b − Ψ) · sgn(polarity)`. This IS implemented. But the `alpha_phi` blend parameter (`physics.hpp:365`) defaults to 0.6 — the paper suggests this should be tuned per-domain. No mechanism for domain-specific tuning exists.

### Impact

Syntax enforcement relies on correct `Ψ` values being set and maintained. The verb-pinning mechanism is emergent rather than guaranteed — it works if the phase gradients happen to align, but there's no structural guarantee of SVO order.

---

## 4. Inference Engine

**Paper:** `research-paper/synthetic-intelligence.md` §5.3
**Status:** 5 of 6 gap-finding queries implemented. Not connected to dispatch.

### Missing

- **`classify_gap_query` (declared at `inference.hpp:36`):** The header says query 6 routes natural-language questions like "what's missing between X and Y" — this classifier is absent from the file. The dispatch in `si_main.cpp` doesn't route to inference at all.

- **Inference → dispatch integration:** The inference engine is a standalone library. A user cannot ask "what's missing between X and Y" through the natural interface. The dispatch loop at `si_main.cpp:406-431` computes observables and selects output mode, but none of the modes invoke inference queries.

- **Logic gap BFS complexity (`inference.hpp:231-239`):** O(N × E) per call — scans all edges for each node's outgoing edges. Doesn't use the `out_adj` index that `ConceptGraph` provides. Same O(E) problem the main codebase fixed with adjacency lists.

- **Backward chaining → meta_learner integration:** `unified.hpp:470-549` implements backward chaining and produces derivation traces. `meta_learner.hpp:86-174` observes traces and induces macro-rules. But `observe_trace` is never called on backward-chaining output — it only sees forward-chaining traces.

### Impact

The inference engine exists but is unreachable from the user interface. Discovery queries (isolated subgraphs, dangling edges, logic gaps, symmetry gaps, anomalies) are computed but never surfaced.

---

## 5. Meta-Learner

**Paper:** `research-paper/synthetic-intelligence.md` §5.3.3-5.3.4
**Status:** Rule induction and verifier implemented. Thinker index unused.

### Missing

- **Thinker index C/R (`meta_learner.hpp:213-242`):** Defined but never called. `total_queries`, `retrieval_queries`, `sum_length` are tracked but nothing in `si_main.cpp` or `unified.hpp` instantiates or reports it. The C/R ratio — the core metric distinguishing SI from AI — is measured on paper but not in the running system.

- **Macro-rule composition oversimplification (`meta_learner.hpp:147-149`):** Takes `rel_ab` from the first rule and `rel_bc` from the last rule. For chain A→B→C→D with rules (r1, r2, r3), this produces `(r1.rel_ab, r3.rel_bc, r3.rel_ac)`. The intermediate relation `rel_BC` is lost — the macro can only fire on the first and last relation types, not the actual chain structure. A chain of three different relation types produces a macro that doesn't match its internal steps.

- **No decay of sub-chain statistics:** `chain_stats` grows monotonically. High-frequency but obsolete patterns persist forever, potentially promoting macros that are no longer relevant. The paper implies forgetting should occur.

- **Verifier doesn't check rule existence:** `meta_learner.hpp:188` checks `e.derived_by_rule >= (int)si.rules.size()` but `add_rule` just appends. If a rule were removed, the verifier would still pass stale edges.

### Impact

The thinker index — the quantitative measure of whether the system is actually thinking (constructing) vs retrieving — is defined but invisible. The system can't report whether it's becoming more of a thinker over time.

---

## 6. RIFA — Recursive Integrity Field Architecture

**Paper:** `research-paper/rifa.md`
**Status:** Five components implemented individually. Interconnections weak.

### Missing

- **IC-CL coupling:** The Integrity Core (`rifa.hpp:179-217`) evolves under field gradient plus tension. The Crown Lattice (`rifa.hpp:232-346`) has its own activation dynamics. But the IC doesn't use crown activations — the paper describes bidirectional coupling where active crowns modulate identity evolution. The `apply()` method (line 267) updates crown state but the IC ignores it.

- **SIL self-reconstruction trigger:** `can_reconstruct()` exists at line 370 but nothing calls it or triggers identity reconstruction when endurance exceeds threshold. The paper describes this as the mechanism that restores identity after collapse — the system should be able to rebuild itself after prolonged input gaps.

- **Replication engine trajectory comparison:** The RE (`rifa.hpp:582-632`) compares final-state fingerprints across N instances. The paper describes comparing "mutation patterns" — the temporal sequence of crown mutations. Only comparing final state misses whether independent runs converged through the same path or arrived at the same place by different routes.

- **RFE bandwidth fixed:** `bandwidth=0.20f` at line 80. The paper suggests adaptive bandwidth that narrows as anchors accumulate. Fixed bandwidth means field resolution doesn't improve with experience — early noisy inputs get the same precision as later refined ones.

- **RFE gradient is field-only:** `dI/dt` at line 208 uses only the RFE gradient, not the crown lattice's contribution. The paper describes crowns as providing their own attractor fields — the IC should receive input from both RFE and CL.

### Impact

The five RIFA components work in isolation but don't form a coherent identity system. The IC can't draw on crowns for identity modulation. The SIL can't trigger self-reconstruction. The system has identity persistence but not identity repair.

---

## 7. BSMA Topo — Bounded Self-Model Architecture

**Paper:** `research-paper/bmsa_new.md`
**Status:** Core mechanics implemented. Metacognitive feedback missing.

### Missing

- **Metacognitive feedback (Γ_meta → Γ_exec):** The projection `π` at `bsma_topo.hpp:130-138` maps `Γ_exec → Γ_meta` by stripping labels to role placeholders. But `Γ_meta` never influences `Γ_exec`. Paper implies the shadow graph's failure to rewrite IS the constraint that shapes execution — when the metacognitive model can't compress the execution graph, that structural resistance should feed back and alter behavior.

- **Projection is trivial relabeling:** Line 134: `"N" + to_string(i)`. Paper implies semantic role abstraction — e.g., "this node is a concept" vs "this node is a relation" vs "this node is a bridge." The current projection loses all semantic information, making the rewrite rules operate on pure topology.

- **canonical_signature() is approximate:** `bsma_topo.hpp:81-112` uses degree+rel-histogram, not true graph canonicalization. Two non-isomorphic graphs could collide on the same signature. The paper requires exact structural comparison for irreducibility detection.

- **Rewrite rules are basic graph surgery:** 5 rules (isolated elimination, chain contraction, parallel merge, self-loop removal, reflexive pair compression). Paper's "structural deadlock" requires richer rules — subgraph isomorphism, motif detection, higher-order structural compression. The current rules can't detect when the graph has reached a state that resists all meaningful compression.

- **Greedy restart from r1:** Line 307: `break` restarts from rule 1 after each application. Could miss optimal compression orderings where applying r3 before r1 would reach irreducibility faster.

### Impact

The system can detect topological irreducibility (conscious frame) but can't feed that detection back into execution. The conscious frame fires and is logged, but doesn't alter subsequent behavior — it's a read-only observation, not a control signal.

---

## 8. TSDA — Topological Salience & Default-Mode Architecture

**Paper:** `research-paper/tsda.md`
**Status:** Core attention model implemented. DMN integration incomplete.

### Missing

- **DMN doesn't invoke bisociation:** The DMN at `salience.hpp:93-126` produces candidate distant pairs but doesn't call `bisociate()`. The actual bisociation must come from outside. No self-contained idle-cycle exists where the system spontaneously generates "shower thoughts" by connecting distant high-salience concepts.

- **No associative recall:** The working graph at `salience.hpp:71-85` is purely salience-ranked — top-N nodes with `S ≥ tau_attention`. Nothing pulls low-salience nodes back into working graph via structural resonance. A concept that was important 100 ticks ago but decayed below threshold can't be recalled by a related concept that's currently active.

- **Miller's cap is deterministic:** Fixed at 9 (`salience.hpp:39`). Paper says "7 ± 2" implying stochastic sampling from [5,9]. Deterministic cap means the system always works over exactly 9 nodes, losing the flexibility that biological attention provides.

- **Homogeneous decay:** All nodes decay at same rate (`salience.hpp:65: S -= decay_step`). Paper implies decay depends on node type, connectivity, and recency of use. Hub nodes that connect many concepts should decay slower than peripheral nodes.

### Impact

The system has attention limits but can't mind-wander. The DMN candidate pairs are generated but never acted on — the system doesn't spontaneously connect distant ideas during idle cycles.

---

## 9. Bisociation — CMA Conceptual Mutation

**Paper:** `research-paper/CMA.md`
**Status:** Core mechanism implemented. Energy model oversimplified.

### Missing

- **Energy model oversimplified:** `U_hyp = path_length` at `bisociation.hpp:126`. Paper describes structural complexity: `U_hyp = Σ R(e_k) - α · C(v_a, v_b)` where `R(e_k)` is relation strength and `C(v_a, v_b)` is semantic distance. A 3-hop bridge through highly connected hubs should be cheaper than through rare intermediates. Current model treats all hops equally.

- **No iterative refinement:** Single-shot search with fixed `max_bridge_hops=5`. Paper suggests resource-budgeted search with backtracking — try a bridge, evaluate it, refine if needed, try again. Current implementation either finds a bridge or gives up.

- **Hinge extraction fragile:** Lines 118-122: `edges.size()/2` midpoint is arbitrary. Long bridges through multiple intermediate concepts lack proper hinge identification. The hinge should be the concept that most tightly constrains the bridge, not the geometric midpoint.

- **No gradual confidence:** Validated/decayed is binary. Paper implies gradual confidence accumulation across attempts — a bridge that survives multiple validation cycles should be stronger than one that just passed once.

- **α parameter (dopamine analog) not exposed:** The paper describes `α` as controlling exploratory state — high α means more forced mutations. The current implementation doesn't expose this parameter or modulate it based on system state.

### Impact

Bisociation works but is crude. The energy model doesn't capture structural complexity, there's no iterative refinement, and confidence is binary. Creative novelty is possible but not robust.

---

## 10. Analogy — L6-A Layer

**Paper:** `research-paper/synthetic-intelligence.md` §6
**Status:** Core isomorphism scoring implemented. Signature incomplete.

### Missing

- **Ingoing edges ignored:** `analogy.hpp:43` only considers outgoing edges (`e.from != node_id`). Paper likely requires bidirectional signature for full isomorphism — a node's role is defined by both what it points to and what points to it.

- **No edge weights/confidence:** Features are `(rel, label)` tuples with no strength component. Real analogies weight some relations higher than others — a causal link should matter more than a taxonomic one.

- **Fisher metric computed but not used for bridge insertion:** `layer2.hpp:844-886` computes Fisher distance between cavities. The paper says L6-A inserts a rubber bridge exactly when Fisher distance falls below discrimination threshold. The metric exists but the insertion decision doesn't read it — bridges are inserted based on shared downstream targets, not Fisher distance.

### Impact

Analogy detection is shallow (outgoing-only, unweighted) and the Fisher-optimal insertion criterion from the paper isn't used for the actual insertion decision.

---

## 11. Novelty — Discovery Queries

**Paper:** `research-paper/synthetic-intelligence.md` §5.3
**Status:** Core queries implemented. Search strategy suboptimal.

### Missing

- **DFS instead of BFS:** `novelty.hpp:76` uses recursive DFS. Results sorted after-the-fact (line 96). DFS explores deep paths before shallow ones, wasting computation on long chains when short connections exist.

- **No multi-hop shared reachability:** `shared_targets` at `novelty.hpp:111-132` only checks direct edges. Paper's "cross-domain isomorphism" likely includes indirect shared downstream nodes — concepts that reach the same downstream nodes within K hops.

- **Hardcoded therapeutic labels:** Lines 163-166: `drug`, `molecule`, etc. are string literals. Should be configurable or type-system driven for domain independence.

- **No uncertainty quantification:** Strength is deterministic. No confidence intervals or Bayesian posterior on hypothesis validity. A drug-repurposing candidate should carry a confidence measure, not just a rank.

- **Endorsement formula ad-hoc:** Line 268: `1 + 0.5*(endorsers-1)`. No theoretical grounding for the 0.5 coefficient. The paper implies endorsement should be weighted by endorsers' own reliability.

### Impact

Discovery works but is slow (DFS), shallow (depth-1 only), and lacks confidence measures. Results are ranked but not可信度-rated.

---

## 12. Language — CLL

**Paper:** `research-paper/cll.md`
**Status:** Core isolation implemented. Morphology missing.

### Missing

- **No morphology beyond articles:** `verb_3sg_suffix` and `plural_suffix` fields exist (`lang.hpp:99-100`) but are never populated or used in `realize()`. The MRS paper's goal of constructing `walked` from `walk` + past-tense is not realized — that belongs to the byte-layer.

- **Bengali cluster incomplete:** `lexicalize` map has fewer entries than English — missing `returns`, `takes_arg`, `imports`, `mutates`. These fall through to underscore-to-space rendering, producing broken Bengali output.

- **No code-switching (Banglish) in `realize()`:** The `transliterate` map is populated but `realize()` only uses `surface_relation` and `surface_concept` which call `transliterate`. This works for simple cases but the paper describes true Banglish as "a strict transliteration relation, not a probabilistic correspondence" — the current implementation can't mix Bengali and English words within a single sentence.

### Impact

Language output works for English and Bengali separately but can't produce inflected forms or true code-switched output.

---

## 13. Cross-Cutting Gaps

These gaps affect multiple modules:

### Edge Weights
No file uses weighted edges. All relations are binary present/absent. The papers describe strength, confidence, and plasticity as continuous values on edges. The tunnel struct has `stiffness` and `plasticity_delta` but the graph layer treats edges as boolean.

### Stochasticity
Everything is deterministic. Miller's cap = 9 (not sampled from [5,9]). Decay = 3 (not stochastic). k = 0.15 (fixed). No sampling, no randomness, no exploration noise. Biological cognition is inherently stochastic.

### Confidence/Probability
Binary validated/decayed. No Bayesian or fuzzy confidence. A derived fact carries the same weight as an asserted fact. The meta-learner tracks frequency but doesn't convert it to confidence.

### Feedback Loops
- Salience → bisociation is one-way (DMN generates pairs but doesn't trigger bisociation)
- BSMA has no Γ_meta → Γ_exec influence
- RIFA IC doesn't draw on crown lattice
- SIL can't trigger self-reconstruction

### Resource Budgets
- Bisociation uses hop cap, not time/compute budget
- Primitives have uniform cost=1.0f, no differentiation
- No global compute budget that forces prioritization

### No Self-Report
The system can't explain why it gave an answer. No derivation trace is surfaced to the user. The meta-learner tracks chains but doesn't present them. The BSMA qualia trace exists but isn't exposed.

---

## Priority Order

Based on impact and paper fidelity:

1. **MRS reverse drive + byte emission** — enables token-free output
2. **CPME momentum filter** — eliminates parroting
3. **Inference → dispatch integration** — makes discovery queries reachable
4. **Thinker index C/R surfacing** — core metric becomes visible
5. **DMN → bisociation integration** — enables mind-wandering
6. **RIFA IC-CL coupling** — identity system becomes coherent
7. **BSMA metacognitive feedback** — consciousness becomes functional
8. **Edge weights + confidence** — enables graded reasoning
9. **Stochasticity** — enables exploration
10. **Self-report** — enables interpretability
