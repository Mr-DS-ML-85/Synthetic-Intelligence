# BENCHMARKS — Terminal Output Analysis

Based on actual harness runs.

---

## harness-codeq — Structural Code Queries

```
graph: 2714 symbols, 5661 relations
```

| Query | Matched | Reach | Recognition |
|-------|---------|-------|-------------|
| `print` | 1 | 0.532 | resonant |
| `void` | 1 | 0.588 | resonant |
| `malloc` | 1 | 0.564 | resonant |

**Observation:** All queries use resonance-based recognition (lexical=0, resonant=1). The system finds meaning through structural resonance, not string matching. Top-10 amplitude-ranked results are returned.

---

## harness-jas — Einstein's Epistemological Cycle

### Round 1: Induction → Refutation

| Step | Value |
|------|-------|
| Observed frequency | 55.0% (191/347 functions carry `takes_arg`) |
| Leap (J) | "every function takes at least one argument" |
| Deduction (A→S) | tick(), run(), reset_field(), drive() predicted to fail with 0 args |
| Experiment | tick() compiles, reset_field() compiles |
| Verdict | **REFUTED** — counterexample ends it |

### Round 2: Revised Axiom → Survival

| Step | Value |
|------|-------|
| Revised axiom | "every function FOR WHICH the graph records a takes_arg edge takes at least one argument" |
| Held-out symbols | bind_byte, fission, drive_byte (3 tested) |
| Prediction | All 3 fail with 0 args |
| Observed | All 3 fail |
| Verdict | **SURVIVES** — 3/3 consistent |

### Impossibility Register

| Theorem | Status | Reason |
|---------|--------|--------|
| Byte multiset cannot distinguish misspellings | SCOPE ESCAPE | Bigram lanes are ordered pairs, not multisets |
| Global viscosity cannot break ground-state locking | SCOPE ESCAPE | Per-cavity coupling through tanh, not global |
| Cognitive architecture cannot eliminate log₂(N) cost | STANDS | Bits relocated from label table to coupling matrix, count unchanged |

**Key insight:** 2 of 3 theorem-claims were escaped by leaving scope, not by violation. No axiom was genuinely violated.

---

## harness-synth — Phase 2 Compile-Verify Loop

### Summary

| Metric | Value |
|--------|-------|
| Targets tested | 4 |
| Total experiments | 26 |
| Confirmed | 4 |
| Corrected | 0 |
| Unsettled | 0 |

### Per-Target Results

| Target | Believed Arity | Discovered Signature | Experiments | Compiler Verdict |
|--------|---------------|---------------------|-------------|------------------|
| `drive` | 4 | `int, float, float, int` | 7 | ACCEPTED |
| `run` | 1 | `int` | 1 | REJECTED (void) |
| `fission` | 3 | `int, float, float` | 7 | REJECTED (void) |
| `bind_byte` | 2 | `char, int` | 7 | REJECTED (no ordinal slots) |

**Key insight:** The system discovers type signatures through experimentation, not declaration. The oracle accepts any type converting safely to the declared one — so discovered signatures are ADMISSIBLE, not necessarily literal.

### Field Dynamics

| Target | Heat Δ | Tension | Affect |
|--------|--------|---------|--------|
| drive | 0.0000 → 0.0000 | 0.0000 | neutral |
| fission | 0.0002 → 0.0023 | 0.2036 | engaged |
| bind_byte | 0.0150 → 0.0168 | 0.3040 | engaged |

---

## ./si — Interactive Query System

```
graph: 2714 symbols, 5661 relations (100 derived)
```

### Query Examples

| Query | Direct Facts | Derived Facts |
|-------|-------------|---------------|
| `physics` | 3 | 1 (quantum mechanics is a physics) |
| `quantum` | 1 | 1 |
| `rust` | 2 | 0 |
| `linux` | 3 | 1 |
| `human` | 3 | 1 (Human is a great ape) |

**Derived fact rate:** 100 / 5661 = 1.8% of total facts are derived.

---

## Incompleteness Calculation

### Module Coverage

| Module | Paper Mechanisms | Implemented | Coverage |
|--------|-----------------|-------------|----------|
| MRS (byte emission) | 5 | 1 (input only) | 20% |
| CPME (momentum filter) | 5 | 2 (phase-collapse, chirality) | 40% |
| CPSB (gauge syntax) | 3 | 2 (offset, Hebbian) | 67% |
| RIFA (identity) | 5 | 3 (IC, CL, SIL) | 60% |
| BSMA (consciousness) | 4 | 2 (projection, rules) | 50% |
| TSDA (attention) | 4 | 3 (salience, working graph, decay) | 75% |
| Inference (discovery) | 6 | 5 (5/6 queries) | 83% |
| Meta-learner (induction) | 4 | 2 (induction, verifier) | 50% |
| Bisociation (creativity) | 5 | 3 (distance, bridge, energy) | 60% |
| Analogy (L6-A) | 3 | 2 (isomorphism, novelty) | 67% |
| Language (CLL) | 3 | 2 (isolation, determinism) | 67% |
| Cross-cutting | 6 | 1 (binary edges) | 17% |

### Overall

| Metric | Value |
|--------|-------|
| Total paper mechanisms | 53 |
| Implemented | 28 |
| **Overall coverage** | **53%** |
| Partially implemented | 12 |
| Missing | 13 |

### By Layer

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

### Terminal Output vs Paper Claims

| Claim | Terminal Evidence | Gap |
|-------|------------------|-----|
| Resonance-based recognition | codeq: lexical=0, resonant=1 | ✅ Demonstrated |
| J-A-S epistemological cycle | jas: axiom induced → refuted → revised → survives | ✅ Demonstrated |
| Compile-verify experimentation | synth: 26 experiments, 4 confirmed | ✅ Demonstrated |
| Token-free output | si: uses stored labels | ❌ Not demonstrated |
| Momentum filtering | codeq: uses amplitude, not momentum | ❌ Not demonstrated |
| Mind-wandering DMN | Not in any harness output | ❌ Not demonstrated |
| Self-report / qualia trace | Not surfaced in terminal | ❌ Not demonstrated |
| Thinker index C/R | Not reported | ❌ Not demonstrated |
| Confidence measures | Binary (confirmed/rejected) | ❌ Not demonstrated |
