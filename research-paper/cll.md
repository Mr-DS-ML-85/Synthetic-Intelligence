# Constructive Language Locking (CLL): Separating Cognitive Freedom from Linguistic Protocol

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

> *"Grammar is not a law of nature awaiting discovery. It is a protocol
> two people agreed to. Confusing the two is why physics-based language
> generation has failed."*

---

## Abstract

Wave-based cognitive substrates have repeatedly failed to produce
grammatical natural language, and the field's response has been to
abandon the mechanical problem in favour of a statistical one:
$P(w_n \mid w_1 \ldots w_{n-1})$. We argue this was a false dilemma
created by a category error. Cognition and language are different kinds
of process — one *discovers*, the other *transmits* — and attempting to
derive grammar from field dynamics conflates a communication protocol
with a physical phenomenon. We propose **Constructive Language Locking
(CLL)**: the cognitive substrate remains entirely unconstrained and
constructive, while surface realisation is delegated to *isolated
language clusters* holding fixed, deterministic grammatical rules. The
core loses no flexibility because no grammar rule ever enters it; the
output gains exact, non-hallucinating surface form because nothing is
sampled. We show the architecture emits one cognitive state into
English, Bengali and Banglish from a single triple, at $O(1)$ lookup
cost, with zero learned parameters.

---

## 1. The Category Error

### 1.1 Two Processes, Wrongly Merged

| | Cognition | Language |
| --- | --- | --- |
| Purpose | discover what is true | transmit what is held |
| Requires | novelty, experiment, resonance, oracle validation | a fixed protocol both parties know |
| Bound by fixed rules? | never — that would cripple it | entirely — that is what it *is* |
| Failure mode | stagnation | unintelligibility |
| Correct source | physics | convention |

English SVO is not a fact about the universe. It is a convention;
Bengali SOV encodes identical cognitive content under a different one.
No experiment discovers that English places verbs medially, because
there is nothing there to discover — only something to *agree*.

### 1.2 Why This Killed Physics-Based Generation

Earlier work in this lineage (`synthetic-intelligence.md` §5.4b,
`mrs.md`) attempted to derive word ordering from field dynamics: syntax
slots with fixed natural frequencies, phase-locked readout, gauge
offsets enforcing subject-before-object. The concept-level ordering
partially succeeded. The *byte*-level emission babbled.

The diagnosis was repeatedly treated as a tuning problem. It was not.
Asking a resonance field to discover that English uses articles is
asking it to derive a convention from a phenomenon. There is no
gradient to follow because there is no fact of the matter.

**The field ran to language models for the same reason.** The
statistical shortcut is not a theory of language production; it is what
remains once the mechanical problem is abandoned. A next-token
distribution requires no account of *why* one word follows another,
only a count of how often it did. It works spectacularly, and it
forecloses the question.

CLL claims the mechanical route was never seriously attempted because
the problem was mis-stated, not because it is closed.

---

## 2. The Locking Principle

**Definition (Constructive Language Locking).** A cognitive
architecture satisfies CLL when:

1. **Core freedom.** No grammatical rule appears anywhere in the
   cognitive substrate. Its dynamics, novelty mechanisms, and structural
   operations are identical whether the output language is English,
   Bengali, or none.
2. **Protocol determinism.** Surface realisation is a total function
   from (cognitive state, language cluster) to string. The same input
   always yields byte-identical output. Nothing is sampled.
3. **Isolation.** Language clusters may be added, removed, or replaced
   without modifying the core. Their number does not affect cognitive
   capability.

The name records the asymmetry: cognition stays **constructive**
(free to build), while language is **locked** (fixed to a protocol).

**Corollary (No cognitive degradation).** Because (1) forbids grammar
in the core, adding a language cluster cannot reduce cognitive
flexibility. This is a structural guarantee, not an empirical claim.

---

## 3. Architecture

```
   [ Core SI Physics Substrate ]
     resonance, phase-locking, fission, L6-A bridging,
     thermodynamic discovery, oracle validation
                  |
                  |  emits raw concept triples
                  |  (subject, relation, object)
                  v
   [ Isolated Language Clusters ]
     +- English   : SVO grid, articles, 3sg agreement
     +- Bengali   : SOV grid, no articles, danda terminator
     +- Banglish  : SOV grid, Latin script, strict transliteration
                  |
                  v
          human-readable stream
```

The interface between the layers is deliberately narrow: a triple of
graph labels plus a relation identifier. Nothing linguistic crosses
upward, and nothing cognitive crosses downward.

### 3.1 Cluster Contents

A language cluster is **data, not code**:

| Component | Role | Size |
| --- | --- | --- |
| Word order | SVO / SOV / VSO | ~2 bits |
| Relation lexicalisation | `part_of` → "is part of" | 1 row per relation |
| Determiner policy | per-construction, not per-language | 1 flag per relation |
| Morphology rules | agreement, plural, conjugation | 1 rule per pattern |
| Transliteration map | তুমি ↔ tumi | 1 row per word pair |
| Terminator | `.` / `।` | 1 char |

Adding a language means adding a table. The substrate is never touched.

### 3.2 Realisation

Given triple $(s, r, o)$ and cluster $L$:

$$
\text{realise}_L(s,r,o) = \Pi_L\bigl(\lambda_L(s),\; \rho_L(r),\; \delta_L(r, o)\bigr)
$$

where $\lambda_L$ is concept surface form (transliteration if any),
$\rho_L$ is relation lexicalisation, $\delta_L$ applies determiner
policy conditioned on the *construction* $r$, and $\Pi_L$ permutes by
word order. Every component is a table lookup or a fixed string
operation. The function is total and deterministic.

### 3.3 Measured Output

One cognitive state, three protocols (`src/lang.hpp`):

```
(biology, is_a, science)      en: Biology is a science.
                              bn: Biology science একটি।
                              bl: Biology science ekta.

(metformin, inhibits, mtor)   en: Metformin inhibits mtor.
                              bn: Metformin mtor বাধা দেয়।
                              bl: Metformin mtor badha dey.

(drive, precedes, tick)       en: Drive must run before tick.
                              bn: Drive tick আগে চলে।
                              bl: Drive tick age chole.
```

---

## 4. A Defect That Testing Disproved Immediately

The first implementation treated determiner insertion as a **global
language property** — "English uses articles" — and produced:

> *"Biology is a the science."*

Whether an object takes a determiner is not a property of the language.
It belongs to the **construction**: `is_a` already supplies its own
article; proper nouns take none. The policy is now a per-relation flag.

This is recorded because it establishes the shape of all remaining work
in this layer. Real grammar exceeds word order, and each additional
rule is discovered by *testing against a speaker's judgement*, not by
gradient descent. Every one is deterministic data.

---

## 5. Transliteration as Strict Relation

Code-switching is where language models fail most visibly and CLL
succeeds most cleanly. The mapping

$$
\texttt{tumi} \;\xleftrightarrow{\;\text{REL\_PHONETIC\_EQUIV}\;}\; \texttt{তুমি}
$$

is a **strict static relation**, not a probable correspondence. There is
no distribution over spellings, so there is no drift. A language model
generating Banglish must sample each character sequence and will
produce *tumi*, *tomi*, *tumii* across a single document. CLL cannot,
because nothing is sampled.

This generalises to dialect: Sylheti, Chittagonian and standard Bengali
differ in phonotactic mapping while sharing cognitive content. Swapping
the transliteration table swaps the dialect. The thought is unchanged.

---

## 6. Cost Accounting

CLL introduces **no hidden cost** and must be honest about the cost it
does introduce.

The Translation Capacity Lemma (`synthetic-intelligence.md` §3.1) proves
that discriminating $N$ semantic outcomes requires $\log_2 N$ bits
somewhere. Layer 3 already pays this by storing label strings. CLL adds:

| Addition | Cost | Category |
| --- | --- | --- |
| Word order | 2 bits per language | protocol |
| Relation lexicalisation | $O(|\mathcal{R}|)$ rows per language | protocol |
| Transliteration | $O(|\text{lexicon}|)$ rows per script | protocol |

All are Layer-3 protocol cost, explicitly named. **None is
intelligence.** A word-order constant does not help the system reason;
it helps the system be understood.

Critically, the cost is $O(|\mathcal{R}|)$ in the number of *relations*,
not $O(N^2)$ in concept pairs — because grammar generalises over
relation types, which is exactly what makes it a protocol rather than a
lookup table of sentences.

---

## 7. Comparison

| Property | Language model | CLL |
| --- | --- | --- |
| Grammar source | inferred from token co-occurrence | stated convention |
| Sequencing | argmax over learned distribution | table permutation |
| Surface form stability | sampled — drifts | exact — cannot drift |
| New language | retrain or scale | add a table |
| Dialect switch | prompt and hope | swap transliteration map |
| Cost per sentence | GPU forward pass | hash lookup + concat |
| Failure mode | fluent hallucination | explicit gap |
| Cognitive coupling | grammar entangled in weights | zero — core untouched |

The last row is the load-bearing one. In a language model, grammatical
competence and world knowledge occupy the same parameters and cannot be
separated. Under CLL they are different files.

---

## 8. What Remains Open

CLL solves *protocol*. It does not solve *morphological productivity*.

**Solved.** Word order, determiner policy, relation lexicalisation,
script and transliteration, terminator conventions, dialect swap.

**Deterministic but unwritten.** Subject-verb agreement (*studies*, not
*study*); pluralisation; Bengali verb conjugation by person and
honorific (তুমি / আপনি / তুই); case marking under free word order;
multi-triple discourse with pronoun resolution. All are tables. None
needs ML. They are labour, not research.

**Genuinely open.** Producing the bytes of a word **not in the
lexicon** — inflecting an unseen stem, coining a compound. This is the
residual of the emission problem (`synthetic-intelligence.md` §8.5),
and CLL narrows it rather than solving it: for any word already in
Layer 3, no byte synthesis is required at all. The open problem is
morphological *productivity*, not speech.

**Not attempted.** Prosody, register, politeness gradation beyond
pronoun choice, and anything requiring modelling the listener.

---

## 9. Failure Modes

1. **Protocol drift from reality.** A hand-written grammar table can be
   simply wrong about the language. There is no gradient to correct it —
   only a native speaker. This is a genuine disadvantage relative to
   corpus-fitted models, and the correct mitigation is a held-out
   judgement set per language, not more rules.

2. **Coverage cliff.** A relation with no lexicalisation entry falls
   back to rendering its identifier. The output stays intelligible but
   degrades visibly. This is preferable to inventing a form, but it is
   a cliff rather than a slope.

3. **The template ceiling.** Single-triple realisation cannot produce
   subordination, relative clauses, or discourse cohesion. Extending to
   multi-triple output requires a discourse layer that does not yet
   exist, and it is not obvious that it remains purely tabular.

4. **False economy on free-word-order languages.** Bengali permits
   reordering because case marking carries role. A fixed SOV template
   is an approximation that will read stiffly, and the fix is case
   morphology — item (2) of §8, not a new mechanism.

---

## 10. Summary

Physics-based language generation failed because it asked a resonance
field to discover a convention. Constructive Language Locking separates
the two: cognition remains fully constructive and unconstrained, while
surface realisation is a deterministic protocol held in isolated,
swappable language clusters.

The core loses nothing — a structural guarantee, since no grammar rule
enters it. The output gains exactness that sampling architectures
cannot have. And the mechanism costs a table per language against a
GPU forward pass per sentence.

The remaining hard problem is smaller and better specified than before:
not *how does a field speak*, but *how does a field inflect a word it
has never seen*.
