// DCHL Thinking Machine — interactive.
//
// The prompt goes into the frozen prompt region of the cognitive state.
// Hamiltonian dynamics on the learned potential Phi evolve ONLY the answer
// region (prompt gradient masked). The evolved answer bytes decode straight
// back to a string. No tokens, no vocab, no argmax, no autoregression.
//
// Whether that string is meaningful depends entirely on whether the training
// corpus + epochs carved out a coherence basin near the prompt. Phi/dPhi
// tell you if it did.

#include "hcma.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace hcma;

int main(int argc, char** argv) {
    std::string model_path = "chatbot.neuron";
    int steps = 200;         // Hamiltonian roll-out steps (legacy decoder)
    int sweeps = 6;          // BED sweeps (novel decoder)
    float dt = 0.01f;
    float temperature = 0.0f;
    bool auto_prompt = false;
    bool use_hamiltonian = false; // default: Byte Energy Descent
    std::string one_shot;

    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", k.c_str()); std::exit(1); }
            return argv[++i];
        };
        if (k == "--steps") steps = std::atoi(next().c_str());
        else if (k == "--sweeps") sweeps = std::atoi(next().c_str());
        else if (k == "--dt") dt = std::atof(next().c_str());
        else if (k == "--temp") temperature = std::atof(next().c_str());
        else if (k == "--model") model_path = next();
        else if (k == "--hamiltonian") use_hamiltonian = true;
        else if (k == "--ask") { one_shot = next(); auto_prompt = true; }
        else model_path = k;
    }

    Potential P;
    OODDetector ood;
    if (!load_neuron(model_path, P, ood)) {
        std::fprintf(stderr, "cannot load model: %s\n", model_path.c_str());
        return 1;
    }
    if (P.D_prompt == 0) {
        std::fprintf(stderr,
            "\n[error] model '%s' was trained without a prompt/answer split\n"
            "        (D_prompt=0). BED would descend the same Phi from the same\n"
            "        initial state for every input — every 'answer' will be\n"
            "        identical. Retrain with lto (which now always writes a\n"
            "        split model) and try again.\n\n", model_path.c_str());
        return 1;
    }

    std::printf("=== DCHL Thinking Machine ===\n");
    std::printf("model=%s  D_prompt=%zu  D_answer=%zu  H=%zu  decoder=%s  cache=%zu\n\n",
                model_path.c_str(), P.D_prompt, P.D_answer, P.H,
                use_hamiltonian ? "Hamiltonian" : "BED (Byte Energy Descent)",
                ood.known_states.size());

    auto answer = [&](const std::string& line) {
        Vec c0 = make_qa_state(line, "", P.D_prompt, P.D_answer);
        float conf = ood.density(c0);
        int is_ood = ood.is_ood(c0) ? 1 : 0;
        ThoughtResult r = use_hamiltonian ? think(P, line, steps, dt)
                                          : think_bytes(P, line, sweeps, temperature);
        float dphi = r.phi_final - r.phi_initial;
        std::printf("  Phi(init)=%.2f  conf=%.3f  ood=%d\n", r.phi_initial, conf, is_ood);
        std::printf("  Phi(fin)=%.2f   dPhi=%.2f  depth=%d\n", r.phi_final, dphi, r.steps);
        std::printf("  answer> %s\n",
                    r.answer.empty() ? "(basin: no bytes decoded)" : r.answer.c_str());
        // Raw byte histogram of what BED actually converged to.
        int hist[8] = {0,0,0,0,0,0,0,0}; // 0-31, 32-63, 64-95, 96-127, 128-159, 160-191, 192-223, 224-255
        int mn = 255, mx = 0, printable = 0;
        for (size_t i = 0; i < P.D_answer; ++i) {
            int b = (int)std::lround(r.c_final[P.D_prompt + i] * 256.0f);
            if (b < 0) b = 0; if (b > 255) b = 255;
            ++hist[b / 32];
            if (b < mn) mn = b;
            if (b > mx) mx = b;
            if (b >= 32 && b <= 126) ++printable;
        }
        std::printf("  bytes  > min=%d max=%d printable=%d/%zu  hist: ",
                    mn, mx, printable, P.D_answer);
        for (int k = 0; k < 8; ++k) std::printf("[%d-%d]:%d ", k*32, k*32+31, hist[k]);
        std::printf("\n");
        if (is_ood)
            std::printf("  * outside the known manifold — low epistemic confidence\n");
        else if (dphi < 0.0f)
            std::printf("  * deepened understanding\n");
        else
            std::printf("  * no deepening (already at a minimum)\n");
    };

    if (auto_prompt) { answer(one_shot); return 0; }

    std::string line;
    while (true) {
        std::printf("> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;
        answer(line);
    }
    std::printf("\n");
    return 0;
}
