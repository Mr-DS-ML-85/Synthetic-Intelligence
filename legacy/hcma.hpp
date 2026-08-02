// HCMA / DCHL Thinking Machine core.
//
// Physics-based cognitive architecture:
//   - No backprop, no loss functions, no gradient descent on parameters.
//   - No tokenizer, no embeddings, no attention, no KV-cache, no vocab.
//   - Learning: Decoupled Contrastive Hebbian Learning (DCHL), purely local.
//   - Inference: Hamiltonian dynamics on the learned potential Phi(c).
//   - Confidence: KDE density of the state against the known manifold region.
//
// State layout for real "thinking" (not just pattern-matching text vs random):
//
//   c = [ prompt bytes | answer bytes ]      (both scaled to [0,1] = byte/256)
//         D_prompt         D_answer          D = D_prompt + D_answer
//
// Training pushes Phi(prompt + correct_answer) below Phi(prompt + wrong_answer).
// During inference (thinking), the prompt region is FROZEN (gradient masked)
// and only the answer region evolves under -grad Phi. The evolved answer
// bytes decode straight back to text via round(state * 256).
//
// This is fundamentally different from next-token prediction: no vocab, no
// argmax, no autoregression; the whole answer is one continuous state that
// descends an energy landscape until it lands in a basin the DCHL Hebbian
// updates carved out during training.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace hcma {

using Vec = std::vector<float>;

inline float dot(const float* a, const float* b, size_t n) {
    double s = 0.0;
    for (size_t i = 0; i < n; ++i) s += (double)a[i] * b[i];
    return (float)s;
}

// Byte <-> float.  byte b maps to b/256 ∈ [0, 1).  This is what the
// augmented-norm scaling (Cauchy–Schwarz |pre|≤1) is calibrated for; the
// zero-centered [-1, 1] variant doubles ||c||² and pushes pre-activations
// even deeper into tanh's linear regime, which makes the "ASCII shift leak"
// worse rather than better.  The real fix for the leak is elsewhere:
// stronger non-linearity (larger effective pre-activation) via the reduced
// aug-norm scaling, tighter w2 decay, and per-epoch normalisation only.
inline float byte_to_val(int b) { return (float)b / 256.0f; }
inline int   val_to_byte(float v) {
    int b = (int)std::lround(v * 256.0f);
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    return b;
}
inline float pad_val() { return 0.0f; }

inline Vec text_to_state(const std::string& text, size_t D) {
    Vec c(D, pad_val());
    size_t n = text.size() < D ? text.size() : D;
    for (size_t i = 0; i < n; ++i)
        c[i] = byte_to_val((int)(uint8_t)text[i]);
    return c;
}

// Decode a float range back to a printable string. Non-printable bytes are
// replaced by space; trailing spaces trimmed.
inline std::string range_to_text(const float* c, size_t n) {
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        int b = val_to_byte(c[i]);
        char ch = (char)b;
        if (ch >= 32 && ch <= 126) s.push_back(ch);
        else s.push_back(' ');
    }
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

// Assemble a cognitive state from a (question, answer) pair. Question fills
// the prompt region; answer fills the answer region; both padded with the
// neutral value 0.
inline Vec make_qa_state(const std::string& q, const std::string& a,
                         size_t D_prompt, size_t D_answer) {
    Vec c(D_prompt + D_answer, pad_val());
    size_t nq = std::min(q.size(), D_prompt);
    for (size_t i = 0; i < nq; ++i) c[i] = byte_to_val((int)(uint8_t)q[i]);
    size_t na = std::min(a.size(), D_answer);
    for (size_t i = 0; i < na; ++i) c[D_prompt + i] = byte_to_val((int)(uint8_t)a[i]);
    return c;
}

// Augmented input norm ||[c, 1]|| — the Cauchy-Schwarz constant that bounds
// |pre-activation| <= 1 given unit-norm weight rows.
inline float aug_norm(const Vec& c) {
    double s = 1.0;
    for (float v : c) s += (double)v * v;
    return (float)std::sqrt(s);
}

// D-invariant pre-activation scale. The plain aug_norm divisor puts typical
// pre-activations at ~1/sqrt(D) — deep in tanh's linear regime for high D
// with byte-scale inputs, which produces the |h|=0.01 dead-lock we hit at
// 1B params. Dividing instead by sqrt((||c||^2 + 1) / D) makes the typical
// pre magnitude ~1.0 regardless of D, so tanh actually acts non-linearly.
// The Cauchy-Schwarz bound is still meaningful: specialized units saturate
// (|h| -> 1), unspecialized ones sit in the useful mid-range.
inline float pre_inv_scale(const Vec& c, size_t D) {
    double s = 1.0;
    for (float v : c) s += (double)v * v;
    // = sqrt(D) / aug_norm(c)
    return (float)(std::sqrt((double)D) / std::sqrt(s));
}

// ---------------------------------------------------------------------------
// The learned potential Phi(c): energy of a cognitive state.
// Single hidden layer, tanh, scalar readout. All params trainable via DCHL.
// ---------------------------------------------------------------------------
struct Potential {
    size_t D = 512;         // total state dim
    size_t H = 128;         // hidden units
    size_t D_prompt = 256;  // frozen region during inference
    size_t D_answer = 256;  // evolved region during inference

    std::vector<float> W1;  // H x D, row-major
    Vec b1;                 // H
    Vec w2;                 // H
    float b2 = 0.0f;
    float baseline = 0.0f;  // running readout center

    Potential() = default;
    Potential(size_t d_prompt, size_t d_answer, size_t h)
        : D(d_prompt + d_answer), H(h),
          D_prompt(d_prompt), D_answer(d_answer),
          W1(h * (d_prompt + d_answer)), b1(h, 0.0f), w2(h, 0.0f) {
        structured_init();
    }

    // Diverse init that gives BOTH orthogonality between units AND a
    // large-enough initial |h| for the Hebbian update to have signal.
    //
    // We draw rows from N(0, 1/D). Each row has ||W1_j|| ≈ 1 in expectation
    // (so |W1_j · c| stays O(1) even without normalisation), and different
    // rows are almost orthogonal in D>>1 dimensions. We do NOT L2-normalise
    // at init — that step happens end-of-epoch (paper v10). Normalising at
    // init together with an aug-norm divisor of ~sqrt(D+1) crushes the
    // pre-activation to ~0 and freezes DCHL at |h_pos - h_neg| ≈ 0.
    void structured_init(unsigned seed = 42) {
        std::mt19937 rng(seed);
        float sigma = 1.0f / std::sqrt((float)D);
        std::normal_distribution<float> g(0.0f, sigma);
        for (size_t j = 0; j < H; ++j) {
            for (size_t i = 0; i < D; ++i) W1[j * D + i] = g(rng);
            b1[j] = g(rng);
        }
    }

    // L2-normalize the combined vector [W1_j, b1_j] to unit norm.
    // Bias is part of the constraint — this stops b1 runaway and makes
    // the augmented-norm scaling produce |pre| <= 1 exactly.
    void normalize_row(size_t j) {
        double s = (double)b1[j] * b1[j];
        float* row = &W1[j * D];
        for (size_t i = 0; i < D; ++i) s += (double)row[i] * row[i];
        float nrm = (float)std::sqrt(s);
        if (nrm < 1e-12f) return;
        float inv = 1.0f / nrm;
        for (size_t i = 0; i < D; ++i) row[i] *= inv;
        b1[j] *= inv;
    }

    // Forward pass.  pre = (W1_j*c + b1_j) * sqrt(D) / ||[c,1]||   (D-invariant
    // scaling, see hcma.hpp header comment on pre_inv_scale for the reason).
    // h = tanh(pre), Phi = w2*h + b2.
    float forward(const Vec& c, Vec* h_out = nullptr) const {
        float inv_s = pre_inv_scale(c, D);
        double phi = b2;
        if (h_out) h_out->resize(H);
        for (size_t j = 0; j < H; ++j) {
            float pre = (dot(&W1[j * D], c.data(), D) + b1[j]) * inv_s;
            float h = std::tanh(pre);
            if (h_out) (*h_out)[j] = h;
            phi += (double)w2[j] * h;
        }
        return (float)phi;
    }

    float phi_centered(const Vec& c) const { return forward(c) - baseline; }

    // Analytic gradient dPhi/dc — force field of the cognitive manifold.
    void gradient(const Vec& c, Vec& g) const {
        float n = aug_norm(c);
        float inv_n = 1.0f / n;
        float inv_n2 = inv_n * inv_n;
        g.assign(D, 0.0f);
        double corr = 0.0;
        for (size_t j = 0; j < H; ++j) {
            float pre = (dot(&W1[j * D], c.data(), D) + b1[j]) * inv_n;
            float h = std::tanh(pre);
            float s = w2[j] * (1.0f - h * h);
            const float* row = &W1[j * D];
            for (size_t i = 0; i < D; ++i) g[i] += s * row[i];
            corr += (double)s * pre;
        }
        for (size_t i = 0; i < D; ++i)
            g[i] = g[i] * inv_n - (float)corr * c[i] * inv_n2;
    }
};

// ---------------------------------------------------------------------------
// DCHL: two independent, purely local, ZeroGrad updates. No backprop, no
// loss function, no autograd, no optimizer state.
// ---------------------------------------------------------------------------

// One contrastive DCHL step on any (positive, negative) state pair.
// This is the low-level primitive used by both text-vs-random and Q->A.
inline void chl_step(Potential& P, const Vec& c_pos, const Vec& c_neg,
                     float lr_hidden, float lr_output) {
    Vec h_pos, h_neg;
    P.forward(c_pos, &h_pos);
    P.forward(c_neg, &h_neg);

    // Hidden layer: per-unit contrastive Hebbian, coupled to c_pos.
    for (size_t j = 0; j < P.H; ++j) {
        float err = h_pos[j] - h_neg[j];
        float* row = &P.W1[j * P.D];
        float step = lr_hidden * err;
        for (size_t i = 0; i < P.D; ++i) row[i] += step * c_pos[i];
        P.b1[j] += step;
    }

    // Output: pure differential dw = h_neg - h_pos.
    // Units firing more for the negative gain weight; units firing more for
    // the positive lose weight (clamp >= 0).
    for (size_t j = 0; j < P.H; ++j) {
        P.w2[j] += lr_output * (h_neg[j] - h_pos[j]);
        if (P.w2[j] < 0.0f) P.w2[j] = 0.0f;
    }
}

// The thinker signal is *coherence*, not memorized (Q,A) pairs. We show the
// network:
//   positive = an actual text window (bytes of a real document slice)
//   negative = an incoherent state of the same shape, one of:
//     NegKind::Random  -> uniform bytes         (trivial contrast)
//     NegKind::Shuffle -> shuffled positive     (same distribution, no order)
//     NegKind::Splice  -> [half of pos | half of another window]  (semantic
//                                                                  seam)
// DCHL pushes Phi(coherent) below Phi(incoherent). Because Phi is a scalar
// potential, F = -grad Phi is curl-free by construction, so composition of
// low-Phi basins is path-independent — that is the transitivity that makes
// this a thinker rather than a table lookup.
enum class NegKind { Random, Shuffle, Splice };

inline Vec make_negative(const Vec& c_pos, NegKind kind,
                         const Vec* other, std::mt19937& rng) {
    Vec c_neg = c_pos;
    if (kind == NegKind::Random) {
        std::uniform_int_distribution<int> b(1, 255);
        for (auto& v : c_neg) v = (float)b(rng) / 256.0f;
    } else if (kind == NegKind::Shuffle) {
        std::shuffle(c_neg.begin(), c_neg.end(), rng);
    } else { // Splice
        if (other && other->size() == c_pos.size()) {
            // Random split point in [D/4, 3D/4]. A fixed split at D/2 lets
            // the model learn a "detect the seam at position 256" shortcut;
            // randomising the point forces it to learn actual coherence.
            size_t D = c_pos.size();
            size_t lo = D / 4, hi = 3 * D / 4;
            std::uniform_int_distribution<size_t> u(lo, hi);
            size_t mid = u(rng);
            for (size_t i = mid; i < D; ++i) c_neg[i] = (*other)[i];
        } else {
            std::shuffle(c_neg.begin(), c_neg.end(), rng);
        }
    }
    return c_neg;
}

// Wrapper used by trainers and tests. Keep the name `chl_update_decoupled`
// for compatibility with the existing chatbot / tests.
inline void chl_update_decoupled(Potential& P, const Vec& c_pos, const Vec& c_neg,
                                 float lr_hidden, float lr_output) {
    chl_step(P, c_pos, c_neg, lr_hidden, lr_output);
}

// Multi-negative DCHL step.  For each positive we score K negatives and
// average their hidden activations, then do a single Hebbian update against
// the averaged negative.  This kills the variance of the "per-negative"
// signal (which is what makes dPhi wander when negatives are hard) and
// gives an effective K× stronger, more consistent gradient direction —
// which is what the hidden layer needs to actually specialise.
inline void chl_step_multi(Potential& P, const Vec& c_pos,
                           const std::vector<Vec>& c_negs,
                           float lr_hidden, float lr_output) {
    Vec h_pos;
    P.forward(c_pos, &h_pos);
    Vec h_neg_avg(P.H, 0.0f);
    for (const Vec& c_neg : c_negs) {
        Vec h;
        P.forward(c_neg, &h);
        for (size_t j = 0; j < P.H; ++j) h_neg_avg[j] += h[j];
    }
    float inv_k = 1.0f / (float)c_negs.size();
    for (size_t j = 0; j < P.H; ++j) h_neg_avg[j] *= inv_k;

    // Hidden layer: err = h_pos - <h_neg>.  Coupling to c_pos only.
    for (size_t j = 0; j < P.H; ++j) {
        float err = h_pos[j] - h_neg_avg[j];
        float* row = &P.W1[j * P.D];
        float step = lr_hidden * err;
        for (size_t i = 0; i < P.D; ++i) row[i] += step * c_pos[i];
        P.b1[j] += step;
    }
    // Output layer: pure differential, averaged neg.
    for (size_t j = 0; j < P.H; ++j) {
        P.w2[j] += lr_output * (h_neg_avg[j] - h_pos[j]);
        if (P.w2[j] < 0.0f) P.w2[j] = 0.0f;
    }
}

// End-of-epoch bookkeeping (v10):
//   - L2-normalize each hidden row [W1_j, b1_j] back to unit norm.
//   - Small L2 decay on w2 keeps the readout from drifting unboundedly.
inline void end_of_epoch(Potential& P, float w2_decay = 1e-3f) {
    for (size_t j = 0; j < P.H; ++j) P.normalize_row(j);
    for (size_t j = 0; j < P.H; ++j) P.w2[j] *= (1.0f - w2_decay);
}

// ---------------------------------------------------------------------------
// Thinking: descend Phi from a prompt-conditioned start to a text answer.
//
// Two decoders are provided:
//
//   1) `think()` — Hamiltonian descent in fp32 with the prompt region frozen,
//      then round each answer float to a byte. This is the classical form.
//      It tends to collapse to a "byte-mean" basin because gradient steps in
//      continuous space are decoupled from the discrete text manifold.
//
//   2) `think_bytes()` — Byte Energy Descent (BED). The state stays on the
//      discrete byte lattice ({0,1,...,255}/256)^D at all times. At each
//      answer position i we evaluate Phi at all 256 candidate byte values
//      and pick the argmin. Positions are updated in a randomized order,
//      not left-to-right, so this is *not* autoregression. There is no
//      vocabulary — the 256 candidates are just the raw bytes. There is no
//      continuous relaxation — every intermediate state decodes to real text.
//
// BED is essentially Iterated Conditional Modes for the DCHL potential's
// Markov random field over bytes, made tractable by caching pre[j] and
// ||c||^2 and doing rank-1 updates when a byte moves.
// ---------------------------------------------------------------------------
struct ThoughtResult {
    Vec c_final;
    std::string answer;
    float phi_initial = 0.0f;
    float phi_final = 0.0f;
    int steps = 0;
};

inline ThoughtResult think(const Potential& P, const std::string& prompt,
                           int steps = 100, float dt = 0.01f) {
    // Encode prompt into the prompt region; answer region starts at 0.5
    // (byte 128, mid-range) so gradient descent has room in both directions.
    Vec c(P.D, 0.5f);
    size_t nq = std::min(prompt.size(), P.D_prompt);
    for (size_t i = 0; i < nq; ++i) c[i] = (float)(uint8_t)prompt[i] / 256.0f;
    for (size_t i = nq; i < P.D_prompt; ++i) c[i] = 0.0f;

    ThoughtResult r;
    r.phi_initial = P.phi_centered(c);

    Vec p(P.D, 0.0f), g;
    for (int t = 0; t < steps; ++t) {
        P.gradient(c, g);
        // Freeze the prompt region: mask gradient AND momentum there.
        for (size_t i = 0; i < P.D_prompt; ++i) { g[i] = 0.0f; p[i] = 0.0f; }
        // Normalize force so the symplectic step is stable regardless of
        // uncapped Phi magnitude — the direction is the physics.
        double gn = 0.0;
        for (size_t i = P.D_prompt; i < P.D; ++i) gn += (double)g[i] * g[i];
        float inv = gn > 1e-20 ? (float)(1.0 / std::sqrt(gn)) : 0.0f;
        for (size_t i = P.D_prompt; i < P.D; ++i) {
            p[i] += -g[i] * inv * dt;
            c[i] += p[i] * dt;
            if (c[i] < 0.0f) { c[i] = 0.0f; p[i] = 0.0f; }
            if (c[i] > 1.0f) { c[i] = 1.0f; p[i] = 0.0f; }
        }
    }
    r.phi_final = P.phi_centered(c);
    r.steps = steps;
    r.answer = range_to_text(c.data() + P.D_prompt, P.D_answer);
    r.c_final = std::move(c);
    return r;
}

// ---------------------------------------------------------------------------
// P-CMA sliding-window Hamiltonian generation.
//
//   State c is a window of D bytes normalised to [0, 1]. Phi(c) has been
//   trained so real text windows are low-energy, random windows are high.
//
//   Generation loop:
//     encode prompt into a D-byte window c (pad with spaces at the tail)
//     initialise momentum p = 0
//     for each output character to produce:
//       for step in 1..steps_per_char:
//         Störmer-Verlet:
//           p += 0.5 * dt * (-∇Phi(c))
//           c += dt * p
//           p += 0.5 * dt * (-∇Phi(c))
//         clamp c into [0, 1]
//       readout: next_byte = round(c[D-1] * 255)
//       append next_byte to output
//       shift the window left by one: c[0..D-2] = c[1..D-1], c[D-1] = next_byte/255
//       shift momentum similarly, p[D-1] = 0
//
// This is *not* discrete argmin like BED. The entire window relaxes
// continuously toward the trained data manifold; the last position is the
// readout because sliding a window in time makes the *last* slot the
// "just-generated" one, while the earlier slots are the running context.
// ---------------------------------------------------------------------------
struct SlidingResult {
    std::string output;
    float phi_avg = 0.0f;      // mean Phi across generation
    int chars = 0;
    int steps_per_char = 0;
};

inline SlidingResult generate_sliding(const Potential& P,
                                      const std::string& prompt,
                                      int n_chars,
                                      int steps_per_char = 10,
                                      float dt = 0.01f,
                                      bool printable_only = true) {
    SlidingResult r;
    r.chars = n_chars;
    r.steps_per_char = steps_per_char;

    // Build the initial window. Pad with byte 32 (space) so unfilled
    // positions have a text-like value rather than 0.
    Vec c(P.D, 32.0f / 256.0f);
    size_t n_p = std::min(prompt.size(), P.D);
    // Right-align the prompt so the "just-emitted" slot is at c[D-1].
    size_t off = P.D - n_p;
    for (size_t i = 0; i < n_p; ++i)
        c[off + i] = (float)(uint8_t)prompt[i] / 256.0f;

    Vec p(P.D, 0.0f), g;
    double phi_sum = 0.0;
    r.output = prompt;

    for (int ch = 0; ch < n_chars; ++ch) {
        for (int s = 0; s < steps_per_char; ++s) {
            // Störmer-Verlet.
            P.gradient(c, g);
            for (size_t i = 0; i < P.D; ++i) p[i] -= 0.5f * dt * g[i];
            for (size_t i = 0; i < P.D; ++i) {
                c[i] += dt * p[i];
                if (c[i] < 0.0f) { c[i] = 0.0f; p[i] = 0.0f; }
                if (c[i] > 1.0f) { c[i] = 1.0f; p[i] = 0.0f; }
            }
            P.gradient(c, g);
            for (size_t i = 0; i < P.D; ++i) p[i] -= 0.5f * dt * g[i];
        }
        phi_sum += P.forward(c);

        // Readout at the tail.
        int b = (int)std::lround(c[P.D - 1] * 256.0f);
        if (b < 0) b = 0; if (b > 255) b = 255;
        // Constrain to printable ASCII if requested — the underlying value
        // is still c[D-1] / 256; we only alter what we emit and re-inject.
        if (printable_only) {
            if (b < 32) b = 32;
            if (b > 126) b = 126;
        }
        r.output.push_back((char)b);

        // Shift the window: drop c[0], everything moves left one slot,
        // c[D-1] becomes the newly-emitted byte (this is the "just wrote it"
        // slot for the next character generation).
        for (size_t i = 0; i + 1 < P.D; ++i) c[i] = c[i + 1];
        c[P.D - 1] = (float)b / 256.0f;
        for (size_t i = 0; i + 1 < P.D; ++i) p[i] = p[i + 1];
        p[P.D - 1] = 0.0f;
    }
    r.phi_avg = (float)(phi_sum / std::max(1, n_chars));
    return r;
}

// ---------------------------------------------------------------------------
// Byte Energy Descent (BED) — the novel state->text decoder.
//
// Contract: at every moment the state is on the byte lattice, so decoding
// is trivial (the state IS the text). We pick each answer byte to minimise
// the potential Phi given all other bytes fixed. Sweeps run in randomised
// order and repeat until Phi stops decreasing.
//
// The trick that makes this fast is that per position, Φ evaluations for
// all 256 candidate bytes share a cached forward pass:
//
//   pre[j]  = W1[j] . c + b1[j]                (H*D + H, once)
//   n2      = 1 + ||c||^2                       (D, once)
//
// Changing c[i] from old_v to v costs O(H):
//
//   pre_new[j]  = pre[j] + W1[j,i] * (v - old_v)
//   n2_new      = n2   + (v^2 - old_v^2)
//   Phi_new     = b2 + Σ_j w2[j] * tanh(pre_new[j] / sqrt(n2_new))
//
// So one position/all-256 candidates is 256 * H tanh operations. One full
// sweep over D_answer positions costs D_answer * 256 * H — no D factor.
// ---------------------------------------------------------------------------
inline ThoughtResult think_bytes(const Potential& P, const std::string& prompt,
                                 int sweeps = 6, float temperature = 0.0f,
                                 unsigned seed = 0xC01D,
                                 bool printable_only = true) {
    // Candidate byte range. Printable ASCII [32, 126] by default: this stops
    // BED from collapsing to the all-zero-byte state (a global Phi minimum
    // for under-trained potentials) that decodes to nothing.
    int b_lo = printable_only ? 32 : 0;
    int b_hi = printable_only ? 126 : 255;

    // Initial state: prompt bytes go into the prompt region, space (byte 32)
    // fills the answer region. All values in the zero-centered byte lattice.
    Vec c(P.D, byte_to_val(32));
    size_t nq = std::min(prompt.size(), P.D_prompt);
    for (size_t i = 0; i < nq; ++i) c[i] = byte_to_val((int)(uint8_t)prompt[i]);
    for (size_t i = nq; i < P.D_prompt; ++i) c[i] = byte_to_val(32);

    ThoughtResult r;

    // Cached forward-pass state.
    Vec pre(P.H, 0.0f);
    double n2 = 1.0;
    for (size_t i = 0; i < P.D; ++i) n2 += (double)c[i] * c[i];
    for (size_t j = 0; j < P.H; ++j) {
        const float* row = &P.W1[j * P.D];
        double s = P.b1[j];
        for (size_t i = 0; i < P.D; ++i) s += (double)row[i] * c[i];
        pre[j] = (float)s;
    }
    auto phi_from_cached = [&]() {
        float inv_n = 1.0f / (float)std::sqrt(n2);
        double phi = P.b2;
        for (size_t j = 0; j < P.H; ++j)
            phi += (double)P.w2[j] * std::tanh(pre[j] * inv_n);
        return (float)phi;
    };
    r.phi_initial = phi_from_cached() - P.baseline;

    // Position order — shuffled each sweep so we don't accidentally do
    // left-to-right / autoregressive.
    std::vector<size_t> order(P.D_answer);
    for (size_t i = 0; i < P.D_answer; ++i) order[i] = P.D_prompt + i;
    std::mt19937 rng(seed);

    float prev_phi = r.phi_initial;
    int done_sweeps = 0;
    for (int sw = 0; sw < sweeps; ++sw) {
        std::shuffle(order.begin(), order.end(), rng);
        for (size_t i : order) {
            float old_v = c[i];
            float best_phi = std::numeric_limits<float>::infinity();
            float best_v = old_v;
            float phi_cands[256];
            for (int b = b_lo; b <= b_hi; ++b) {
                float v = byte_to_val(b);
                float dv = v - old_v;
                double n2_new = n2 + (double)v * v - (double)old_v * old_v;
                float inv_n = 1.0f / (float)std::sqrt(n2_new);
                double phi = P.b2;
                for (size_t j = 0; j < P.H; ++j) {
                    float col = P.W1[j * P.D + i];
                    float p_new = pre[j] + col * dv;
                    phi += (double)P.w2[j] * std::tanh(p_new * inv_n);
                }
                float pf = (float)phi;
                phi_cands[b] = pf;
                if (pf < best_phi) { best_phi = pf; best_v = v; }
            }
            float chosen_v = best_v;
            if (temperature > 1e-6f) {
                double denom = 0.0;
                double weights[256];
                for (int b = b_lo; b <= b_hi; ++b) {
                    weights[b] = std::exp(-(double)(phi_cands[b] - best_phi) / (double)temperature);
                    denom += weights[b];
                }
                std::uniform_real_distribution<double> u(0.0, denom);
                double r_ = u(rng);
                double acc = 0.0;
                int chosen_b = b_lo;
                for (int b = b_lo; b <= b_hi; ++b) {
                    acc += weights[b];
                    if (acc >= r_) { chosen_b = b; break; }
                }
                chosen_v = (float)chosen_b / 256.0f;
            }
            // Apply the chosen byte via rank-1 update of the cache.
            if (chosen_v != old_v) {
                float dv = chosen_v - old_v;
                for (size_t j = 0; j < P.H; ++j)
                    pre[j] += P.W1[j * P.D + i] * dv;
                n2 += (double)chosen_v * chosen_v - (double)old_v * old_v;
                c[i] = chosen_v;
            }
        }
        float cur_phi = phi_from_cached() - P.baseline;
        ++done_sweeps;
        if (std::fabs(cur_phi - prev_phi) < 1e-3f && temperature < 1e-6f) {
            prev_phi = cur_phi;
            break;
        }
        prev_phi = cur_phi;
    }

    r.phi_final = prev_phi;
    r.steps = done_sweeps;
    r.answer = range_to_text(c.data() + P.D_prompt, P.D_answer);
    r.c_final = std::move(c);
    return r;
}

// ---------------------------------------------------------------------------
// Physics-based confidence / OOD: KDE density on the known manifold region.
// This is geometry, not a trained classifier head.
// ---------------------------------------------------------------------------
struct OODDetector {
    std::vector<Vec> known_states;
    float sigma = 0.04f;
    float threshold = 0.10f;

    // Only compare on shared non-zero dims (paper Problem 6): the union
    // over pad zeros inflates distance and makes everything look known.
    float density(const Vec& c) const {
        float best = 0.0f;
        for (const Vec& x : known_states) {
            double d2 = 0.0;
            size_t shared = 0;
            size_t n = std::min(c.size(), x.size());
            for (size_t i = 0; i < n; ++i) {
                if (c[i] != 0.0f && x[i] != 0.0f) {
                    double diff = (double)c[i] - x[i];
                    d2 += diff * diff;
                    ++shared;
                }
            }
            if (shared == 0) continue;
            float k = (float)std::exp(-(d2 / (double)shared) /
                                      (2.0 * (double)sigma * sigma));
            if (k > best) best = k;
        }
        return best;
    }

    bool is_ood(const Vec& c) const { return density(c) < threshold; }
};

// ---------------------------------------------------------------------------
// .neuron file: potential + OOD cache.
// ---------------------------------------------------------------------------
inline bool save_neuron(const std::string& path, const Potential& P,
                        const OODDetector& ood) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    const char magic[4] = {'H', 'C', 'M', 'B'}; // "B" = has prompt/answer split
    uint32_t d = (uint32_t)P.D, h = (uint32_t)P.H;
    uint32_t dp = (uint32_t)P.D_prompt, da = (uint32_t)P.D_answer;
    f.write(magic, 4);
    f.write((const char*)&d, 4);
    f.write((const char*)&h, 4);
    f.write((const char*)&dp, 4);
    f.write((const char*)&da, 4);
    f.write((const char*)P.W1.data(), (std::streamsize)(P.W1.size() * 4));
    f.write((const char*)P.b1.data(), (std::streamsize)(P.b1.size() * 4));
    f.write((const char*)P.w2.data(), (std::streamsize)(P.w2.size() * 4));
    f.write((const char*)&P.b2, 4);
    f.write((const char*)&P.baseline, 4);
    uint32_t nk = (uint32_t)ood.known_states.size();
    f.write((const char*)&nk, 4);
    f.write((const char*)&ood.sigma, 4);
    f.write((const char*)&ood.threshold, 4);
    for (const Vec& s : ood.known_states)
        f.write((const char*)s.data(), (std::streamsize)(s.size() * 4));
    return (bool)f;
}

inline bool load_neuron(const std::string& path, Potential& P, OODDetector& ood) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char magic[4];
    f.read(magic, 4);
    if (std::memcmp(magic, "HCM", 3) != 0) return false;
    uint32_t d = 0, h = 0, dp = 0, da = 0;
    f.read((char*)&d, 4);
    f.read((char*)&h, 4);
    if (magic[3] == 'B') {
        f.read((char*)&dp, 4);
        f.read((char*)&da, 4);
    } else {
        dp = 0; da = d; // old file with no split → treat all as answer
    }
    P.D = d; P.H = h; P.D_prompt = dp; P.D_answer = da;
    P.W1.resize((size_t)d * h);
    P.b1.resize(h);
    P.w2.resize(h);
    f.read((char*)P.W1.data(), (std::streamsize)(P.W1.size() * 4));
    f.read((char*)P.b1.data(), (std::streamsize)(P.b1.size() * 4));
    f.read((char*)P.w2.data(), (std::streamsize)(P.w2.size() * 4));
    f.read((char*)&P.b2, 4);
    f.read((char*)&P.baseline, 4);
    uint32_t nk = 0;
    f.read((char*)&nk, 4);
    f.read((char*)&ood.sigma, 4);
    f.read((char*)&ood.threshold, 4);
    ood.known_states.assign(nk, Vec(d, 0.0f));
    for (uint32_t i = 0; i < nk; ++i)
        f.read((char*)ood.known_states[i].data(), (std::streamsize)((size_t)d * 4));
    return (bool)f;
}

} // namespace hcma
