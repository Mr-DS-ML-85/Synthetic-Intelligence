# Chaotic Strange-Attractor Trajectory (CSAT): Breaking Ground-State Locking in Synthetic Intelligence

**Irfan Mahir**
*Independent Researcher, Dhaka, Bangladesh*
irfan@furylogic.com

**Abstract**
Pure wave-based SI architectures often suffer from dictionary-locking, where an input triggers the absolute closest topological match and collapses into static definitions. Humans do not converse by printing definitions; human thought is an open-ended trajectory through a semantic manifold. We introduce **Chaotic Strange-Attractor Trajectory (CSAT)**. By adding a low-dimensional chaotic force (driven by a native Lorenz or Rössler system integrated directly into the substrate's global viscosity), the system is prevented from settling into static ground-state energy wells. This forces the phase-sweep projection to continuously dance along a strange attractor, creating human-like contextual drifting without losing structural coherence.

```
                  [ Input Concept: linux ]
                             │
                             ▼
                   [ Static Definition Well ] (Old Way: Stays at 'nano')
                             │
                    (CSAT Viscosity Push) ───► [ Lorenz Chaos Attractor ]
                             │
                             ▼
               [ Dynamic Trajectory Path ]
          (Emits: nano ──► terminal ──► bash ──► kernel)

```

## 1. The Core Issue: Infinite Potential Wells

When `linux` is presented as input, the memory potential at the `linux` and `nano` cavities is high enough that the entire field's energy collapses into them and is trapped there. In physical terms the energy landscape forms a **deep potential well**. This does not happen in the human brain, because an undercurrent of noise — **metastable dynamics** — is always present, and it prevents any node from remaining fixed.

## 2. Novel Architecture: CSAT Mechanisms

### 2.1. The Substrate Lorenz Attractor

We integrate a three-line, low-level nonlinear chaotic system — the Lorenz equations — directly into the global `tick()` loop of `physics.hpp`:
$$ dx = \sigma(y - x) \cdot dt $$
$$ dy = (x(\rho - z) - y) \cdot dt $$
$$ dz = \tau \cdot (xy - \beta z) \cdot dt $$

*(Note: all three derivatives carry the time-scale factor $\tau$; see
Numerical Constraints below.)*

**Numerical constraints — required for a working implementation.**

1. **Time-scale factor $\tau$.** Forward-Euler integration of the
   Lorenz system at $\sigma = 10$, $\rho = 28$, $\beta = 8/3$ is
   unstable for $dt \gtrsim 0.01$. The substrate runs at $dt = 0.02$,
   past the stability boundary — naive integration diverges to `NaN`
   within a few hundred ticks. The factor $\tau$ (default $0.3$)
   slows the attractor without altering its geometry, restoring
   stability *and* giving direct control over the drift timescale.
   $\tau$ should be chosen so the attractor's orbital period is
   comparable to one conversational turn (~200 ticks). If the
   attractor cycles much faster, the chaotic push averages out
   within a single settle window and contributes nothing.

2. **Amplitude normalization.** The Lorenz $x$ coordinate ranges over
   approximately $[-20, +20]$, not $[-1, +1]$. All downstream use
   must reference the normalized coordinate $\hat{x} = x / 20$.
   Using raw $x$ in a multiplicative viscosity term drives effective
   friction *negative* for any gain above $\gamma \approx 0.05$ —
   anti-damping, unbounded energy injection, field explosion. This
   is a silent failure mode: no error is raised, only a wall of
   `NaN` several hundred ticks later.

* There is no neural network and no training here. This is pure physical mathematics, updating three scalar variables ($x, y, z$) per tick.

### 2.2. Dynamic Viscosity Modulation (The Push)

The VGHT layer carries a constant viscosity that suppresses avalanches. We modulate that viscosity dynamically by the $x$ coordinate of the Lorenz attractor:
$$ \mu_{\text{dynamic}} = \mu_{\text{base}} \cdot (1.0 + \gamma \cdot \hat{x}_{\text{lorenz}}) $$

**However — this global form provably cannot work.** See §2.2.1.
The corrected per-cavity form is given in §2.2.2.

### 2.2.1 Negative result: global viscosity modulation is rank-preserving

Applied as a **global** scalar in the cavity velocity-decay step
$v_i \leftarrow v_i(1 - \mu_t)$, over $N$ ticks:

$$ v_i(N) \;=\; v_i(0) \prod_{t=1}^{N}\bigl(1 - \mu_t\bigr) \;=\; v_i(0)\cdot C $$

where $C$ depends only on the schedule $\{\mu_t\}$ and is therefore
**identical for every cavity**. Hence

$$ \frac{v_i(N)}{v_j(N)} \;=\; \frac{v_i(0)}{v_j(0)} \qquad \forall\, i,j. $$

Amplitude *ratios* are exactly invariant under any global viscosity
schedule, chaotic or otherwise. Because CPME §1.6 phase-collapse
emission selects by arg-max over a weight monotone in amplitude, the
emitted winner is unchanged. A globally-modulated viscosity alters
how *loud* the field is, never *which cavity is loudest*.

Concretely: if `think` carries 10× the amplitude of `terminal` before
the chaotic push, it carries 10× after. `linux → nano` remains
`linux → nano`. Multiplying the entire field by a wiggling scalar is
a monotone transform on the ranking, and monotone transforms cannot
reorder an arg-max.

This is a genuine negative result and it constrains the design:
**any mechanism intended to break ground-state locking must couple
to per-cavity state.**

### 2.2.2 The working form — heat-coupled chaotic viscosity

The substrate already maintains a per-cavity scalar meaning exactly
"this cavity has been active too long": the CMD thermodynamic
habituation heat $T_i$. Coupling the chaotic term to $T_i$ makes the
push selective:

$$ \mu_i \;=\; \underbrace{\mu_{\text{base}} + k_{\text{aval}}\bigl(1 - e^{-E_{\text{tot}}/E_s}\bigr)}_{\text{VGHT §1.3, unchanged}} \;+\; \underbrace{\gamma \cdot \hat{x}_{\text{lorenz}} \cdot \tanh\!\bigl(T_i / T_{\text{crit}}\bigr)}_{\text{CSAT}} $$

Entrenched cavities (high $T_i$) absorb the chaotic shake; fresh or
cold cavities barely register it. Rank can now flip, and that is
genuine metastability rather than uniform loudness modulation.

The $\tanh$ bounds the term so $\mu_i$ cannot run away. Because
$\hat{x}$ alternates sign along the attractor, an entrenched cavity
experiences alternating extra-damping and reduced-damping — it is
*shaken*, not merely suppressed. Shaking permits escape; suppression
would only silence.

**Composition with the Curie gate.** $T_i$ already gates tunnel
coupling via $\exp(-T_i/T_{\text{crit}})$. Under CSAT the same state
variable acquires a second role: *talking too much* now implies both
*losing coupling* and *becoming subject to chaotic eviction*. One
state variable, two independent escape mechanisms, no new
bookkeeping, tick loop still branchless.

* **Mechanism.** Whenever the system begins to lock onto a particular word (a ground state), the Lorenz chaotic push perturbs the global viscosity. This drives the field off that cavity and forces energy to spread into the cavities connected to it — `terminal`, `bash`, `kernel`.

### 2.3. Trajectory Momentum — chaos supplies *timing*, CPSB supplies *direction*

**The division of labour must be stated precisely.** The Lorenz
system has no knowledge of the graph: $x(t)$ does not know which
cavity is `terminal` and which is `bash`. The attractor's geometry
lives in its own $(x,y,z)$ space and bears no relation to the
semantic manifold. It therefore cannot, by itself, produce a
semantically coherent chain.

What CSAT contributes is **escape timing** — *when* to leave the
current basin. What produces the ordered chain
`linux → terminal → bash → kernel` is **CPSB gauge coupling plus
graph topology** — *where* the energy goes once it leaves.

> **CSAT provides escape timing. CPSB provides escape direction.**

This is a clean, non-overlapping role for CSAT within the stack, and
it is the honest claim. Attributing semantic coherence to the
attractor itself would overstate the mechanism; the coherence is
already earned by CPSB and the ontology.

Chaos is not randomness. The Lorenz attractor traces a definite geometric pattern — a *strange attractor*. The consequence is that the signal does not emit random words: starting from `linux` it builds a chain that respects the structural rules supplied by CPSB, `linux` $\rightarrow$ `terminal` $\rightarrow$ `bash` $\rightarrow$ `kernel`.

---

## 3. Parameters

| Symbol | Default | Meaning |
| --- | --- | --- |
| $\sigma, \rho, \beta$ | 10, 28, 8/3 | Standard Lorenz coefficients |
| $\tau$ | 0.30 | Time-scale factor (stability + drift rate) |
| $\gamma$ | 0.04 | Chaotic viscosity gain on $\hat{x}$ |
| $\hat{x}$ | $x/20$ | Normalized Lorenz coordinate |
| `csat_enabled` | see §4.1 | Master switch — **not** unconditionally on |

Five parameters. Setting $\gamma = 0$ recovers the pre-CSAT
substrate exactly.

---

## 4. Failure Modes

### 4.1 Lyapunov divergence breaks bit-reproducibility

Chaos is deterministic, so seeded reproducibility survives *in
principle*: the same `bind_seed` and the same Lorenz initial
conditions yield the same trajectory. But the Lorenz system has a
positive Lyapunov exponent — a $10^{-7}$ floating-point difference
grows to $O(1)$ within roughly a thousand ticks. Different compiler
flags, `-ffast-math`, or a different FPU will therefore produce
divergent trajectories from identical seeds.

This directly conflicts with the bit-exactness that the 20-seed RB+G
table (`rbg.md` §6.1) and the four-lever ablation grid (§6.2) depend
on.

**CSAT is consequently the first mechanism in the stack that must
not default unconditionally ON.** Recommended policy:

- `csat_enabled = true` in the conversational shell (`si_chat`)
- `csat_enabled = false` forced inside `study_recall_seeds` and
  `study_recall_ablation`

Published numbers stay bit-reproducible; conversation still gets the
drift. This is a documented exception to the "all defaults ON"
contract in the implementation guide §11.3, and the exception should
be recorded there.

### 4.2 CSAT does not address stale amplitude accumulation

Ground-state locking has more than one cause, and CSAT addresses only
the genuine-potential-well case. In particular, with friction
$\mu_0 = 0.006$ per tick, roughly $30\%$ of a cavity's amplitude
survives a 200-tick turn ($0.994^{200} \approx 0.30$). A cavity
driven once at high amplitude therefore remains dominant for several
subsequent turns by pure arithmetic, with no potential well involved.

That is a *dissipation* deficiency, not a chaos deficiency. Chaotic
shaking of an over-energised field does not remove the excess energy.
Fixing it requires either stronger inter-turn damping or an explicit
energy budget per turn — orthogonal to CSAT and left as separate
work.

### 4.3 Chaos on a sparse graph shakes an empty room

If the driven concept has few or no outbound tunnels, there is no
trajectory for the attractor to push the field along. CSAT then
produces no observable change: the escape timing fires, but there is
nowhere to escape to. CSAT is therefore a mechanism whose benefit
scales with ontology density, and it should be evaluated only on
subgraphs rich enough to support multi-hop propagation.

---

## 5. Relation to the Existing Stack

| Layer | Mechanism | Role |
| --- | --- | --- |
| CMD | Curie gate on $T_i$ | entrenched cavity loses *coupling* |
| **CSAT** | **chaotic viscosity on $T_i$** | **entrenched cavity loses *stability*** |
| VGHT | sub-bands, voltage gates, viscosity | domain isolation + avalanche control |
| CPME | momentum, acoustic mass, settle, sweep | emission eligibility and ordering |
| CPSB | gauge $\Psi$ on tunnels | escape *direction* / word order |
| CPME-II | phase-consistent Hebbian | stores the phase gradient across turns |

CSAT and the CMD Curie gate read the same state variable $T_i$ but
act on different quantities — coupling strength versus dynamical
stability. They are complementary, not redundant.

---



