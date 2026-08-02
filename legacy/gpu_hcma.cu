// GPU DCHL trainer with ANQ 4-bit quantization.
//
// Design (matches paper v10 in how_we_implement.md):
//   - W1 stored as 4-bit values, packed 2/byte, per-row scale+zero (fp16).
//   - ANQ_BLK = D_STATE  (one block per hidden row  =>  no cross-row crosstalk).
//   - b1, w2 kept in fp32 (small, high-frequency).
//   - Cognitive state c kept in fp16 on device.
//   - Hebbian update: dequantise the row, apply local Hebbian delta, then
//     requantise back to 4-bit with a fresh row scale.  This is Local Error
//     Accumulation in the sense of anq.md: sub-quantum steps sit in the
//     dequantised buffer until they cross a quantum, then land in the 4-bit
//     store.
//
// The whole file is a monolith so that a single `nvcc` invocation builds it.

#include "hcma.hpp"
#include "windows_bin.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime.h>
#include <random>
#include <string>
#include <vector>

using namespace hcma;

// -------------------------------------------------------------------------
// ANQ 4-bit weight matrix.
//   packed: (H * D / 2) uint8 — two nibbles per byte
//   scale:  H floats — per-row (block == D_STATE)
//   zero:   H floats — per-row midpoint (dequant(q) = zero + scale * (q - 7.5))
// -------------------------------------------------------------------------
struct ANQMatrix {
    uint8_t* packed = nullptr; // device
    float*   scale  = nullptr; // device
    float*   zero   = nullptr; // device
    size_t H = 0, D = 0;
};

static void cuda_check(cudaError_t e, const char* what) {
    if (e != cudaSuccess) {
        std::fprintf(stderr, "cuda error in %s: %s\n", what, cudaGetErrorString(e));
        std::exit(1);
    }
}

__device__ __forceinline__ float anq_deq(uint8_t byte, bool hi, float scale, float zero) {
    int q = hi ? ((byte >> 4) & 0xF) : (byte & 0xF); // 0..15
    return zero + scale * ((float)q - 7.5f);
}

// Forward + Hebbian update fused per row (one block per hidden unit).
// The block dequantises its own row into shared memory, computes h_pos and
// h_neg via two dot products, applies the DCHL updates (row + w2), then
// requantises the row back into `packed` with a fresh scale/zero.
//
// Grid:  (H)         Block: (T)  with T = 256 threads
extern "C" __global__ void dchl_row_kernel(
    // weight matrix (in/out)
    uint8_t* __restrict__ W_packed, float* __restrict__ W_scale, float* __restrict__ W_zero,
    // biases (fp32, in/out)
    float* __restrict__ b1, float* __restrict__ w2,
    // inputs
    const float* __restrict__ c_pos, const float* __restrict__ c_neg,
    float inv_n_pos, float inv_n_neg,
    // output accumulators (Phi(pos), Phi(neg))
    float* __restrict__ phi_pos, float* __restrict__ phi_neg,
    float* __restrict__ abs_h_pos_sum,
    // hyperparams
    float lr_hidden, float lr_output,
    int D)
{
    extern __shared__ float sh[]; // 2*D fp32 (row buffer) + reductions
    float* row = sh;               // [D]
    float* red = sh + D;           // [T]

    const int j = blockIdx.x;
    const int t = threadIdx.x;
    const int T = blockDim.x;

    // ---- load and dequantise this row into shared memory ----
    float sc = W_scale[j], zr = W_zero[j];
    for (int i = t; i < D; i += T) {
        uint8_t byte = W_packed[(size_t)j * (D / 2) + (i / 2)];
        row[i] = anq_deq(byte, (i & 1) != 0, sc, zr);
    }
    __syncthreads();

    // ---- dot products for h_pos and h_neg ----
    float dp = 0.f, dn = 0.f;
    for (int i = t; i < D; i += T) {
        dp += row[i] * c_pos[i];
        dn += row[i] * c_neg[i];
    }
    red[t] = dp;
    __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s) red[t] += red[t + s];
        __syncthreads();
    }
    float sum_p = red[0];
    __syncthreads();
    red[t] = dn;
    __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s) red[t] += red[t + s];
        __syncthreads();
    }
    float sum_n = red[0];

    float bj = b1[j];
    float w2_old = w2[j];
    float pre_p = (sum_p + bj) * inv_n_pos;
    float pre_n = (sum_n + bj) * inv_n_neg;
    float h_p = tanhf(pre_p);
    float h_n = tanhf(pre_n);
    float err = h_p - h_n;

    // ---- per-unit Hebbian on the dequantised row (NO per-step norm) ----
    // Per-step L2-normalisation of [row, b1_j] snaps each row back to unit
    // magnitude every step and destroys the accumulated Hebbian direction —
    // this is the mechanism that produces the |h|=0.043 death-lock and the
    // dPhi sawtooth. Normalisation is deferred to a separate end-of-epoch
    // kernel (normalize_epoch_kernel) which matches the CPU path.
    for (int i = t; i < D; i += T)
        row[i] += lr_hidden * err * c_pos[i];
    if (t == 0) b1[j] = bj + lr_hidden * err;
    __syncthreads();

    // ---- w2 update (pure differential, clamp >= 0). Decay is end-of-epoch.
    if (t == 0) {
        float new_w2 = w2[j] + lr_output * (h_n - h_p);
        if (new_w2 < 0.f) new_w2 = 0.f;
        w2[j] = new_w2;
    }

    // ---- requantise: find min/max across the row, pack 4-bit ----
    float lo =  1e30f, hi = -1e30f;
    for (int i = t; i < D; i += T) {
        float v = row[i];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    red[t] = lo;
    __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s && red[t + s] < red[t]) red[t] = red[t + s];
        __syncthreads();
    }
    float row_lo = red[0];
    __syncthreads();
    red[t] = hi;
    __syncthreads();
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
        W_zero[j]  = nz;
        red[0] = ns; red[1] = nz;
    }
    __syncthreads();
    float new_scale = red[0], new_zero = red[1];
    float inv_s = 1.f / new_scale;
    // Each thread packs one full byte: nibbles from row[2k] and row[2k+1].
    for (int k = t; k < D / 2; k += T) {
        int qi0 = (int)lrintf((row[2 * k]     - new_zero) * inv_s + 7.5f);
        int qi1 = (int)lrintf((row[2 * k + 1] - new_zero) * inv_s + 7.5f);
        if (qi0 < 0) qi0 = 0; if (qi0 > 15) qi0 = 15;
        if (qi1 < 0) qi1 = 0; if (qi1 > 15) qi1 = 15;
        W_packed[(size_t)j * (D / 2) + k] = (uint8_t)(qi0 | (qi1 << 4));
    }

    // ---- output accumulators (once per block) ----
    if (t == 0) {
        atomicAdd(phi_pos, w2_old * h_p);
        atomicAdd(phi_neg, w2_old * h_n);
        atomicAdd(abs_h_pos_sum, fabsf(h_p));
    }
}

// End-of-epoch normalization kernel. One block per hidden row:
//   - dequantise into shared memory
//   - compute L2 norm of [row, b1_j]
//   - divide row and b1_j by that norm  ==>  unit-norm row (paper v10)
//   - apply w2 L2 decay (once per epoch, not per step)
//   - requantise the row
//
// Only runs at end of each epoch, so it costs O(H) launches per epoch total,
// negligible next to the O(samples * K * H) main training work.
extern "C" __global__ void normalize_epoch_kernel(
    uint8_t* __restrict__ W_packed, float* __restrict__ W_scale, float* __restrict__ W_zero,
    float* __restrict__ b1, float* __restrict__ w2,
    float w2_decay, int D)
{
    extern __shared__ float sh[];
    float* row = sh;             // [D]
    float* red = sh + D;         // [T]

    const int j = blockIdx.x;
    const int t = threadIdx.x;
    const int T = blockDim.x;

    // Dequantise this row.
    float sc = W_scale[j], zr = W_zero[j];
    for (int i = t; i < D; i += T) {
        uint8_t byte = W_packed[(size_t)j * (D / 2) + (i / 2)];
        row[i] = anq_deq(byte, (i & 1) != 0, sc, zr);
    }
    __syncthreads();

    // L2 norm of [row, b1_j].
    float loc = 0.f;
    for (int i = t; i < D; i += T) loc += row[i] * row[i];
    red[t] = loc;
    __syncthreads();
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
        w2[j] = w2[j] * (1.f - w2_decay);
    }
    __syncthreads();
    float inv = red[0];
    for (int i = t; i < D; i += T) row[i] *= inv;
    __syncthreads();

    // Requantise (new min/max of the normalised row).
    float lo =  1e30f, hi = -1e30f;
    for (int i = t; i < D; i += T) {
        float v = row[i];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    red[t] = lo;
    __syncthreads();
    for (int s = T / 2; s > 0; s >>= 1) {
        if (t < s && red[t + s] < red[t]) red[t] = red[t + s];
        __syncthreads();
    }
    float row_lo = red[0];
    __syncthreads();
    red[t] = hi;
    __syncthreads();
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
        W_zero[j]  = nz;
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

// -------------------------------------------------------------------------
// Host wrapper.
// -------------------------------------------------------------------------
static float aug_norm_host(const float* c, int D) {
    double s = 1.0;
    for (int i = 0; i < D; ++i) s += (double)c[i] * c[i];
    return (float)sqrt(s);
}

static void quantise_row_host(const float* row, int D, uint8_t* packed,
                              float& out_scale, float& out_zero) {
    float lo = row[0], hi = row[0];
    for (int i = 1; i < D; ++i) {
        if (row[i] < lo) lo = row[i];
        if (row[i] > hi) hi = row[i];
    }
    float span = hi - lo;
    out_scale = span > 1e-20f ? span / 15.f : 1e-20f;
    out_zero  = (lo + hi) * 0.5f;
    float inv = 1.f / out_scale;
    for (int i = 0; i < D; i += 2) {
        int q0 = (int)lrintf((row[i] - out_zero) * inv + 7.5f);
        int q1 = (i + 1 < D) ? (int)lrintf((row[i + 1] - out_zero) * inv + 7.5f) : 0;
        if (q0 < 0) q0 = 0; if (q0 > 15) q0 = 15;
        if (q1 < 0) q1 = 0; if (q1 > 15) q1 = 15;
        packed[i / 2] = (uint8_t)((q1 << 4) | q0);
    }
}

extern "C" void train_gpu(Potential& P, const Windows& W, int epochs,
                          float lr_hidden, float lr_output,
                          float p_random, float p_shuffle, float p_splice,
                          size_t samples_per_epoch, unsigned seed,
                          const char* save_path,
                          int k_neg, int warmup_epochs)
{
    int D = (int)P.D;
    int H = (int)P.H;
    if (D & 1) { std::fprintf(stderr, "D must be even for 4-bit packing\n"); return; }
    if (D != (int)W.win) {
        std::fprintf(stderr, "D (%d) != window size (%u)\n", D, W.win);
        return;
    }

    // ---- allocate ANQ matrix on device, quantise from host P.W1 ----
    ANQMatrix Wq;
    Wq.H = H; Wq.D = D;
    size_t packed_bytes = (size_t)H * (D / 2);
    cuda_check(cudaMalloc(&Wq.packed, packed_bytes), "malloc packed");
    cuda_check(cudaMalloc(&Wq.scale,  (size_t)H * 4), "malloc scale");
    cuda_check(cudaMalloc(&Wq.zero,   (size_t)H * 4), "malloc zero");
    {
        std::vector<uint8_t> hp(packed_bytes);
        std::vector<float> hs(H), hz(H);
        for (int j = 0; j < H; ++j)
            quantise_row_host(&P.W1[(size_t)j * D], D, &hp[(size_t)j * (D / 2)], hs[j], hz[j]);
        cuda_check(cudaMemcpy(Wq.packed, hp.data(), packed_bytes, cudaMemcpyHostToDevice), "H2D packed");
        cuda_check(cudaMemcpy(Wq.scale,  hs.data(), (size_t)H * 4, cudaMemcpyHostToDevice), "H2D scale");
        cuda_check(cudaMemcpy(Wq.zero,   hz.data(), (size_t)H * 4, cudaMemcpyHostToDevice), "H2D zero");
    }
    // ---- biases on device ----
    float *d_b1 = nullptr, *d_w2 = nullptr;
    cuda_check(cudaMalloc(&d_b1, H * 4), "malloc b1");
    cuda_check(cudaMalloc(&d_w2, H * 4), "malloc w2");
    cuda_check(cudaMemcpy(d_b1, P.b1.data(), H * 4, cudaMemcpyHostToDevice), "H2D b1");
    cuda_check(cudaMemcpy(d_w2, P.w2.data(), H * 4, cudaMemcpyHostToDevice), "H2D w2");
    // ---- input buffers, output accumulators ----
    float *d_cp = nullptr, *d_cn = nullptr;
    cuda_check(cudaMalloc(&d_cp, D * 4), "malloc cp");
    cuda_check(cudaMalloc(&d_cn, D * 4), "malloc cn");
    float *d_phi_pos = nullptr, *d_phi_neg = nullptr, *d_absh = nullptr;
    cuda_check(cudaMalloc(&d_phi_pos, 4), "malloc phi_pos");
    cuda_check(cudaMalloc(&d_phi_neg, 4), "malloc phi_neg");
    cuda_check(cudaMalloc(&d_absh,    4), "malloc absh");

    size_t vram = ((size_t)packed_bytes + (size_t)H * 8 + (size_t)H * 8 + (size_t)D * 8);
    std::printf("[gpu] D=%d H=%d ANQ packed=%.1f MB total weights VRAM=%.1f MB\n",
                D, H, packed_bytes / 1e6, vram / 1e6);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uni(0.f, 1.f);
    std::vector<uint32_t> order(W.n);
    for (uint32_t i = 0; i < W.n; ++i) order[i] = i;

    int T = 256;
    size_t sh_bytes = (size_t)(D + T) * 4;

    size_t n_per_epoch = samples_per_epoch ? samples_per_epoch : W.n;
    int K = std::max(1, k_neg);
    std::printf("[gpu-cfg] K=%d warmup=%d p_rand=%.2f p_shuf=%.2f p_spl=%.2f\n",
                K, warmup_epochs, p_random, p_shuffle, p_splice);

    // Pinned host buffers so H2D copies can overlap with kernel execution.
    float *hp_cp = nullptr, *hp_cn = nullptr;
    cuda_check(cudaHostAlloc((void**)&hp_cp, D * 4, cudaHostAllocDefault), "pinned cp");
    cuda_check(cudaHostAlloc((void**)&hp_cn, D * 4, cudaHostAllocDefault), "pinned cn");
    float zero = 0.f;
    // The kernel already atomicAdds into d_phi_pos, d_phi_neg, d_absh, so we
    // simply reset them once per epoch and read once per epoch — cutting the
    // per-sample syncs that were costing ~200us each (~72K syncs/epoch at K=8).

    auto t0 = std::chrono::steady_clock::now();
    for (int ep = 1; ep <= epochs; ++ep) {
        std::shuffle(order.begin(), order.end(), rng);

        // Curriculum: after warmup, drop random+shuffle, go pure hard-splice.
        float pr = p_random, ps = p_shuffle;
        if (warmup_epochs > 0 && ep > warmup_epochs) { pr = 0.f; ps = 0.f; }

        // Reset device accumulators once for the epoch.
        cuda_check(cudaMemcpy(d_phi_pos, &zero, 4, cudaMemcpyHostToDevice), "zero pp");
        cuda_check(cudaMemcpy(d_phi_neg, &zero, 4, cudaMemcpyHostToDevice), "zero pn");
        cuda_check(cudaMemcpy(d_absh,    &zero, 4, cudaMemcpyHostToDevice), "zero h");

        size_t seen = 0;
        for (size_t idx_i = 0; idx_i < n_per_epoch; ++idx_i) {
            uint32_t idx = order[idx_i % order.size()];
            Vec c_pos = W.state(idx);
            float inv_np = 1.f / aug_norm_host(c_pos.data(), D);
            std::memcpy(hp_cp, c_pos.data(), D * 4);
            cuda_check(cudaMemcpyAsync(d_cp, hp_cp, D * 4, cudaMemcpyHostToDevice, 0), "H2D cp");

            for (int k = 0; k < K; ++k) {
                NegKind kind;
                float r = uni(rng);
                if (r < pr) kind = NegKind::Random;
                else if (r < pr + ps) kind = NegKind::Shuffle;
                else kind = NegKind::Splice;
                size_t partner = order[(idx_i + 1 + k * 13) % order.size()];
                Vec c_other = W.state(partner);
                Vec c_neg = make_negative(c_pos, kind, &c_other, rng);

                float inv_nn = 1.f / aug_norm_host(c_neg.data(), D);
                std::memcpy(hp_cn, c_neg.data(), D * 4);
                cuda_check(cudaMemcpyAsync(d_cn, hp_cn, D * 4, cudaMemcpyHostToDevice, 0), "H2D cn");

                dchl_row_kernel<<<H, T, sh_bytes>>>(
                    Wq.packed, Wq.scale, Wq.zero,
                    d_b1, d_w2,
                    d_cp, d_cn, inv_np, inv_nn,
                    d_phi_pos, d_phi_neg, d_absh,
                    lr_hidden, lr_output, D);
            }
            ++seen;
        }
        // End of epoch: (1) wait for all training kernels to finish,
        // (2) run the normalisation kernel (row L2 + w2 decay) once,
        // (3) read the epoch accumulators.  This is the CPU end_of_epoch()
        // moved to the device — deferring norm to per-epoch is what breaks
        // the |h|=0.043 death-lock the per-step normaliser was causing.
        cuda_check(cudaDeviceSynchronize(), "sync train");
        normalize_epoch_kernel<<<H, T, sh_bytes>>>(
            Wq.packed, Wq.scale, Wq.zero, d_b1, d_w2, 1e-3f, D);
        cuda_check(cudaGetLastError(), "launch normalize");
        cuda_check(cudaDeviceSynchronize(), "sync normalize");
        float sum_pos = 0, sum_neg = 0, sum_h = 0;
        cuda_check(cudaMemcpy(&sum_pos, d_phi_pos, 4, cudaMemcpyDeviceToHost), "D2H pp");
        cuda_check(cudaMemcpy(&sum_neg, d_phi_neg, 4, cudaMemcpyDeviceToHost), "D2H pn");
        cuda_check(cudaMemcpy(&sum_h,   d_absh,    4, cudaMemcpyDeviceToHost), "D2H h");

        double denom = (double)seen * (double)K;
        double phi_pos = (double)sum_pos / denom;
        double phi_neg = (double)sum_neg / denom;
        P.baseline += 0.01f * (float)((phi_pos + phi_neg) * 0.5 - P.baseline);
        std::printf("epoch %3d  Phi(coh)=%9.2f  Phi(incoh)=%9.2f  dPhi=%9.2f  |h|=%.3f\n",
                    ep, phi_pos, phi_neg, phi_neg - phi_pos,
                    (double)sum_h / (denom * (double)H));
    }
    cudaFreeHost(hp_cp); cudaFreeHost(hp_cn);
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();

    // ---- pull weights back to host (dequantise from ANQ) ----
    std::vector<uint8_t> hp(packed_bytes);
    std::vector<float> hs(H), hz(H);
    cuda_check(cudaMemcpy(hp.data(), Wq.packed, packed_bytes, cudaMemcpyDeviceToHost), "D2H packed");
    cuda_check(cudaMemcpy(hs.data(), Wq.scale,  H * 4, cudaMemcpyDeviceToHost), "D2H scale");
    cuda_check(cudaMemcpy(hz.data(), Wq.zero,   H * 4, cudaMemcpyDeviceToHost), "D2H zero");
    cuda_check(cudaMemcpy(P.b1.data(), d_b1, H * 4, cudaMemcpyDeviceToHost), "D2H b1");
    cuda_check(cudaMemcpy(P.w2.data(), d_w2, H * 4, cudaMemcpyDeviceToHost), "D2H w2");
    for (int j = 0; j < H; ++j) {
        for (int i = 0; i < D; ++i) {
            uint8_t byte = hp[(size_t)j * (D / 2) + (i / 2)];
            int q = (i & 1) ? ((byte >> 4) & 0xF) : (byte & 0xF);
            P.W1[(size_t)j * D + i] = hz[j] + hs[j] * ((float)q - 7.5f);
        }
    }
    cudaFree(Wq.packed); cudaFree(Wq.scale); cudaFree(Wq.zero);
    cudaFree(d_b1); cudaFree(d_w2);
    cudaFree(d_cp); cudaFree(d_cn);
    cudaFree(d_phi_pos); cudaFree(d_phi_neg); cudaFree(d_absh);

    OODDetector ood;
    size_t take = std::min<size_t>((size_t)512, (size_t)W.n);
    for (size_t i = 0; i < take; ++i) ood.known_states.push_back(W.state(order[i]));

    std::string save = save_path;
    if (!save_neuron(save, P, ood))
        std::fprintf(stderr, "failed to save %s\n", save.c_str());
    else
        std::printf("\n[gpu] trained in %.1fs; saved %s (%zu cached states)\n",
                    secs, save.c_str(), ood.known_states.size());
}
