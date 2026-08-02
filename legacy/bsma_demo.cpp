// bsma_demo — running the BSMA fixed-point detector on three canonical
// scenarios:
//
//   A. Stable perception — a constant input signal. Prediction: BSMA
//      converges quickly; every tick is a phenomenal candidate.
//   B. Noise storm — a rapidly changing random input. Prediction:
//      BSMA does NOT converge within k_max; no phenomenal candidates.
//      This is BSMA's account of *unconscious* moments — a system
//      whose self-model cannot catch up with itself has no fixed point.
//   C. Attention shift — a step change from one stable input to another.
//      Prediction: BSMA converges, then briefly loses convergence during
//      the transition, then re-converges. This is the phenomenological
//      "flicker" of attention changing.
//
// None of the observations constitute proof of phenomenal consciousness;
// they exhibit the *structural* signatures that BSMA claims should
// accompany it. See the paper's §5 and §8 for the honest scope.

#include "bsma.hpp"

#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

using namespace bsma;
using rifa::Vec;

// ---------- A. Stable perception ----------
static void scenario_stable() {
    std::printf("---- Scenario A: stable perception (constant input) ----\n");
    BSMAEngine eng;
    eng.stack.max_levels = 12;
    eng.stack.epsilon    = 1e-3f;
    Vec stable(64, 0.5f);
    for (int t = 0; t < 20; ++t) {
        auto r = eng.ingest(stable, /*content_id=*/1);
        std::printf("  tick %2d  depth k*=%2d  |dc| = %.5f  %s\n",
            t, r.depth, r.final_distance,
            r.converged ? "converged (phenomenal candidate)" : "not converged");
    }
    std::printf("  summary: ");
    eng.report();
    std::printf("\n");
}

// ---------- B. Noise storm ----------
static void scenario_noise() {
    std::printf("---- Scenario B: noise storm (chaotic input) ----\n");
    BSMAEngine eng;
    eng.stack.max_levels = 12;
    eng.stack.epsilon    = 1e-3f;
    std::mt19937 rng(0xB0A70);
    std::normal_distribution<float> g(0.5f, 0.5f);
    for (int t = 0; t < 20; ++t) {
        Vec noise(64);
        for (auto& v : noise) v = g(rng);
        auto r = eng.ingest(noise, /*content_id=*/-1);
        std::printf("  tick %2d  depth k*=%2d  |dc| = %.5f  %s\n",
            t, r.depth, r.final_distance,
            r.converged ? "converged" : "not converged (unconscious moment)");
    }
    std::printf("  summary: ");
    eng.report();
    std::printf("\n");
}

// ---------- C. Attention shift ----------
static void scenario_shift() {
    std::printf("---- Scenario C: attention shift (step change) ----\n");
    BSMAEngine eng;
    eng.stack.max_levels = 12;
    eng.stack.epsilon    = 1e-3f;
    Vec first(64, 0.3f);
    Vec second(64, 0.7f);
    for (int t = 0; t < 20; ++t) {
        const Vec& in = (t < 8) ? first : (t == 8 ? Vec(64, 0.5f) : second);
        int content = (t < 8) ? 1 : (t == 8 ? -1 : 2);
        auto r = eng.ingest(in, content);
        std::printf("  tick %2d  focus=%s  depth k*=%2d  |dc| = %.5f  %s\n",
            t,
            (t < 8 ? "A" : (t == 8 ? "transition" : "B")),
            r.depth, r.final_distance,
            r.converged ? "converged" : "not converged");
    }
    std::printf("  summary: ");
    eng.report();
    std::printf("\n");
}

// ---------- Structural observation: fixed-point content differs ----------
// Reproduces §3.6 of the paper: different modeling operators produce
// different fixed points from the same input. This is BSMA's structural
// counterpart to the philosophical-zombie intuition.
static void scenario_operator_variance() {
    std::printf("---- Structural: same input, two operators, different fixed points ----\n");
    BSMAEngine a, b;
    a.stack.T.alpha = 0.4f;
    b.stack.T.alpha = 0.9f;   // stronger contraction
    Vec in(64);
    for (size_t i = 0; i < in.size(); ++i)
        in[i] = 0.5f + 0.25f * std::sin((float)i * 0.3f);

    auto ra = a.ingest(in);
    auto rb = b.ingest(in);
    std::printf("  operator A (alpha=%.2f): depth=%d, converged=%s\n",
        a.stack.T.alpha, ra.depth, ra.converged ? "yes" : "no");
    std::printf("  operator B (alpha=%.2f): depth=%d, converged=%s\n",
        b.stack.T.alpha, rb.depth, rb.converged ? "yes" : "no");
    // Both may converge — but to DIFFERENT points.
    if (ra.converged && rb.converged) {
        float d = fisher_distance(ra.fp, rb.fp);
        std::printf("  Fisher distance between the two fixed points: %.4f\n", d);
        std::printf("  --> same input, different operators, different fixed points.\n"
                    "      BSMA's structural account of the zombie thought experiment:\n"
                    "      the phenomenal content depends on the operator, not only on\n"
                    "      the input. Two zombies could differ in operator without\n"
                    "      differing in input.\n");
    }
    std::printf("\n");
}

int main(int argc, char** argv) {
    std::string cmd = (argc >= 2) ? argv[1] : "all";
    std::printf("=== BSMA: Bounded Self-Model Architecture demo ===\n");
    std::printf("(structural convergence tests — NOT a claim of consciousness)\n\n");
    if (cmd == "stable")   { scenario_stable(); return 0; }
    if (cmd == "noise")    { scenario_noise();  return 0; }
    if (cmd == "shift")    { scenario_shift();  return 0; }
    if (cmd == "operator") { scenario_operator_variance(); return 0; }
    if (cmd == "all") {
        scenario_stable();
        scenario_noise();
        scenario_shift();
        scenario_operator_variance();
        return 0;
    }
    std::fprintf(stderr, "usage: %s [stable|noise|shift|operator|all]\n", argv[0]);
    return 1;
}
