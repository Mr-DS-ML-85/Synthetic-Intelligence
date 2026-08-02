# ROADMAP — from prototype to public release

**Project:** PCMA / Synthetic Intelligence substrate
**Author:** Irfan Mahir
**Status:** working prototype, pre-release
**Target:** GitHub public release with reproducible claims

---

## Guiding rule

Ship a **narrow thing that provably works** before a broad thing that
almost works. Every phase below ends in a command someone else can run
that either passes or fails. No phase is "done" because the code
compiles — it is done when its success criterion is measurable by a
stranger with a clean checkout.

The second rule: **claim exactly what the harness proves.** A release
that says "domain-locked semantic association with reproducible
20-seed statistics" and delivers it will be respected. A release that
says "artificial general intelligence" and delivers the same thing
will not.

---

## Phase 0 — Where the prototype actually is

Honest inventory, verified this session.

### Working and reproducible

| Component | Evidence |
| --- | --- |
| Layer-1 physics substrate | `physics.hpp` — one tick, no if/else on relations |
| RB+G fission + audit ledger | 20-seed harness, `data/fission_ledger.jsonl` |
| L6-A analogical bridges | `discover_analogies`, drug-repurposing demo |
| VGHT domain sub-bands | medical prompts stay medical across turns |
| CPME phase-collapse emission | variable-length output, non-parroting |
| CPSB gauge Ψ + CPME-II Hebbian | implemented, activity-gated, stable |
| **AST ontology extraction** | **3,864 triples from own source, zero ML** |
| **Code-structure relations** | **`calls`/`precedes`/`takes_arg`/`returns`/`imports`** |
| Nine reasoning primitives | `make nine && ./nine` |
| BSMA, Dreamer, dispatcher | built, wired into `SI::query` |

### Known-broken or unproven

| Issue | Status |
| --- | --- |
| MRS byte-level construction | babbles; repeated-letter collision is architectural (`mrs.md` §5.3) |
| Sparse-graph echo | input echoes when concept has no outbound edges |
| Stale amplitude across turns | ~30% amplitude survives a turn; no inter-turn budget |
| CSAT | specified, **not implemented** |
| Natural-language chat quality | not release-ready |

**Decision that follows from this table:** the release vertical is
**code structure**, not conversation. Everything in the working column
supports it; three of the four broken items are conversation-only.

---

## Phase 1 — Code-structure vertical (4–6 weeks)

Make the substrate demonstrably useful on one task: **answering
structural questions about a codebase it has ingested.**

### 1.1 Harden the extractor — partially done

- [x] **Scope-aware `precedes`.** `_py_block` / `_cpp_block` walk one
      statement list at a time and recurse into nested blocks as
      independent scopes, so calls in mutually-exclusive branches no
      longer receive ordering edges. Edge count fell 1110 → 944; the
      loss is entirely spurious edges.
- [x] **`mutates` extraction.** Python: `ast.Assign` to an attribute,
      and `AugAssign`. C++: `obj.field =` / `obj->field =` at each
      brace depth. 291 edges on own source, verified correct
      (`add_or_get_node mutates anchor`).
- [ ] Resolve calls through imports so `mod.fn` maps to the definition
- [ ] Multi-language via tree-sitter (Rust, Go, JS)
- [ ] Regression corpus: 5 repos, snapshot triple counts, fail CI on drift

### 1.2b Resonant dispatch — ✅ DONE

**The 40-binaries problem.** Every mechanism previously required its
own binary and its own command palette. A person asked a question does
not first select a mode; the shape of the question determines the
shape of the answer.

`codeq` now takes plain input and selects the readout **from the
field's physical state, inspecting no strings.** Observables measured
after propagation:

| Observable | Meaning |
| --- | --- |
| `n_matched` | how many input windows resonated with a known symbol |
| `hot_frac` | fraction of the field the wavefront reached |
| `path_strength` | whether a relation chain joins the matched symbols |
| `shared_frac` | overlap of their direct neighbourhoods |

Four modes are scored as continuous functions of those observables and
selected by arg-max — the same idiom as the phase-collapse emission
sweep, not a branch table over keywords:

```
score[SILENCE]      = exp(-6·hot_frac) · exp(-2·n_matched)
score[NEIGHBORHOOD] = exp(-(n_matched-1)²) · (0.2 + hot_frac)
score[CHAIN]        = σ(4(n_matched-1.5)) · path_strength
score[BRIDGE]       = σ(4(n_matched-1.5)) · (1-path_strength) · (0.1+shared_frac)
```

Verified, no command words used:

```
$ ./codeq does drive come before tick
  [relational chain]  matched=2  reach=0.562  path=1.00
    dataflow: tick -> sqrt -> tanh -> drive

$ ./codeq how are fission and cavity related
  [relational chain]  matched=2  reach=0.581  path=0.70
    fission --calls--> fission_locked --calls--> push_back
            --precedes--> end --precedes--> bind --calls--> cavity

$ ./codeq tell me about metformin
  [resonance neighbourhood]
    hyperglycemia, hmg_coa_reductase, glucose_reabsorption, ...

$ ./codeq zzz qqq wubble
  [silence]  matched=0  reach=0.000
    nothing in the graph resonated with that.
```

**Topic selection.** Incidental matches on common words ("come",
"and") enter the graph through extraction noise and carry degree 1–2,
while the real topic of a question is wired into structure. Matched
symbols are therefore ranked by degree descending, with a floor of 3.
*The first implementation used inverse degree on an
information-theoretic argument and was empirically backwards* — noted
here so the reasoning is not repeated.

Explicit verbs (`neighbors`, `calls`, `order`, …) remain available so
the reproducibility harnesses can pin an exact readout.

**Remaining:** extend the same dispatcher to `si_chat` so there is one
conversational entry point, and retire the demo binaries that only
exist to expose one mechanism each.

**Success:** extract from a 100k-LOC repo in under 60s, ≥95% of
`takes_arg` edges match the real signature on a hand-checked sample of 100.

### 1.2 Structural query interface — ✅ DONE

`src/codeq.cpp`, `make codeq`. Two query classes:

**Symbolic** — exact edge traversal. `calls`, `callers`, `signature`,
`fields`, `imports`, `order`, `path`, `stats`.

**Resonant** — `neighbors`. Drives a cavity, propagates the field
through material-weighted tunnels, ranks by displacement. Multi-hop
influence accumulates through the impedance of intervening relations,
so this surfaces structurally-related symbols sharing *no direct
edge*. This is the query `grep` and `ctags` cannot answer.

Verified against the project's own source (2161 symbols, 4732
relations, of which 3376 are code-structure):

```
$ ./codeq signature drive
    takes_arg  amp / duration_ticks / freq / node_id      ← exact signature

$ ./codeq callers fission_locked
    fission, structural_tick_once                          ← both real callers

$ ./codeq order drive tick
    yes — dataflow chain: drive -> tick

$ ./codeq neighbors fission 8
    tick_index, streak, parent, incident_beta,
    tau, child_alpha, incident_alpha, child_beta
```

That last result is the notable one: it recovered **the exact field
set of `FissionRecord`** — the audit struct belonging to the fission
mechanism — with no edge in the graph directly connecting `fission`
to those fields. The association was reconstructed by propagation
through the call and field topology.

**Remaining for 1.2:** `path` needs `rel_mask` plumbed through so
callers can restrict to code relations; currently it searches all
relation types and can route through medical ontology when the
ontologies share a label.

### 1.3 Benchmark against a non-ML baseline

The honest comparison is not GPT-4. It is `grep`, `ctags`, and a
static call-graph tool. Build a labelled set of 200 structural
questions over a known repo and report precision/recall against
`cscope` or `clangd`.

**Success:** any category where the substrate beats static tooling is
a real, defensible contribution. Categories where it loses get
reported too — that is what makes the win credible.

---

### 1.3b Relatedness vs ordering — ✅ DONE

Two bugs found and fixed while testing `path`.

**Hub traversal.** Hop-count BFS returns the shortest chain, which in
a call graph is almost always the one threading a hub — `end`, `size`,
`push_back` touch hundreds of functions, so any two symbols sit ~3
hops apart through one of them. Such a chain is true but vacuous.
`path_masked` now runs Dijkstra with **cost = degree of the node being
entered**, so traversing a hub is expensive and a longer, more
specific chain wins. Same principle as the MIMS material table:
passage costs what the structure makes it cost.

**`precedes` is not a relatedness relation.** It records that two
calls appeared in sequence in *some* body. `count precedes fprintf`
holds in a dozen unrelated functions, so admitting it into path search
makes every symbol reachable from every other in two hops through
incidental co-occurrence. It is now excluded from `CODE_REL_MASK`;
`order` queries use the dedicated PRECEDES walk.

Result: `path fission cavity` now honestly reports **no path found**
(a function and a struct genuinely share no structural edge chain),
where before it produced a confident, meaningless six-hop chain.
`order drive tick` still returns `drive -> tick`.

*Note: the earlier diagnosis in this roadmap — that `path` was
detouring through the medical ontology — was wrong. The detour was
entirely within code relations, via `precedes`. Recorded so the wrong
fix is not attempted again.*

---

## Phase 1.4 — Substrate state as an artifact (`.si`) — ✅ DONE

**The problem.** Every binary rebuilt the ontology from
`data/*.axioms` at startup, bound a fresh field, and discarded every
learned quantity on exit. Nothing the substrate learned survived the
process. Knowledge was *wired into the run*, not held in a file. The
`.neuron` files from the legacy DCHL substrate had this property; the
current engine had lost it.

**`src/si_state.hpp`** defines a self-contained binary format with a
magic + version header. A `.si` carries the graph *and* the learned
overlay:

| Section | Content |
| --- | --- |
| graph | node labels, edges (from, to, relation) |
| thermal | per-cavity `heat_T` (CMD habituation) |
| acoustic | per-cavity `acoustic_mass` (CPME §1.3) |
| plasticity | per-tunnel `plasticity_delta` (ZeroGrad) |
| gauge | per-tunnel `psi_target` (CPSB §2.1) |
| sensory | byte→concept bindings (CMD byte-wave layer) |
| lanes | lane↔lane tunnels + learned Ψ (MRS §1.3) |
| fission | RB+G parent → {α, β} map |

None of it is derivable from the axiom files, which is exactly why it
needs serialising.

```
$ ./codeq save pcma.si
  saved 2240 symbols, 4916 relations -> pcma.si     (145 KB)

$ ./codeq
codeq> load pcma.si
  loaded 2240 symbols, 4916 relations <- pcma.si
codeq> signature drive
    takes_arg  amp / duration_ticks / freq / node_id
```

**Remaining:** wire `--load` / `--save` into every binary so a trained
substrate is the normal unit of exchange; add a `.si` version-migration
path before v0.1.0 so early files do not become unreadable.

---

## Phase 1.5 — Retire the binary sprawl (open, next)

**The problem, stated plainly.** Roughly 20 binaries and 40+ CLI verbs
exist, each surfacing one research-paper mechanism. A user must know
which paper a capability came from in order to invoke it. That is a
packaging failure, not a research one — every mechanism works; none of
them is reachable without a manual.

**What is already proven.** Phase 1.2b showed the readout can be
selected from the field's own physical state with no string
inspection. That mechanism generalises.

**Plan.**

- [ ] Single entry point `si`, with `--load model.si`. All interaction
      is plain input; the dispatcher selects the readout.
- [ ] Extend the mode set beyond the current four (silence,
      neighbourhood, chain, bridge) to cover the mechanisms currently
      stranded in their own binaries:
      analogy (`discover_analogies`), abduction
      (`discover_abductions`), novelty (`novelty.hpp`), fission audit,
      Fisher distance. Each becomes a mode scored on observables it
      already produces — incoherence for fission, tension for
      abduction, cluster overlap for analogy.
- [ ] Demote the per-mechanism binaries to `tools/` as reproducibility
      harnesses, invoked by `docs/REPRODUCE.md`, not by users.
- [ ] Keep explicit verbs on the single binary so the seed/ablation
      harnesses can still pin an exact readout.

**Success:** a new user runs `./si --load pcma.si`, types a question in
plain English, and never learns that CPME, VGHT, RB+G or L6-A exist.

---

## Phase 2 — The correctness loop

### 2.0 Compile-verify loop — ✅ FIRST MILESTONE DONE

`src/synth.cpp`, `make synth`. The categorical break from retrieval.

Everything before this phase stayed inside what the axioms entailed:
lookup, ranking by propagation, forward chaining. `synth` forms a
**hypothesis** from the graph, emits real source expressing it, submits
it to an oracle that knows nothing about the substrate — the C++
compiler — and updates its knowledge from the verdict.

**Target chosen because the graph is provably incomplete there.** AST
extraction records `f takes_arg x` edges but loses cardinality: the
recorded argument count is a *claim*, not a fact.

Measured:

```
$ ./synth bind_byte drive_byte fission
  target 'bind_byte'
    graph claims arity = 1
    attempt 1: bind_byte(1 args) ... rejected — no matching function
    attempt 2: bind_byte(0 args) ... rejected — no matching function
    attempt 3: bind_byte(2 args) ... ACCEPTED
    CORRECTED  graph said 1, compiler says 2  (3 attempts)
    -> written to data/verified.axioms

  target 'drive_byte'   CONFIRMED arity=1 in 1 attempt
  target 'fission'      CONFIRMED arity=3 in 1 attempt
```

The true signature is `bind_byte(unsigned char, int, float = 0.02f)` —
two required arguments. Extraction had recorded one. **The substrate was
wrong, ran an experiment, and learned the correct answer.**

Search starts at the hypothesis rather than at zero, so a correct graph
is confirmed in one experiment and the attempt count measures how wrong
the graph was. That number is the learning signal.

**Knowledge persists and re-enters reasoning:**

```
$ cat data/verified.axioms
bind_byte    has    arity_2

$ ./codeq fields bind_byte
  [fields of bind byte]
    has    arity 2
```

Corrections are written to a separate file because provenance matters:
this fact was neither asserted by a human nor derived by a rule. A
third category — *established by experiment* — now exists in the
substrate's knowledge.

**Why this answers the standing criticism.** A search engine returns
what was put in. Forward chaining returns what was entailed. Neither
can produce `bind_byte has arity_2` when every input said arity 1. Only
generate-and-test against an external oracle can, and the oracle here is
a deterministic program — no loss function, no gradient, no fitted
parameter.

### 2.1 Positional type signatures — ✅ DONE

The graph records argument *names* but no types at all. Under
`-Werror=conversion` a narrowing `float`→`int` argument is rejected
while a widening `int`→`float` is accepted, so the compiler
discriminates between candidate type assignments. For arity *n* the
space is 2ⁿ; search is exhaustive and terminates.

```
$ ./synth drive
  believes arity = 4  [from AST extraction]
    attempt 1: drive(4 args) ... ACCEPTED
  discovering positional types:
    attempt 7: drive(int, float, float, int) ... ACCEPTED
  TYPE SIGNATURE drive(int, float, float, int)  (7 experiments)
```

True declaration:
`void drive(int node_id, float freq, float amp, int duration_ticks)`.
Recovered exactly, from a graph containing no type information.

**Bug worth recording.** The first run rejected all 16 candidates
*including the correct one*: `-Werror=conversion` was firing on
conversions inside `physics.hpp` itself, so the translation unit failed
regardless of the call. Fixed with `-isystem src`, which marks the
header as a system header so only the generated call site is judged.
Before that fix the oracle was answering a different question than the
one being asked — a failure mode worth watching for whenever an
external tool is used as ground truth.

### 2.2 Learning criterion — ✅ MET

The criterion that separates learning from search: **the same task must
get cheaper with experience.**

`claimed_arity()` now consults verified experimental facts *before*
falling back to counting `takes_arg` edges. Experimental knowledge
outranks extracted knowledge, because extraction was already shown
wrong about it. That feedback path is what makes the loop a learning
loop.

```
TRIAL 1   believes arity = 1  [from AST extraction]
          attempt 1: bind_byte(1 args) ... rejected
          attempt 2: bind_byte(0 args) ... rejected
          attempt 3: bind_byte(2 args) ... ACCEPTED
          CORRECTED — 3 attempts

TRIAL 2   believes arity = 2  [from prior experiment]
          attempt 1: bind_byte(2 args) ... ACCEPTED
          CONFIRMED — 1 attempt
```

**3 experiments → 1, on the identical task.** Attempt count is now an
observable of accumulated experience, and it is falling. Facts already
known are not re-written, so `verified.axioms` does not grow on repeat
runs.

### 2.3 Argument ordinals — ✅ DONE (extractor fix)

Order is not recoverable by experiment, so it was fixed where it was
lost: extraction. Position is a property of the *(function, index)*
pair, not of the parameter name — `node_id` is arg0 of one function and
arg2 of another — so a reified slot node carries it:

```
drive__arg0   is_a   node_id
drive__arg1   is_a   freq
drive__arg2   is_a   amp
drive__arg3   is_a   duration_ticks
```

Against the true declaration
`void drive(int node_id, float freq, float amp, int duration_ticks)` —
exact. Sorting slots by index reconstructs the signature, which plain
`takes_arg` edges could not do because the axiom format has no ordering
guarantee. Slot nodes are unique per position and are never dropped by
the frequency cap.

Extraction now yields 416 `is_a` ordinal edges alongside 414
`takes_arg` edges.

### 2.4 Widened type alphabet — ✅ DONE

`{int, float}` could not express `bind_byte(unsigned char, int)`; the
system correctly reported *no assignment accepted* but could not
succeed. The alphabet is now
`{int, unsigned, char, unsigned char, float, double, bool}`.

Exhaustive enumeration would be |T|ⁿ — 7⁴ = 2401 compilations for
arity 4, which is not a search but a denial of service. Positions are
resolved **greedily, one at a time**, holding the others at their
current best: O(n·|T|) per sweep, repeated until a sweep changes
nothing.

```
$ ./synth bind_byte
  attempt 1: bind_byte(2 args) ... ACCEPTED
  attempt 3: bind_byte(char, int) ... ACCEPTED
  TYPE SIGNATURE bind_byte(char, int)  (3 experiments)
```

This is hill-climbing on a discrete space with an exact oracle. It is
**not** gradient descent: there is no loss surface and no derivative,
only accept/reject verdicts. It can stall in a local configuration,
and that is reported rather than hidden.

**Honest caveat, now printed at runtime.** The result is an
*admissible* signature, not necessarily the literal declaration: the
compiler accepts any type that converts safely to the declared one, so
`char` is accepted where `unsigned char` was declared. Recovering the
exact declaration would need a different oracle (debug info, or
`-fdump-lang-class`), not more compilations.

### 2.5 RIFA feedback — failure as field energy — ✅ DONE

Until this point a refuted hypothesis updated the *knowledge base*: a
line appended to `verified.axioms`. The substrate itself was unchanged
— the physics field had no idea an experiment had failed.

The verdict now enters the field. A rejected candidate drives the
target's cavity, so failure becomes literal energy at that concept.
CMD thermodynamic habituation integrates it —
$dT/dt = \alpha|v| - \beta T$ — and the Curie gate then attenuates every
tunnel touching a hot cavity by $e^{-T/T_{crit}}$.

**A concept that keeps failing decouples itself from the graph**, by
exactly the mechanism that cooks stopwords out of the field. Nothing
new was introduced: `drive()`, `heat_T` and the Curie gate all already
existed. What is new is that an *external verdict* is now one of the
things that can heat a cavity. Unreliability became a physical property
of the concept rather than a note in a file.

Measured:

| target | failures | heat before → after | Curie coupling |
| --- | --- | --- | --- |
| `bind_byte` | 2 | 0.0000 → **0.7116** | $e^{-0.71/3}$ = **0.79** |
| `drive_byte` | 0 | 0.0131 → 0.0216 | $e^{-0.02/3}$ = 0.99 |

A 33× difference in accumulated heat. `bind_byte` has lost ~21% of its
coupling to the rest of the graph; `drive_byte` is untouched.

**Success is not rewarded.** A confirmed hypothesis triggers no drive —
the concept simply cools through the $-\beta T$ term. There is no
reward signal anywhere in this substrate, only punishment by
experience and recovery by time.

### 2.6 Field persistence — ✅ DONE

Heat earned by failure must survive the process, or every run starts
trusting a concept it has already been burned by.
`si_save_overlay_only` / `si_load_overlay_only` carry the physics
overlay (per-cavity `heat_T`, `acoustic_mass`, global `ic.tension`)
without the graph, since the graph is rebuilt from axioms each run.

```
SESSION 1  (cold field)
  field: heat 0.0000 -> 0.7116        ← 2 failures
  (field state saved to synth_field.si)

SESSION 2  (field restored)
  believes arity = 2  [from prior experiment]
  field: heat 0.7116 -> 0.2118        ← no failures; cooled
```

The two learning channels compound. `verified.axioms` records **what is
true**; `synth_field.si` records **how much the substrate trusts each
concept**. Knowing the right answer means no failures, which means the
concept recovers trust. Being wrong repeatedly means heat, which means
decoupling.

### 2.7 Affect — dopamine reward, cortisol punishment — ✅ DONE

The substrate already carried three neuromodulators wired into the tick
as multipliers: dopamine on drive, cortisol on damping, serotonin on
the Hebbian rate. They were set by hand. They are now moved by outcome.

**This is neuromodulation, not reinforcement learning.** RL requires a
value function, a policy, and gradient updates toward expected return.
There is none of that here — three scalars multiply physical constants,
are nudged by accept/reject verdicts, and drift back to baseline.
Nothing estimates a return; nothing is optimised.

| outcome | dopamine | cortisol | serotonin | effect in the tick |
| --- | --- | --- | --- | --- |
| accepted | ×1.15 | ×0.90 | ×1.10 | field responds harder, consolidates faster |
| rejected | ×0.85 | ×1.25 | ×0.92 | damping rises, propagation narrows |

Cortisol is the stress axis and the **"sad" state is not a label**:
raising it raises `damp_scale`, so a substrate that has just failed
propagates its own wavefront less far. Being wrong makes it literally
more conservative.

Homeostasis pulls all three toward 1.0, so neither elation nor despair
is a fixed point.

Measured:

```
success (drive)              dopamine 1.14  cortisol 0.90  serotonin 1.10
failure (bind_byte, 2 rej.)  dopamine 0.86  cortisol 1.35  serotonin 0.95
                             heat 0.0000 -> 0.5269
```

### 2.8 Graph-walk serializer — ✅ DONE (Phase 2 complete)

Everything before this probed an *existing* function. This **emits**
one — a complete compilable function assembled entirely from graph
structure, with each element traceable to how it was learned:

| element | source |
| --- | --- |
| parameter order | `drive__arg0..N is_a <name>` — extractor ordinals |
| parameter types | `drive has sig_int_float_float_int` — established by experiment |
| body ordering | `precedes` edges — the CPSB gauge |

```cpp
// emitted by graph walk — not written by a human
void drive_forward(int node_id, float freq, float amp, int duration_ticks) {
  PhysicsEngine px;
  px.drive(node_id, freq, amp, duration_ticks);
}
COMPILES — reconstructed signature is correct
```

The forward call is the point: if any element of the reconstructed
signature is wrong the program does not compile, so this is a single
end-to-end test of order, arity and types at once. The compile verdict
feeds back into affect and field heat like any other experiment.

**Regression found and fixed.** The greedy type search from §2.4
*failed* where the earlier exhaustive search had succeeded: reaching
`(int,float,float,int)` from all-`int` requires changing two positions
simultaneously, and a one-position-at-a-time hill climb never sees that
move. Textbook local optimum — documented as possible in §2.4, then
actually observed. Search is now **staged**: exhaustive over
`{int,float}` first (2ⁿ = 16 at arity 4, cheaper than being clever),
greedy over the full 7-type alphabet only if that finds nothing. This
recovers `drive` in 7 experiments while still reaching
`bind_byte(char, int)`.

### 2.9 Known limits (measured, not assumed)

- **Type alphabet is `{int, float}`.** `bind_byte(unsigned char, int)`
  matches nothing in that alphabet, and the system correctly reports
  *no type assignment accepted* rather than guessing. Widening the
  alphabet to `{int, unsigned, char, unsigned char, float, double,
  bool}` raises the space to 7ⁿ; pruning by first discovering each
  position independently would keep it tractable.
- **Argument ORDER is not recoverable by experiment.** A compiler
  type-checks; it does not report parameter names. Mapping
  `{amp, duration_ticks, freq, node_id}` onto positions therefore
  cannot be settled by any compilation. This is an *extractor* defect —
  `extract_ast_axioms.py` discards source order via alphabetical sort
  and frequency capping — and the fix is to emit ordinals during
  extraction (Phase 1.1), not to permutation-search against an oracle
  that cannot answer.

**Remaining in Phase 2:**

- [ ] Argument *order* discovery. The graph holds `{amp, duration_ticks,
      freq, node_id}` alphabetically; the real signature is
      `(node_id, freq, amp, duration_ticks)`. Order is absent entirely.
      Searching 4! permutations under `-Werror=conversion` makes the
      compiler discriminate, since `float`→`int` narrowing is rejected.
- [ ] Type discovery per parameter (int/float), jointly with order.
- [ ] Graph-walk serializer for whole function bodies, ordered by the
      CPSB `precedes` gauge.
- [ ] Error-line → cavity mapping, injecting failure as RIFA tension so
      repeated exposure *reduces* attempts. Decreasing attempt count over
      trials is the criterion that proves learning rather than search.

---

## Phase 2 — remaining detail (6–10 weeks)

This is the phase that turns a query engine into something that
*writes* code. The compiler is the oracle; no ML is introduced.

### 2.1 Graph-walk serializer

Turn an emitted concept sequence into syntactically valid source.
Structure comes from the AST relations (`takes_arg` → parameter list,
`returns` → return type, `precedes` → statement order), so this is
mechanical, not template-based.

- [ ] Emit a single well-formed function body from a phase-collapse walk
- [ ] Respect `precedes` ordering (CPSB Ψ = −2π/3 already enforces it physically)

### 2.2 Compile-verify feedback

```
emit → write to temp file → invoke compiler → parse exit code + error line
     → map error location back to the responsible cavity
     → inject as RIFA tension → structural tick adapts
```

The tension signal already exists (`IntegrityCore::tension`). This
wires a deterministic external oracle into it.

**Success criterion (the one that matters):** starting from a prompt
naming a function and its inputs, the system emits code that compiles
on ≥1 of 5 attempts, and the attempt count *decreases* over repeated
exposure to the same task. Decreasing attempts is the proof that the
loop learns.

### 2.3 Scope honesty

It will compose known patterns before inventing algorithms. Target
5-line correct functions, then 20. Do not promise more in the README
than the harness demonstrates.

---

## Phase 3 — Fix or formally retire the broken parts

### 3.1 Inter-turn energy homeostasis — ✅ DONE (root cause of the echo)

Friction alone never cleared the field. At μ = 0.006/tick,
0.994²⁰⁰ = **30%** of a cavity's amplitude survives a 200-tick turn, so
a cavity driven once at amp 6 stayed the loudest thing in the field for
several turns and won the readout while doing so.

Every previous attempt at this problem — emission floors, wavefront
gates, acoustic mass, cluster depletion — was a *selection* fix for a
*dissipation* problem. No readout scoring can rescue a field that never
sheds what it was given.

`renormalize_energy(budget)` applies global gain control at each turn
boundary: if total kinetic energy exceeds budget, the whole field is
scaled by √(budget/total). Scaling is **uniform**, so no amplitude
ratio changes — it removes accumulated loudness without altering which
cavity is currently most active. It only prevents the past from
drowning the present. (Biological analogue: synaptic scaling.)

```
BEFORE   linux  ->  think                       (pure residual echo)
AFTER    linux  ->  kernel application software open source license
```

That is a correct answer to a question that returned stale garbage for
most of this project's history.

### 3.2 Structural nodes leaking into emission — ✅ FIXED (self-inflicted)

The `f__argN` ordinal slots added in §2.3 are graph scaffolding, and
they immediately began appearing in replies (`arg2`, `arg4`). Filtered
at the I/O boundary only — they remain fully available to reasoning and
to the serializer, which is where they belong. A regression introduced
and closed in the same session.

### 3.3 CSAT — ✅ IMPLEMENTED (§2.2.2 form), opt-in

Implemented as specified in CSAT §2.2.2: a Lorenz system integrated
alongside the field (three scalar updates, τ-scaled for stability at
dt = 0.02, x normalised by 20), coupled **per-cavity** through
`tanh(T_i / T_crit)`.

The per-cavity coupling is the whole point. The paper's original global
formulation is provably rank-preserving (CSAT §2.2.1) — scaling every
velocity identically leaves all ratios unchanged, so the emission
arg-max cannot move. Heat is the correct coupling variable because it
already means *this cavity has dominated for a long time*.

Opt-in via `SI_CSAT=1`, because Lyapunov sensitivity breaks the
bit-exact reproducibility the seed and ablation harnesses require
(CSAT §4.1). This remains the only mechanism in the stack that is not
default-on.

**Measured, honestly:**

```
CSAT off   linux -> posix compliance open source license artifact
CSAT on    linux -> software open source license artifact application
```

The trajectory changes, so the mechanism is live. But **both
configurations still collapse to `linux` on repeated identical input**
at γ = 0.04. CSAT is firing without yet defeating ground-state
locking. Raising γ, or coupling it to `quiet_ticks` as well as heat, is
untested. Reported as partial rather than working.

### 3.4 MRS emission — ⚠️ RETIRED as EXPERIMENTAL

Bigram lanes fixed **recognition** (7/7 on misspellings, §MRS 4b.2) and
that path is live and default-on. The **emission** side — synthesising
word forms by sweeping the byte-phase field — still babbles, and no
evidence was produced this session that it works.

Decision: `mrs_enabled = false`, reachable via `SI_MRS=1`, with the
measured failure recorded in `research-paper/mrs.md` §4b and in the
source. It is not claimed to function. Shipping it silently would
violate the project's own contract.

The honest position is that MRS is two mechanisms sharing a paper: one
that works (bigram recognition) and one that does not (phase-collapse
byte emission). They should probably be split into separate documents
before release so the working half is not judged by the failing half.

### 3.5 Remaining Phase 3 work

Do this *before* release, not after. An honest limitations section is
an asset; an undocumented failure discovered by a stranger is not.

- [ ] **Inter-turn energy budget** — cap total field energy per turn so
      stale amplitude cannot dominate. Fixes the echo at its root.
- [ ] **Sparse-graph echo** — with a dense code ontology this largely
      disappears; verify and document.
- [ ] **MRS** — either implement delay-line lanes (multiple arrival
      phases per lane, fixes the repeated-letter collision) or mark it
      `EXPERIMENTAL` and default-off with the failure documented.
      Do not ship it silently broken.
- [ ] **CSAT** — implement §2.2.2 (heat-coupled, per-cavity) or leave
      as paper-only. The global form is proven not to work; make sure
      no one implements it from the old text.

---

## Phase 4 — Release engineering (2–3 weeks)

### 4.1 Repository hygiene

- [ ] `LICENSE` — pick one and commit (Apache-2.0 or MIT for adoption)
- [ ] Remove build artifacts from the tree (`*.o`, binaries, `*.neuron`,
      `*.bin` — currently checked in, several are large)
- [ ] `.gitignore` for the above
- [ ] `CMakeLists.txt` alongside the Makefile for portability
- [ ] CI: build + `make nine` + extractor regression on push

### 4.2 Documentation set

| File | Purpose |
| --- | --- |
| `README.md` | what it is, what it does, 60-second demo, honest scope |
| `ROADMAP.md` | this file |
| `docs/ARCHITECTURE.md` | the layer stack, one page |
| `docs/REPRODUCE.md` | every published number, one command each |
| `research-paper/` | the 30+ papers, with the status matrix from `how_we_implement.md` |

### 4.3 The README scope paragraph

Write this early and let it constrain the rest. Suggested frame:

> A graph-physics substrate for structural reasoning. Concepts are
> coupled oscillators; relations are materials with stiffness, damping
> and phase offset; inference is wave propagation; output is a
> phase-swept projection. No gradients, no backpropagation, no
> learned embeddings, no next-token prediction. Ontology is extracted
> mechanically from source ASTs. Currently demonstrates: domain-locked
> structural association, reproducible fission statistics over 20
> seeds, and analogical bridging across disjoint ontologies.

Everything in that paragraph is currently true and testable. Nothing
in it says AGI.

---

## Phase 5 — Claims and evidence (ongoing)

For each claim in the README there must be one command that produces
the evidence.

| Claim | Command |
| --- | --- |
| Reproducible under seed | `study_recall_seeds metformin 5 20` |
| Fission mechanism ablation | `study_recall_ablation metformin 5 5` |
| Analogical bridging | `discover_analogies 2` + `repurpose` |
| Zero-ML ontology extraction | `tools/extract_ast_axioms.py src/` |
| Nine primitives fire | `make nine && ./nine` |
| Structural code queries | `./codeq neighbors drive` (Phase 1.2) |
| Compile-verified emission | Phase 2.2 harness |

Claims without a row do not go in the README.

---

## Suggested sequencing

Working solo, roughly:

```
Weeks 1–6    Phase 1  (code vertical + query interface + benchmark)
Weeks 7–16   Phase 2  (serializer + compile loop)   ← the real contribution
Weeks 17–20  Phase 3  (fix or retire broken parts)
Weeks 21–23  Phase 4  (release engineering)
Week  24     Tag v0.1.0
```

Roughly six months to a defensible public release. That estimate
assumes the compile-verify loop in Phase 2 takes the longest, because
it is the part with genuine unknowns.

---

## What to resist

1. **Do not broaden before Phase 2 succeeds.** More domains, more
   languages, more papers — all of it is cheaper and more valuable
   after there is one task the system provably does well.
2. **Do not ship MRS or CSAT as working.** Both are honest research
   directions. Neither currently produces a result. Label accordingly.
3. **Do not benchmark against LLMs in the README.** Different problem
   class, and the comparison invites the one critique the project
   cannot yet answer. Benchmark against static analysis tools, where
   the comparison is fair and winnable.
4. **Do not remove the limitations sections from the papers.** They
   are the most credible thing in the repository.

---

## Why the code vertical is the right bet

- Ontology is free and mechanical — no annotation team, no crowdsourcing,
  no LLM in the pipeline. Every repo on disk is a corpus.
- Relations form a small closed set that maps cleanly onto
  `material_for` / `psi_for` / `verb_pull_for`.
- Dataflow ordering is genuinely a phase gradient, so CPSB earns its keep.
- **A compiler is a free, deterministic, perfect oracle.** Nothing else
  in the project has ground truth this clean.
- The failure mode that broke the conversational path — orthography,
  morphology, repeated letters — does not exist here.

The substrate was built for structural reasoning over typed graphs.
Code is a typed graph with a built-in verifier. That is the match.
