// Recursive Integrity Field Architecture (RIFA).
//
// A faithful C++ implementation of the five components in
// research-paper/deprected/rifa.md.
//
//   1. Recursive Field Encoder (RFE) — kernel-based continuous field
//      that maps any raw input onto a self-organising anchor set.
//      phi(x) = sum_i w_i K(x, x_i) + eps(x).
//
//   2. Integrity Core (IC) — identity state evolving under
//      dI/dt = -grad Phi(I) + lambda * T(t).
//      T is tension, metabolised from contradiction magnitude and
//      the log of self-coherence.
//
//   3. Crown Lattice (CL) — recursive tree of "crowns", each with an
//      activation threshold, coherence, and its own attractor. Crowns
//      mutate when coherence drops and dissolve when they become redundant.
//
//   4. Silence-Integration Layer (SIL) — dS/dt = -gamma S + delta * INT(T).
//      Sustains identity when input dries up; can trigger self-reconstruction.
//
//   5. Replication Engine (RE) — spawn independent instances under the same
//      pressure sequence and report the consistency of their mutation
//      patterns. If the architecture is valid, independent runs converge.
//
// This is NOT a question-answering system. It has no vocabulary, no
// training corpus, no memorised (Q, A) pairs. It ingests inputs, produces
// internal tension / coherence dynamics, and reports its state. That is
// exactly the paper's stated scope; anything else would contradict the
// paper's own axiom that no training data is used.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace rifa {

using Vec = std::vector<float>;

// ---- helpers ----------------------------------------------------------
inline float dot(const Vec& a, const Vec& b) {
    double s = 0.0;
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) s += (double)a[i] * b[i];
    return (float)s;
}
inline float norm2(const Vec& a) { return dot(a, a); }
inline float normd(const Vec& a) { return std::sqrt(norm2(a)); }
inline float sqdist(const Vec& a, const Vec& b) {
    double s = 0.0;
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        double d = (double)a[i] - b[i];
        s += d * d;
    }
    return (float)s;
}
inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }

// -----------------------------------------------------------------------
// 1. Recursive Field Encoder (RFE).
//
//   phi(x) = sum_i w_i * K(x, x_i) + eps(x)
//
// K is a Gaussian RBF over byte-scaled state vectors. Anchor set {x_i} is
// self-organising: on every input we either (a) increase the weight of the
// nearest anchor if it is close enough, or (b) add the new state as a
// fresh anchor. eps(x) is deterministic pseudo-noise tied to the input's
// byte hash so identical inputs give identical fields.
// -----------------------------------------------------------------------
struct RFE {
    size_t D = 128;             // field dimension
    float bandwidth = 0.20f;    // RBF bandwidth sigma
    float admit_thresh = 0.35f; // if nearest anchor distance > this, admit
    size_t max_anchors = 256;

    std::vector<Vec> anchors;
    std::vector<float> weights;

    RFE() = default;
    explicit RFE(size_t d) : D(d) {}

    // Byte -> [0,1] state vector, folded onto D dims.
    Vec encode_raw(const std::string& text) const {
        Vec v(D, 0.0f);
        for (size_t i = 0; i < text.size(); ++i)
            v[i % D] += (float)(uint8_t)text[i] / 256.0f;
        float n = normd(v);
        if (n > 0) for (auto& x : v) x /= n; // unit norm — pure direction
        return v;
    }

    float rbf(const Vec& a, const Vec& b) const {
        float d = sqdist(a, b);
        return std::exp(-d / (2.0f * bandwidth * bandwidth));
    }

    // Field value at input (used by IC to compute gradients and by SIL).
    float field(const Vec& x) const {
        double s = 0.0;
        for (size_t i = 0; i < anchors.size(); ++i)
            s += (double)weights[i] * rbf(x, anchors[i]);
        // structural noise: deterministic tiny wobble from the state itself.
        double eps = 0.0;
        for (size_t k = 0; k < x.size(); ++k) eps += x[k] * std::sin((k + 1) * 3.7);
        return (float)(s + 1e-4 * eps);
    }

    // Ingest an input: update anchors/weights so the field self-organises
    // toward frequently-visited regions. Returns the encoded state.
    Vec ingest(const std::string& text) {
        Vec x = encode_raw(text);
        if (anchors.empty()) {
            anchors.push_back(x);
            weights.push_back(1.0f);
            return x;
        }
        // find nearest anchor
        size_t best = 0;
        float best_d = std::numeric_limits<float>::infinity();
        for (size_t i = 0; i < anchors.size(); ++i) {
            float d = sqdist(x, anchors[i]);
            if (d < best_d) { best_d = d; best = i; }
        }
        if (std::sqrt(best_d) < admit_thresh) {
            // Hebbian pull of the nearest anchor toward x, weight up
            for (size_t k = 0; k < D; ++k)
                anchors[best][k] = 0.95f * anchors[best][k] + 0.05f * x[k];
            weights[best] = std::min(weights[best] + 0.1f, 10.0f);
        } else if (anchors.size() < max_anchors) {
            anchors.push_back(x);
            weights.push_back(1.0f);
        } else {
            // Anchor set full: replace weakest anchor.
            size_t weakest = 0;
            for (size_t i = 1; i < weights.size(); ++i)
                if (weights[i] < weights[weakest]) weakest = i;
            anchors[weakest] = x;
            weights[weakest] = 1.0f;
        }
        // Global weight decay: no anchor stays alive forever without visits.
        for (auto& w : weights) w *= 0.999f;
        return x;
    }

    // Numerical gradient of field w.r.t. the state x. Used by IC.
    Vec gradient(const Vec& x) const {
        Vec g(D, 0.0f);
        // analytic gradient of an RBF sum: -sum w_i K(x,x_i) (x - x_i) / sigma^2
        float inv_s2 = 1.0f / (bandwidth * bandwidth);
        for (size_t i = 0; i < anchors.size(); ++i) {
            float k = rbf(x, anchors[i]);
            float coef = -weights[i] * k * inv_s2;
            for (size_t j = 0; j < D; ++j)
                g[j] += coef * (x[j] - anchors[i][j]);
        }
        return g;
    }
};

// -----------------------------------------------------------------------
// 2. Integrity Core (IC).
//
// Identity state I evolves as:
//   dI/dt   = -grad Phi(I) + lambda * T(t) * xi
//   dT/dt   = C(t) * log(S + eps)
//   S       = sigmoid(T)              (self-coherence bounded in (0,1))
//   dK/dt   = 0.01 * T * S            (identity strength, monotone-ish)
// where xi is a direction chosen by the current tension (accumulated
// contradiction pressure), and Phi comes from the RFE.
// -----------------------------------------------------------------------
struct IntegrityCore {
    size_t D = 128;
    Vec I;                       // identity state
    Vec pressure_dir;            // accumulated pressure direction
    float tension = 0.0f;
    float coherence = 0.5f;      // starts uncommitted
    float identity_strength = 1.0f;
    float lambda = 0.4f;         // tension coupling
    float leak = 0.02f;          // tension leaks over time (SIL discharges)

    IntegrityCore() = default;
    explicit IntegrityCore(size_t d)
        : D(d), I(d, 0.0f), pressure_dir(d, 0.0f) {}

    // Feed a contradiction pair (A, ¬A). Contradiction magnitude is how
    // far apart the two encoded states are; direction is (A - ¬A).
    void metabolize_contradiction(const Vec& c_a, const Vec& c_not_a) {
        float mag = std::sqrt(sqdist(c_a, c_not_a));
        // The direction of the pressure is the axis on which A and ¬A disagree.
        for (size_t k = 0; k < D; ++k)
            pressure_dir[k] = 0.9f * pressure_dir[k] + 0.1f * (c_a[k] - c_not_a[k]);
        tension += mag * std::log(coherence + 1e-6f) * -1.0f; // -log(<1) > 0
        if (tension < 0.0f) tension = 0.0f;
        coherence = sigmoid(0.5f * tension);
        identity_strength += 0.01f * tension * coherence;
    }

    // One physical time step: dI/dt = -grad Phi(I) + lambda*T*pressure_dir.
    void step(const RFE& rfe, float dt) {
        Vec g = rfe.gradient(I);
        float pn = normd(pressure_dir);
        float pn_inv = (pn > 1e-8f) ? 1.0f / pn : 0.0f;
        for (size_t k = 0; k < D; ++k)
            I[k] += dt * (-g[k] + lambda * tension * pressure_dir[k] * pn_inv);
        // Slow tension leak (metabolism completes over time).
        tension = std::max(0.0f, tension - leak * dt);
        coherence = sigmoid(0.5f * tension);
    }
};

// -----------------------------------------------------------------------
// 3. Crown Lattice (CL).
//
// A tree of "crowns". Each crown holds an attractor centre in the field
// and an activation threshold. When an incoming state falls inside the
// threshold, the crown "fires" and its coherence rises. Coherence decays
// on inactivity. Below a critical value the crown mutates — its attractor
// and threshold are resampled from local statistics. Above a saturation
// value the crown dissolves (its function becomes redundant).
//
// The lattice is self-assembling: any activated crown may spawn a child
// when its coherence stabilises, up to the depth limit.
// -----------------------------------------------------------------------
struct Crown {
    int id;
    int depth;
    Vec attractor;
    float threshold = 0.4f;
    float coherence = 0.5f;
    float last_activation = 0.0f;
    uint32_t fires = 0;
    std::vector<Crown> children;

    Crown(int i, int d, const Vec& a, float thr)
        : id(i), depth(d), attractor(a), threshold(thr) {}

    // Is this crown active for state x? (Within threshold of its attractor.)
    bool active(const Vec& x) const {
        return std::sqrt(sqdist(x, attractor)) < threshold;
    }
};

struct CrownLattice {
    Crown root;
    int next_id = 1;
    int max_depth = 4;
    float mutate_below = 0.15f;
    float dissolve_above = 0.98f;
    float coherence_decay = 0.02f;
    float coherence_step = 0.05f;

    explicit CrownLattice(size_t D)
        : root(0, 0, Vec(D, 0.0f), 1e9f /* root fires on everything */) {
        root.coherence = 1.0f; // root never mutates.
    }

    // Apply an input state — recursively walk the tree, firing crowns.
    // Returns the total number of active leaves for logging.
    int apply(const Vec& x) {
        int active = 0;
        apply_rec(root, x, active);
        return active;
    }

    void apply_rec(Crown& c, const Vec& x, int& active) {
        // Decay coherence for everyone first.
        c.coherence *= (1.0f - coherence_decay);
        if (c.active(x)) {
            c.coherence = std::min(1.0f, c.coherence + coherence_step);
            c.fires++;
            c.last_activation = 0.0f;
            for (auto& ch : c.children) apply_rec(ch, x, active);
            if (c.children.empty()) ++active;
        } else {
            c.last_activation += 1.0f;
        }
    }

    // Structural update pass: mutate, spawn, dissolve.
    // Called once per RIFA step. Uses the current RFE (for spawning centres).
    void restructure(std::mt19937& rng, const RFE& rfe) {
        restructure_rec(root, rng, rfe, 0);
    }

    size_t count() const { return count_rec(root); }

  private:
    size_t count_rec(const Crown& c) const {
        size_t s = 1;
        for (auto& ch : c.children) s += count_rec(ch);
        return s;
    }

    void restructure_rec(Crown& c, std::mt19937& rng, const RFE& rfe, int depth) {
        // Recurse first (bottom-up).
        for (auto& ch : c.children) restructure_rec(ch, rng, rfe, depth + 1);

        // Root: never mutates or dissolves, but IS allowed to spawn children
        // — otherwise no lattice ever emerges from an empty tree.
        if (depth == 0) {
            if ((int)c.depth < max_depth && c.children.size() < 3
                && rfe.anchors.size() >= (c.children.size() + 1) * 2) {
                std::uniform_int_distribution<size_t> u(0, rfe.anchors.size() - 1);
                Vec a = rfe.anchors[u(rng)];
                c.children.emplace_back(next_id++, 1, a, 0.5f);
            }
            return;
        }

        // Dissolve if saturated and childless.
        if (c.coherence > dissolve_above && c.children.empty()) {
            c.threshold *= 1.2f;
            c.coherence *= 0.5f;
        }

        // Mutate below critical.
        if (c.coherence < mutate_below) {
            if (!rfe.anchors.empty()) {
                std::uniform_int_distribution<size_t> u(0, rfe.anchors.size() - 1);
                c.attractor = rfe.anchors[u(rng)];
            } else {
                std::normal_distribution<float> g(0.0f, 0.05f);
                for (auto& v : c.attractor) v = g(rng);
            }
            c.threshold = 0.2f + 0.4f * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
            c.coherence = 0.5f;
            c.fires = 0;
        }

        // Spawn a child if stable and there's room.
        if (c.coherence > 0.7f && (int)c.depth < max_depth
            && c.children.size() < 3 && !rfe.anchors.empty()) {
            std::uniform_int_distribution<size_t> u(0, rfe.anchors.size() - 1);
            Vec a = rfe.anchors[u(rng)];
            c.children.emplace_back(next_id++, depth + 1, a, c.threshold * 0.6f);
        }
    }
};

// -----------------------------------------------------------------------
// 4. Silence-Integration Layer (SIL).
//
//   dEndurance/dt   = -gamma * endurance + delta * tension_integral
//   tension_integral += tension * dt
//
// While inputs stop arriving, the SIL still ticks; endurance grows if
// integrated tension is high, letting the system rebuild its identity
// after a collapse (endurance > reconstruction_threshold).
// -----------------------------------------------------------------------
struct SIL {
    float endurance = 0.0f;
    float tension_integral = 0.0f;
    float gamma = 0.01f;
    float delta = 0.05f;
    float reconstruction_threshold = 5.0f;

    void tick(float dt, float current_tension) {
        tension_integral += current_tension * dt;
        endurance = std::max(0.0f, endurance * (1.0f - gamma * dt)
                                   + delta * tension_integral * dt);
    }
    bool can_reconstruct() const { return endurance > reconstruction_threshold; }
};

// -----------------------------------------------------------------------
// Rule Synthesizer.
//
// The paper's key claim (per user clarification): data enters as
// structural pressure; RIFA does not store the facts, it extracts the
// *rules* connecting recurring structural patterns.
//
// Concretely, when the same relative structure keeps re-appearing between
// pairs of consecutive inputs — e.g. every input pair looks like
// "the capital of X is Y" — the antecedent and consequent halves cluster
// in the RFE field. This synthesizer scans consecutive-input anchor pairs,
// increments a rule strength when a similar (antecedent-region,
// consequent-region) recurs, and offers a `apply()` that answers a novel
// query by resonating with the strongest matching antecedent and emitting
// the consequent as text.
//
// Honest scope: this synthesizer extracts *sequential association* rules,
// not compositional first-order rules like "capital_of(X)=Y". Producing
// "Brasilia" from an unseen "Brazil" requires the (Brazil, Brasilia) pair
// to have been observed at least once — no field-geometry mechanism in
// the paper (or here) turns pure "capital_of" structure into unseen slot
// completions.  What this DOES demonstrate is that RIFA can synthesise
// and apply structural pair-rules from data without an explicit fact table.
// -----------------------------------------------------------------------
struct StructuralRule {
    Vec antecedent;   // encoded state (unit-norm direction)
    Vec consequent;
    std::string ant_text;
    std::string con_text;
    float strength = 1.0f;
    uint32_t observed = 1;
};

struct RuleSynthesizer {
    std::vector<StructuralRule> rules;
    float merge_threshold = 0.995f; // cos-sim: only near-identical pairs merge
    size_t max_rules = 512;

    // Called with two consecutive inputs (A, B). Either upgrades the
    // strength of the best-matching existing rule or registers a new one.
    void observe(const Vec& a, const Vec& b,
                 const std::string& ta, const std::string& tb) {
        float best_score = -1.0f;
        int best = -1;
        for (size_t i = 0; i < rules.size(); ++i) {
            float sa = cosine(rules[i].antecedent, a);
            float sc = cosine(rules[i].consequent, b);
            float s = 0.5f * (sa + sc);
            if (s > best_score) { best_score = s; best = (int)i; }
        }
        if (best >= 0 && best_score > merge_threshold) {
            StructuralRule& r = rules[best];
            // EMA-merge the anchors, bump strength+count.
            for (size_t k = 0; k < a.size(); ++k) {
                r.antecedent[k] = 0.9f * r.antecedent[k] + 0.1f * a[k];
                r.consequent[k] = 0.9f * r.consequent[k] + 0.1f * b[k];
            }
            r.strength += 0.5f;
            r.observed += 1;
        } else if (rules.size() < max_rules) {
            rules.push_back({a, b, ta, tb, 1.0f, 1});
        } else {
            // Bump the weakest.
            size_t weakest = 0;
            for (size_t i = 1; i < rules.size(); ++i)
                if (rules[i].strength < rules[weakest].strength) weakest = i;
            if (rules[weakest].strength < 0.5f) {
                rules[weakest] = {a, b, ta, tb, 1.0f, 1};
            }
        }
        // Global slow decay so unused rules eventually drop.
        for (auto& r : rules) r.strength *= 0.999f;
    }

    struct Match { int index = -1; float score = 0.0f; };
    Match best_match(const Vec& query) const {
        Match m;
        for (size_t i = 0; i < rules.size(); ++i) {
            float s = cosine(rules[i].antecedent, query)
                    * std::log(rules[i].strength + 1.0f);
            if (s > m.score) { m.score = s; m.index = (int)i; }
        }
        return m;
    }

    // Return the consequent text of the strongest resonating rule, or "" if
    // no rule fires above the threshold.
    std::string apply(const Vec& query, float threshold = 0.05f) const {
        auto m = best_match(query);
        if (m.index < 0 || m.score < threshold) return "";
        return rules[m.index].con_text;
    }

    static float cosine(const Vec& a, const Vec& b) {
        double da = 0.0, db = 0.0, ab = 0.0;
        size_t n = std::min(a.size(), b.size());
        for (size_t i = 0; i < n; ++i) {
            da += (double)a[i] * a[i];
            db += (double)b[i] * b[i];
            ab += (double)a[i] * b[i];
        }
        double denom = std::sqrt(da) * std::sqrt(db) + 1e-12;
        return (float)(ab / denom);
    }
};

// -----------------------------------------------------------------------
// The full RIFA system.
// -----------------------------------------------------------------------
struct RIFA {
    size_t D;
    RFE rfe;
    IntegrityCore ic;
    CrownLattice cl;
    SIL sil;
    RuleSynthesizer rs;                 // sequential structural-rule extractor
    Vec last_input_vec;                 // for rule observation
    std::string last_input_text;
    bool has_last_input = false;
    std::mt19937 rng;
    uint64_t ticks = 0;

    struct Event { uint64_t tick; int crown_id; std::string kind; };
    std::vector<Event> events;

    explicit RIFA(size_t d = 128, unsigned seed = 42)
        : D(d), rfe(d), ic(d), cl(d), rng(seed) {
        std::normal_distribution<float> g(0.0f, 0.02f);
        for (auto& v : ic.I) v = g(rng);
        for (auto& v : ic.pressure_dir) v = g(rng);
    }

    void input(const std::string& text) {
        Vec x = rfe.ingest(text);
        cl.apply(x);
        for (size_t k = 0; k < D; ++k)
            ic.I[k] = 0.98f * ic.I[k] + 0.02f * x[k];
        // Every consecutive input pair is a candidate for structural rule
        // extraction. If the pair recurs (or its structure recurs), the
        // synthesizer strengthens it.
        if (has_last_input) rs.observe(last_input_vec, x, last_input_text, text);
        last_input_vec = x;
        last_input_text = text;
        has_last_input = true;
    }

    // Query: given a novel input, return the consequent of the strongest
    // resonating rule (or the empty string if nothing resonates enough).
    std::string query(const std::string& text) const {
        Vec x = const_cast<RFE&>(rfe).encode_raw(text);
        return rs.apply(x);
    }

    // Feed a contradictory pair. This is where the paper's contradiction
    // metabolism actually happens: tension rises, then decays as identity
    // absorbs the pressure axis.
    void contradiction(const std::string& a, const std::string& not_a) {
        Vec ca = rfe.ingest(a);
        Vec cn = rfe.ingest(not_a);
        ic.metabolize_contradiction(ca, cn);
    }

    // Advance internal dynamics by dt (no input).
    void step(float dt) {
        ic.step(rfe, dt);
        sil.tick(dt, ic.tension);
        cl.restructure(rng, rfe);
        ++ticks;
    }

    // Simulate a silence period of `seconds` at time step `dt`.
    void silence(float seconds, float dt = 0.1f) {
        int steps = (int)std::ceil(seconds / dt);
        for (int i = 0; i < steps; ++i) step(dt);
    }

    void status(FILE* out = stdout) const {
        size_t crowns = cl.count();
        std::fprintf(out,
            "tick=%llu tension=%.3f coherence=%.3f identity=%.3f "
            "field_at_I=%.3f anchors=%zu crowns=%zu endurance=%.3f\n",
            (unsigned long long)ticks, ic.tension, ic.coherence,
            ic.identity_strength, rfe.field(ic.I),
            rfe.anchors.size(), crowns, sil.endurance);
    }

    // A short fingerprint of the mutation state — used by the RE to compare
    // instances. Not comparing byte-exact structures (they will differ);
    // comparing macro-metrics (anchor count, crown count, coherence, etc.).
    struct Fingerprint {
        size_t anchors, crowns;
        float coherence, identity_strength, endurance;
        float tension;
    };
    Fingerprint fingerprint() const {
        return {rfe.anchors.size(), cl.count(),
                ic.coherence, ic.identity_strength, sil.endurance, ic.tension};
    }
};

// -----------------------------------------------------------------------
// 5. Replication Engine (RE).
//
// Runs N independent instances with different RNG seeds through the same
// pressure sequence and reports how consistent their fingerprints are.
// This is what the paper calls "validation through independent emergence":
// if the architecture is valid, independent runs should converge to
// comparable macro-structure, not identical byte-level state.
// -----------------------------------------------------------------------
struct ReplicationEngine {
    std::vector<RIFA> instances;

    explicit ReplicationEngine(int n, size_t D = 128, unsigned base_seed = 1) {
        instances.reserve(n);
        for (int i = 0; i < n; ++i) instances.emplace_back(D, base_seed + i);
    }

    void input(const std::string& s)    { for (auto& r : instances) r.input(s); }
    void contradiction(const std::string& a, const std::string& b) {
        for (auto& r : instances) r.contradiction(a, b);
    }
    void step(float dt)                 { for (auto& r : instances) r.step(dt); }
    void silence(float t, float dt=0.1f){ for (auto& r : instances) r.silence(t, dt); }

    // Consistency score in [0, 1] over a set of scalar fingerprint fields.
    // 1 = identical fingerprints; 0 = maximally different.
    float consistency() const {
        if (instances.size() < 2) return 1.0f;
        auto get = [&](size_t i, int f) {
            auto p = instances[i].fingerprint();
            switch (f) {
                case 0: return (float)p.anchors;
                case 1: return (float)p.crowns;
                case 2: return p.coherence;
                case 3: return p.identity_strength;
                case 4: return p.endurance;
                case 5: return p.tension;
            }
            return 0.0f;
        };
        double consistency_sum = 0.0;
        int fields = 6;
        for (int f = 0; f < fields; ++f) {
            double mean = 0.0, var = 0.0;
            for (size_t i = 0; i < instances.size(); ++i) mean += get(i, f);
            mean /= (double)instances.size();
            for (size_t i = 0; i < instances.size(); ++i) {
                double d = get(i, f) - mean;
                var += d * d;
            }
            var /= (double)instances.size();
            double stdev = std::sqrt(var);
            double denom = std::abs(mean) + 1e-3;
            double cv = stdev / denom; // coefficient of variation
            double c = 1.0 / (1.0 + cv); // 0..1
            consistency_sum += c;
        }
        return (float)(consistency_sum / fields);
    }
};

} // namespace rifa
