// generate — P-CMA sliding-window Hamiltonian text generation.
//
//   ./generate model.neuron "prompt" [--chars N] [--steps K] [--dt X]
//                                     [--raw]
//
// The state c is a D-byte window normalised to [0, 1]. The trained Phi(c)
// has been shaped by DCHL so real-text windows are low-energy. We evolve
// c under Störmer-Verlet Hamiltonian dynamics for K steps, read the last
// byte, shift the window left one slot, and repeat. This is not BED
// (no discrete argmin); it is continuous energy descent + tail readout.
//
// --raw removes the printable-ASCII constraint on the readout.

#include "hcma.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace hcma;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
            "usage: %s model.neuron \"prompt\" [--chars N] [--steps K] [--dt X] [--raw]\n",
            argv[0]);
        return 1;
    }
    std::string model_path = argv[1];
    std::string prompt = argv[2];
    int chars = 200;
    int steps = 10;
    float dt = 0.01f;
    bool raw = false;
    for (int i = 3; i < argc; ++i) {
        std::string k = argv[i];
        auto nx = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (k == "--chars") chars = std::atoi(nx().c_str());
        else if (k == "--steps") steps = std::atoi(nx().c_str());
        else if (k == "--dt") dt = std::atof(nx().c_str());
        else if (k == "--raw") raw = true;
    }

    Potential P; OODDetector ood;
    if (!load_neuron(model_path, P, ood)) {
        std::fprintf(stderr, "cannot load %s\n", model_path.c_str());
        return 1;
    }

    std::printf("=== P-CMA sliding-window generator ===\n");
    std::printf("model=%s  D=%zu  H=%zu  chars=%d  steps/char=%d  dt=%g\n\n",
                model_path.c_str(), P.D, P.H, chars, steps, dt);

    SlidingResult r = generate_sliding(P, prompt, chars, steps, dt, !raw);
    std::printf("--- generated ---\n%s\n", r.output.c_str());
    std::printf("\n--- metrics ---\nPhi(avg)=%.3f  chars=%d  steps/char=%d\n",
                r.phi_avg, r.chars, r.steps_per_char);
    return 0;
}
