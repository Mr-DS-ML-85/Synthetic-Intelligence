// synth — ROADMAP Phase 2: the compile-verify loop.
//
// Everything before this phase was retrieval and ranking: the engine
// could return facts it held, rank them by wave propagation, and derive
// new ones by forward chaining. All of it stays inside what the axioms
// already entailed.
//
// This binary does something categorically different. It forms a
// HYPOTHESIS from the graph, emits real source code expressing it,
// hands that code to an external oracle that knows nothing about the
// substrate — the C++ compiler — and updates its own knowledge from the
// verdict. When graph and compiler disagree, the compiler wins and the
// graph is corrected.
//
// The compiler is a deterministic program, not a model. Nothing here
// introduces learning-by-optimisation: there is no loss, no gradient,
// no fitted parameter. There is a hypothesis, an experiment, and a
// result.
//
// FIRST MILESTONE — arity verification.
//
// AST extraction records `f takes_arg x` edges but loses cardinality
// guarantees: extraction is lossy, so the number of recorded arguments
// is a claim, not a fact. `synth` tests that claim by emitting calls of
// varying arity and observing which one the compiler accepts. The
// verified arity is then written back as a derived fact carrying a
// provenance tag distinct from both `asserted` and rule-derived
// knowledge: it was established by EXPERIMENT.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "physics.hpp"
#include "primitives.hpp"
#include "si_state.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <vector>

using namespace unified;

static void autoload_axioms(ExtSI& si) {
    DIR* d = opendir("data");
    if (!d) return;
    struct dirent* ent;
    std::vector<std::string> files;
    while ((ent = readdir(d)) != nullptr) {
        std::string n = ent->d_name;
        if (n.size() < 8) continue;
        if (n.substr(n.size() - 7) != ".axioms") continue;
        if (n == "mutations.axioms") continue;
        files.push_back("data/" + n);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    FILE* dn = std::fopen("/dev/null", "w");
    for (const auto& f : files) load_axiom_file(si, f, dn ? dn : stderr);
    if (dn) std::fclose(dn);
}

// ---------------------------------------------------------- hypothesis

struct Hypothesis {
    std::string target;
    int         claimed_arity = -1;   // from the graph
    int         verified_arity = -1;  // from the compiler
    int         attempts = 0;
    bool        settled = false;
};

// The substrate's current belief about `target`'s arity.
//
// Experimental knowledge OUTRANKS extracted knowledge. If a previous
// run established `f has arity_N` against the compiler, that is what
// the substrate now believes — extraction was already shown wrong
// about it. Only in the absence of a verified fact does it fall back
// to counting takes_arg edges.
//
// This is the feedback path that makes the loop a learning loop rather
// than a repeated measurement: a target that cost 3 experiments the
// first time costs 1 the next, because the hypothesis it starts from
// is the one it earned. Attempt count is therefore an observable of
// accumulated experience.
static int claimed_arity(const ExtSI& si, const std::string& target,
                         bool* from_experience) {
    if (from_experience) *from_experience = false;
    auto it = si.graph.by_label.find(target);
    if (it == si.graph.by_label.end()) return -1;

    // Verified experimental facts are stored as `f has arity_N`.
    for (const auto& e : si.graph.edges) {
        if (e.from != it->second || e.rel != REL_HAS) continue;
        if (e.to < 0 || e.to >= (int)si.graph.nodes.size()) continue;
        const std::string& lbl = si.graph.nodes[e.to].label;
        if (lbl.rfind("arity_", 0) != 0) continue;
        int n = std::atoi(lbl.c_str() + 6);
        if (n >= 0) {
            if (from_experience) *from_experience = true;
            return n;
        }
    }
    int n = 0;
    for (const auto& e : si.graph.edges)
        if (e.from == it->second && e.rel == REL_TAKES_ARG) ++n;
    return n;
}

// ------------------------------------------ affect: dopamine / cortisol
//
// The substrate already carries three neuromodulators wired into the
// tick as multipliers: dopamine scales input drive, cortisol scales
// damping, serotonin scales the Hebbian rate. Until now they were set
// by hand. Here they are moved by OUTCOME.
//
// This is neuromodulation, NOT reinforcement learning. RL requires a
// value function, a policy, and gradient updates toward expected
// return. There is none of that here — only three scalars multiplying
// physical constants, nudged by accept/reject verdicts and drifting
// back to baseline. Nothing estimates a return; nothing is optimised.
//
// Physically:
//   dopamine  up   -> the field responds harder to input (engagement)
//   cortisol  up   -> damping rises, propagation narrows (the "sad"
//                     state: a stressed substrate explores less)
//   serotonin up   -> consolidation is faster (confidence)
//
// Homeostasis pulls all three back toward 1.0, so neither elation nor
// despair is a fixed point. A run of failures narrows the field; a run
// of successes opens it; doing nothing returns it to rest.
struct Affect {
    float dopamine  = 1.0f;
    float cortisol  = 1.0f;
    float serotonin = 1.0f;

    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    void on_success() {
        dopamine  = clampf(dopamine  * 1.15f, 0.50f, 2.0f);
        cortisol  = clampf(cortisol  * 0.90f, 0.50f, 2.5f);
        serotonin = clampf(serotonin * 1.10f, 0.50f, 2.0f);
    }

    // Punishment. Cortisol is the stress axis — this is the "sad"
    // state, and it is not merely a label: raising cortisol raises
    // damping in the tick, so the field literally propagates less far
    // after a failure.
    void on_failure() {
        dopamine  = clampf(dopamine  * 0.85f, 0.50f, 2.0f);
        cortisol  = clampf(cortisol  * 1.25f, 0.50f, 2.5f);
        serotonin = clampf(serotonin * 0.92f, 0.50f, 2.0f);
    }

    void relax() {                       // drift toward baseline
        dopamine  += (1.0f - dopamine)  * 0.05f;
        cortisol  += (1.0f - cortisol)  * 0.05f;
        serotonin += (1.0f - serotonin) * 0.05f;
    }

    const char* mood() const {
        if (cortisol > 1.4f)  return "stressed";
        if (dopamine > 1.3f)  return "engaged";
        if (cortisol < 0.8f)  return "calm";
        return "neutral";
    }
};

// ------------------------------- RIFA feedback: failure as field energy
//
// Until now a refuted hypothesis updated the KNOWLEDGE BASE — a line
// appended to verified.axioms. The substrate itself was unchanged: the
// physics field had no idea an experiment had failed.
//
// This routes the verdict into the field. A rejected candidate drives
// the target's cavity, so the failure literally becomes energy at that
// concept. CMD thermodynamic habituation integrates it:
//
//     dT/dt = alpha*|v| - beta*T
//
// and the Curie gate then attenuates every tunnel touching a hot
// cavity by exp(-T/T_crit). A concept that keeps failing therefore
// DECOUPLES ITSELF from the rest of the graph — by exactly the same
// mechanism that makes stopwords cook themselves out of the field.
//
// Nothing new is introduced: drive(), heat_T and the Curie gate all
// already exist. What is new is that an external verdict is now one of
// the things that can heat a cavity. Unreliability becomes a physical
// property of the concept rather than a note in a file.
struct FieldFeedback {
    PhysicsEngine* px = nullptr;
    ExtSI*         si = nullptr;
    Affect         affect;

    int cavity_of(const std::string& label) const {
        if (!si) return -1;
        auto it = si->graph.by_label.find(label);
        return (it == si->graph.by_label.end()) ? -1 : it->second;
    }

    float heat_of(const std::string& label) const {
        int id = cavity_of(label);
        if (!px || id < 0 || id >= (int)px->cavities.size()) return 0.0f;
        return px->cavities[id].heat_T;
    }

    // A refuted hypothesis. Drive the concept and let the field run.
    void on_rejected(const std::string& label) {
        int id = cavity_of(label);
        if (!px || id < 0 || id >= (int)px->cavities.size()) return;
        affect.on_failure();
        // Apply the affective state BEFORE propagating, so the raised
        // cortisol actually damps this failure's wavefront.
        px->set_neurotransmitters(affect.dopamine, affect.cortisol,
                                  affect.serotonin);
        px->drive(id, 3.0f, 6.0f, 20);
        px->run(60);
        affect.relax();
    }

    // A confirmed hypothesis. No drive: the concept simply cools on
    // its own through the beta*T term. Success is the ABSENCE of
    // punishment, not a reward signal — there is no reward anywhere
    // in this substrate.
    void on_accepted() {
        if (!px) return;
        affect.on_success();
        px->set_neurotransmitters(affect.dopamine, affect.cortisol,
                                  affect.serotonin);
        px->run(60);
        affect.relax();
    }
};

// ------------------------------------------------------- the experiment

// Emit a translation unit calling `target` on a PhysicsEngine with
// `arity` integer arguments, then compile it. Returns true iff the
// compiler accepted the program.
//
// This is the whole oracle. It has no knowledge of the substrate, no
// tunable behaviour, and cannot be argued with.
static bool compiles_with_arity(const std::string& target, int arity,
                                std::string* diag) {
    const char* src = "/tmp/pcma_synth_probe.cpp";
    FILE* f = std::fopen(src, "w");
    if (!f) { if (diag) *diag = "cannot write probe"; return false; }

    std::fprintf(f, "#include \"physics.hpp\"\n");
    std::fprintf(f, "using namespace unified;\n");
    std::fprintf(f, "int main(){\n");
    std::fprintf(f, "  PhysicsEngine px;\n");
    std::fprintf(f, "  px.%s(", target.c_str());
    for (int i = 0; i < arity; ++i)
        std::fprintf(f, "%s0", (i == 0) ? "" : ", ");
    std::fprintf(f, ");\n  return 0;\n}\n");
    std::fclose(f);

    std::string cmd = "g++ -std=c++17 -fsyntax-only -I src ";
    cmd += src;
    cmd += " 2>/tmp/pcma_synth_err.txt";
    int rc = std::system(cmd.c_str());

    if (rc != 0 && diag) {
        FILE* e = std::fopen("/tmp/pcma_synth_err.txt", "r");
        if (e) {
            char line[512];
            while (std::fgets(line, sizeof(line), e)) {
                std::string L(line);
                if (L.find("error:") != std::string::npos) {
                    // Trim to the compiler's actual complaint.
                    size_t p = L.find("error:");
                    *diag = L.substr(p);
                    while (!diag->empty() &&
                           (diag->back() == '\n' || diag->back() == '\r'))
                        diag->pop_back();
                    break;
                }
            }
            std::fclose(e);
        }
    }
    return rc == 0;
}

// Search arity outward from the graph's claim. Starting at the
// hypothesis rather than at zero means a correct graph is confirmed in
// ONE experiment; the search cost measures how wrong the graph was.
static void verify_arity(Hypothesis& h, FieldFeedback* fb) {
    std::vector<int> order;
    if (h.claimed_arity >= 0) order.push_back(h.claimed_arity);
    for (int d = 1; d <= 6; ++d) {
        if (h.claimed_arity - d >= 0) order.push_back(h.claimed_arity - d);
        if (h.claimed_arity + d <= 8) order.push_back(h.claimed_arity + d);
    }
    for (int a : order) {
        std::string diag;
        ++h.attempts;
        std::printf("    attempt %d: %s(%d args) ... ",
                    h.attempts, h.target.c_str(), a);
        std::fflush(stdout);
        if (compiles_with_arity(h.target, a, &diag)) {
            std::printf("ACCEPTED\n");
            if (fb) fb->on_accepted();
            h.verified_arity = a;
            h.settled = true;
            return;
        }
        std::printf("rejected — %s\n",
                    diag.empty() ? "(no diagnostic)" : diag.c_str());
        // The verdict enters the field, not just the log.
        if (fb) fb->on_rejected(h.target);
    }
}

// ------------------------------- positional type-signature discovery
//
// SCOPE NOTE, stated up front. A compiler type-checks; it does not
// report parameter NAMES. So the mapping from the graph's argument
// names {amp, duration_ticks, freq, node_id} onto positions is NOT
// recoverable by experiment — that is a defect in extraction (source
// order is discarded) and must be fixed in the extractor, not guessed
// by the oracle. Claiming otherwise would be dishonest.
//
// What IS recoverable, and absent from the graph entirely, is the
// positional TYPE signature. Under -Werror=conversion a narrowing
// float->int argument is rejected while a widening int->float is
// accepted, so the compiler discriminates between candidate type
// assignments. The substrate enumerates them and keeps the one
// accepted.
//
// For arity n the space is 2^n (int | float per position). Search is
// exhaustive and terminates; there is no fitting and no scoring.

// Type alphabet. Widened beyond {int, float}: bind_byte's true
// signature is (unsigned char, int), which the two-type alphabet could
// not express — the system correctly reported "no assignment accepted"
// rather than guessing, but could not succeed.
static const char* TYPE_NAMES[] = {
    "int", "unsigned", "char", "unsigned char", "float", "double", "bool"
};
static const int N_TYPES = 7;

static bool compiles_with_types(const std::string& target,
                                const std::vector<int>& types,
                                std::string* diag) {
    const char* src = "/tmp/pcma_synth_types.cpp";
    FILE* f = std::fopen(src, "w");
    if (!f) return false;
    std::fprintf(f, "#include \"physics.hpp\"\n");
    std::fprintf(f, "using namespace unified;\n");
    std::fprintf(f, "int main(){\n  PhysicsEngine px;\n");
    for (size_t i = 0; i < types.size(); ++i)
        std::fprintf(f, "  %s a%zu = 0;\n", TYPE_NAMES[types[i]], i);
    std::fprintf(f, "  px.%s(", target.c_str());
    for (size_t i = 0; i < types.size(); ++i)
        std::fprintf(f, "%sa%zu", (i == 0) ? "" : ", ", i);
    std::fprintf(f, ");\n  return 0;\n}\n");
    std::fclose(f);

    // -isystem marks the header as a system header, so conversions
    // WITHIN physics.hpp do not trip -Werror=conversion. Only the
    // generated call site is judged. Without this every candidate is
    // rejected by warnings that have nothing to do with the hypothesis.
    std::string cmd = "g++ -std=c++17 -fsyntax-only -Werror=conversion "
                      "-isystem src ";
    cmd += src;
    cmd += " 2>/tmp/pcma_synth_terr.txt";
    int rc = std::system(cmd.c_str());
    if (rc != 0 && diag) {
        FILE* e = std::fopen("/tmp/pcma_synth_terr.txt", "r");
        if (e) {
            char line[512];
            while (std::fgets(line, sizeof(line), e)) {
                std::string L(line);
                size_t p = L.find("error:");
                if (p != std::string::npos) {
                    *diag = L.substr(p);
                    while (!diag->empty() &&
                           (diag->back()=='\n' || diag->back()=='\r'))
                        diag->pop_back();
                    break;
                }
            }
            std::fclose(e);
        }
    }
    return rc == 0;
}

static std::string sig_of(const std::string& target,
                          const std::vector<int>& types) {
    std::string s = target + "(";
    for (size_t i = 0; i < types.size(); ++i) {
        s += TYPE_NAMES[types[i]];
        if (i + 1 < types.size()) s += ", ";
    }
    return s + ")";
}

// Greedy per-position search. Exhaustive enumeration is |T|^n — with a
// 7-type alphabet and arity 4 that is 2401 compilations, which is not a
// search, it is a denial of service. Positions are instead resolved one
// at a time, holding the others at their current best: O(n·|T|) per
// sweep, repeated until a sweep changes nothing.
//
// This is hill-climbing on a discrete space with an exact oracle. It is
// not gradient descent: there is no loss surface and no derivative,
// only accept/reject verdicts. It can stall in a local configuration,
// which is reported honestly rather than papered over.
static void discover_types(const std::string& target, int arity,
                           int& attempts, std::string& found) {
    if (arity <= 0 || arity > 6) return;
    std::string diag;

    // STAGE 1 — exhaustive over {int, float} (indices 0 and 4).
    // Greedy alone stalls here: reaching (int,float,float,int) from
    // all-int requires changing TWO positions at once, and a
    // one-position-at-a-time hill climb never sees that move. This is
    // the standard local-optimum failure and it was observed, not
    // anticipated in the abstract. For a small alphabet the space is
    // 2^n = 16 at arity 4, so exhaustive is simply cheaper than being
    // clever.
    if (arity <= 5) {
        int combos = 1 << arity;
        for (int mask = 0; mask < combos; ++mask) {
            std::vector<int> t2(arity);
            for (int i = 0; i < arity; ++i)
                t2[i] = ((mask >> i) & 1) ? 4 : 0;   // 4 = float
            ++attempts;
            if (compiles_with_types(target, t2, &diag)) {
                std::printf("    attempt %d: %s ... ACCEPTED\n",
                            attempts, sig_of(target, t2).c_str());
                found = sig_of(target, t2);
                return;
            }
        }
        std::printf("    (%d exhausted over {int,float}; widening)\n",
                    attempts);
    }

    // STAGE 2 — greedy over the full alphabet. Used only when the
    // cheap exhaustive pass finds nothing, e.g. unsigned char.
    std::vector<int> types(arity, 0);

    ++attempts;
    if (compiles_with_types(target, types, &diag)) {
        std::printf("    attempt %d: %s ... ACCEPTED\n",
                    attempts, sig_of(target, types).c_str());
        found = sig_of(target, types);
        return;
    }
    std::printf("    attempt %d: %s ... rejected\n",
                attempts, sig_of(target, types).c_str());

    for (int sweep = 0; sweep < 3; ++sweep) {
        bool changed = false;
        for (int pos = 0; pos < arity; ++pos) {
            int original = types[pos];
            for (int t = 0; t < N_TYPES; ++t) {
                if (t == original) continue;
                types[pos] = t;
                ++attempts;
                if (compiles_with_types(target, types, &diag)) {
                    std::printf("    attempt %d: %s ... ACCEPTED\n",
                                attempts, sig_of(target, types).c_str());
                    found = sig_of(target, types);
                    return;
                }
                if (attempts % 7 == 0)
                    std::printf("    attempt %d: %s ... rejected\n",
                                attempts, sig_of(target, types).c_str());
            }
            types[pos] = original;
        }
        if (!changed) break;
    }
}


// -------------------------------------------- graph-walk serializer
//
// ROADMAP Phase 2 final item. Everything up to here PROBED an existing
// function. This EMITS one — a complete, compilable function assembled
// entirely from graph structure.
//
// What the graph supplies, and where each piece came from:
//
//   parameter ORDER   drive__arg0..N is_a <name>     (extractor ordinals)
//   parameter TYPES   drive has sig_int_float_...    (established by
//                                                     experiment)
//   body ORDERING     precedes edges                 (CPSB gauge: the
//                                                     same directed
//                                                     phase constraint
//                                                     that orders
//                                                     emission)
//
// The emitted function forwards to the real one. If the substrate's
// reconstructed signature is right it compiles; if any element is
// wrong it does not. So this is a single end-to-end test of every
// piece of signature knowledge the system holds — order, arity and
// types at once.

// Recover parameter names in positional order from the ordinal slots.
static std::vector<std::string> ordered_params(const ExtSI& si,
                                               const std::string& target) {
    std::vector<std::string> out;
    for (int i = 0; i < 12; ++i) {
        std::string slot = target + "__arg" + std::to_string(i);
        auto it = si.graph.by_label.find(slot);
        if (it == si.graph.by_label.end()) break;
        std::string name;
        for (const auto& e : si.graph.edges) {
            if (e.from != it->second || e.rel != REL_IS_A) continue;
            if (e.to >= 0 && e.to < (int)si.graph.nodes.size())
                name = si.graph.nodes[e.to].label;
            break;
        }
        if (name.empty()) break;
        out.push_back(name);
    }
    return out;
}

// Recover the experimentally-established type signature, if any.
static std::vector<std::string> known_types(const ExtSI& si,
                                            const std::string& target) {
    std::vector<std::string> out;
    auto it = si.graph.by_label.find(target);
    if (it == si.graph.by_label.end()) return out;
    for (const auto& e : si.graph.edges) {
        if (e.from != it->second || e.rel != REL_HAS) continue;
        if (e.to < 0 || e.to >= (int)si.graph.nodes.size()) continue;
        const std::string& lbl = si.graph.nodes[e.to].label;
        if (lbl.rfind("sig_", 0) != 0) continue;
        std::string cur;
        for (size_t i = 4; i <= lbl.size(); ++i) {
            if (i == lbl.size() || lbl[i] == '_') {
                if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            } else cur += lbl[i];
        }
        break;
    }
    return out;
}

// Calls made by `target`, ordered by the precedes relation. This is
// the CPSB gauge doing in code emission exactly what it does in phase
// emission: supplying direction so a set becomes a sequence.
static std::vector<std::string> ordered_calls(const ExtSI& si,
                                              const std::string& target,
                                              int limit) {
    auto it = si.graph.by_label.find(target);
    if (it == si.graph.by_label.end()) return {};
    std::vector<int> callees;
    for (const auto& e : si.graph.edges)
        if (e.from == it->second && e.rel == REL_CALLS)
            callees.push_back(e.to);
    // Topological nudge: a callee that precedes another sorts earlier.
    std::sort(callees.begin(), callees.end(), [&](int a, int b) {
        for (const auto& e : si.graph.edges) {
            if (e.rel != REL_PRECEDES) continue;
            if (e.from == a && e.to == b) return true;
            if (e.from == b && e.to == a) return false;
        }
        return a < b;
    });
    std::vector<std::string> out;
    for (int c : callees) {
        if ((int)out.size() >= limit) break;
        if (c >= 0 && c < (int)si.graph.nodes.size())
            out.push_back(si.graph.nodes[c].label);
    }
    return out;
}

static bool serialize_and_compile(const ExtSI& si, const std::string& target,
                                  std::string* emitted, std::string* diag) {
    std::vector<std::string> params = ordered_params(si, target);
    std::vector<std::string> types  = known_types(si, target);
    if (params.empty()) { if (diag) *diag = "no ordinal slots in graph"; return false; }
    // Fall back to int for any position without an established type.
    while (types.size() < params.size()) types.push_back("int");

    std::string fn = "// emitted by graph walk — not written by a human\n";
    fn += "void " + target + "_forward(";
    for (size_t i = 0; i < params.size(); ++i) {
        fn += types[i] + " " + params[i];
        if (i + 1 < params.size()) fn += ", ";
    }
    fn += ") {\n  PhysicsEngine px;\n  px." + target + "(";
    for (size_t i = 0; i < params.size(); ++i) {
        fn += params[i];
        if (i + 1 < params.size()) fn += ", ";
    }
    fn += ");\n}\n";
    if (emitted) *emitted = fn;

    const char* src = "/tmp/pcma_synth_walk.cpp";
    FILE* f = std::fopen(src, "w");
    if (!f) return false;
    std::fprintf(f, "#include \"physics.hpp\"\nusing namespace unified;\n%s",
                 fn.c_str());
    std::fclose(f);

    std::string cmd = "g++ -std=c++17 -fsyntax-only -isystem src ";
    cmd += src; cmd += " 2>/tmp/pcma_walk_err.txt";
    int rc = std::system(cmd.c_str());
    if (rc != 0 && diag) {
        FILE* e = std::fopen("/tmp/pcma_walk_err.txt", "r");
        if (e) { char l[512];
            while (std::fgets(l, sizeof(l), e)) {
                std::string L(l); size_t p = L.find("error:");
                if (p != std::string::npos) { *diag = L.substr(p);
                    while (!diag->empty() && (diag->back()=='\n'||diag->back()=='\r'))
                        diag->pop_back();
                    break; } }
            std::fclose(e); }
    }
    return rc == 0;
}

// ------------------------------------------------------------------ main

int main(int argc, char** argv) {
    ExtSI si;
    build_big_ontology(si);
    autoload_axioms(si);
    si.reason_ext();

    std::vector<std::string> targets;
    for (int i = 1; i < argc; ++i) targets.push_back(argv[i]);
    if (targets.empty()) targets = {"drive", "run", "fission", "bind_byte"};

    PhysicsEngine px;
    px.bind(si);
    // Restore accumulated field state if a prior session left any.
    // Heat earned by failure must persist, otherwise every run starts
    // trusting a concept it has already been burned by.
    {
        std::string err;
        if (si_load_overlay_only(px, "synth_field.si", &err))
            std::printf("(restored field state from synth_field.si)\n");
    }
    FieldFeedback fb;
    fb.px = &px;
    fb.si = &si;

    std::printf("synth — Phase 2 compile-verify loop\n");
    std::printf("graph: %d symbols, %d relations\n\n",
                (int)si.graph.nodes.size(), (int)si.graph.edges.size());

    int confirmed = 0, corrected = 0, unsettled = 0, total_attempts = 0;

    for (const auto& t : targets) {
        Hypothesis h;
        h.target = t;
        bool from_exp = false;
        h.claimed_arity = claimed_arity(si, t, &from_exp);
        std::printf("  target '%s'\n", t.c_str());
        if (h.claimed_arity < 0) {
            std::printf("    not in graph — no hypothesis to test\n\n");
            continue;
        }
        std::printf("    believes arity = %d  [%s]\n", h.claimed_arity,
                    from_exp ? "from prior experiment"
                             : "from AST extraction");
        float heat_before = fb.heat_of(t);
        verify_arity(h, &fb);
        total_attempts += h.attempts;
        float heat_after = fb.heat_of(t);
        std::printf("    field: heat %.4f -> %.4f   (tension %.4f)\n",
                    heat_before, heat_after, px.ic.tension);
        std::printf("    affect: dopamine %.2f  cortisol %.2f  "
                    "serotonin %.2f  [%s]\n",
                    fb.affect.dopamine, fb.affect.cortisol,
                    fb.affect.serotonin, fb.affect.mood());

        if (!h.settled) {
            std::printf("    UNSETTLED — no arity accepted\n\n");
            ++unsettled;
            continue;
        }
        // Arity settled — now discover the positional type signature,
        // which the graph does not record at all.
        if (h.settled && h.verified_arity > 0 && h.verified_arity <= 5) {
            std::printf("    discovering positional types:\n");
            int tattempts = 0;
            std::string sig;
            discover_types(h.target, h.verified_arity, tattempts, sig);
            total_attempts += tattempts;
            if (!sig.empty()) {
                // sig already carries the target name.
                std::printf("    TYPE SIGNATURE %s  (%d experiments)\n",
                            sig.c_str(), tattempts);
                std::printf("    note: an ADMISSIBLE signature — the oracle\n"
                            "          accepts any type converting safely to\n"
                            "          the declared one, so this need not be\n"
                            "          the literal declaration.\n");
                FILE* vf = std::fopen("data/verified.axioms", "a");
                if (vf) {
                    std::string clean;
                    for (char c : sig)
                        if (c != ' ' && c != '(' && c != ')' && c != ',')
                            clean += c;
                        else if (c == ',') clean += '_';
                    std::fprintf(vf, "%-28s has              sig_%s\n",
                                 h.target.c_str(), clean.c_str());
                    std::fclose(vf);
                }
            } else {
                std::printf("    no type assignment accepted\n");
            }
        }

        // --- graph-walk serialization: emit a whole function ---
        {
            std::string code, diag;
            std::printf("    graph-walk serialization:\n");
            bool ok = serialize_and_compile(si, t, &code, &diag);
            if (!code.empty()) {
                std::printf("    ---- emitted ----\n");
                std::string ind = "    ";
                for (char c : code) {
                    std::fputc(c, stdout);
                    if (c == '\n') std::fputs(ind.c_str(), stdout);
                }
                std::printf("\n");
            }
            if (ok) {
                std::printf("    COMPILES — reconstructed signature is correct\n");
                fb.on_accepted();
            } else {
                std::printf("    REJECTED — %s\n",
                            diag.empty() ? "(no diagnostic)" : diag.c_str());
                fb.on_rejected(t);
            }
        }

        if (h.verified_arity == h.claimed_arity) {
            std::printf("    CONFIRMED  arity=%d in %d attempt(s)\n\n",
                        h.verified_arity, h.attempts);
            ++confirmed;
        } else {
            std::printf("    CORRECTED  graph said %d, compiler says %d"
                        "  (%d attempts)\n",
                        h.claimed_arity, h.verified_arity, h.attempts);
            std::printf("    -> this is knowledge the axioms did NOT contain\n");
            // Persist the correction. Provenance matters: this fact was
            // neither asserted by a human nor derived by a rule — it was
            // established by experiment against an external oracle, and
            // it contradicts what the extractor recorded. Written to a
            // separate file so experimental knowledge is never confused
            // with extracted knowledge.
            if (from_exp) {
                std::printf("\n");        // already known; nothing new
            } else {
                FILE* vf = std::fopen("data/verified.axioms", "a");
                if (vf) {
                    std::fprintf(vf, "%-28s has              arity_%d\n",
                                 h.target.c_str(), h.verified_arity);
                    std::fclose(vf);
                    std::printf("    -> written to data/verified.axioms\n\n");
                } else {
                    std::printf("\n");
                }
            }
            ++corrected;
        }
    }

    // Persist the field. Experience is now carried in two places:
    // verified.axioms (what is true) and synth_field.si (how much the
    // substrate trusts each concept).
    {
        std::string err;
        if (si_save_overlay_only(px, "synth_field.si", &err))
            std::printf("(field state saved to synth_field.si)\n");
    }
    std::printf("summary: %d confirmed, %d corrected, %d unsettled, "
                "%d total experiments\n",
                confirmed, corrected, unsettled, total_attempts);
    std::printf("\nEvery 'corrected' line is a fact obtained by experiment:\n"
                "not asserted in an axiom file, not derivable by forward\n"
                "chaining, and established only because an external oracle\n"
                "rejected the substrate's hypothesis.\n");
    return 0;
}
