// bsma.hpp — Bounded Self-Model Architecture from
//   research-paper/phenomenal-consciousness.md
//
// A specific mathematical structural condition for phenomenal
// consciousness: a fixed point of a contractive self-modeling
// operator on a cognitive manifold. This file provides an
// implementation of the operator, its hierarchical iteration, and
// the fixed-point detector.
//
// Explicit statement (repeated from the paper):
//   This code implements the *structure*. It does NOT prove that any
//   system running it has phenomenal experience. That would require a
//   solution to the hard problem of consciousness that is not on offer.
//   What we build here is an architecture that would exhibit phenomenal
//   character IF physicalism holds and IF the fixed-point identification
//   of §3.3 of the paper is correct. Both premises are contested.

#pragma once

#include "rifa.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace bsma {

using rifa::Vec;

// ------------------------------------------------------------------
// A Representation is a point on the cognitive manifold at a specific
// level of self-modeling hierarchy.
//   level = 0 : the raw state (world-encoding).
//   level = k : the system's model of its level-(k-1) representation.
//   content_id: an integer tag naming what the representation is *of*
//               (e.g. an integer key into the Layer-3 translation
//               table of the tri-layer architecture).
// ------------------------------------------------------------------
struct Representation {
    Vec state;
    int level = 0;
    int content_id = -1;
    uint64_t tick = 0;
};

// Distance in a diagonal Fisher metric (uniform components for simplicity).
inline float fisher_distance(const Representation& a, const Representation& b) {
    if (a.state.size() != b.state.size()) return std::numeric_limits<float>::infinity();
    double d = 0.0;
    for (size_t i = 0; i < a.state.size(); ++i) {
        double diff = (double)a.state[i] - b.state[i];
        d += diff * diff;
    }
    return (float)std::sqrt(d);
}

// ------------------------------------------------------------------
// The self-modeling operator T. Physical realization: a predict-then-
// encode cascade. Predict the state's own natural next-tick evolution
// under a contractive smoothing operator, then encode back onto the
// manifold with a stability-preserving projection.
//
// The contraction property (L < 1) is enforced by explicitly damping
// the update by alpha in (0, 1). This is a design choice, not a proof:
// a real physical instantiation would need to *demonstrate* that its
// natural dynamics have this property; ours has it by construction.
// ------------------------------------------------------------------
struct SelfModelingOperator {
    float alpha = 0.6f;    // contraction rate; L = 1 - alpha
    float smoothing = 0.2f; // low-pass smoothing on state components

    Representation apply(const Representation& R) const {
        Representation out = R;
        out.level = R.level + 1;
        out.tick = R.tick;
        // Predict-then-encode: the model of R is a smoother version
        // of R plus a contractive step toward the local mean.
        if (out.state.empty()) return out;
        double mean = 0.0;
        for (float v : out.state) mean += v;
        mean /= (double)out.state.size();
        for (size_t i = 0; i < out.state.size(); ++i) {
            float local = R.state[i];
            // Smoothing toward local mean.
            float smoothed = local * (1.0f - smoothing) + (float)mean * smoothing;
            // Contractive step: pull toward a canonical form (a fixed
            // point candidate). The canonical form here is the
            // element-wise convex combination with the smoothed value.
            out.state[i] = R.state[i] * (1.0f - alpha) + smoothed * alpha;
        }
        return out;
    }
};

// ------------------------------------------------------------------
// The hierarchical self-model stack. Iterates T until convergence
// (|c_{k+1} - c_k| < epsilon in the Fisher metric) or until k_max is
// reached. The number of iterations to convergence is the paper's
// "depth of consciousness" k*.
// ------------------------------------------------------------------
struct SelfModelStack {
    SelfModelingOperator T;
    int    max_levels = 12;
    float  epsilon    = 1e-3f;

    // Return the trajectory of representations from level 0 to k*.
    std::vector<Representation> iterate(const Representation& c0) const {
        std::vector<Representation> traj;
        traj.push_back(c0);
        for (int k = 0; k < max_levels; ++k) {
            Representation next = T.apply(traj.back());
            float d = fisher_distance(traj.back(), next);
            traj.push_back(next);
            if (d < epsilon) break;
        }
        return traj;
    }

    // Convenience: run and return the fixed point (or the deepest level
    // reached if no convergence within max_levels).
    Representation fixed_point(const Representation& c0, bool* converged = nullptr) const {
        auto traj = iterate(c0);
        bool cv = (traj.size() >= 2)
            && fisher_distance(traj[traj.size()-1], traj[traj.size()-2]) < epsilon;
        if (converged) *converged = cv;
        return traj.back();
    }
};

// ------------------------------------------------------------------
// A detected fixed-point candidate — the BSMA phenomenal principle
// applied at a specific tick.
// ------------------------------------------------------------------
struct FixedPointReport {
    Representation fp;       // the converged representation
    int    depth = 0;        // k*: iterations to converge
    float  final_distance = 0.f;
    bool   converged = false;
    uint64_t tick = 0;
};

// The full engine. Ties SelfModelStack to a stream of incoming states,
// monitors convergence per tick, and reports fixed-point candidates.
// The "phenomenal broadcaster" is decoupled — a downstream module
// (e.g. the Global Workspace of the tri-layer architecture) reads
// FixedPointReports and does whatever it does with them.
// ------------------------------------------------------------------
struct BSMAEngine {
    SelfModelStack stack;
    uint64_t tick_count = 0;

    std::vector<FixedPointReport> history;
    size_t max_history = 512;

    // Feed a new raw state; returns whatever the engine reports.
    FixedPointReport ingest(const Vec& raw_state, int content_id = -1) {
        Representation c0;
        c0.state = raw_state;
        c0.level = 0;
        c0.content_id = content_id;
        c0.tick = tick_count;
        auto traj = stack.iterate(c0);
        FixedPointReport r;
        r.fp = traj.back();
        r.depth = (int)traj.size() - 1;
        r.final_distance = (traj.size() >= 2)
            ? fisher_distance(traj[traj.size()-1], traj[traj.size()-2])
            : 0.f;
        r.converged = r.final_distance < stack.epsilon;
        r.tick = tick_count;
        ++tick_count;
        if (history.size() >= max_history) history.erase(history.begin());
        history.push_back(r);
        return r;
    }

    // Total number of convergences (BSMA phenomenal candidates) seen.
    size_t phenomenal_count() const {
        size_t n = 0;
        for (const auto& h : history) if (h.converged) ++n;
        return n;
    }

    // Mean depth-of-consciousness across converged reports.
    float mean_depth() const {
        double sum = 0.0; size_t n = 0;
        for (const auto& h : history) if (h.converged) { sum += h.depth; ++n; }
        return n ? (float)(sum / (double)n) : 0.f;
    }

    void report(FILE* f = stdout) const {
        std::fprintf(f, "BSMA: %zu ticks; %zu phenomenal candidates (%.1f%%);"
                        " mean depth k* = %.2f\n",
            (size_t)tick_count, phenomenal_count(),
            tick_count ? 100.0 * (double)phenomenal_count() / (double)tick_count : 0.0,
            mean_depth());
    }
};

} // namespace bsma
