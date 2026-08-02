// LTO: DCHL trainer for the thinking machine.
//
// Positive samples come from a windows.bin (fixed-width byte windows of real
// text). Negatives are synthesised on the fly (random / shuffled / spliced).
// The learning rule is purely local (Hebbian, ZeroGrad) — no backprop, no
// loss, no gradient optimizer.
//
// CPU path is here. --use-gpu delegates to gpu_hcma.cu (4-bit ANQ).

#include "hcma.hpp"
#include "windows_bin.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace hcma;

struct Args {
    int epochs = 5;
    // Smaller default LR so hidden units don't saturate in the first epoch —
    // saturation is what produces the |h|=0.512 dead-lock and the linear
    // "ASCII shift" leak in BED output.
    float lr_hidden = 0.005f;
    float lr_output = 0.05f;
    size_t d_prompt = 256;
    size_t d_answer = 256;
    size_t d_hidden = 1024;
    std::string windows_path;
    std::string save = "chatbot.neuron";
    unsigned seed = 42;
    bool use_gpu = false;
    // Negative mix (probabilities sum to 1); default: mostly hard negatives.
    float p_random = 0.2f, p_shuffle = 0.4f, p_splice = 0.4f;
    size_t cache_max = 512;   // OOD cache upper bound
    size_t samples_per_epoch = 0; // 0 = all windows
    int k_neg = 1;            // negatives per positive (averaged)
    int warmup_epochs = 0;    // curriculum: use easy negatives for the first N
};

static Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", k.c_str()); std::exit(1); }
            return argv[++i];
        };
        if (k == "--epochs") a.epochs = std::atoi(next().c_str());
        else if (k == "--lr-hidden") a.lr_hidden = std::atof(next().c_str());
        else if (k == "--lr-output") a.lr_output = std::atof(next().c_str());
        else if (k == "--d-prompt") a.d_prompt = (size_t)std::atoi(next().c_str());
        else if (k == "--d-answer") a.d_answer = (size_t)std::atoi(next().c_str());
        else if (k == "--d-hidden") a.d_hidden = (size_t)std::atoi(next().c_str());
        else if (k == "--windows") a.windows_path = next();
        else if (k == "--save") a.save = next();
        else if (k == "--seed") a.seed = (unsigned)std::atoi(next().c_str());
        else if (k == "--use-gpu") a.use_gpu = true;
        else if (k == "--p-random") a.p_random = std::atof(next().c_str());
        else if (k == "--p-shuffle") a.p_shuffle = std::atof(next().c_str());
        else if (k == "--p-splice") a.p_splice = std::atof(next().c_str());
        else if (k == "--cache-max") a.cache_max = (size_t)std::atoi(next().c_str());
        else if (k == "--samples-per-epoch") a.samples_per_epoch = (size_t)std::atoi(next().c_str());
        else if (k == "--k-neg") a.k_neg = std::atoi(next().c_str());
        else if (k == "--warmup") a.warmup_epochs = std::atoi(next().c_str());
        else { std::fprintf(stderr, "unknown flag: %s\n", k.c_str()); std::exit(1); }
    }
    return a;
}

#ifdef HCMA_WITH_GPU
extern "C" void train_gpu(Potential& P, const Windows& W, int epochs,
                          float lr_hidden, float lr_output,
                          float p_random, float p_shuffle, float p_splice,
                          size_t samples_per_epoch, unsigned seed, const char* save,
                          int k_neg, int warmup_epochs);
#endif

int main(int argc, char** argv) {
    Args args = parse(argc, argv);

    Windows W;
    Vec fallback_neg;
    if (!args.windows_path.empty()) {
        if (!load_windows(args.windows_path, W)) {
            std::fprintf(stderr, "cannot load windows: %s\n", args.windows_path.c_str());
            return 1;
        }
        // Adjust state dims to match the file so we don't silently mis-align.
        if (args.d_prompt + args.d_answer != W.win) {
            args.d_prompt = W.win / 2;
            args.d_answer = W.win - args.d_prompt;
            std::printf("[warn] adjusted state dims to match windows: D_prompt=%zu D_answer=%zu (win=%u)\n",
                        args.d_prompt, args.d_answer, W.win);
        }
    }
    size_t D = args.d_prompt + args.d_answer;
    size_t n_params = D * args.d_hidden + args.d_hidden + args.d_hidden + 1;
    std::printf("=== LTO / DCHL coherence training (ZeroGrad) ===\n");
    std::printf("windows=%u  D_prompt=%zu  D_answer=%zu  H=%zu  params=%.1fM  epochs=%d\n",
                W.n, args.d_prompt, args.d_answer, args.d_hidden, n_params / 1e6, args.epochs);

    Potential P(args.d_prompt, args.d_answer, args.d_hidden);

    if (args.use_gpu) {
#ifdef HCMA_WITH_GPU
        train_gpu(P, W, args.epochs, args.lr_hidden, args.lr_output,
                  args.p_random, args.p_shuffle, args.p_splice,
                  args.samples_per_epoch, args.seed, args.save.c_str(),
                  args.k_neg, args.warmup_epochs);
        return 0;
#else
        std::fprintf(stderr, "GPU support not compiled in (rebuild with `make gpu`)\n");
        return 1;
#endif
    }

    // ---------- CPU path ----------
    if (W.n == 0) {
        std::fprintf(stderr, "no windows provided; run tools/prepare_windows first.\n");
        return 1;
    }
    std::mt19937 rng(args.seed);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    std::vector<size_t> order(W.n);
    for (size_t i = 0; i < W.n; ++i) order[i] = i;

    auto t0 = std::chrono::steady_clock::now();
    size_t n_per_epoch = args.samples_per_epoch ? args.samples_per_epoch : W.n;
    int K = std::max(1, args.k_neg);
    std::printf("[cfg] K=%d warmup=%d p_rand=%.2f p_shuf=%.2f p_spl=%.2f\n",
                K, args.warmup_epochs, args.p_random, args.p_shuffle, args.p_splice);

    for (int ep = 1; ep <= args.epochs; ++ep) {
        std::shuffle(order.begin(), order.end(), rng);

        // Curriculum: during warmup epochs, use a soft mix (helps units
        // differentiate). After warmup, go pure hard-splice-only.
        float pr = args.p_random, ps = args.p_shuffle;
        if (args.warmup_epochs > 0 && ep > args.warmup_epochs) {
            pr = 0.0f; ps = 0.0f;
        }

        double sum_pos = 0, sum_neg = 0, sum_h = 0;
        size_t seen = 0;
        std::vector<Vec> c_negs;
        c_negs.reserve(K);
        for (size_t idx_i = 0; idx_i < n_per_epoch; ++idx_i) {
            size_t idx = order[idx_i % order.size()];
            Vec c_pos = W.state(idx);

            c_negs.clear();
            for (int k = 0; k < K; ++k) {
                NegKind kind;
                float r = uni(rng);
                if (r < pr) kind = NegKind::Random;
                else if (r < pr + ps) kind = NegKind::Shuffle;
                else kind = NegKind::Splice;
                size_t partner = order[(idx_i + 1 + k * 13) % order.size()];
                Vec c_other = W.state(partner);
                c_negs.push_back(make_negative(c_pos, kind, &c_other, rng));
            }

            if (K == 1) chl_step(P, c_pos, c_negs[0], args.lr_hidden, args.lr_output);
            else chl_step_multi(P, c_pos, c_negs, args.lr_hidden, args.lr_output);

            Vec h;
            sum_pos += P.forward(c_pos, &h);
            for (float v : h) sum_h += std::fabs(v);
            double neg_avg = 0;
            for (const Vec& cn : c_negs) neg_avg += P.forward(cn);
            sum_neg += neg_avg / (double)K;
            ++seen;
        }
        end_of_epoch(P);
        double phi_pos = sum_pos / (double)seen;
        double phi_neg = sum_neg / (double)seen;
        P.baseline += 0.01f * (float)((phi_pos + phi_neg) * 0.5 - P.baseline);
        std::printf("epoch %3d  Phi(coh)=%9.2f  Phi(incoh)=%9.2f  dPhi=%9.2f  |h|=%.3f\n",
                    ep, phi_pos, phi_neg, phi_neg - phi_pos,
                    sum_h / ((double)seen * (double)args.d_hidden));
    }
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();

    // OOD cache: subsample from the training windows themselves.
    OODDetector ood;
    size_t take = std::min<size_t>(args.cache_max, W.n);
    ood.known_states.reserve(take);
    for (size_t i = 0; i < take; ++i) ood.known_states.push_back(W.state(order[i]));

    if (!save_neuron(args.save, P, ood)) {
        std::fprintf(stderr, "failed to save %s\n", args.save.c_str());
        return 1;
    }
    std::printf("\ntrained in %.1fs; saved %s (%zu cached states)\n",
                secs, args.save.c_str(), ood.known_states.size());
    return 0;
}
