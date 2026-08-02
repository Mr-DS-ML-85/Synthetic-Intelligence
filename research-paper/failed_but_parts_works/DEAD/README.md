# DEAD — formally retired papers

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

**Every paper in this directory is dead.** They are retained for
provenance and priority only. They do **not** describe the current
system and must not be cited as if they did.

Formally announced in `research-paper/synthetic-intelligence.md` §6.2.3.

| File | Why dead |
|---|---|
| `bug_fix.md` | roadmap, fully superseded by `ROADMAP.md` |
| `future_fix.md` | roadmap, fully superseded by `ROADMAP.md` |
| `deprected/bed.md` | Byte Energy Descent — converged to byte-mean states with no prompt-conditioned structure |
| `deprected/scer.md` | multi-mode routing — subsumed by the field-observable dispatcher |
| `deprected/zerograd.md` | see below — **paper dead, rule alive** |
| `deprected/anq.md` | see below — **paper dead, one idea alive** |
| `deprected/hmhra.md` | superseded before implementation |
| `deprected/hrm.md` | superseded before implementation |
| `deprected/omt.md` | superseded before implementation |
| `deprected/cb.md` | superseded before implementation |
| `deprected/cg.md` | superseded before implementation |
| `deprected/fixes.md` | ad-hoc patch log, no longer navigable |

## Two mechanisms outlived their papers

A dead paper is not a retracted one. Two ideas here are running in
production while their papers sit in this directory.

**ZeroGrad** — the paper described backprop-free training of the DCHL
potential, and that substrate is retired. The *learning rule* is live
and foundational. `src/physics.hpp` line 20:

```
ZeroGrad : plasticity_delta = pd·hebbian_decay + plastic_rate·v_a·v_b
```

Every Hebbian update in the engine is this rule.

**ANQ** — the 4-bit quantisation scheme is retired with the GPU trainer
it served. One idea survived: continuous integrator in place of
threshold-commit, cited in `src/physics.hpp` line 21 as `ANQ LEA`, used
by every saturating accumulator in the tick.

**Citation rule.** Cite these two as *sources of mechanisms*. Never as
*descriptions of the system*.
