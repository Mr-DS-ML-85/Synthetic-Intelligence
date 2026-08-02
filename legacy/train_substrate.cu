// train_substrate.cu — GPU trainer for the tri-layer architecture's
// Layer 1 substrate (research-paper/synthetic-intelligence.md).
//
// Objective: DENOISING HEBBIAN — the manifold-sculpting form of DCHL
// (research-paper/how_we_implement.md §4.4). For each real window c
// from the corpus we synthesize a negative c' = c + eta where eta is a
// small Gaussian perturbation. The DCHL update sculpts Phi so that
//   Phi(c)  <  Phi(c + eta)
// monotone-in-||eta||. There is no class of negatives, no shuffle, no
// splice, no random-byte baseline — the negatives are continuous
// perturbations of the positive, so ∇Phi at c points *back toward the
// data manifold*. That is what Layer 1 in the tri-layer needs to do:
// pull paraphrases into the same basin under Hamiltonian evolution.
//
// Storage: ANQ 4-bit for the H×D matrix W1 (per-row scale+zero fp32),
// fp32 for b1 and w2. A 1B-parameter substrate (D=1024, H≈1M) fits in
// ~540 MB VRAM.
//
// This trainer does NOT do:
//   - text-vs-random contrast   (that's a binary classifier)
//   - Q|A triplet learning       (that's memorization)
//   - retrieval-based readout    (that's attention in disguise)
// See synthetic-intelligence.md §1.2 for why each is a trap.

#include "hcma.hpp"
#include "windows_bin.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime.h>
#include <random>
#include <string>
#include <vector>

using namespace hcma;

// ------------------------------------------------------------------
// Error check.
// ------------------------------------------------------------------
static void cuda_check(cudaError_t e, const char* what) {
    if (e != cudaSuccess) {
        std::fprintf(stderr, "cuda error [%s]: %s\n", what, cudaGetErrorString(e));
        std::exit(1);
    }
}

// ------------------------------------------------------------------
// ANQ 4-bit weight matrix. Per-row (block == D) scale + zero.
// dequant(q) = zero + scale * (q - 7.5),  q in {0..15}.
// ------------------------------------------------------------------
struct ANQ {
    uint8_t* packed = nullptr; // H * (D/2) bytes on device
    float*   scale  = nullptr; // H fp32 on device
    float*   zero   = nullptr; // H fp32 on device
    int H = 0, D = 0;
};

__device__ __forceinline__ float anq_dequant(uint8_t byte, bool hi, float s, float z) {
    int q = hi ? ((byte >> 4) & 0xF) : (byte & 0xF);
    return z + s * ((float)q - 7.5f);
}

// ------------------------------------------------------------------
// Denoising DCHL kernel. Grid: (H blocks), Block: T threads.
//
// One block per hidden unit j. It:
//   1) dequantizes row_j into shared memory,
//   2) computes pre_pos and pre_neg via dot with c_pos and c_neg,
//   3) applies Hebbian update on the dequantized row:
//        row  += lr_h * (h_pos - h_neg) * c_pos
//        b1_j += lr_h * (h_pos - h_neg)
//   4) applies pure-differential update on w2:
//        w2_j += lr_o * (h_neg - h_pos), clamped >= 0
//   5) re-quantizes the row back into 4-bit storage with a fresh
//      per-row min/max (that's ANQ's local error accumulation).
//   6) contributes phi_pos, phi_neg, |h| to epoch accumulators
//      atomically (once per block, not once per thread).
//
// Per-step normalization is deliberately NOT done here — it's the
// mechanism that produced the |h|=0.043 death-lock in the old trainer.
// Row L2 normalization runs once per epoch in a separate kernel.
// ------------------------------------------------------------------
extern "C" __global__ void substrate_step_kernel(
    uint8_t* __restrict__ W_packed,
    float*   __restrict__ W_scale,
    float*   __restrict__ W_zero,
    float*   __restrict__ b1,
    float*   __restrict__ w2,
    const float* __restrict__ c_pos,
    const float* __restrict__ c_neg,
    float inv_n_pos, float inv_n_neg,
    float* __restrict__ acc_phi_pos,
    float* __restrict__ acc_phi_neg,
    float* __restrict__ acc_abs_h,
    float lr_hidden, float lr_output,
    int D)
{
    extern __shared__ float sh[];
    float* row = sh;             // [D]
    float* red = sh + D;         // [T]

    const int j = blockIdx.x;
    const int t = threadIdx.x;
    const int T = blockDim.x;

    // 1. dequantize this row into shared memory
    float rs = W_scale[j], rz = W_zero[j];
    for (int i = t; i < D; i += T) {
        uint8_t byte = W_packed[(size_t)j * (D / 2) + (i / 2)];
        row[i] = anq_dequant(byte, (i & 1) != 0, rs, rz);
    }
    __syncthreads();

    // 2a. dot products for h_pos and h_neg (block reduction in shared mem)
    float dp = 0.f, dn = 0.f;
    for (int i = t; i < D; i += T) {
        dp += row[i] * c_pos[i];
        dn += row[i] * c_neg[i];
    }
    red[t] = dp; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s) red[t] += red[t + s];
        __syncthreads();
    }
    float sum_p = red[0];
    __syncthreads();
    red[t] = dn; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s) red[t] += red[t + s];
        __syncthreads();
    }
    float sum_n = red[0];

    float bj      = b1[j];
    float w2_old  = w2[j];
    float pre_pos = (sum_p + bj) * inv_n_pos;
    float pre_neg = (sum_n + bj) * inv_n_neg;
    float h_pos   = tanhf(pre_pos);
    float h_neg   = tanhf(pre_neg);
    float err     = h_pos - h_neg;

    // 3. Hebbian update on the row + b1 (no per-step normalization)
    for (int i = t; i < D; i += T)
        row[i] += lr_hidden * err * c_pos[i];
    if (t == 0) b1[j] = bj + lr_hidden * err;

    // 4. w2 pure differential; clamp >= 0
    if (t == 0) {
        float w = w2_old + lr_output * (h_neg - h_pos);
        w2[j] = (w < 0.f) ? 0.f : w;
    }
    __syncthreads();

    // 5. re-quantize with fresh min/max (ANQ)
    float lo = 1e30f, hi = -1e30f;
    for (int i = t; i < D; i += T) {
        float v = row[i];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    red[t] = lo; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s && red[t + s] < red[t]) red[t] = red[t + s];
        __syncthreads();
    }
    float row_lo = red[0]; __syncthreads();
    red[t] = hi; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s && red[t + s] > red[t]) red[t] = red[t + s];
        __syncthreads();
    }
    float row_hi = red[0];
    if (t == 0) {
        float span = row_hi - row_lo;
        float ns = (span > 1e-20f) ? span / 15.f : 1e-20f;
        float nz = (row_lo + row_hi) * 0.5f;
        W_scale[j] = ns;
        W_zero [j] = nz;
        red[0] = ns; red[1] = nz;
    }
    __syncthreads();
    float new_scale = red[0], new_zero = red[1];
    float inv_s = 1.f / new_scale;
    for (int k = t; k < D / 2; k += T) {
        int qi0 = (int)lrintf((row[2 * k]     - new_zero) * inv_s + 7.5f);
        int qi1 = (int)lrintf((row[2 * k + 1] - new_zero) * inv_s + 7.5f);
        if (qi0 < 0) qi0 = 0; if (qi0 > 15) qi0 = 15;
        if (qi1 < 0) qi1 = 0; if (qi1 > 15) qi1 = 15;
        W_packed[(size_t)j * (D / 2) + k] = (uint8_t)(qi0 | (qi1 << 4));
    }

    // 6. atomic accumulators once per block
    if (t == 0) {
        atomicAdd(acc_phi_pos, w2_old * h_pos);
        atomicAdd(acc_phi_neg, w2_old * h_neg);
        atomicAdd(acc_abs_h,   fabsf(h_pos));
    }
}

// End-of-epoch: L2-normalize the combined [row_j, b1_j] to unit norm,
// apply gentle w2 L2 decay, re-quantize.
extern "C" __global__ void end_of_epoch_kernel(
    uint8_t* __restrict__ W_packed,
    float*   __restrict__ W_scale,
    float*   __restrict__ W_zero,
    float*   __restrict__ b1,
    float*   __restrict__ w2,
    float w2_decay, int D)
{
    extern __shared__ float sh[];
    float* row = sh;
    float* red = sh + D;
    const int j = blockIdx.x;
    const int t = threadIdx.x;
    const int T = blockDim.x;

    float rs = W_scale[j], rz = W_zero[j];
    for (int i = t; i < D; i += T) {
        uint8_t byte = W_packed[(size_t)j * (D / 2) + (i / 2)];
        row[i] = anq_dequant(byte, (i & 1) != 0, rs, rz);
    }
    __syncthreads();

    float loc = 0.f;
    for (int i = t; i < D; i += T) loc += row[i] * row[i];
    red[t] = loc; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s) red[t] += red[t + s];
        __syncthreads();
    }
    float bj = b1[j];
    if (t == 0) {
        float nrm2 = red[0] + bj * bj;
        float inv = (nrm2 > 1e-20f) ? rsqrtf(nrm2) : 0.f;
        red[0] = inv;
        b1[j] = bj * inv;
        w2[j] *= (1.f - w2_decay);
    }
    __syncthreads();
    float inv = red[0];
    for (int i = t; i < D; i += T) row[i] *= inv;
    __syncthreads();

    float lo = 1e30f, hi = -1e30f;
    for (int i = t; i < D; i += T) {
        float v = row[i];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    red[t] = lo; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s && red[t + s] < red[t]) red[t] = red[t + s];
        __syncthreads();
    }
    float row_lo = red[0]; __syncthreads();
    red[t] = hi; __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s && red[t + s] > red[t]) red[t] = red[t + s];
        __syncthreads();
    }
    float row_hi = red[0];
    if (t == 0) {
        float span = row_hi - row_lo;
        float ns = (span > 1e-20f) ? span / 15.f : 1e-20f;
        float nz = (row_lo + row_hi) * 0.5f;
        W_scale[j] = ns;
        W_zero [j] = nz;
        red[0] = ns; red[1] = nz;
    }
    __syncthreads();
    float new_scale = red[0], new_zero = red[1];
    float inv_s = 1.f / new_scale;
    for (int k = t; k < D / 2; k += T) {
        int qi0 = (int)lrintf((row[2 * k]     - new_zero) * inv_s + 7.5f);
        int qi1 = (int)lrintf((row[2 * k + 1] - new_zero) * inv_s + 7.5f);
        if (qi0 < 0) qi0 = 0; if (qi0 > 15) qi0 = 15;
        if (qi1 < 0) qi1 = 0; if (qi1 > 15) qi1 = 15;
        W_packed[(size_t)j * (D / 2) + k] = (uint8_t)(qi0 | (qi1 << 4));
    }
}

// ------------------------------------------------------------------
// Host: init an ANQ matrix from a random Gaussian, quantize row-wise.
// ------------------------------------------------------------------
static void init_anq_gaussian(int H, int D, std::mt19937& rng,
                              std::vector<uint8_t>& packed,
                              std::vector<float>& scale,
                              std::vector<float>& zero) {
    packed.assign((size_t)H * (D / 2), 0);
    scale.assign(H, 0);
    zero.assign(H, 0);
    float sigma = 1.0f / std::sqrt((float)D); // Xavier-ish
    std::normal_distribution<float> g(0.f, sigma);
    std::vector<float> row(D);
    for (int j = 0; j < H; ++j) {
        for (int i = 0; i < D; ++i) row[i] = g(rng);
        float lo = row[0], hi = row[0];
        for (int i = 1; i < D; ++i) {
            if (row[i] < lo) lo = row[i];
            if (row[i] > hi) hi = row[i];
        }
        float span = hi - lo;
        float s = (span > 1e-20f) ? span / 15.f : 1e-20f;
        float z = (lo + hi) * 0.5f;
        scale[j] = s;
        zero [j] = z;
        float inv_s = 1.f / s;
        for (int i = 0; i < D; ++i) {
            int qi = (int)std::lround((row[i] - z) * inv_s + 7.5f);
            if (qi < 0) qi = 0; if (qi > 15) qi = 15;
            int k = i / 2;
            if ((i & 1) == 0) packed[(size_t)j * (D/2) + k] = (uint8_t)qi;
            else              packed[(size_t)j * (D/2) + k] |= (uint8_t)(qi << 4);
        }
    }
}

// Host: dequantize the packed ANQ matrix into fp32 (for saving).
static void dequant_all(const std::vector<uint8_t>& packed,
                        const std::vector<float>& scale,
                        const std::vector<float>& zero,
                        int H, int D,
                        std::vector<float>& out) {
    out.assign((size_t)H * D, 0.f);
    for (int j = 0; j < H; ++j) {
        float s = scale[j], z = zero[j];
        for (int i = 0; i < D; ++i) {
            uint8_t byte = packed[(size_t)j * (D/2) + (i/2)];
            int q = ((i & 1) ? (byte >> 4) : byte) & 0xF;
            out[(size_t)j * D + i] = z + s * ((float)q - 7.5f);
        }
    }
}

// ------------------------------------------------------------------
// aug-norm on host (matches Potential::aug_norm).
// ------------------------------------------------------------------
static float aug_norm_host(const float* c, int D) {
    double s = 1.0;
    for (int i = 0; i < D; ++i) s += (double)c[i] * c[i];
    return (float)std::sqrt(s);
}

// ------------------------------------------------------------------
// CLI.
// ------------------------------------------------------------------
struct Args {
    std::string windows = "windows_1b.bin";
    std::string save    = "substrate_1b.neuron";
    int D_prompt = 512;
    int D_answer = 512;
    int H        = 1048576;   // 2^20 for ~1B params at D=1024
    int epochs   = 6;
    int samples_per_epoch = 1500;
    float noise_sigma = 0.05f;   // denoising perturbation size
    float lr_hidden = 0.01f;
    float lr_output = 0.1f;
    unsigned seed = 42;
    int block_threads = 256;
};

static Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto nx = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", k.c_str()); std::exit(1); }
            return argv[++i];
        };
        if      (k == "--windows")            a.windows = nx();
        else if (k == "--save")               a.save    = nx();
        else if (k == "--d-prompt")           a.D_prompt = std::atoi(nx().c_str());
        else if (k == "--d-answer")           a.D_answer = std::atoi(nx().c_str());
        else if (k == "--d-hidden")           a.H       = std::atoi(nx().c_str());
        else if (k == "--epochs")             a.epochs  = std::atoi(nx().c_str());
        else if (k == "--samples-per-epoch")  a.samples_per_epoch = std::atoi(nx().c_str());
        else if (k == "--noise")              a.noise_sigma = std::atof(nx().c_str());
        else if (k == "--lr-hidden")          a.lr_hidden = std::atof(nx().c_str());
        else if (k == "--lr-output")          a.lr_output = std::atof(nx().c_str());
        else if (k == "--seed")               a.seed = (unsigned)std::atoi(nx().c_str());
        else if (k == "--threads")            a.block_threads = std::atoi(nx().c_str());
        else { std::fprintf(stderr, "unknown flag: %s\n", k.c_str()); std::exit(1); }
    }
    return a;
}

int main(int argc, char** argv) {
    Args args = parse(argc, argv);
    int D = args.D_prompt + args.D_answer;
    if (D & 1) { std::fprintf(stderr, "D must be even for 4-bit packing\n"); return 1; }
    int H = args.H;

    // ---- load windows ----
    Windows W;
    if (!load_windows(args.windows, W)) {
        std::fprintf(stderr, "cannot load windows: %s\n", args.windows.c_str());
        return 1;
    }
    if ((int)W.win != D) {
        std::fprintf(stderr, "window size (%u) != D (%d); adjust --d-prompt/--d-answer\n",
                     W.win, D);
        return 1;
    }
    size_t n_params = (size_t)D * H + H + H + 1;
    std::printf("=== substrate trainer (denoising DCHL, ANQ 4-bit) ===\n");
    std::printf("  windows    : %u @ %u bytes\n", W.n, W.win);
    std::printf("  D          : %d (prompt=%d + answer=%d)\n", D, args.D_prompt, args.D_answer);
    std::printf("  H          : %d\n", H);
    std::printf("  params     : %.2f M   (~%.2f GB fp32,  %.1f MB in 4-bit)\n",
                n_params / 1e6, n_params * 4.0 / 1e9,
                ((size_t)H * (D / 2) + H * 8) / 1e6);
    std::printf("  epochs     : %d  |  samples/epoch: %d  |  noise sigma: %.3f\n",
                args.epochs, args.samples_per_epoch, args.noise_sigma);
    std::printf("  lr_hidden  : %g  |  lr_output: %g\n", args.lr_hidden, args.lr_output);
    std::printf("  save       : %s\n\n", args.save.c_str());

    // ---- init ANQ matrix on host, copy to device ----
    std::mt19937 rng(args.seed);
    std::vector<uint8_t> h_packed;
    std::vector<float>   h_scale, h_zero;
    init_anq_gaussian(H, D, rng, h_packed, h_scale, h_zero);
    std::vector<float>   h_b1(H, 0.f), h_w2(H, 0.f);

    ANQ Wq;
    Wq.H = H; Wq.D = D;
    cuda_check(cudaMalloc(&Wq.packed, h_packed.size()),                "malloc packed");
    cuda_check(cudaMalloc(&Wq.scale,  h_scale.size()  * sizeof(float)),"malloc scale");
    cuda_check(cudaMalloc(&Wq.zero,   h_zero.size()   * sizeof(float)),"malloc zero");
    cuda_check(cudaMemcpy(Wq.packed, h_packed.data(), h_packed.size(), cudaMemcpyHostToDevice), "H2D packed");
    cuda_check(cudaMemcpy(Wq.scale,  h_scale.data(),  H * 4,           cudaMemcpyHostToDevice), "H2D scale");
    cuda_check(cudaMemcpy(Wq.zero,   h_zero.data(),   H * 4,           cudaMemcpyHostToDevice), "H2D zero");

    float *d_b1 = nullptr, *d_w2 = nullptr, *d_cp = nullptr, *d_cn = nullptr;
    float *d_phi_pos = nullptr, *d_phi_neg = nullptr, *d_absh = nullptr;
    cuda_check(cudaMalloc(&d_b1, H * 4), "b1");
    cuda_check(cudaMalloc(&d_w2, H * 4), "w2");
    cuda_check(cudaMalloc(&d_cp, D * 4), "cp");
    cuda_check(cudaMalloc(&d_cn, D * 4), "cn");
    cuda_check(cudaMalloc(&d_phi_pos, 4), "phi_pos");
    cuda_check(cudaMalloc(&d_phi_neg, 4), "phi_neg");
    cuda_check(cudaMalloc(&d_absh,    4), "absh");
    cuda_check(cudaMemcpy(d_b1, h_b1.data(), H * 4, cudaMemcpyHostToDevice), "H2D b1");
    cuda_check(cudaMemcpy(d_w2, h_w2.data(), H * 4, cudaMemcpyHostToDevice), "H2D w2");

    float *hp_cp = nullptr, *hp_cn = nullptr;
    cuda_check(cudaHostAlloc((void**)&hp_cp, D * 4, cudaHostAllocDefault), "pinned cp");
    cuda_check(cudaHostAlloc((void**)&hp_cn, D * 4, cudaHostAllocDefault), "pinned cn");

    std::vector<uint32_t> order(W.n);
    for (uint32_t i = 0; i < W.n; ++i) order[i] = i;
    std::normal_distribution<float> noise(0.f, args.noise_sigma);

    size_t vram_est = h_packed.size() + H * 8 + H * 8 + D * 8 + 12;
    std::printf("  estimated device VRAM: %.1f MB\n\n", vram_est / 1e6);

    const int T = args.block_threads;
    size_t sh_bytes = ((size_t)D + T) * 4;
    size_t norm_sh  = sh_bytes;

    // ---- training loop ----
    auto t0 = std::chrono::steady_clock::now();
    for (int ep = 1; ep <= args.epochs; ++ep) {
        std::shuffle(order.begin(), order.end(), rng);
        float zero_f = 0.f;
        cuda_check(cudaMemcpy(d_phi_pos, &zero_f, 4, cudaMemcpyHostToDevice), "reset pp");
        cuda_check(cudaMemcpy(d_phi_neg, &zero_f, 4, cudaMemcpyHostToDevice), "reset pn");
        cuda_check(cudaMemcpy(d_absh,    &zero_f, 4, cudaMemcpyHostToDevice), "reset h");

        int seen = 0;
        auto ep_start = std::chrono::steady_clock::now();
        for (int i = 0; i < args.samples_per_epoch; ++i) {
            uint32_t idx = order[i % W.n];
            Vec c_pos = W.state(idx);
            // Denoising negative: c_neg = c_pos + Gaussian noise clipped to [0,1]
            Vec c_neg = c_pos;
            for (auto& v : c_neg) {
                v += noise(rng);
                if (v < 0.f) v = 0.f;
                if (v > 1.f) v = 1.f;
            }
            float inv_np = 1.f / aug_norm_host(c_pos.data(), D);
            float inv_nn = 1.f / aug_norm_host(c_neg.data(), D);
            std::memcpy(hp_cp, c_pos.data(), D * 4);
            std::memcpy(hp_cn, c_neg.data(), D * 4);
            cuda_check(cudaMemcpyAsync(d_cp, hp_cp, D * 4, cudaMemcpyHostToDevice, 0), "H2D cp");
            cuda_check(cudaMemcpyAsync(d_cn, hp_cn, D * 4, cudaMemcpyHostToDevice, 0), "H2D cn");

            substrate_step_kernel<<<H, T, sh_bytes>>>(
                Wq.packed, Wq.scale, Wq.zero,
                d_b1, d_w2,
                d_cp, d_cn, inv_np, inv_nn,
                d_phi_pos, d_phi_neg, d_absh,
                args.lr_hidden, args.lr_output, D);
            ++seen;
        }
        cuda_check(cudaDeviceSynchronize(), "sync train");
        end_of_epoch_kernel<<<H, T, norm_sh>>>(
            Wq.packed, Wq.scale, Wq.zero, d_b1, d_w2, 1e-3f, D);
        cuda_check(cudaDeviceSynchronize(), "sync eoe");

        float sp = 0, sn = 0, sh_ = 0;
        cuda_check(cudaMemcpy(&sp, d_phi_pos, 4, cudaMemcpyDeviceToHost), "D2H pp");
        cuda_check(cudaMemcpy(&sn, d_phi_neg, 4, cudaMemcpyDeviceToHost), "D2H pn");
        cuda_check(cudaMemcpy(&sh_, d_absh,   4, cudaMemcpyDeviceToHost), "D2H h");
        double denom = (double)seen;
        double phi_pos = sp / denom;
        double phi_neg = sn / denom;
        double mean_h  = sh_ / (denom * (double)H);
        auto ep_end = std::chrono::steady_clock::now();
        double ep_sec = std::chrono::duration<double>(ep_end - ep_start).count();
        std::printf("  epoch %2d  Phi(data)=%10.2f  Phi(noised)=%10.2f  dPhi=%9.2f  |h|=%.3f  (%.1fs)\n",
                    ep, phi_pos, phi_neg, phi_neg - phi_pos, mean_h, ep_sec);
    }
    auto t1 = std::chrono::steady_clock::now();
    double total_sec = std::chrono::duration<double>(t1 - t0).count();
    std::printf("\ntrained in %.1fs\n", total_sec);

    // ---- pull ANQ + fp params back to host, save as .neuron ----
    cuda_check(cudaMemcpy(h_packed.data(), Wq.packed, h_packed.size(), cudaMemcpyDeviceToHost), "D2H packed");
    cuda_check(cudaMemcpy(h_scale.data(),  Wq.scale,  H * 4,           cudaMemcpyDeviceToHost), "D2H scale");
    cuda_check(cudaMemcpy(h_zero.data(),   Wq.zero,   H * 4,           cudaMemcpyDeviceToHost), "D2H zero");
    cuda_check(cudaMemcpy(h_b1.data(),     d_b1,      H * 4,           cudaMemcpyDeviceToHost), "D2H b1");
    cuda_check(cudaMemcpy(h_w2.data(),     d_w2,      H * 4,           cudaMemcpyDeviceToHost), "D2H w2");

    // Dequantize W1 to fp32 for the hcma::Potential save format.
    Potential P;
    P.D = D; P.H = H;
    P.D_prompt = args.D_prompt; P.D_answer = args.D_answer;
    P.b1.assign(h_b1.begin(), h_b1.end());
    P.w2.assign(h_w2.begin(), h_w2.end());
    P.b2 = 0.f;
    P.baseline = 0.f;
    dequant_all(h_packed, h_scale, h_zero, H, D, P.W1);

    OODDetector ood;
    // A small OOD anchor cache: first 256 real windows.
    size_t take = std::min<size_t>(256, (size_t)W.n);
    for (size_t k = 0; k < take; ++k) ood.known_states.push_back(W.state(order[k]));

    if (!save_neuron(args.save, P, ood)) {
        std::fprintf(stderr, "failed to save %s\n", args.save.c_str());
    } else {
        std::printf("saved %s  (%zu MB fp32-serialized, %zu OOD anchors)\n",
                    args.save.c_str(),
                    (P.W1.size() * 4 + P.b1.size() * 4 + P.w2.size() * 4) / (1<<20),
                    ood.known_states.size());
    }

    // ---- cleanup ----
    cudaFree(Wq.packed); cudaFree(Wq.scale); cudaFree(Wq.zero);
    cudaFree(d_b1); cudaFree(d_w2); cudaFree(d_cp); cudaFree(d_cn);
    cudaFree(d_phi_pos); cudaFree(d_phi_neg); cudaFree(d_absh);
    cudaFreeHost(hp_cp); cudaFreeHost(hp_cn);
    return 0;
}
