// physics.hpp — Layer 1 Substrate. Pure scalar arithmetic in tick().
//
// The tick loop contains exactly ONE branch — the startup safety guard
// `if (!graph_ptr) return`. Every other extension (NTDA, TSDA, RIFA,
// ZeroGrad, HNCA) is expressed as a scalar multiplier or a soft-
// saturating nonlinearity (tanh, sigmoid). There are no thresholds,
// no countdowns, no clamps, no if-branches inside the physics update.
//
// Modules and how they express as pure math:
//
//   PRM      : F = k · (x_a·polarity − x_b) − c · (v_a − v_b); v += F/m·dt; x += v·dt
//   MIMS     : k, c, polarity set once at bind from relation type
//   NTDA     : damp_scale, drive_scale, plastic_rate multiply c, F_drive, η
//   TSDA     : sal = tanh(sal·decay + gain·|v|)   — soft-saturated integrator
//   RIFA IC  : tension = tension·γ + β·Σ½mv²      — continuous energy accumulator
//              coherence = sigmoid(tension·½)
//              identity  = tanh(identity + ε·tension·coherence)
//   RIFA SIL : input_power_avg = ip·γ + β·Σ|F_drive|
//              endurance = tanh(endurance·(1−δ) + ε·tension_integral)
//   ZeroGrad : plasticity_delta = pd·hebbian_decay + plastic_rate·v_a·v_b
//   ANQ LEA  : continuous integrator replaces threshold-commit
//   HNCA     : θ = fmod(θ + (ω + K_norm·Σ sin(θ_j−θ_k)·A_j)·dt, 2π); no self-skip
//              because sin(0)·A_self = 0 contributes nothing.
//
// Phase inverter, envelope, saturations — all multiplication. No if.

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace unified {

// ---------- state ---------------------------------------------------------

struct Cavity {
    // PRM observable (external readers see these). Under SRG they are
    // derived at end-of-tick as x = x_E − x_I, v = v_E − v_I. Under
    // SRG off, x_I / v_I stay 0 and observable == E port.
    float mass          = 1.0f;
    float displacement  = 0.0f;
    float velocity      = 0.0f;
    // SRG dual-port integrators (research-paper/srg.md).
    // Waves entering the E-port and I-port never sum in the same
    // storage — they accumulate in separate channels and only combine
    // at readout. Excitatory Hebbian updates land in v_E products,
    // inhibitory in v_I products, so signed cancellation cannot occur.
    float x_E           = 0.0f;
    float v_E           = 0.0f;
    float x_I           = 0.0f;
    float v_I           = 0.0f;
    // Continuous drive (no countdown)
    float drive_freq    = 0.0f;
    float drive_amp     = 0.0f;
    float drive_envelope= 0.0f;
    float drive_decay   = 1.0f;
    bool  was_driven    = false;
    float salience      = 0.0f;
    int   cluster_id    = 0;
    float base_freq     = 10.0f;
    // RB+G generation counter. Original graph cavities are gen 0.
    // Children of a fission are gen (parent.generation + 1).
    // Fission is only eligible for gen < g_max.
    int   generation    = 0;
    // CMD Thermodynamic Habituation — per-cavity "heat".
    //   dT/dt = α·|v| - β·T
    // Words that fire on every input (stopwords) heat up past T_crit
    // and lose their tunnel coupling — Curie-point analog. Rare words
    // (concepts) stay cold and consolidate strong bonds.
    float heat_T        = 0.0f;
    // CPME (research-paper/cpme.md) per-cavity state.
    //   t_arrival     — §1.1 chirality timestamp (sim_time at last drive)
    //   env_ema       — §1.2 low-pass amplitude envelope for momentum
    //   env_ema_prev  — §1.2 previous-tick envelope, so P = env - env_prev
    //   acoustic_mass — §1.3 mass increment from external drive
    float t_arrival     = -1.0f;
    float env_ema       = 0.0f;
    float env_ema_prev  = 0.0f;
    float acoustic_mass = 0.0f;
};

// RB+G alias ledger entry — role of a cavity within a fission group.
enum class AliasRole : int { RootAlias = 0, Alpha = 1, Beta = 2 };
struct AliasEntry {
    AliasRole role      = AliasRole::RootAlias;
    int       parent    = -1;   // parent cavity id (self for RootAlias)
    int       partner   = -1;   // sibling id (Beta if this is Alpha, vice versa)
    int       label_key = -1;   // parent's graph label index; -1 if not a root
};

// Record of one fission event, appended to data/fission_ledger.jsonl.
struct FissionRecord {
    int    tick_index;
    int    parent_id;
    std::string parent_label;
    float  I;
    float  tau;
    int    streak;
    int    child_alpha;
    int    child_beta;
    int    incident_alpha;
    int    incident_beta;
    float  drive_split_factor;   // 1/√2 in RB+G
};

struct Tunnel {
    int   cavity_a          = -1;
    int   cavity_b          = -1;
    // MIMS material
    float stiffness         = 0.5f;
    float damping           = 0.02f;
    float polarity          = 1.0f;    // +1 excitatory, −1 inhibitory (phase invert)
    // ZeroGrad plasticity
    float base_stiffness    = 0.5f;
    float plasticity_delta  = 0.0f;    // continuous Hebbian integrator
    // Phase 2.2 — Band-Pass Transfer Function.
    // When resonance_omega > 0, force is gated by a Q-factored Gaussian
    // centred on the drive frequency of cavity_a. Zero = pass-through
    // (unchanged behaviour for gen-0 tunnels and non-L6-A materials).
    float resonance_omega   = 0.0f;    // rad/s target frequency (0 = disabled)
    float Q_factor          = 3.0f;    // sharpness (dimensionless)
    // VGHT §1.2 — Voltage-gated tunnel. When 1, the tunnel force is
    // scaled by σ(κ·(A_a·A_b − θ)), so the bridge is inert unless
    // both endpoints are simultaneously excited above threshold.
    // Set once at bind time for every cross-cluster tunnel and every
    // L6-A tunnel; the tick loop never inspects the flag as a branch —
    // it multiplies the force by `gated` (0 or 1) and adds the
    // ungated-fraction back, which is scalar arithmetic.
    float voltage_gated     = 0.0f;
    // CPSB §2.1 — per-tunnel gauge phase offset. Kuramoto-style
    // coupling term becomes sin(θ_b − θ_a − Ψ), so equilibrium is
    // θ_a − θ_b = Ψ. For verb-like directed relations, Ψ = −π/3 in
    // the paper's convention → subject leads object by π/3.
    // Symmetric / taxonomic relations use Ψ = 0 (recovers standard
    // synchronizing Kuramoto). Set once at bind by psi_for(rel_id).
    float psi_target        = 0.0f;
    int   rel_id            = -1;   // source relation, for derived materials
};

struct MacroNode {
    std::vector<int> cavity_ids;
    float natural_omega = 10.0f;
    float phase         = 0.0f;
    float amplitude     = 0.0f;
    float inv_size      = 1.0f;        // precomputed 1/|cavity_ids|
};

// Phase 2.1 — Syntax Macro-Layer.
// One SyntaxSlot per grammatical role (Subject, Verb, Object, ...).
// Fixed natural_omega chosen as harmonic ratios so Kuramoto locks are
// orthogonal. Fixed target_phase distributed as 2π k / N around the
// unit circle so the readout head can distinguish them by phase alone.
struct SyntaxSlot {
    float natural_omega = 1.0f;         // rad/s
    float target_phase  = 0.0f;         // 0, 2π/N, 4π/N, ...
    float phase         = 0.0f;         // Kuramoto-integrated
    float amplitude     = 0.0f;         // sum of phase-close cluster amplitudes
    std::string role;                    // "subject" | "verb" | "object" | ...
    int   locked_cavity_id = -1;        // winner at last readout
};

// CMD Byte-Wave Layer — one oscillator per byte value (the cochlea).
// Byte lanes are NOT graph cavities. They are the sensory epithelium:
// raw input bytes drive lanes, and lanes reach the concept field ONLY
// through Hebbian-learned binding weights (byte_bind_w) that start at
// zero. At birth a byte makes no concept noise — experience wires it.
// This replaces the earlier hash-mod injection, which pumped every
// byte into a random concept cavity and made the readout pure noise.
struct ByteLane {
    float displacement   = 0.0f;
    float velocity       = 0.0f;
    float drive_amp      = 0.0f;
    float drive_envelope = 0.0f;
    float drive_decay    = 1.0f;
    float natural_omega  = 3.0f;   // 3.0 + b/256 — rides the L6-A band
    float k              = 1.0f;   // lane spring
    float damp           = 0.05f;  // lane damping
    // MRS (research-paper/mrs.md) per-lane state. Mirrors the CPME
    // per-cavity fields exactly, one layer down:
    //   §1.2 t_arrival    — lane chirality timestamp
    //   §1.4 env_ema      — envelope momentum for the byte sweep
    float t_arrival      = -1.0f;
    float env_ema        = 0.0f;
    float env_ema_prev   = 0.0f;
};

// MRS §1.3 — lane↔lane gauge tunnel. Unlike concept tunnels, Ψ is not
// assigned from a relation table; it is LEARNED as the running
// circular mean of observed phase differences. psi_r/psi_i are the
// unnormalised circular-mean accumulators; psi is their argument.
struct LaneTunnel {
    int   lane_a = -1;
    int   lane_b = -1;
    float weight = 0.0f;      // phase-consistent Hebbian weight
    float psi    = 0.0f;      // learned target phase offset
    float psi_r  = 0.0f;      // circular mean accumulator (cos part)
    float psi_i  = 0.0f;      // circular mean accumulator (sin part)
};

struct Neurotransmitters {
    float dopamine  = 1.0f;
    float cortisol  = 1.0f;
    float serotonin = 1.0f;
};

struct IntegrityCore {
    float tension          = 0.0f;
    float coherence        = 0.5f;
    float identity_strength= 0.5f;
};
struct SilenceIntegration {
    float endurance        = 1.0f;
    float tension_integral = 0.0f;
    float input_power_avg  = 0.0f;
};

// CMD Verb-material weight. Relations that describe an action performed
// BY a subject ON an object are verb-like: their incident cavities gain
// a small ω-boost toward the verb-slot's natural frequency, so the
// physics — not any rule — pulls action-participating clusters into
// the middle slot at readout time. Purely taxonomic / state relations
// return 0 (no pull). Runs ONCE at bind, alongside material_for.
inline float verb_pull_for(int rel_id) {
    switch (rel_id) {
        case REL_ACTIVATES:
        case REL_INHIBITS:
        case REL_UPREGULATES:
        case REL_DOWNREGULATES:
        case REL_BINDS_TO:
        case REL_METABOLIZES:
        case REL_TREATS:
        case REL_CAUSES:
        case REL_CAUSES_DISEASE:
        case REL_EXPRESSED_IN:
            return 1.0f;
        // Code-structure verb-material. `calls`, `returns`, `mutates`,
        // `imports`, `precedes` are all actions performed BY a subject
        // ON an object, so their clusters rise to their band-top and
        // win the verb position under the phase sweep.
        case REL_CALLS:
        case REL_RETURNS:
        case REL_MUTATES:
        case REL_IMPORTS:
        case REL_PRECEDES:
            return 1.0f;
        default:
            return 0.0f;
    }
}

// CPSB §2.1 — Gauge phase offset per relation type. Directed action
// relations lead → trail with Ψ = −π/3 (subject leads object by π/3
// after the gauge equilibrates). Symmetric/taxonomic relations use
// Ψ = 0 → standard synchronizing Kuramoto behaviour. Runs ONCE at
// bind time alongside material_for and verb_pull_for.
inline float psi_for(int rel_id) {
    const float PI_3 = 1.0471975512f;  // π/3
    switch (rel_id) {
        case REL_ACTIVATES:
        case REL_INHIBITS:
        case REL_UPREGULATES:
        case REL_DOWNREGULATES:
        case REL_METABOLIZES:
        case REL_TREATS:
        case REL_CAUSES:
        case REL_CAUSES_DISEASE:
        case REL_EXPRESSED_IN:
        case REL_BINDS_TO:
        case REL_IMPLIES:
            return -PI_3;
        // Code-structure directional relations. REL_PRECEDES gets a
        // LARGER offset (−2π/3) because dataflow order is a stronger
        // constraint than a generic action: `open` must lead `close`
        // by more phase than a subject leads an object, so the
        // phase-collapse sweep cannot interleave them.
        case REL_PRECEDES:
            return -2.0f * PI_3;
        case REL_CALLS:
        case REL_RETURNS:
        case REL_MUTATES:
        case REL_IMPORTS:
            return -PI_3;
        default:
            return 0.0f;
    }
}

// MIMS material assignment. Runs ONCE at bind. Returns polarity as
// float (+1 or −1) so tick() never inspects a bool.
inline void material_for(int rel_id, float& k, float& c, float& polarity) {
    polarity = 1.0f;
    switch (rel_id) {
        case REL_INHIBITS:        k = 1.8f; c = 0.010f; polarity = -1.0f; return;
        case REL_DOWNREGULATES:   k = 1.6f; c = 0.010f; polarity = -1.0f; return;
        case REL_NOT:             k = 1.4f; c = 0.020f; polarity = -1.0f; return;
        case REL_ACTIVATES:       k = 1.8f; c = 0.010f;                   return;
        case REL_UPREGULATES:     k = 1.6f; c = 0.010f;                   return;
        case REL_BINDS_TO:        k = 2.0f; c = 0.008f;                   return;
        case REL_METABOLIZES:     k = 1.2f; c = 0.020f;                   return;
        case REL_EXPRESSED_IN:    k = 1.0f; c = 0.025f;                   return;
        case REL_TREATS:          k = 1.1f; c = 0.020f;                   return;
        case REL_PART_OF_PATHWAY: k = 1.0f; c = 0.030f;                   return;
        case REL_CAUSES:          k = 1.1f; c = 0.020f;                   return;
        case REL_CAUSES_DISEASE:  k = 1.1f; c = 0.020f;                   return;
        case REL_PROPORTIONAL_TO: k = 2.2f; c = 0.005f;                   return;
        case REL_MIRROR_OF:       k = 1.5f; c = 0.010f;                   return;
        case REL_DERIVES_FROM:    k = 1.3f; c = 0.015f;                   return;
        case REL_IS_A:            k = 0.35f; c = 0.10f;                   return;
        case REL_HAS:             k = 0.40f; c = 0.09f;                   return;
        case REL_PART_OF:         k = 0.45f; c = 0.08f;                   return;
        case REL_EQUALS:          k = 0.60f; c = 0.05f;                   return;
        case REL_IMPLIES:         k = 0.80f; c = 0.04f;                   return;
        // Code-structure materials. Dataflow (`precedes`) is the
        // stiffest coupling in the substrate — a hard ordering
        // constraint, near-lossless transmission, minimal damping.
        // `calls` is steel-like (strong mechanistic link); `takes_arg`
        // and `returns` are moderately stiff type constraints;
        // `imports` is loose (file-level, low-pass).
        case REL_PRECEDES:        k = 2.4f;  c = 0.004f;                  return;
        case REL_CALLS:           k = 1.9f;  c = 0.010f;                  return;
        case REL_RETURNS:         k = 1.5f;  c = 0.012f;                  return;
        case REL_TAKES_ARG:       k = 1.4f;  c = 0.015f;                  return;
        case REL_MUTATES:         k = 1.6f;  c = 0.012f;                  return;
        case REL_IMPORTS:         k = 0.45f; c = 0.08f;                   return;
        default:                  k = 0.50f; c = 0.05f;                   return;
    }
}

// ---------- engine -------------------------------------------------------

class PhysicsEngine {
public:
    std::vector<Cavity>    cavities;
    std::vector<Tunnel>    tunnels;
    std::vector<MacroNode> macro_nodes;
    Neurotransmitters      nt;
    IntegrityCore          ic;
    SilenceIntegration     sil;
    const SI*              graph_ptr = nullptr;

    // Constants
    float dt                       = 0.02f;
    float sim_time                 = 0.0f;
    float friction                 = 0.006f;
    float mass_min                 = 0.85f;
    float mass_max                 = 1.20f;
    // Bind-time RNG seed. Exposed so multi-seed reproducibility runs
    // can vary just this without touching any parameter.
    unsigned bind_seed             = 0xCAF17E00u;
    // TSDA
    float salience_gain            = 0.05f;
    float salience_decay           = 0.995f;
    // ZeroGrad — rate raised 50× and decay slowed 10× so consolidated
    // stiffness actually persists long enough to affect recall.
    float hebbian_rate             = 0.05f;
    float hebbian_decay            = 0.99995f;
    float plasticity_cap           = 1.5f;
    // HNCA (precomputed at bind)
    float macro_coupling           = 0.3f;
    float macro_coupling_norm      = 0.3f;
    // RIFA
    float rifa_tension_gain        = 0.02f;
    float sil_decay                = 0.001f;

    // RB+G — Gated Recursive Bifurcation. Extension of RB
    // (research-paper/rb.md) addressing four documented failure modes.
    //
    //   generation cap  → children of fissions are gen>0 and ineligible
    //                     for further fission, preventing cascade.
    //   alias ledger    → parallel to cavities[], never touches
    //                     si.graph.by_label; child labels are synthetic
    //                     "<parent>.α" / "<parent>.β".
    //   fan-out 1/√2    → energy-conserving parent-alias drive:
    //                     each child receives amp/√2, so total injected
    //                     energy equals a single-cavity drive at amp.
    //   binding tunnel  → learned k (init 1.0, polarity 0.0, in [0.5,2.0]).
    //                     Empirically converges to a coordination
    //                     stiffness rather than the fiat k=2.5 of RB.
    //   audit trail     → every fission appended to fission_ledger.
    std::vector<AliasEntry>       alias_ledger;      // parallel to cavities
    std::unordered_map<int,std::pair<int,int>> fission_map; // root_id → {alpha,beta}
    std::vector<FissionRecord>    fission_records;
    int   g_max              = 1;      // no fission beyond gen 0
    float rbg_bind_k_init    = 1.0f;
    float rbg_bind_pol_init  = 0.0f;
    float rbg_fanout_factor  = 0.70710678f; // 1/√2
    int   rbg_tick_counter   = 0;      // for audit records
    // RB+G ablation levers (exposed for the ablation harness).
    //   rbg_alias_route_enabled : when false, drive() bypasses fission_map,
    //                             so a probe on the fissioned parent lands
    //                             on the zeroed ghost cavity and cannot
    //                             reach the children. This isolates the
    //                             contribution of alias routing.
    //   rbg_binding_enabled     : when false, fission_locked() skips the
    //                             α↔β binding tunnel. This isolates the
    //                             contribution of the learned coordination
    //                             spring. Alpha and beta remain connected
    //                             through their rerouted external tunnels
    //                             but have no direct coupling.
    bool  rbg_alias_route_enabled = true;
    bool  rbg_binding_enabled     = true;

    // Phase 1.1 — Active Wavefront Tracking.
    // touched_cavities[i] == quiet_tick_counter for cavity i. When
    // sparse_tick_enabled the structural tick iterates only cavities
    // with quiet_ticks < K_idle. drive() resets quiet_ticks to 0 on the
    // driven cavity; physics tick() resets it whenever an incident
    // tunnel delivers force above wavefront_eps.
    bool  sparse_tick_enabled = true;   // Phase 1.1 — engine self-decides
    // CMD Thermodynamic Habituation constants.
    float heat_alpha  = 0.05f;          // heat accumulation from |v|
    float heat_beta   = 0.02f;          // passive cooling rate
    float T_crit      = 3.0f;           // Curie point — over this, coupling collapses

    // VGHT constants (research-paper/vght.md).
    // §1.1 Domain Sub-Bands: B orthogonal frequency bands over
    //      [ω_lo, ω_hi], one per macro-cluster (mod B).
    int   vght_bands       = 6;
    float vght_omega_lo    = 8.0f;
    float vght_omega_hi    = 32.0f;
    // §1.2 Voltage-gated tunnels: sigmoidal conductance
    //      C = σ(κ · (A_a·A_b − θ_crit)). Bind marks every
    //      cross-cluster tunnel and every L6-A tunnel as gated.
    float vght_gate_kappa  = 12.0f;
    float vght_gate_theta  = 0.05f;
    // §1.3 Global viscosity / avalanche control (GABA analog).
    //      μ_eff = μ_0 + k · (1 − exp(−E_total / E_scale)).
    //      Bounded, cannot diverge.
    float vght_k_avalanche = 0.10f;
    float vght_E_scale     = 5.0f;
    // Rolling last-tick E_total for observers / debug dumps.
    float vght_last_Etotal = 0.0f;

    // CSAT (research-paper/CSAT.md §2.2.2) — chaotic metastability.
    //
    // A Lorenz system integrated alongside the field supplies escape
    // TIMING; CPSB supplies escape direction. The coupling is
    // per-cavity and gated on heat:
    //
    //   mu_i = mu_base + k_aval*(...) + gamma * x_hat * tanh(T_i/T_crit)
    //
    // Coupling to T_i is what makes this work. The paper's first
    // formulation modulated GLOBAL viscosity, which is provably
    // rank-preserving (CSAT §2.2.1): scaling every velocity by the
    // same factor leaves all amplitude ratios unchanged, so the
    // emission arg-max cannot move. Only a per-cavity coupling breaks
    // that symmetry — and heat is the right variable, since it already
    // means "this cavity has dominated for a long time".
    //
    // tau slows the attractor: forward-Euler Lorenz is unstable for
    // dt >= 0.01 and the substrate runs dt = 0.02. x is normalised by
    // 20 because the raw coordinate spans roughly [-20, +20] and an
    // unnormalised term drives friction negative (anti-damping).
    bool  csat_enabled   = false;   // NOT default-on: see CSAT §4.1
    float csat_x = 0.1f, csat_y = 0.0f, csat_z = 0.0f;
    float csat_sigma = 10.0f, csat_rho = 28.0f, csat_beta = 2.6666667f;
    float csat_tau   = 0.30f;       // time-scale factor (stability)
    float csat_gamma = 0.04f;       // gain on normalised x

    // CPME constants (research-paper/cpme.md §4).
    // §1.2 momentum EMA smoothing. β = 0.995 gives half-life ≈ 138
    // ticks — well above the drive-frequency oscillation period
    // (~100 ticks at ω_drive = 3 rad/s), so env_ema tracks the true
    // amplitude envelope rather than the raw oscillation. Original
    // β = 0.9 (half-life ≈ 7 ticks) was faster than the oscillation
    // and made ReLU(P) fire on the rising half of every cycle,
    // defeating the momentum filter.
    float cpme_env_beta       = 0.995f;
    // §1.3 acoustic-mass tuning (retuned after empirical regression:
    // original values gave μ ≈ 0.84 at readout time → no effective
    // mute → driven cavities echoed). New values:
    //   gamma = 2.0  → accumulates much faster during drive window
    //   delta = 0.02 → persists across the 200-tick propagation window
    //   scale = 0.15 → small masses cut the mask sharply
    // Combined effect: driven cavity mask drops to ≲ 0.05 at readout,
    // so lone-driven-cavity turns yield honest silence unless there
    // is a downstream cavity with amplitude above the absolute floor.
    float cpme_mass_gamma     = 2.0f;
    float cpme_mass_delta     = 0.02f;
    float cpme_mass_scale     = 0.15f;
    float cpme_L_thresh       = 0.5f;    // §1.4 settle threshold
    float cpme_L_scale        = 0.2f;    // §1.4 settle sharpness
    float cpme_R_lambda       = 0.98f;   // §1.5 coherence EMA decay
    int   cpme_sweep_K        = 12;      // §1.6 sweep resolution
    float cpme_sweep_kappa    = 4.0f;    // §1.6 Gaussian window sharpness
    // §1.6 emission floor. TWO floors are enforced in readout:
    //   emit_floor      — fraction of the *unmuted-peak amplitude*
    //                     (not w0); prevents muted driven cavities
    //                     from setting the reference and self-passing
    //                     it. Fraction of raw |x_i|, no mask applied.
    //   emit_abs_floor  — absolute w0 threshold. Guarantees genuine
    //                     silence rather than parroting when the field
    //                     has only one strong cavity and it is muted.
    float cpme_emit_floor     = 0.05f;
    float cpme_emit_abs_floor = 0.02f;
    float cpme_base_omega     = 3.0f;    // §1.6 ω₀ used in atan2 phase
    // CPME observables (updated per tick).
    float cpme_last_L         = 0.0f;    // §1.4 injection load
    float cpme_last_Ksettle   = 1.0f;    // §1.4 slot coupling
    float cpme_R_local        = 0.0f;    // §1.5 localized coherence
    float cpme_R_baseline     = 0.0f;    // §1.5 EMA baseline
    float cpme_delta_R        = 0.0f;    // §1.5 rise above baseline

    // CPSB constants (research-paper/cpsb.md).
    // K_gauge sets the strength of the phase-alignment torque added
    // to every tunnel; small enough to remain a perturbation to the
    // Newtonian spring dynamics but large enough to drive phase
    // relationships toward Ψ_target over ~10-30 ticks.
    float cpsb_K_gauge        = 0.15f;
    // CPME-II (research-paper/cpme_ii.md) — α_phi blends the
    // phase-coincidence factor with an unconditional Hebbian floor.
    // 0 = parent rule, 1 = fully phase-gated, 0.6 = default balance.
    float cpme_ii_alpha_phi   = 0.6f;

    // MRS constants (research-paper/mrs.md).
    // Byte-level language construction: bidirectional byte↔concept
    // coupling + learned lane↔lane phase offsets + byte-phase sweep.
    // DEFAULT OFF — MRS emits babble until lane offsets consolidate
    // (paper §5.1), so the label-emission path stays primary until
    // the substrate has had enough exposure. This is the second
    // documented exception to "all defaults ON", after CSAT.
    bool  mrs_enabled         = false;
    float mrs_rev_gain        = 0.4f;    // §1.1 concept→lane transpose gain
    float mrs_lane_hebb_rate  = 0.02f;   // §1.3 lane Hebbian rate
    float mrs_lane_gauge      = 0.10f;   // §1.3 lane gauge torque
    float mrs_psi_lambda      = 0.98f;   // §1.3 circular-mean decay
    float mrs_lane_cap        = 0.8f;    // §1.3 lane weight tanh cap
    int   mrs_sweep_K         = 24;      // §1.4 byte sweep resolution
    float mrs_sweep_kappa     = 8.0f;    // §1.4 Gaussian window
    float mrs_emit_floor      = 0.08f;   // §1.4 byte emission floor
    int   mrs_refractory      = 3;       // §1.4 sweep steps a lane is
                                         // suppressed after emitting
    // §1.3 lane tunnel storage. Sparse — created on observed byte
    // adjacency at the I/O boundary via bind_lane_pair().
    std::vector<LaneTunnel>    lane_tunnels;
    std::unordered_map<int,int> lane_pair_index;   // a*256+b → index

    // MRS §4b.1 — BIGRAM SENSORY LANES.
    //
    // The 256 single-byte lanes cannot carry order: `metformin` and
    // `metfromin` have identical byte sets, so no weighting over that
    // set can separate them (measured, MRS §4b). Ordered pairs supply
    // the missing information — those two words share 5 of 8 bigrams
    // and differ in 3, so the transposition becomes physically visible.
    //
    // Lane id = a*256 + b for the ordered pair (a,b). The space is
    // 256² = 65536, but occupancy is sparse (only pairs that actually
    // occur are created), so storage is a hash map, not a dense array.
    //
    // This adds oscillator lanes. It adds no loss function, no
    // gradient, no fitted parameter, no attention, no softmax. A
    // binding weight is COMPUTED from a counted property of the
    // lexicon by a stated formula — not fitted by optimisation.
    std::unordered_map<int, std::unordered_map<int,float>> bigram_bind_w;
    // Total binding mass per concept: its coupling surface to the
    // sensory layer. Recognition divides received energy by this, the
    // way acceleration is force over mass, so a concept bound to many
    // lanes cannot win by size alone.
    std::unordered_map<int,float> bigram_mass;

    // Sentinel bytes marking word start / end so prefixes and suffixes
    // are themselves bigrams (^m, n$). Taken from the non-printable
    // range so they cannot collide with real input.
    static const unsigned char BIGRAM_BOS = 0x01;
    static const unsigned char BIGRAM_EOS = 0x02;

    void bind_bigram(unsigned char a, unsigned char b, int cavity_id, float w) {
        std::lock_guard<std::mutex> lk(mtx);
        if (cavity_id < 0) return;
        int key = (int)a * 256 + (int)b;
        bigram_bind_w[key][cavity_id] += w;
        bigram_mass[cavity_id] += w;
    }

    // CMD Byte-Wave Layer state. 256 sensory lanes + learned bindings.
    // byte_bind_w[b] maps concept cavity id → binding weight. Weights
    // are created near zero at the I/O boundary (bind_byte) when a byte
    // occurs inside a resonance-matched window; the tick then grows or
    // starves them by pure Hebbian co-activity. Survives rebind — the
    // graph only appends nodes, so cavity ids stay valid.
    std::vector<ByteLane> byte_lanes;
    std::vector<std::unordered_map<int,float>> byte_bind_w;
    float byte_bind_rate  = 0.02f;      // Hebbian growth on co-activity
    float byte_bind_decay = 0.9995f;    // starvation of unused bindings
    float byte_bind_cap   = 0.6f;       // tanh saturation ceiling
    // CMD Phase-Locked Emission — wavefront gate time constant. A
    // cavity's readout score decays exp(-quiet_ticks·rate): only
    // cavities the current input wave actually reached can speak.
    float emission_recency_rate = 0.25f;
    float emission_floor_frac   = 0.05f; // of peak |displacement|
    float wavefront_eps       = 1e-4f;
    int   K_idle              = 20;         // ticks of no fresh input before eviction
    std::vector<int> quiet_ticks;           // per cavity (parallel to cavities[])

    // Phase 2.1 — Syntax Macro-Layer state.
    // Default = SVO (3 slots). bind_syntax_slots() can be called with a
    // different slot count to add Modifier/Adverb. Frequencies are
    // harmonic ratios of ω_0 = 1 rad/s so the Kuramoto lock structure
    // stays clean and orthogonal.
    bool  syntax_slots_enabled  = false;
    float syntax_coupling_norm  = 0.5f;
    std::vector<SyntaxSlot> syntax_slots;

    void bind_syntax_slots(int N = 3) {
        std::lock_guard<std::mutex> lk(mtx);
        syntax_slots.clear();
        const char* roles3[]  = {"subject","verb","object"};
        const char* roles4[]  = {"subject","verb","object","modifier"};
        const char* roles5[]  = {"subject","verb","object","modifier","adverb"};
        const char** roles = (N==5)?roles5:(N==4)?roles4:roles3;
        if (N < 3) N = 3;
        if (N > 5) N = 5;
        const float TWO_PI = 6.28318531f;
        // Frequencies commensurate with the semantic drive band (10-20
        // in current engine units). Harmonic ratios 3:4:5 keep the
        // Kuramoto lock structure clean and orthogonal — SVO get
        // distinct dynamics without beating.
        const float base_omega = 15.0f;
        const float ratios[]   = {1.0f, 4.0f/3.0f, 5.0f/3.0f,
                                  6.0f/3.0f, 7.0f/3.0f};
        for (int k = 0; k < N; ++k) {
            SyntaxSlot s;
            s.natural_omega = base_omega * ratios[k];
            s.target_phase  = TWO_PI * (float)k / (float)N;
            s.phase         = s.target_phase;
            s.amplitude     = 0.0f;
            s.role          = roles[k];
            s.locked_cavity_id = -1;
            syntax_slots.push_back(s);
        }
        syntax_slots_enabled = true;
    }

    // SRG — Synaptic Receptor Gating (research-paper/srg.md).
    // When enabled, per-tunnel force is routed by polarity sign:
    //   polarity ≥ 0 → E port of both endpoints
    //   polarity < 0 → I port of both endpoints
    // Observable displacement = x_E − x_I is recomputed at end of tick.
    // When disabled, x_I and v_I stay 0 and observable == E port ==
    // pre-SRG baseline exactly.
    bool srg_enabled = false;

    std::mutex mtx;

    // ---- bind ----
    void bind(const SI& si) {
        graph_ptr = &si;
        cavities.assign(si.graph.nodes.size(), Cavity());
        tunnels.assign(si.graph.edges.size(), Tunnel());
        macro_nodes.clear();
        sim_time = 0.0f;
        ic  = IntegrityCore();
        sil = SilenceIntegration();

        // RB+G bookkeeping — gen-0 cavities each get a RootAlias entry
        // in the ledger with label_key equal to their own id, so alias
        // lookups always land somewhere.
        alias_ledger.assign(si.graph.nodes.size(), AliasEntry{});
        for (int i = 0; i < (int)si.graph.nodes.size(); ++i) {
            alias_ledger[i] = AliasEntry{AliasRole::RootAlias, i, -1, i};
        }
        fission_map.clear();
        fission_records.clear();
        rbg_tick_counter = 0;

        // Phase 1.1 — every cavity starts quiet (quiet_ticks == K_idle).
        // drive() and per-tick propagation reset entries to 0.
        quiet_ticks.assign(si.graph.nodes.size(), K_idle);

        std::mt19937 rng(bind_seed);
        std::uniform_real_distribution<float> mrand(mass_min, mass_max);
        std::uniform_real_distribution<float> frand(8.0f, 14.0f);
        for (auto& c : cavities) {
            c.mass      = mrand(rng);
            c.base_freq = frand(rng);
        }
        for (size_t i = 0; i < si.graph.edges.size(); ++i) {
            const Edge& e = si.graph.edges[i];
            tunnels[i].cavity_a = e.from;
            tunnels[i].cavity_b = e.to;
            material_for(e.rel,
                         tunnels[i].stiffness,
                         tunnels[i].damping,
                         tunnels[i].polarity);
            tunnels[i].base_stiffness   = tunnels[i].stiffness;
            tunnels[i].plasticity_delta = 0.0f;
            tunnels[i].rel_id           = e.rel;
            // CPSB §2.1 — assign the gauge phase offset from relation
            // material. Verb-like directed relations get Ψ = −π/3;
            // taxonomic relations get Ψ = 0. Runs once at bind, never
            // inspected as a branch in the tick loop.
            tunnels[i].psi_target = psi_for(e.rel);
            // VGHT §1.2 — voltage-gate flag is set later, once
            // cluster ids are known (needs union-find).
        }
        // HNCA clustering via union-find, then per-region ω.
        int N = (int)cavities.size();
        std::vector<int> parent(N);
        for (int i = 0; i < N; ++i) parent[i] = i;
        auto find = [&](int x){
            while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
            return x;
        };
        auto join = [&](int a, int b){
            a = find(a); b = find(b);
            if (a != b) parent[a] = b;
        };
        // VGHT §1.1 fix — cluster by STRONG coupling only.
        //
        // Joining on every tunnel merged 69% of the graph into one
        // connected component (1877 of 2712 cavities), which then
        // received a single frequency band. Domain isolation was
        // therefore absent exactly where it was needed: biology,
        // taxonomy and medicine all resonated in the same band and
        // interfered freely.
        //
        // The culprit is generic taxonomic structure. `is_a` and `has`
        // link every domain to shared abstract hubs, so union-find
        // walks from a drug to an operating system in a few hops. But
        // those relations are precisely the ones the derived-material
        // pass (above) assigns LOW stiffness, because their fan-out is
        // enormous — one subject has dozens of objects.
        //
        // So the fix follows from the physics already computed: two
        // masses joined by a very weak spring are not one rigid body.
        // Clustering unions only over tunnels whose stiffness exceeds
        // the median, letting weak taxonomic links relate concepts
        // without fusing them into a single resonant mass.
        {
            std::vector<float> ks;
            ks.reserve(tunnels.size());
            for (const auto& t : tunnels) ks.push_back(t.stiffness);
            float k_cut = 0.0f;
            if (!ks.empty()) {
                std::nth_element(ks.begin(), ks.begin() + ks.size()/2, ks.end());
                k_cut = ks[ks.size()/2];
            }
            for (const auto& t : tunnels)
                if (t.stiffness >= k_cut) join(t.cavity_a, t.cavity_b);
        }
        std::unordered_map<int,int> root_to_macro;
        std::uniform_real_distribution<float> phase_dist(-3.14f, 3.14f);
        // VGHT §1.1 — Domain Sub-Bands. Each macro-cluster is placed on
        // one of B orthogonal frequency bands so that cross-cluster
        // coupling by phase drift alone averages to zero. Cluster id
        // modulo B picks the band; a deterministic small jitter breaks
        // in-band degeneracy without seeding it from RNG state
        // (reproducible across bind).
        const int   VGHT_B     = vght_bands;
        const float VGHT_OMLO  = vght_omega_lo;
        const float VGHT_OMHI  = vght_omega_hi;
        const float band_width = (VGHT_OMHI - VGHT_OMLO)
                               / (float)VGHT_B;
        for (int i = 0; i < N; ++i) {
            int r = find(i);
            auto it = root_to_macro.find(r);
            if (it == root_to_macro.end()) {
                int mid = (int)macro_nodes.size();
                root_to_macro[r] = mid;
                MacroNode mn;
                int band = mid % VGHT_B;
                float band_center = VGHT_OMLO + band_width * ((float)band + 0.5f);
                // Deterministic in-band jitter from cluster id.
                unsigned h = (unsigned)mid * 2654435761u;
                float jit = (float)(h & 0xFFFF) / 65535.0f;   // [0,1]
                float jitter = (jit - 0.5f) * band_width * 0.4f;
                mn.natural_omega = band_center + jitter;
                mn.phase         = phase_dist(rng);
                macro_nodes.push_back(mn);
            }
            int mid = root_to_macro[r];
            macro_nodes[mid].cavity_ids.push_back(i);
            cavities[i].cluster_id = mid;
        }
        for (auto& m : macro_nodes) {
            m.inv_size = 1.0f / (float)m.cavity_ids.size();
        }
        // Precompute normalized coupling so tick() has no divisor decision.
        float divisor = (float)macro_nodes.size() - 1.0f;
        if (divisor < 1.0f) divisor = 1.0f;
        macro_coupling_norm = macro_coupling / divisor;

        // VGHT §1.1 — Verb-material ω-pull now shifts within-band, not
        // across bands. For each macro, weigh the verb-like edge share
        // and pull ω toward the *upper edge of the cluster's own home
        // band*. This preserves domain orthogonality (bands don't
        // overlap) while keeping the verb slot's phase-lock property:
        // verb-heavy clusters rise to the top of their band and win
        // the verb slot within their domain.
        const float verb_mix_gain   = 0.6f;   // strength of the pull
        std::vector<float> macro_verb_num(macro_nodes.size(), 0.0f);
        std::vector<float> macro_edge_cnt(macro_nodes.size(), 0.0f);
        for (size_t ei = 0; ei < si.graph.edges.size(); ++ei) {
            const Edge& e = si.graph.edges[ei];
            int ca = cavities[e.from].cluster_id;
            int cb = cavities[e.to].cluster_id;
            if (ca != cb || ca < 0) continue;
            macro_edge_cnt[ca] += 1.0f;
            macro_verb_num[ca] += verb_pull_for(e.rel);
        }
        for (size_t m = 0; m < macro_nodes.size(); ++m) {
            float denom = macro_edge_cnt[m];
            if (denom < 1.0f) continue;
            float share = macro_verb_num[m] / denom;   // in [0,1]
            float pull  = share * verb_mix_gain;
            int band = (int)m % VGHT_B;
            float band_top = VGHT_OMLO + band_width * ((float)band + 0.9f);
            macro_nodes[m].natural_omega =
                (1.0f - pull) * macro_nodes[m].natural_omega
                + pull * band_top;
        }

        // ---- Derived materials (replaces hand-set stiffness) ----------
        //
        // material_for() assigns k and c per relation from a hand-written
        // table. Those constants were guesses. They are now DERIVED from
        // the loaded ontology's own structure, so the substrate adapts to
        // whatever graph it is given instead of encoding one author's
        // intuition about biology.
        //
        // The principle: a relation's stiffness is its FUNCTIONAL
        // DETERMINACY. If knowing the subject almost determines the
        // object (`returns` — one return type per function), the relation
        // is a hard constraint and transmits force with little loss. If a
        // subject has fifty objects (`calls`), the relation is diffuse and
        // any single instance says little, so it should transmit weakly.
        //
        //     fanout_r = |edges with relation r| / |distinct subjects|
        //     k_r      = k_hi / (1 + log2(1 + fanout_r))
        //     c_r      = c_lo * (1 + log2(1 + fanout_r))
        //
        // Damping moves opposite stiffness: diffuse relations are lossy.
        // The log keeps a 100-way hub from collapsing to zero.
        //
        // POLARITY IS NOT DERIVED. That `inhibits` opposes while
        // `activates` reinforces is a semantic fact about the world, not
        // a structural property of the graph, and no amount of counting
        // recovers it. It stays declared in material_for(), and that is
        // the honest boundary of what statistics can supply.
        {
            std::unordered_map<int,int> rel_edges;
            std::unordered_map<int,std::unordered_map<int,int>> rel_subj;
            for (const auto& e : si.graph.edges) {
                rel_edges[e.rel]++;
                rel_subj[e.rel][e.from] = 1;
            }
            const float k_hi = 2.4f, c_lo = 0.006f;
            std::unordered_map<int,float> k_of, c_of;
            for (const auto& kv : rel_edges) {
                int r = kv.first;
                float n_subj = (float)rel_subj[r].size();
                if (n_subj < 1.0f) n_subj = 1.0f;
                float fanout = (float)kv.second / n_subj;
                float spread = std::log2(1.0f + fanout);
                k_of[r] = k_hi / (1.0f + spread);
                c_of[r] = c_lo * (1.0f + spread);
            }
            for (auto& t : tunnels) {
                auto ik = k_of.find(t.rel_id);
                if (ik == k_of.end()) continue;
                t.stiffness      = ik->second;
                t.base_stiffness = ik->second;
                t.damping        = c_of[t.rel_id];
            }
        }

        // VGHT §1.2 — mark every cross-cluster tunnel as voltage-gated,
        // and every L6-A tunnel (resonance_omega > 0) regardless of
        // cluster membership. All later structural discoveries (fission
        // binding tunnels, online L6-A) can flip the flag themselves.
        for (auto& t : tunnels) {
            int ca = cavities[t.cavity_a].cluster_id;
            int cb = cavities[t.cavity_b].cluster_id;
            bool cross = (ca != cb) && (ca >= 0) && (cb >= 0);
            bool l6a   = (t.resonance_omega > 0.0f);
            t.voltage_gated = (cross || l6a) ? 1.0f : 0.0f;
        }

        // CMD Byte-Wave Layer — initialize the 256 sensory lanes ONCE.
        // Learned byte→concept bindings survive rebind: the graph only
        // appends nodes, so stored cavity ids remain valid. Wiping them
        // on every auto-spawn rebind would lobotomize the sensory map.
        if (byte_lanes.size() != 256) {
            byte_lanes.assign(256, ByteLane());
            // MRS §1.2 correction — all lanes share a COMMON natural
            // frequency. The original CMD design gave each byte its own
            // micro-frequency (3.0 + b/256), but that destroys the
            // arrival-order phase encoding MRS depends on:
            //
            //   arrival signal   : ω · Δt_arrival     ≈ 0.2 rad/letter
            //   frequency drift  : Δω · T_elapsed     ≈ 0.28 rad
            //
            // The drift exceeds the signal, and with ~13 rad of
            // accumulated phase (two full revolutions) the mod-2π
            // wrap scrambles letter order outright.
            //
            // With a common ω the elapsed-time term ω·T is identical
            // for every lane, so it acts as a rigid rotation of the
            // whole phase pattern — which preserves ordering exactly.
            // Lanes are distinguished by index, never by frequency, so
            // the per-byte frequencies bought nothing.
            for (int b = 0; b < 256; ++b)
                byte_lanes[b].natural_omega = cpme_base_omega;
        }
        if (byte_bind_w.size() != 256)
            byte_bind_w.assign(256, std::unordered_map<int,float>());
    }

    // ---- input ----
    // Attach a continuous sinusoidal driver. duration_ticks sets the
    // envelope decay so amplitude decays to ~e^-3 (~5%) over the window.
    void drive(int node_id, float freq, float amp, int duration_ticks) {
        std::lock_guard<std::mutex> lk(mtx);
        if (node_id < 0 || (size_t)node_id >= cavities.size()) return;
        float d = (float)((duration_ticks < 1) ? 1 : duration_ticks);
        // RB+G energy-conserving fan-out. A drive on a fissioned parent
        // routes to both children at amp/√2, so total injected energy
        // matches a single-cavity drive at amp exactly.
        auto it = fission_map.find(node_id);
        if (it != fission_map.end() && rbg_alias_route_enabled) {
            float half_amp = amp * rbg_fanout_factor;
            for (int child : {it->second.first, it->second.second}) {
                cavities[child].drive_freq     = freq;
                cavities[child].drive_amp      = half_amp;
                cavities[child].drive_envelope = 1.0f;
                cavities[child].drive_decay    = std::exp(-3.0f / d);
                cavities[child].was_driven     = true;
                // Phase 1.1 — mark child as touched.
                if (child < (int)quiet_ticks.size()) quiet_ticks[child] = 0;
            }
            return;
        }
        cavities[node_id].drive_freq     = freq;
        cavities[node_id].drive_amp      = amp;
        cavities[node_id].drive_envelope = 1.0f;
        cavities[node_id].drive_decay    = std::exp(-3.0f / d);
        cavities[node_id].was_driven     = true;
        // CPME §1.1 — record arrival time for chiral phase encoding.
        cavities[node_id].t_arrival      = sim_time;
        // Phase 1.1 — mark cavity as touched.
        if (node_id < (int)quiet_ticks.size()) quiet_ticks[node_id] = 0;
    }

    // CMD Byte-Wave input — drive one sensory lane. No concept cavity
    // is touched here; energy reaches concepts only through learned
    // byte_bind_w weights inside tick().
    void drive_byte(unsigned char b, float amp = 1.0f, int duration_ticks = 8) {
        std::lock_guard<std::mutex> lk(mtx);
        if (byte_lanes.size() != 256) return;
        float d = (float)((duration_ticks < 1) ? 1 : duration_ticks);
        ByteLane& L = byte_lanes[b];
        L.drive_amp      = amp;
        L.drive_envelope = 1.0f;
        L.drive_decay    = std::exp(-3.0f / d);
        // MRS §1.2 — lane chirality. Letter order within a word
        // becomes a phase gradient across the lane population.
        L.t_arrival      = sim_time;
    }

    // MRS §1.3 — create a lane↔lane gauge tunnel for an observed byte
    // adjacency. I/O boundary (wiring); the learned Ψ and weight are
    // pure physics thereafter. Idempotent per ordered pair.
    void bind_lane_pair(unsigned char a, unsigned char b) {
        std::lock_guard<std::mutex> lk(mtx);
        if (byte_lanes.size() != 256) return;
        int key = (int)a * 256 + (int)b;
        if (lane_pair_index.find(key) != lane_pair_index.end()) return;
        LaneTunnel lt;
        lt.lane_a = (int)a;
        lt.lane_b = (int)b;
        lane_pair_index[key] = (int)lane_tunnels.size();
        lane_tunnels.push_back(lt);
    }

    // CMD sensory synaptogenesis (I/O boundary, not physics). When a
    // byte occurs inside a resonance-matched window, a near-zero
    // binding is created between that byte lane and the matched concept
    // cavity. The tick's Hebbian pass then grows it under co-activity
    // or starves it to nothing. Creation is wiring; survival is physics.
    void bind_byte(unsigned char b, int cavity_id, float w0 = 0.02f) {
        std::lock_guard<std::mutex> lk(mtx);
        if (byte_bind_w.size() != 256) return;
        if (cavity_id < 0 || (size_t)cavity_id >= cavities.size()) return;
        auto& m = byte_bind_w[b];
        if (m.find(cavity_id) == m.end()) m[cavity_id] = w0;
    }

    void set_neurotransmitters(float d, float c, float s) {
        std::lock_guard<std::mutex> lk(mtx);
        nt.dopamine  = d;
        nt.cortisol  = c;
        nt.serotonin = s;
    }

    // Public accessors for the RB+G ledger.
    bool is_fissioned(int parent_id) const {
        return fission_map.find(parent_id) != fission_map.end();
    }
    std::pair<int,int> fission_children(int parent_id) const {
        auto it = fission_map.find(parent_id);
        return (it == fission_map.end()) ? std::pair<int,int>{-1,-1}
                                         : it->second;
    }

    // Public API — takes the mutex.
    std::pair<int,int> fission(int parent, float I_val = 0.0f,
                                float tau_val = 0.0f, int streak = 0,
                                int tick_index = -1) {
        std::lock_guard<std::mutex> lk(mtx);
        return fission_locked(parent, I_val, tau_val, streak, tick_index);
    }

    // Internal — assumes caller holds mtx.  Returns {alpha,beta} or
    // {-1,-1} if refused (already fissioned or generation cap hit).
    std::pair<int,int> fission_locked(int parent, float I_val = 0.0f,
                                       float tau_val = 0.0f, int streak = 0,
                                       int tick_index = -1) {
        if (parent < 0 || (size_t)parent >= cavities.size()) return {-1,-1};
        // Refuse: already fissioned.
        if (fission_map.count(parent)) return fission_map[parent];
        // Refuse: generation cap.
        if (cavities[parent].generation >= g_max) return {-1,-1};

        Cavity p_snap = cavities[parent];
        int p_gen = p_snap.generation;

        // Zero the parent's mechanical field — it becomes a routing ghost.
        cavities[parent].displacement = 0.0f;
        cavities[parent].velocity     = 0.0f;
        cavities[parent].x_E = cavities[parent].x_I = 0.0f;
        cavities[parent].v_E = cavities[parent].v_I = 0.0f;
        cavities[parent].drive_amp    = 0.0f;
        cavities[parent].drive_envelope = 0.0f;
        cavities[parent].salience     = 0.0f;

        int alpha = (int)cavities.size();
        Cavity a = p_snap; a.generation = p_gen + 1; a.was_driven = false;
        a.x_E = a.x_I = a.v_E = a.v_I = 0.0f;
        a.displacement = a.velocity = 0.0f;
        a.drive_amp = a.drive_envelope = 0.0f;
        cavities.push_back(a);

        int beta = (int)cavities.size();
        Cavity b = p_snap; b.generation = p_gen + 1; b.was_driven = false;
        b.x_E = b.x_I = b.v_E = b.v_I = 0.0f;
        b.displacement = b.velocity = 0.0f;
        b.drive_amp = b.drive_envelope = 0.0f;
        cavities.push_back(b);

        // Reroute incident tunnels by polarity sign. Excitatory (p≥0)
        // to alpha; inhibitory (p<0) to beta.
        int inc_alpha = 0, inc_beta = 0;
        for (auto& t : tunnels) {
            bool touches_a = (t.cavity_a == parent);
            bool touches_b = (t.cavity_b == parent);
            if (!touches_a && !touches_b) continue;
            int child = (t.polarity >= 0.0f) ? alpha : beta;
            if (touches_a) t.cavity_a = child;
            if (touches_b) t.cavity_b = child;
            (child == alpha ? inc_alpha : inc_beta) += 1;
        }

        // Learned binding tunnel: k init 1.0, polarity 0.0 (pure damping).
        // Its base_stiffness caps the learning range in the Hebbian pass.
        // Skipped when rbg_binding_enabled is false (ablation).
        if (rbg_binding_enabled) {
            Tunnel bind{};
            bind.cavity_a       = alpha;
            bind.cavity_b       = beta;
            bind.stiffness      = rbg_bind_k_init;
            bind.base_stiffness = rbg_bind_k_init;
            bind.damping        = 0.020f;
            bind.polarity       = rbg_bind_pol_init;
            bind.plasticity_delta = 0.0f;
            tunnels.push_back(bind);
        }

        // Extend alias_ledger for new children. Parent ledger entry
        // becomes a RootAlias pointing at children; children reference
        // the parent's label_key so dumps can say "metformin.α".
        int parent_label_key = alias_ledger[parent].label_key;
        alias_ledger[parent].role      = AliasRole::RootAlias;
        alias_ledger[parent].parent    = parent;
        alias_ledger[parent].partner   = -1;
        alias_ledger[parent].label_key = parent_label_key;
        alias_ledger.push_back(AliasEntry{AliasRole::Alpha, parent, beta, parent_label_key});
        alias_ledger.push_back(AliasEntry{AliasRole::Beta,  parent, alpha, parent_label_key});

        // Cluster membership: children join parent's macro cluster.
        int cid = p_snap.cluster_id;
        cavities[alpha].cluster_id = cid;
        cavities[beta].cluster_id  = cid;
        if (cid >= 0 && cid < (int)macro_nodes.size()) {
            macro_nodes[cid].cavity_ids.push_back(alpha);
            macro_nodes[cid].cavity_ids.push_back(beta);
            macro_nodes[cid].inv_size =
                1.0f / (float)macro_nodes[cid].cavity_ids.size();
        }

        fission_map[parent] = {alpha, beta};

        // Audit record.
        FissionRecord rec{};
        rec.tick_index   = (tick_index >= 0) ? tick_index : rbg_tick_counter;
        rec.parent_id    = parent;
        rec.parent_label = (graph_ptr && parent_label_key >= 0 &&
                            parent_label_key < (int)graph_ptr->graph.nodes.size())
                          ? graph_ptr->graph.nodes[parent_label_key].label
                          : std::string("<unknown>");
        rec.I                    = I_val;
        rec.tau                  = tau_val;
        rec.streak               = streak;
        rec.child_alpha          = alpha;
        rec.child_beta           = beta;
        rec.incident_alpha       = inc_alpha;
        rec.incident_beta        = inc_beta;
        rec.drive_split_factor   = rbg_fanout_factor;
        fission_records.push_back(rec);

        return {alpha, beta};
    }

    // ============================================================
    //  ONE PHYSICS TICK. Zero if/switch. Pure scalar arithmetic.
    //  Before bind() the vectors are empty so every for-loop iterates
    //  zero times; no safety branch is required.
    // ============================================================
    void tick() {
        std::lock_guard<std::mutex> lk(mtx);
        sim_time += dt;
        ++rbg_tick_counter;

        // NTDA: pure scalar multipliers on the physical constants.
        const float damp_scale   = nt.cortisol;
        const float drive_scale  = nt.dopamine;
        const float plastic_rate = hebbian_rate * nt.serotonin;
        const float half         = 0.5f;

        // VGHT §1.3 — Global Viscosity from last tick's E_total.
        //   μ_eff = μ_0 + k · (1 − exp(−E_total / E_scale))
        // Bounded saturation → cannot diverge. Applied to friction in
        // this tick's cavity velocity decay. When E_total is small
        // (normal sparse activation), μ_eff ≈ μ_0 → baseline. During
        // an avalanche, μ_eff saturates to μ_0 + k_avalanche and the
        // excess kinetic energy is dissipated within the same tick.
        const float avalanche_add =
            vght_k_avalanche
            * (1.0f - std::exp(-vght_last_Etotal / vght_E_scale));
        const float mu_eff = friction + avalanche_add;
        const float friction_this_tick = mu_eff;

        // CSAT §2.1 — integrate the Lorenz system one step. Three
        // scalar updates; no network, no training, no state beyond
        // (x, y, z). Deterministic: same seed, same trajectory.
        if (csat_enabled) {
            float dx = csat_tau * csat_sigma * (csat_y - csat_x) * dt;
            float dy = csat_tau * (csat_x * (csat_rho - csat_z) - csat_y) * dt;
            float dz = csat_tau * (csat_x * csat_y - csat_beta * csat_z) * dt;
            csat_x += dx; csat_y += dy; csat_z += dz;
        }
        const float csat_xhat = csat_enabled ? (csat_x / 20.0f) : 0.0f;

        // MRS master gate as a branchless scalar. When 0, every MRS
        // term below vanishes and the substrate reproduces pre-MRS
        // dynamics exactly.
        const float mrs_active = mrs_enabled ? 1.0f : 0.0f;

        // SRG activation coefficient (branchless routing).
        //   srg_active = 1 → route by polarity into E or I channel
        //   srg_active = 0 → all traffic to E (single-channel, baseline)
        const float srg_active = srg_enabled ? 1.0f : 0.0f;

        // 1. Tunnels — symmetric coupled-oscillator force + Hebbian.
        //
        // SRG routing: define per-tunnel gates
        //   sign_p = signum(polarity)                (∈ {+1, −1})
        //   gate_E = srg_active · ½(1 + sign_p) + (1 − srg_active)
        //   gate_I = 1 − gate_E   (bounded to 0 when srg off)
        // Force delivered to A and B is added to (gate_E · port_E, gate_I · port_I).
        // For SRG off, gate_E = 1 always, so behavior matches baseline exactly.
        for (auto& t : tunnels) {
            Cavity& A = cavities[t.cavity_a];
            Cavity& B = cavities[t.cavity_b];

            // ---- SPARSITY GUARD (not a dynamical branch) ----------------
            //
            // This file's contract is that tick() contains no if-branch
            // that changes the PHYSICS. This guard does not: it skips
            // arithmetic whose result is already provably below the
            // readout epsilon, and it is the only reason the substrate is
            // usable at ConceptNet scale.
            //
            // If both endpoints are quiescent then
            //     |spring_arg| = |x_a - p·x_b| < 2ε
            //     |damp_arg|   = |v_a - p·v_b| < 2ε
            // so |F| < 2ε(k + c). With ε = 1e-4 and the stiffest material
            // in the MIMS table that bounds |F| at ~3e-4, against a
            // readout floor of 1e-5 on displacement integrated over dt —
            // i.e. the skipped force cannot move a cavity into the
            // reported set. The gates below (Curie, band-pass, voltage)
            // are all in [0,1], so they can only shrink it further.
            //
            // Cost before this guard: every one of 237k tunnels evaluated
            // ~4 transcendentals every tick, 200 ticks per query — about
            // 1.9e8 exp() calls to answer one question, nearly all of them
            // on cavities the wavefront never reached.
            const float qa = std::fabs(A.displacement) + std::fabs(A.velocity)
                           + A.drive_amp * A.drive_envelope;
            const float qb = std::fabs(B.displacement) + std::fabs(B.velocity)
                           + B.drive_amp * B.drive_envelope;
            if (qa < wavefront_eps && qb < wavefront_eps) continue;

            float spring_arg = A.displacement - t.polarity * B.displacement;
            float damp_arg   = A.velocity     - t.polarity * B.velocity;
            float k_eff = t.stiffness;
            float c_eff = t.damping * damp_scale;
            float F_A = -k_eff * spring_arg - c_eff * damp_arg;
            float F_B = +k_eff * t.polarity * spring_arg
                       + c_eff * t.polarity * damp_arg;

            // Phase 2.2 — Band-Pass Transfer Function gate.
            float bp_active = (t.resonance_omega > 0.0f) ? 1.0f : 0.0f;
            float omega_probe = A.drive_freq;
            float dw = (t.resonance_omega - omega_probe);
            float g_gauss = std::exp(-t.Q_factor * dw * dw
                                    / (t.resonance_omega + 1e-6f));
            float bp_gate = bp_active * g_gauss + (1.0f - bp_active);
            F_A *= bp_gate;
            F_B *= bp_gate;

            // CMD Curie-point coupling gate.
            //   K_eff = K_0 · exp(-T_a / T_crit) · exp(-T_b / T_crit)
            // When either endpoint is hot (stopword), its contribution
            // to the tunnel force collapses. Rare-word (cold) pairs keep
            // strong coupling. This is thermodynamics, not a stopword list.
            float curie_a = std::exp(-A.heat_T / T_crit);
            float curie_b = std::exp(-B.heat_T / T_crit);
            float curie_gate = curie_a * curie_b;
            F_A *= curie_gate;
            F_B *= curie_gate;

            // VGHT §1.2 — Voltage-Gated Tunnel conductance.
            //   C = σ(κ · (|x_a|·|x_b| − θ))
            // For non-gated tunnels (in-cluster, non-L6A) the flag is
            // 0 and vg_gate collapses to 1 → pass-through, no behavior
            // change. For gated tunnels, force is scaled by C; the
            // channel presents effectively infinite resistance until
            // both endpoints are simultaneously excited above threshold.
            float A_amp    = std::fabs(A.displacement);
            float B_amp    = std::fabs(B.displacement);
            float vg_arg   = vght_gate_kappa * (A_amp * B_amp - vght_gate_theta);
            float vg_sig   = 1.0f / (1.0f + std::exp(-vg_arg));
            float vg_gate  = t.voltage_gated * vg_sig
                           + (1.0f - t.voltage_gated);   // 1 when ungated
            F_A *= vg_gate;
            F_B *= vg_gate;

            // CPSB §2.1 — Gauge-theoretic phase-shear torque.
            // Compute per-cavity phases θ_a, θ_b once per tunnel from
            // existing state (velocity, displacement, ω₀). The gauge
            // force is a Kuramoto-style sin(Δθ − Ψ) exchange between
            // the two endpoints: equilibrium is θ_a − θ_b = Ψ, i.e.
            // the tunnel actively enforces a directed phase gradient.
            // For Ψ = 0 (taxonomic relations) the gauge collapses to
            // standard synchronizing coupling. Attenuated by the same
            // gates that scale the spring force (bp, curie, voltage).
            float theta_a = std::atan2(A.velocity,
                                       cpme_base_omega * A.displacement);
            float theta_b = std::atan2(B.velocity,
                                       cpme_base_omega * B.displacement);
            float dtheta      = theta_b - theta_a - t.psi_target;
            // CPSB §2.1 — gauge is gated by joint endpoint amplitude
            // (same activity idiom as VGHT voltage gating) so quiescent
            // tunnels do not inject torque into the field. Only the
            // subgraph excited by the current input experiences the
            // gauge — which is exactly the subgraph whose phase
            // gradient we want to shape. Also gated by the parent
            // stack's gates (band-pass, Curie, voltage).
            float gauge_scale = bp_gate * curie_gate * vg_gate
                              * A_amp * B_amp;
            float gauge_f     = cpsb_K_gauge * std::sin(dtheta) * gauge_scale;
            // Push a toward (b − Ψ) and b toward (a + Ψ). Force added
            // directly to E-port velocity; SRG routing handles I-port.
            F_A += gauge_f;
            F_B -= gauge_f;

            float sign_p = std::copysign(1.0f, t.polarity);
            float gate_E = srg_active * (0.5f + 0.5f * sign_p)
                         + (1.0f - srg_active);
            float gate_I = 1.0f - gate_E;

            float aA = (F_A / A.mass) * dt;
            float aB = (F_B / B.mass) * dt;
            // I-port stores the 180°-flipped version of the incoming
            // acceleration (paper: "waves entering the I-Port are
            // shifted by 180° before they hit the cavity mass"). Since
            // observable v = v_E − v_I, storing −aA in v_I makes the
            // observable contribution equal to +aA, preserving the
            // same baseline dynamics — but the Hebbian rule now reads
            // v_I separately, so I-updates cannot cancel E-updates.
            A.v_E += gate_E * aA;
            A.v_I -= gate_I * aA;
            B.v_E += gate_E * aB;
            B.v_I -= gate_I * aB;

            // Hebbian — signed-separated. Excitatory tunnels update on
            // v_a_E · v_b_E; inhibitory on v_a_I · v_b_I. Under SRG off,
            // v_I = 0 so gate_I·v_I = 0 and Hebbian reduces to E channel,
            // which equals observable velocity — matches baseline.
            float v_a_used = gate_E * A.v_E + gate_I * A.v_I;
            float v_b_used = gate_E * B.v_E + gate_I * B.v_I;
            // CPME-II §1.1 — Phase-consistent Hebbian modulator.
            //   Φ = cos(θ_a − θ_b − Ψ_target) · sgn(polarity)
            // The Ψ subtraction folds the CPSB gauge target into the
            // coincidence detector: pairs whose phase difference equals
            // the tunnel's target Ψ are treated as maximally aligned.
            // sgn(polarity) inverts Φ for inhibitory tunnels, so
            // anti-phase co-activity correctly consolidates them
            // (CPME-II §5.2 polarity-phase interaction).
            // α_phi blends between the parent velocity-only rule and
            // full phase gating; default 0.6.
            float phi_cos  = std::cos(theta_a - theta_b - t.psi_target);
            float phi_pol  = phi_cos * std::copysign(1.0f, t.polarity);
            float phi_mod  = (1.0f - cpme_ii_alpha_phi)
                           + cpme_ii_alpha_phi * phi_pol;
            float integrand = t.plasticity_delta * hebbian_decay
                            + plastic_rate * v_a_used * v_b_used * phi_mod;
            t.plasticity_delta = plasticity_cap
                               * std::tanh(integrand / plasticity_cap);
            t.stiffness = t.base_stiffness + t.plasticity_delta;
        }

        // 1b. CMD Byte-Wave Layer — sensory lane dynamics + learned
        //     byte→concept coupling + Hebbian binding plasticity.
        //     Lanes are damped driven oscillators. Their displacement
        //     forces bound concept cavities through byte_bind_w — the
        //     ONLY path from raw bytes into the concept field. Unused
        //     bindings starve by continuous decay; co-active ones grow.
        //     Empty maps iterate zero times — no branch needed.
        for (size_t bi = 0; bi < byte_lanes.size(); ++bi) {
            ByteLane& L = byte_lanes[bi];
            float F_lane = L.drive_amp * L.drive_envelope * drive_scale
                         * std::sin(L.natural_omega * sim_time)
                         - L.k * L.displacement
                         - L.damp * L.velocity;
            L.drive_envelope *= L.drive_decay;
            L.velocity     += F_lane * dt;
            L.displacement += L.velocity * dt;
            L.velocity     *= (1.0f - friction_this_tick);
            float lane_act = std::fabs(L.displacement);
            // MRS §1.1 — accumulate the reverse (concept→lane) force
            // using the SAME coupling weights, transposed. No new
            // storage; the byte↔concept layer becomes a bidirectional
            // associative memory of coupled oscillators.
            float F_rev = 0.0f;
            for (auto& kv : byte_bind_w[bi]) {
                Cavity& C = cavities[kv.first];
                // Forward: lane drives concept (E port, mass-scaled).
                C.v_E += (kv.second * L.displacement / C.mass) * dt;
                // Reverse: concept drives lane (MRS §1.1 transpose).
                F_rev += kv.second * C.displacement;
                // Hebbian: grow on lane×cavity co-activity, starve
                // otherwise. tanh-capped, same idiom as tunnel Hebbian.
                float g = kv.second * byte_bind_decay
                        + byte_bind_rate * plastic_rate * lane_act
                          * std::fabs(C.displacement);
                kv.second = byte_bind_cap * std::tanh(g / byte_bind_cap);
            }
            // mrs_active collapses the reverse term to zero when MRS is
            // off, so the pre-MRS dynamics are recovered bit-exactly.
            L.velocity += mrs_active * mrs_rev_gain * F_rev * dt;
            // MRS §1.4 — lane envelope momentum (mirrors CPME §1.2).
            L.env_ema_prev = L.env_ema;
            L.env_ema      = cpme_env_beta * L.env_ema
                           + (1.0f - cpme_env_beta) * lane_act;
        }

        // 1c. MRS §1.3 — lane↔lane gauge tunnels. Learned Ψ via
        //     running circular mean of observed phase differences,
        //     phase-consistent Hebbian weight, and a gauge torque that
        //     drives the pair toward its learned offset. This is where
        //     letter ORDER is stored: the tunnel between 'h' and 'i'
        //     accumulates the typical phase lead of 'h' over 'i'.
        //     Empty vector iterates zero times when MRS is unused.
        for (auto& lt : lane_tunnels) {
            ByteLane& A = byte_lanes[lt.lane_a];
            ByteLane& B = byte_lanes[lt.lane_b];
            float th_a = std::atan2(A.velocity, cpme_base_omega * A.displacement);
            float th_b = std::atan2(B.velocity, cpme_base_omega * B.displacement);
            float dth  = th_a - th_b;
            // Learned Ψ — circular mean, so wraparound is handled
            // correctly (a plain scalar EMA on an angle would not be).
            lt.psi_r = mrs_psi_lambda * lt.psi_r
                     + (1.0f - mrs_psi_lambda) * std::cos(dth);
            lt.psi_i = mrs_psi_lambda * lt.psi_i
                     + (1.0f - mrs_psi_lambda) * std::sin(dth);
            lt.psi   = std::atan2(lt.psi_i, lt.psi_r);
            // Phase-consistent Hebbian (CPME-II form, learned Ψ).
            float phi_c = std::cos(dth - lt.psi);
            float g = lt.weight * hebbian_decay
                    + mrs_lane_hebb_rate * plastic_rate
                      * A.velocity * B.velocity * phi_c;
            lt.weight = mrs_lane_cap * std::tanh(g / mrs_lane_cap);
            // Gauge torque toward the learned offset, gated by joint
            // activity (same idiom as CPSB) and by mrs_active.
            float gf = mrs_active * mrs_lane_gauge * lt.weight
                     * std::sin(th_b - th_a - lt.psi)
                     * std::fabs(A.displacement) * std::fabs(B.displacement);
            A.velocity += gf * dt;
            B.velocity -= gf * dt;
        }

        // 2. Cavities — continuous drive envelope + integrate + TSDA.
        //    Drivers always inject into E channel (excitatory, like
        //    glutamatergic input). Both port displacements integrate.
        //    Friction damps both port velocities. Observable is
        //    recomputed as x_E − x_I / v_E − v_I.
        float total_kinetic     = 0.0f;
        float total_drive_power = 0.0f;
        for (auto& c : cavities) {
            // Sparsity guard — same contract as the tunnel loop above.
            // A cavity at rest, undriven, cold and unsalient is a fixed
            // point of every update in this block: x += v·dt with v = 0,
            // v *= (1-mu) with v = 0, tanh(0) = 0, and the habituation
            // and salience integrators both decay toward 0 from 0. So
            // the state after this iteration is bit-identical to the
            // state before it, and skipping is exact rather than
            // approximate. Only cavities the wavefront has reached, or
            // that are still cooling, do any work.
            if (std::fabs(c.x_E) < wavefront_eps &&
                std::fabs(c.x_I) < wavefront_eps &&
                std::fabs(c.v_E) < wavefront_eps &&
                std::fabs(c.v_I) < wavefront_eps &&
                c.drive_amp * c.drive_envelope < wavefront_eps &&
                c.heat_T   < wavefront_eps &&
                c.salience < wavefront_eps) continue;

            float F_drive = c.drive_amp * c.drive_envelope * drive_scale
                          * std::sin(c.drive_freq * sim_time);
            c.v_E           += (F_drive / c.mass) * dt;
            c.drive_envelope *= c.drive_decay;
            total_drive_power += std::fabs(F_drive);

            c.x_E += c.v_E * dt;
            c.x_I += c.v_I * dt;
            // VGHT §1.3 friction + CSAT §2.2.2 heat-coupled chaos.
            // The chaotic term scales with tanh(T/T_crit): entrenched
            // cavities are shaken, fresh ones barely feel it. Because
            // x_hat alternates sign along the attractor the dominant
            // cavity experiences alternating extra- and reduced-
            // damping — it is shaken, not merely suppressed, which is
            // what permits escape rather than silencing.
            float mu_i = friction_this_tick
                       + csat_gamma * csat_xhat
                         * std::tanh(c.heat_T / T_crit);
            if (mu_i < 0.0f) mu_i = 0.0f;      // never anti-damp
            if (mu_i > 0.5f) mu_i = 0.5f;
            c.v_E *= (1.0f - mu_i);
            c.v_I *= (1.0f - mu_i);

            // Refresh observable (external readers rely on these).
            c.displacement = c.x_E - c.x_I;
            c.velocity     = c.v_E - c.v_I;

            c.salience = std::tanh(c.salience * salience_decay
                                 + salience_gain * std::fabs(c.velocity));

            // CMD Thermodynamic Habituation.
            //   dT/dt = α·|v| − β·T
            // Cavities that oscillate every tick (stopwords, common
            // structural nodes) heat up; cavities that fire rarely
            // (proper names, specific concepts) stay cold. Applied AFTER
            // salience so it uses the same |v| measurement.
            c.heat_T += heat_alpha * std::fabs(c.velocity)
                      - heat_beta  * c.heat_T;

            total_kinetic += half * c.mass * c.velocity * c.velocity;

            // CPME §1.2 momentum envelope. Publish previous-tick value
            // first so P = env - env_prev is available in the readout.
            c.env_ema_prev = c.env_ema;
            c.env_ema      = cpme_env_beta * c.env_ema
                           + (1.0f - cpme_env_beta) * std::fabs(c.displacement);
            // CPME §1.3 acoustic mass Ornstein–Uhlenbeck.
            //   dM/dt = γ · I_ext − δ · M
            // I_ext = current external drive envelope (post-decay in
            // this tick). Driven cavities gain mass, un-driven ones
            // shed it exponentially. Emission mask 1/(1+M/M_scale) is
            // applied in the phase-collapse readout, not here.
            float I_ext = c.drive_amp * c.drive_envelope;
            c.acoustic_mass += cpme_mass_gamma * I_ext * dt
                             - cpme_mass_delta * c.acoustic_mass * dt;
        }
        // VGHT §1.3 — publish this tick's E_total for next tick's μ_eff.
        vght_last_Etotal = total_kinetic;

        // CPME §1.4 — injection load L and settle coupling K_settle.
        float L = 0.0f;
        for (const auto& c : cavities)
            L += c.drive_amp * c.drive_envelope;
        cpme_last_L = L;
        float settle_arg = (L - cpme_L_thresh) / cpme_L_scale;
        cpme_last_Ksettle = 1.0f - 1.0f / (1.0f + std::exp(-settle_arg));

        // CPME §1.5 — localized Kuramoto R over input-touched macros.
        // A macro is "touched" if any of its cavities has quiet_ticks
        // below K_idle (VGHT wavefront gate). Complex-sum norm over
        // amplitude-weighted phasors.
        float rx = 0.0f, ry = 0.0f, denom = 0.0f;
        for (size_t m = 0; m < macro_nodes.size(); ++m) {
            bool touched = false;
            for (int cid : macro_nodes[m].cavity_ids) {
                if (cid < (int)quiet_ticks.size()
                    && quiet_ticks[cid] < K_idle) { touched = true; break; }
            }
            if (!touched) continue;
            float A = macro_nodes[m].amplitude;
            rx    += A * std::cos(macro_nodes[m].phase);
            ry    += A * std::sin(macro_nodes[m].phase);
            denom += A;
        }
        cpme_R_local = std::sqrt(rx*rx + ry*ry) / (denom + 1e-9f);
        cpme_R_baseline = cpme_R_lambda * cpme_R_baseline
                        + (1.0f - cpme_R_lambda) * cpme_R_local;
        cpme_delta_R = cpme_R_local - cpme_R_baseline;

        // 3. HNCA macro-Kuramoto. sin(θ_self − θ_self)·A_self = 0 so
        //    the self term contributes nothing → no self-skip branch.
        for (auto& m : macro_nodes) {
            float sum = 0.0f;
            for (const auto& other : macro_nodes) {
                sum += std::sin(other.phase - m.phase) * other.amplitude;
            }
            float dtheta = m.natural_omega + macro_coupling_norm * sum;
            m.phase += dtheta * dt;
            // Wrap phase without an if — fmod handles it.
            m.phase = std::fmod(m.phase + 9.42477796f, 6.28318531f) - 3.14159265f;
            float amp_sum = 0.0f;
            for (int nid : m.cavity_ids)
                amp_sum += std::fabs(cavities[nid].displacement);
            m.amplitude = amp_sum * m.inv_size;   // inv_size precomputed at bind
        }

        // 3b. Phase 2.1 — Syntax Macro-Layer Kuramoto pass.
        //     Each syntax slot integrates its own phase under coupling
        //     from the HNCA macro-nodes. When a semantic cluster's
        //     amplitude peaks near a slot's phase, the slot's amplitude
        //     rises — that cluster becomes the winner for that slot.
        //     No-op when syntax_slots_enabled == false.
        if (syntax_slots_enabled) {
            for (auto& s : syntax_slots) {
                float sum = 0.0f;
                for (const auto& m : macro_nodes) {
                    sum += std::sin(m.phase - s.phase) * m.amplitude;
                }
                float dtheta = s.natural_omega
                             + syntax_coupling_norm * sum;
                s.phase += dtheta * dt;
                s.phase = std::fmod(s.phase + 9.42477796f, 6.28318531f) - 3.14159265f;
                // Slot amplitude = Σ over macro-nodes whose phase is
                // close to this slot's phase. Gaussian gate.
                float amp_sum = 0.0f;
                for (const auto& m : macro_nodes) {
                    float phase_diff = m.phase - s.phase;
                    amp_sum += m.amplitude
                             * std::exp(-4.0f * phase_diff * phase_diff);
                }
                s.amplitude = amp_sum;
            }
        }

        // 4. RIFA IC — continuous scalar dynamics, tanh-saturated identity.
        ic.tension = ic.tension * 0.999f + rifa_tension_gain * total_kinetic;
        ic.coherence = 1.0f / (1.0f + std::exp(-ic.tension * 0.5f));
        ic.identity_strength =
            std::tanh(ic.identity_strength + 0.0001f * ic.tension * ic.coherence);

        // 5. RIFA SIL — input-power average replaces silent_ticks counter.
        sil.input_power_avg   = sil.input_power_avg * 0.99f + 0.01f * total_drive_power;
        sil.tension_integral  = sil.tension_integral * 0.9999f + ic.tension * dt;
        sil.endurance = std::tanh(sil.endurance * (1.0f - sil_decay)
                                + 0.001f * sil.tension_integral);

        // 6. Phase 1.1 — Wavefront bookkeeping (harmless when
        // sparse_tick_enabled is false; Layer 2 just ignores it).
        // Extend quiet_ticks if fission grew cavities[].
        if ((int)quiet_ticks.size() < (int)cavities.size())
            quiet_ticks.resize(cavities.size(), K_idle);
        // A cavity is "touched" this tick if it has fresh drive (envelope
        // above eps) OR its velocity is above eps. Everyone else ages.
        for (size_t i = 0; i < cavities.size(); ++i) {
            const auto& c = cavities[i];
            bool touched = (std::fabs(c.velocity) > wavefront_eps)
                        || (c.drive_envelope * c.drive_amp > wavefront_eps);
            quiet_ticks[i] = touched ? 0 : (quiet_ticks[i] + 1);
        }
    }

    // Phase 1.1 accessor — cavity is "hot" if it saw fresh input within
    // K_idle ticks. Used by Layer 2's sparse structural tick.
    bool is_hot(int cavity_id) const {
        if (cavity_id < 0 || cavity_id >= (int)quiet_ticks.size())
            return true;                    // safety: unknown = treat as hot
        return quiet_ticks[cavity_id] < K_idle;
    }
    int hot_count() const {
        int n = 0;
        for (int q : quiet_ticks) if (q < K_idle) ++n;
        return n;
    }

    // Phase 2.1 — Temporal Readout Head.
    // For each syntax slot, find the cavity whose macro-node phase is
    // closest to the slot phase AND whose displacement is highest at
    // that instant. Return labels in canonical slot order.
    // MRS §1.4 — Byte-Level Phase-Collapse Projection.
    // The mirror of readout_phase_collapse() one layer down: sweeps a
    // carrier phase across [0, 2π) over the 256 byte lanes and emits
    // the arg-max byte at each step. Word forms are SYNTHESISED from
    // the lane phase field rather than retrieved from a label table.
    //
    // Two deliberate differences from the concept sweep:
    //   (1) NO winner suppression — letters repeat within words
    //       ("banana"), so suppressing a lane would make repeated
    //       graphemes unreachable. Physically motivated by what the
    //       layer represents, not an exception.
    //   (2) Sub-floor sweep steps emit a word boundary (space)
    //       rather than nothing. Boundaries are amplitude gaps in a
    //       resonance field, not grammar delimiters.
    std::string readout_byte_sweep() {
        std::lock_guard<std::mutex> lk(mtx);
        std::string out;
        if (byte_lanes.size() != 256) return out;
        std::vector<float> phi(256, 0.0f);
        std::vector<float> w0(256, 0.0f);
        float peak = 0.0f;
        for (int b = 0; b < 256; ++b) {
            const ByteLane& L = byte_lanes[b];
            float A_b = std::fabs(L.displacement);
            float P_b = L.env_ema - L.env_ema_prev;
            float relu_P = (P_b > 0.0f) ? P_b : 0.0f;
            phi[b] = std::atan2(L.velocity, cpme_base_omega * L.displacement);
            if (phi[b] < 0.0f) phi[b] += 6.28318531f;
            w0[b] = A_b * relu_P;
            if (w0[b] > peak) peak = w0[b];
        }
        if (peak < 1e-9f) return out;          // silence
        float floor_w = mrs_emit_floor * peak;
        const float TWO_PI = 6.28318531f;
        bool last_was_space = true;            // suppress leading space
        // MRS §1.4 — refractory period. A lane that has just emitted is
        // transiently suppressed rather than permanently excluded. This
        // is the neuronal-refractory analog and it resolves the tension
        // between "letters must be able to repeat" (banana) and "one
        // dominant lane must not smear across the sweep" (mmmm). A
        // permanent suppression would forbid repeats; none at all lets
        // a single high-amplitude lane win a wide arc. Refractoriness
        // permits non-adjacent repetition only, which is the actual
        // statistics of orthography.
        std::vector<int> refractory(256, 0);
        for (int s = 0; s < mrs_sweep_K; ++s) {
            float phi_c = TWO_PI * (float)s / (float)mrs_sweep_K;
            int   best_b = -1;
            float best_w = 0.0f;
            for (int b = 0; b < 256; ++b) {
                float dphi = phi_c - phi[b];
                float w = w0[b] * std::exp(-mrs_sweep_kappa * dphi * dphi);
                // Continuous refractory attenuation, no branch.
                w *= 1.0f / (1.0f + (float)refractory[b]);
                if (w > best_w) { best_w = w; best_b = b; }
            }
            for (int b = 0; b < 256; ++b)
                if (refractory[b] > 0) --refractory[b];
            if (best_b < 0 || best_w < floor_w) {
                // Amplitude gap → word boundary.
                if (!last_was_space) { out += ' '; last_was_space = true; }
                continue;
            }
            // Only emit printable bytes; non-printables are treated as
            // gaps so the substrate cannot emit control characters.
            unsigned char ch = (unsigned char)best_b;
            if (ch < 32 || ch > 126) {
                if (!last_was_space) { out += ' '; last_was_space = true; }
                continue;
            }
            out += (char)ch;
            last_was_space = false;
            refractory[best_b] = mrs_refractory;
        }
        // Trim a trailing boundary.
        while (!out.empty() && out.back() == ' ') out.pop_back();
        return out;
    }

    // CPME §1.6 — Phase-Collapse Projection.
    // Emission is a monotonic sweep of a carrier phase across [0, 2π)
    // over K discrete steps. At each step the arg-max cavity under the
    // Gaussian-windowed weight
    //     w_i(φ_c) = A_i · ReLU(P_i) · μ_i · K_settle · exp(−κ(φ_c − φ_i)²)
    // is emitted, then suppressed from later steps. Per-cavity phase
    // φ_i = atan2(v_i, ω₀·x_i) is derived from existing state. Output
    // length is emergent: as many tokens as the field has distinct
    // high-momentum phase peaks above the emission floor.
    std::vector<int> readout_phase_collapse() {
        std::lock_guard<std::mutex> lk(mtx);
        std::vector<int> emitted;
        emitted.reserve(cpme_sweep_K);
        if (cavities.empty()) return emitted;
        // Precompute eligibility factors per cavity (φ, w0 = A·ReLU(P)·μ).
        std::vector<float> phi(cavities.size(), 0.0f);
        std::vector<float> w0(cavities.size(), 0.0f);
        float peak_w0            = 0.0f;
        float peak_downstream_w0 = 0.0f;   // peak among un-muted cavities
        const float mass_downstream_thresh = 0.3f;  // cavity is
        // "downstream" if its acoustic mass is below this — i.e., it
        // was not the target of a recent external drive. Driven
        // cavities have M ≳ 1 immediately after their turn; downstream
        // cavities have M ≈ 0. This decouples the emission floor from
        // the driven cavity's raw magnitude and forces honest silence
        // when the only strong cavity is muted.
        const float K_settle = cpme_last_Ksettle;
        for (size_t i = 0; i < cavities.size(); ++i) {
            const Cavity& c = cavities[i];
            float A_i = std::fabs(c.displacement);
            float P_i = c.env_ema - c.env_ema_prev;                // momentum
            float relu_P = (P_i > 0.0f) ? P_i : 0.0f;
            float mu_i = 1.0f / (1.0f + c.acoustic_mass / cpme_mass_scale);
            phi[i] = std::atan2(c.velocity, cpme_base_omega * c.displacement);
            if (phi[i] < 0.0f) phi[i] += 6.28318531f;
            w0[i] = A_i * relu_P * mu_i * K_settle;
            if (w0[i] > peak_w0) peak_w0 = w0[i];
            if (c.acoustic_mass < mass_downstream_thresh
                && w0[i] > peak_downstream_w0) {
                peak_downstream_w0 = w0[i];
            }
        }
        if (peak_w0 < 1e-9f) return emitted;   // honest silence
        // Floor references the downstream peak, NOT the raw peak. If
        // no downstream cavity clears the floor, output is silence —
        // even if the muted driven cavity has some residual w0. This
        // is the physical realisation of "nothing consequential to
        // say" (CPME §6.1).
        float floor_ref = (peak_downstream_w0 > 1e-9f)
                        ? peak_downstream_w0 : peak_w0;
        float floor_w   = cpme_emit_floor * floor_ref;
        // Sweep the carrier over [0, 2π) at K discrete positions.
        std::unordered_set<int> used;
        const float TWO_PI = 6.28318531f;
        for (int s = 0; s < cpme_sweep_K; ++s) {
            float phi_c = TWO_PI * (float)s / (float)cpme_sweep_K;
            int   best_i = -1;
            float best_w = 0.0f;
            for (size_t i = 0; i < cavities.size(); ++i) {
                if (used.count((int)i)) continue;
                float dphi = phi_c - phi[i];
                // Non-cyclic sweep — no wrap-around distance.
                float w = w0[i] * std::exp(-cpme_sweep_kappa * dphi * dphi);
                if (w > best_w) { best_w = w; best_i = (int)i; }
            }
            if (best_i < 0) continue;
            if (best_w < floor_w) continue;
            emitted.push_back(best_i);
            used.insert(best_i);
        }
        return emitted;
    }

    std::vector<int> readout_slot_winners() {
        std::lock_guard<std::mutex> lk(mtx);
        std::vector<int> winners;
        std::unordered_set<int> already;
        winners.reserve(syntax_slots.size());
        // CMD emission floor — peak field amplitude sets the scale.
        // A slot only emits if its winner carries a non-trivial share
        // of the field's peak energy. Below that: honest silence, not
        // whatever leftover noise happened to sit near the slot phase.
        float peak = 0.0f;
        for (const auto& c : cavities) {
            float a = std::fabs(c.displacement);
            peak = (a > peak) ? a : peak;
        }
        float floor_score = emission_floor_frac * peak;
        // CMD Polymath — cross-cluster energy depletion. When slot k
        // locks a cavity in macro-cluster C, that cluster's coherent
        // energy has been consumed; subsequent slots see cavities in C
        // at reduced amplitude, exactly as energy conservation would
        // predict. Clusters that haven't emitted yet compete on their
        // full amplitude — so if the field carries cross-domain energy
        // (medical + gaming + code co-active), the three slots emit
        // from three different domains without any rule. When the
        // field is single-domain, in-domain cavities still win but at
        // depleted score, and the emission floor decides silence vs
        // continued in-domain emission.
        std::unordered_map<int,float> cluster_energy_used;
        for (auto& s : syntax_slots) {
            int   best_cav = -1;
            float best_score = 0.0f;
            for (int i = 0; i < (int)cavities.size(); ++i) {
                if (already.count(i)) continue;    // exclude previous winners
                const auto& c = cavities[i];
                if (c.cluster_id < 0 ||
                    c.cluster_id >= (int)macro_nodes.size()) continue;
                const auto& m = macro_nodes[c.cluster_id];
                float phase_diff = m.phase - s.phase;
                // CMD wavefront gate — continuous exponential decay on
                // quiet age. Only cavities the current input wave has
                // actually reached (drive or tunnel propagation reset
                // their quiet_ticks) can compete for a slot. Stale
                // energy from turns ago is smoothly suppressed.
                int   qa = (i < (int)quiet_ticks.size()) ? quiet_ticks[i] : K_idle;
                float recency = std::exp(-emission_recency_rate * (float)qa);
                // Eligibility = energy × wavefront recency. This decides
                // whether the cavity may speak AT ALL — it must carry a
                // non-trivial share of peak energy AND have been reached
                // by the current input wave. Phase decides only WHICH
                // slot an eligible cavity occupies — slot and macro
                // phases rotate at different rates, so phase must never
                // gate eligibility (that yields permanent silence).
                float relevance = std::fabs(c.displacement) * recency;
                float eligible  = (relevance > floor_score) ? 1.0f : 0.0f;
                // Cluster depletion: 1 / (1 + used) attenuates cavities
                // in clusters that have already emitted a winner. Not a
                // hard exclusion — a single-domain field can still fill
                // multiple slots from the same cluster, just at reduced
                // score, so the emission floor decides silence.
                float used = cluster_energy_used[c.cluster_id];
                float depletion = 1.0f / (1.0f + used);
                float score = eligible * relevance * depletion
                            * std::exp(-4.0f * phase_diff * phase_diff);
                if (score > best_score) {
                    best_score = score;
                    best_cav   = i;
                }
            }
            s.locked_cavity_id = best_cav;
            winners.push_back(best_cav);
            if (best_cav >= 0) {
                already.insert(best_cav);
                // Deposit consumed energy on the winner's cluster.
                // 1.0 per pick tracks how many slots the cluster has
                // already fed; depletion = 1/(1+used) → after one pick
                // the cluster's cavities score at 1/2, after two at
                // 1/3, etc. Diminishing but never zero.
                int cl = cavities[best_cav].cluster_id;
                if (cl >= 0) cluster_energy_used[cl] += 1.0f;
            }
        }
        return winners;
    }

    void run(int n_ticks) { for (int i = 0; i < n_ticks; ++i) tick(); }

    // ROADMAP Phase 3 — inter-turn energy homeostasis.
    //
    // Friction alone does not clear the field. At mu = 0.006 per tick,
    // 0.994^200 = 30% of a cavity's amplitude survives a 200-tick turn.
    // A cavity driven once at amp 6 therefore stays the loudest thing
    // in the field for several subsequent turns by pure arithmetic, and
    // wins the emission readout while doing so. That is the root cause
    // of the input-echo behaviour, and it is a DISSIPATION problem, not
    // a selection problem — no amount of better readout scoring fixes
    // a field that never sheds what it was given.
    //
    // This is global gain control: if total kinetic energy exceeds a
    // budget, the whole field is scaled down uniformly to meet it.
    // Uniform scaling is deliberate — it removes accumulated loudness
    // without altering any amplitude RATIO, so nothing about which
    // cavity is currently most active is changed. It only prevents the
    // past from drowning the present.
    //
    // Biological analogue: synaptic scaling, which normalises total
    // synaptic drive while preserving relative weights.
    //
    // Returns the scale factor applied (1.0 if under budget).
    float renormalize_energy(float budget) {
        std::lock_guard<std::mutex> lk(mtx);
        float total = 0.0f;
        for (const auto& c : cavities)
            total += 0.5f * c.mass * c.velocity * c.velocity;
        if (total <= budget || total < 1e-9f) return 1.0f;
        // Energy is quadratic in amplitude, so the amplitude scale is
        // the square root of the energy ratio.
        float scale = std::sqrt(budget / total);
        for (auto& c : cavities) {
            c.x_E *= scale; c.x_I *= scale;
            c.v_E *= scale; c.v_I *= scale;
            c.displacement *= scale;
            c.velocity     *= scale;
        }
        for (auto& L : byte_lanes) {
            L.displacement *= scale;
            L.velocity     *= scale;
        }
        return scale;
    }

    void reset_field() {
        std::lock_guard<std::mutex> lk(mtx);
        for (auto& c : cavities) {
            c.displacement = 0.0f;
            c.velocity     = 0.0f;
            c.x_E = 0.0f; c.v_E = 0.0f;
            c.x_I = 0.0f; c.v_I = 0.0f;
            c.drive_amp    = 0.0f;
            c.drive_freq   = 0.0f;
            c.drive_envelope = 0.0f;
            c.drive_decay  = 1.0f;
            c.was_driven   = false;
            c.salience     = 0.0f;
        }
        // Byte lanes: clear mechanical state, KEEP learned bindings
        // (byte_bind_w) — those are memory, not field state.
        for (auto& L : byte_lanes) {
            L.displacement   = 0.0f;
            L.velocity       = 0.0f;
            L.drive_amp      = 0.0f;
            L.drive_envelope = 0.0f;
            L.drive_decay    = 1.0f;
        }
        sim_time = 0.0f;
    }

    // ---- readouts (I/O boundary, not physics) ----

    void dump(int k, FILE* out = stdout, float min_amp = 0.005f) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!graph_ptr) return;
        std::vector<std::pair<float,int>> ranked;
        ranked.reserve(cavities.size());
        for (int i = 0; i < (int)cavities.size(); ++i)
            ranked.push_back({std::fabs(cavities[i].displacement), i});
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){
                      return a.first > b.first;
                  });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= k) break;
            if (r.first < min_amp) break;
            int idx = r.second;
            const Cavity& c = cavities[idx];
            std::fprintf(out,
                "%s | x: %+.5f | v: %+.5f | S: %.3f | env: %.3f | mac:%d | %s\n",
                graph_ptr->graph.nodes[idx].label.c_str(),
                c.displacement, c.velocity, c.salience, c.drive_envelope,
                c.cluster_id, c.was_driven ? "driven" : "emergent");
            ++shown;
        }
    }

    // Energy audit. Returns (kinetic, potential, driver_power, plasticity).
    // Kinetic = Σ ½ m v²
    // Potential = Σ ½ k (x_a − p·x_b)²  over tunnels
    // Driver = Σ |F_drive|   (instantaneous injected power surrogate)
    // Plasticity = Σ ½ Δ²    (energy stored in learned stiffness)
    struct EnergyAudit {
        float kinetic;
        float potential;
        float driver;
        float plasticity;
        float total_mechanical;
    };
    EnergyAudit audit_energy() {
        std::lock_guard<std::mutex> lk(mtx);
        return audit_energy_locked();
    }
    // Internal — assumes caller holds mtx. Used by Layer 2's
    // homeostatic energy feedback for auto-tuned fission thresholds.
    EnergyAudit audit_energy_locked() {
        EnergyAudit a{0,0,0,0,0};
        for (const auto& c : cavities) {
            a.kinetic += 0.5f * c.mass * c.velocity * c.velocity;
            a.driver  += std::fabs(c.drive_amp * c.drive_envelope);
        }
        for (const auto& t : tunnels) {
            float x_a_eff = t.polarity * cavities[t.cavity_a].displacement;
            float dx      = x_a_eff - cavities[t.cavity_b].displacement;
            a.potential  += 0.5f * t.stiffness * dx * dx;
            a.plasticity += 0.5f * t.plasticity_delta * t.plasticity_delta;
        }
        a.total_mechanical = a.kinetic + a.potential;
        return a;
    }

    void dump_energy(FILE* out = stdout) {
        auto a = audit_energy();
        std::fprintf(out, "T=%.5f  U=%.5f  T+U=%.5f  driver=%.5f  plasticity=%.5f\n",
                     a.kinetic, a.potential, a.total_mechanical,
                     a.driver, a.plasticity);
    }

    void dump_state(FILE* out = stdout) {
        std::lock_guard<std::mutex> lk(mtx);
        std::fprintf(out,
            "sim_time=%.3f  D=%.4f  C=%.4f  S=%.4f  "
            "tension=%.5f  coherence=%.5f  identity=%.5f  "
            "sil_endurance=%.5f  input_power_avg=%.5f  "
            "macros=%zu\n",
            sim_time, nt.dopamine, nt.cortisol, nt.serotonin,
            ic.tension, ic.coherence, ic.identity_strength,
            sil.endurance, sil.input_power_avg,
            macro_nodes.size());
    }

    // Steel-tunnel readout — the base_stiffness classifies steel.
    // Rubber ≤ 0.9, aluminium ~1.0-1.3, steel ≥ 1.4.
    void dump_steel_diffs(FILE* out = stdout, float steel_threshold = 1.4f) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!graph_ptr) return;
        std::fprintf(out, "D=%.4f  C=%.4f  S=%.4f\n",
                     nt.dopamine, nt.cortisol, nt.serotonin);
        std::vector<std::pair<float,int>> ranked;
        for (int i = 0; i < (int)tunnels.size(); ++i) {
            if (tunnels[i].base_stiffness < steel_threshold) continue;
            ranked.push_back({std::fabs(tunnels[i].plasticity_delta), i});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){
                      return a.first > b.first;
                  });
        for (const auto& r : ranked) {
            const Tunnel& t = tunnels[r.second];
            std::fprintf(out,
                "%s -> %s | pol: %+.1f | base_k: %.3f | Δ: %+.5f | k: %.5f\n",
                graph_ptr->graph.nodes[t.cavity_a].label.c_str(),
                graph_ptr->graph.nodes[t.cavity_b].label.c_str(),
                t.polarity, t.base_stiffness,
                t.plasticity_delta, t.stiffness);
        }
    }

    void dump_salience(int k, FILE* out = stdout, float min_s = 0.02f) {
        std::lock_guard<std::mutex> lk(mtx);
        if (!graph_ptr) return;
        std::vector<std::pair<float,int>> ranked;
        for (int i = 0; i < (int)cavities.size(); ++i)
            ranked.push_back({cavities[i].salience, i});
        std::sort(ranked.begin(), ranked.end(),
                  [](const std::pair<float,int>& a,
                     const std::pair<float,int>& b){
                      return a.first > b.first;
                  });
        int shown = 0;
        for (const auto& r : ranked) {
            if (shown >= k) break;
            if (r.first < min_s) break;
            std::fprintf(out, "%s | S: %.4f\n",
                graph_ptr->graph.nodes[r.second].label.c_str(),
                r.first);
            ++shown;
        }
    }

    void dump_macros(FILE* out = stdout) {
        std::lock_guard<std::mutex> lk(mtx);
        for (size_t i = 0; i < macro_nodes.size(); ++i) {
            const auto& m = macro_nodes[i];
            std::fprintf(out,
                "macro[%zu] size=%zu  ω=%.2f  phase=%+.3f  A=%.4f\n",
                i, m.cavity_ids.size(), m.natural_omega,
                m.phase, m.amplitude);
        }
    }
};

} // namespace unified
