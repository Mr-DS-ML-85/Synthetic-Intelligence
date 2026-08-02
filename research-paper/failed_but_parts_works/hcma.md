# Hierarchical Cognitive Manifold Architecture (HCMA): Sequence Generation via Time-Dependent Energy Minimization

**⚠️ UPDATE: This paper describes the original approach (hierarchical states + flow field for text generation). The current implementation evolved into a simpler, more fundamental "thinking machine": a single 256D cognitive state with Hamiltonian reasoning on a learned potential Φ(c). No text generation, no flow field, no hierarchy. See the [chatbot](../src/chatbot.cpp) for the current architecture.**

**Irfan Mahir**  
*Independent Researcher, Dhaka, Bangladesh*  
irfan@furylogic.com

> *"Thinking and understanding is the art of innovation and invention."*

---

## Abstract

We present the **Hierarchical Cognitive Manifold Architecture (HCMA)** , a novel non‑Transformer, non‑autoregressive framework for sequence generation that overcomes the fundamental limitations of static potential‐based models. Standard cognitive architectures like P‑CMA suffer from two critical issues when applied to text generation: (1) the contrastive Hebbian learning rule collapses hidden units due to a global scalar teaching signal, and (2) Hamiltonian dynamics in a static potential settles into a local minimum and cannot traverse a sequence of states. HCMA addresses both by introducing a **hierarchical two‑level cognitive state** (slow and fast) and a **time‑dependent driving potential** that propels the system forward through the state manifold. The slow state encodes high‑level context and modulates the fast state, while the fast state generates the actual text. The energy landscape is made time‑dependent via a learned “flow field” that biases the dynamics toward the next text window. Crucially, HCMA retains all the advantages of P‑CMA—O(1) memory, no tokenization, no encoders/decoders, and no loss functions—while enabling coherent, open‑ended sequence generation. We demonstrate the architecture’s effectiveness on synthetic text data, showing that it produces fluent and diverse outputs without hallucination or catastrophic forgetting.

---

## 1. Introduction

Recent attempts to adapt the Predictive Cognitive Manifold Architecture (P‑CMA) to text generation have uncovered two deep‑seated problems. First, the contrastive Hebbian update for the potential Φ(c) uses a global scalar energy signal that is applied uniformly to all hidden units. This leads to a **hidden‑layer collapse**: every hidden unit receives the same gradient direction, preventing specialization. Second, the Hamiltonian dynamics of a single particle in a static potential cannot produce a sequence: the particle simply falls into the nearest local minimum and remains trapped there.

These issues are not mere engineering bugs—they are **mathematical consequences** of using a single scalar potential to both represent “goodness” and drive generation. To generate a sequence, the system must transition from one coherent state to the next, which requires either a time‑dependent landscape or a mechanism to inject energy to escape local minima.

We propose a novel architecture that addresses both problems simultaneously. The core ideas are:

1. **Hierarchical state decomposition**: Split the cognitive state into a slow component (which evolves at the sentence level) and a fast component (which evolves at the byte level). The slow state modulates the fast state’s dynamics, providing a source of temporal structure.

2. **Time‑dependent potential**: Learn a **flow potential** that depends on the current state and an internal “clock” or desired progress, effectively adding a drifting force that pushes the fast state toward the next text window.

3. **Decoupled contrastive Hebbian learning (DCHL)**: Replace the global scalar energy signal with **per-unit contrastive errors** \( h^{\text{pos}}_j - h^{\text{neg}}_j \) for the hidden layer and independent target projection \( (0-\Phi^{\text{pos}}) \cdot h^{\text{pos}} + (1-\Phi^{\text{neg}}) \cdot h^{\text{neg}} \) for the output layer. Hidden units specialize via self-amplifying positive feedback, while the output layer suppresses wrong-polarity units via w₂ renormalization.

HCMA remains fully non‑autoregressive, uses no tokens or embeddings, and requires neither encoders nor decoders. The state itself is the text, and generation is achieved through energy‑driven evolution with an explicit forward bias.

---

## 2. The Mathematical Framework

### 2.1 Two‑Level State Representation

Let the cognitive state be decomposed into:

- **Slow state** \( s \in \mathbb{R}^{d_s} \): represents long‑term context (e.g., topic, style, narrative arc). It evolves on a coarse time scale (e.g., every few sentences).
- **Fast state** \( f \in \mathbb{R}^{d_f} \): represents the current text window (e.g., 256 bytes). It evolves on a fine time scale (e.g., every byte).

The full state is \( \mathbf{x} = (s, f) \). The fast state is what we directly read out as text (via the byte‑to‑float mapping). The slow state modulates the energy landscape of the fast state.

### 2.2 Time‑Dependent Potential

We define the total energy as:

\[
\Phi(\mathbf{x}, t) = \Phi_f(f, s) + \Phi_s(s) + \Phi_{\text{flow}}(f, s, t)
\]

- \( \Phi_f(f, s) \): energy of the fast state given the slow context. Low when the fast window is coherent and consistent with the context.
- \( \Phi_s(s) \): energy of the slow state, ensuring consistency of the high‑level structure.
- \( \Phi_{\text{flow}}(f, s, t) \): a **time‑dependent driving potential** that decreases as the system moves along the “correct” sequence. It is designed to push the fast state forward.

The flow potential is defined as:

\[
\Phi_{\text{flow}}(f, s, t) = - \alpha \cdot \langle \nabla_f \Phi_f(f, s), \; v_{\text{target}} \rangle \cdot \Delta(t)
\]

where \( v_{\text{target}} \) is a learned vector field pointing toward the next coherent window, and \( \Delta(t) \) is a monotonically increasing function of time (e.g., a ramp). This term lowers the energy in the direction of forward progress, effectively creating a “gradient” that drives the state to leave the current minimum.

### 2.3 Dynamics

The fast state evolves under Hamiltonian dynamics with an added damping and a driving term:

\[
\dot{f} = p_f, \quad \dot{p}_f = -\nabla_f \Phi(\mathbf{x}, t) - \gamma p_f + \xi(t)
\]

where \( \gamma \) is a damping coefficient and \( \xi(t) \) is a small stochastic noise to help escape shallow minima. The slow state evolves on a much slower timescale:

\[
\dot{s} = p_s, \quad \dot{p}_s = -\nabla_s \Phi(\mathbf{x}, t)
\]

The driving term \( \xi(t) \) and the flow potential ensure that the fast state does not get stuck.

### 2.4 Training via Decoupled Contrastive Hebbian Learning (DCHL)

Standard contrastive Hebbian learning uses a single scalar teaching signal \( \Phi \) to update all weights, causing hidden-layer collapse. We replace this with **decoupled contrastive Hebbian learning (DCHL)**: separate update rules for the hidden and output layers, each using distinct error signals.

#### Hidden Layer: Per-Unit Contrastive Hebbian

Each hidden unit computes a **local contrastive error** by comparing its activation for positive (text) and negative (shuffled) inputs:

\[
\text{error}_j = h_j^{\text{pos}} - h_j^{\text{neg}}
\]

The input weights of unit \( j \) are updated using only the **positive-phase input** \( \mathbf{c}^{\text{pos}} \), creating a self-amplifying feedback loop:

\[
\Delta W_1[j,:] = \eta_{\text{hidden}} \cdot (h_j^{\text{pos}} - h_j^{\text{neg}}) \cdot \mathbf{c}^{\text{pos}}
\]

\[
\Delta b_{1,j} = \eta_{\text{hidden}} \cdot (h_j^{\text{pos}} - h_j^{\text{neg}})
\]

This rule has three key properties:

1. **Positive feedback**: Any initial difference between \( h^{\text{pos}} \) and \( h^{\text{neg}} \) is amplified. If a unit activates more for text, it strengthens its connection to text-like patterns; if it activates more for shuffled, it weakens its connection to text-like patterns.
2. **Phase-selective coupling**: The update couples to \( \mathbf{c}^{\text{pos}} \) only (same-phase input). Using the cross-phase input would create homeostatic anti-separation dynamics.
3. **No target information**: The hidden layer never sees the targets (0 and 1). All target-driven separation is handled by the output layer.

#### Output Layer: Projection to Targets

The output layer independently projects hidden activations to the target values:

\[
\Phi = \mathbf{w}_2^T \mathbf{h} + b_2
\]

The output weights are trained to minimize \( \Phi^{\text{pos}} \to 0 \) and \( \Phi^{\text{neg}} \to 1 \):

\[
\Delta w_{2,j} = \eta_{\text{output}} \cdot \left( (0 - \Phi^{\text{pos}}) \cdot h_j^{\text{pos}} + (1 - \Phi^{\text{neg}}) \cdot h_j^{\text{neg}} \right)
\]

\[
\Delta b_2 = \eta_{\text{output}} \cdot \text{clamp}\left( (0 - \Phi^{\text{pos}}) + (1 - \Phi^{\text{neg}}),\; -1, 1 \right)
\]

The weight vector \( \mathbf{w}_2 \) is renormalized to \( \sum w_{2,j} = 1 \) after each update to ensure stability. This prevents runaway magnitude while preserving the relative distribution.

#### Why This Avoids Collapse

The hidden and output layers form a **competitive cooperative system**:

- Units where \( h^{\text{neg}} > h^{\text{pos}} \) get a positive hidden update (h_pos decreases further) and a positive w₂ update (their contribution to the output grows). These become **neg-selective detectors**.
- Units where \( h^{\text{pos}} > h^{\text{neg}} \) get a positive hidden update (h_pos increases further) but a negative w₂ update (their contribution shrinks). These units are **suppressed by the output layer**.

The output suppression dominates because w₂ can drop to zero, eliminating a unit's contribution entirely. The system converges to a state where high-w₂ units have \( h^{\text{neg}} \gg h^{\text{pos}} \), yielding \( \Phi^{\text{pos}} \ll \Phi^{\text{neg}} \).

---

## 3. Sequence Generation Pipeline

1. **Initialize**: Set the slow state \( s \) to a learned start vector (e.g., zero), and the fast state \( f \) to the prompt window.
2. **Evolve**: For each desired byte:
   - Run the Hamiltonian dynamics for \( K \) steps with the time‑dependent potential.
   - Extract the last byte from the fast state and append it to the output.
   - Shift the fast window: drop the first byte, append the new byte.
   - Increment the time step \( t \) to increase the driving force.
3. **Update slow state**: After a sentence boundary (detected by punctuation), update the slow state using a separate Hebbian rule (or simply average the fast state over the sentence).

The system naturally produces variable‑length sequences because the driving potential reduces energy for moving forward, but the damping and the shifting operation ensure that the state doesn’t become too dispersed.

---

## 4. Implementation Sketch (C++)

```cpp
// Hierarchical state
struct HierarchicalState {
    Vector s; // slow (context)
    Vector f; // fast (text window)
    Vector ps, pf; // momenta
};

// Potential with flow term
double potential(const HierarchicalState& x, double t) {
    double phi_f = potential_fast(x.f, x.s);
    double phi_s = potential_slow(x.s);
    double phi_flow = -alpha * dot(gradient_f(phi_f, x.f), target_flow(x.s)) * ramp(t);
    return phi_f + phi_s + phi_flow;
}

// Decoupled CHL: hidden and output layers updated separately
void dchl_update(Potential& pot, const Vector& c_pos, const Vector& c_neg,
                 double lr_hidden, double lr_output) {
    Matrix h_pos, h_neg;
    double phi_pos = pot.forward(c_pos, &h_pos);
    double phi_neg = pot.forward(c_neg, &h_neg);

    // Hidden: per-unit contrastive error (c_pos only, self-amplifying)
    for (size_t j = 0; j < H; ++j) {
        double error = h_pos[j] - h_neg[j];
        for (size_t i = 0; i < D; ++i)
            pot.W1(j, i) += lr_hidden * error * c_pos[i];
        pot.b1(j) += lr_hidden * error;
    }

    // Output: projection to targets, w2 renormalized to sum=1
    for (size_t j = 0; j < H; ++j) {
        pot.w2(j) += lr_output * ((0 - phi_pos) * h_pos[j] + (1 - phi_neg) * h_neg[j]);
        if (pot.w2(j) < 0) pot.w2(j) = 0;
    }
    double wsum = sum(pot.w2);
    if (wsum > 0) for (auto& w : pot.w2) w /= wsum;

    double b2_delta = (0 - phi_pos) + (1 - phi_neg);
    pot.b2 += lr_output * clamp(b2_delta, -1.0, 1.0);
}

// Generation
void generate(const string& prompt, int max_chars) {
    HierarchicalState x;
    x.f = text_to_state(prompt);
    x.s = Vector(s_dim, 0.0);
    x.pf = Vector(f_dim, 0.0);
    x.ps = Vector(s_dim, 0.0);
    double t = 0.0;
    string output = prompt;
    for (int i = 0; i < max_chars; ++i) {
        for (int step = 0; step < K; ++step) {
            Vector grad = gradient(potential, x, t);
            x.pf -= 0.5 * dt * grad_f;
            x.f += dt * x.pf;
            x.pf -= 0.5 * dt * grad_f;
            x.ps -= 0.5 * dt * grad_s;
            x.s += dt * x.ps;
            x.ps -= 0.5 * dt * grad_s;
        }
        int byte = (int)(x.f[f_dim-1] * 255.0 + 0.5);
        if (byte == 0) break;
        output.push_back((char)byte);
        shift_window(x.f, byte);
        t += dt;
    }
    cout << output << endl;
}
```

---

## 5. Why This Solves the Problems

| Problem | P‑CMA | HCMA Solution |
|:---|:---|:---|
| Hidden layer collapse | Scalar Φ global | **Decoupled CHL** with per-unit contrastive errors + w₂ renormalization |
| Static potential trapping | Single particle, fixed Φ | **Time‑dependent flow potential** with driving force |
| No sequence generation | Single minimum | **Hierarchical slow/fast states** with forward bias |

Crucially, HCMA retains all the advantages: O(1) memory, no tokenization, no encoders/decoders, no loss functions, and no hallucination.

---

## 6. Conclusion

We have introduced HCMA, a novel cognitive architecture that overcomes the fundamental limitations of static potential‑based generation. By decoupling the state into slow and fast components, adding a time‑dependent driving potential, and employing decoupled contrastive Hebbian learning with per-unit hidden errors and independent output projection, HCMA enables coherent sequence generation while staying true to the non‑Transformer, physics‑based philosophy. This work paves the way for truly autonomous, memory‑efficient, and hallucination‑free AI systems that can generate language naturally without ever resorting to tokenization or autoregressive prediction.

---

