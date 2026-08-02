// ask — Layer 1 substrate shell (PRM + MIMS + HNCA + NTDA + TSDA +
// RIFA + ZeroGrad Hebbian).
//
// Commands (labels are I/O boundary only):
//   drive <label> <freq> <amp> [duration_ticks]   attach a sinusoidal driver
//   step [n_ticks]                                integrate the field
//   dump [top_k]                                  emit vibrating cavities
//   salience [top_k]                              working-memory readout
//   plasticity [top_k]                            learned tunnels
//   macros                                        HNCA macro-node phases
//   physstate                                     NTDA + RIFA scalars
//   nt <D> <C> <S>                                set neurotransmitters
//   reset                                         zero the field (keep learning)
//   rebind                                        rebuild engine from graph
//   load <path.csv>                               ingest a CSV of triples
//   dream on                                      start the discovery daemon
//   dream off                                     stop the daemon
//   state                                         node/edge/daemon count
//   quit
//
// Every output line has fixed structure. No sentences.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "csv_load.hpp"
#include "dreamer.hpp"
#include "layer2.hpp"
#include "physics.hpp"
#include "primitives.hpp"
#include "rade.hpp"
#include "stream.hpp"
#include "unified.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace unified;

static void autoload_axioms(ExtSI& si) {
    DIR* d = opendir("data");
    if (!d) return;
    struct dirent* ent;
    std::vector<std::string> files;
    while ((ent = readdir(d)) != nullptr) {
        std::string name = ent->d_name;
        if (name.size() < 8) continue;
        if (name.substr(name.size() - 7) != ".axioms") continue;
        if (name == "mutations.axioms") continue;   // ledger, not axioms
        files.push_back("data/" + name);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    FILE* devnull = std::fopen("/dev/null", "w");
    for (const auto& f : files)
        load_axiom_file(si, f, devnull ? devnull : stderr);
    if (devnull) std::fclose(devnull);
}

int main(int, char**) {
    ExtSI si;
    build_big_ontology(si);
    autoload_axioms(si);
    si.reason_ext();

    PhysicsEngine physics;
    physics.bind(si);
    ConstructiveEngine constructive;
    constructive.bind(&physics);
    RADE rade;
    rade.bind(&physics, &constructive);
    StreamIngester stream;
    stream.bind(&physics, si);

    Dreamer dreamer;

    std::string line;
    while (true) {
        if (!std::getline(std::cin, line)) break;
        // trim
        size_t a = 0, b = line.size();
        while (a < b && std::isspace((unsigned char)line[a])) ++a;
        while (b > a && std::isspace((unsigned char)line[b-1])) --b;
        line = line.substr(a, b - a);
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "drive") {
            std::string label; float freq = 0.0f; float amp = 0.0f; int dur = 20;
            if (!(iss >> label >> freq >> amp)) continue;
            iss >> dur;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) {
                std::fprintf(stderr, "unknown_label %s\n", label.c_str());
                continue;
            }
            physics.drive(it->second, freq, amp, dur);
        }
        else if (cmd == "step") {
            int n = 100;
            iss >> n;
            physics.run(n);
        }
        else if (cmd == "dump") {
            int k = 20;
            iss >> k;
            physics.dump(k);
        }
        else if (cmd == "salience") {
            int k = 15;
            iss >> k;
            physics.dump_salience(k);
        }
        else if (cmd == "steel") {
            physics.dump_steel_diffs();
        }
        else if (cmd == "macros") {
            physics.dump_macros();
        }
        else if (cmd == "physstate") {
            physics.dump_state();
        }
        else if (cmd == "nt") {
            float d = 1, c = 1, s = 1;
            if (iss >> d >> c >> s) physics.set_neurotransmitters(d, c, s);
        }
        else if (cmd == "stick") {
            int n = 1;
            iss >> n;
            for (int i = 0; i < n; ++i) constructive.structural_tick();
        }
        else if (cmd == "tension") {
            int k = 15;
            iss >> k;
            constructive.dump_tension(k);
        }
        else if (cmd == "verify") {
            std::string label; int ticks = 200;
            if (!(iss >> label)) continue;
            iss >> ticks;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) { std::fprintf(stderr, "unknown_label %s\n", label.c_str()); continue; }
            constructive.verify_probe(it->second, ticks);
            constructive.dump_echo();
        }
        else if (cmd == "bsma") {
            int n = 5;
            iss >> n;
            constructive.bsma_iterate(n);
            constructive.dump_bsma();
        }
        else if (cmd == "rade") {
            std::string label; int steps = 30; float amp = 1.0f;
            if (!(iss >> label)) continue;
            iss >> steps >> amp;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) { std::fprintf(stderr, "unknown_label %s\n", label.c_str()); continue; }
            rade.search(it->second, amp, steps);
            rade.dump_peak();
        }
        else if (cmd == "rade_resolve") {
            rade.resolve();
        }
        else if (cmd == "polarities") {
            int k = 20;
            iss >> k;
            rade.dump_polarities(k);
        }
        else if (cmd == "stream_open") {
            std::string path;
            if (!(iss >> path)) continue;
            bool ok = stream.open(path);
            std::fprintf(stderr, "stream_open %s = %s\n", path.c_str(), ok ? "ok" : "fail");
        }
        else if (cmd == "stream_step") {
            int n = 1; iss >> n;
            int applied = stream.step_rows(n);
            std::fprintf(stderr, "stream_step applied %d rows\n", applied);
        }
        else if (cmd == "stream_cols") {
            stream.dump_columns();
        }
        else if (cmd == "energy") {
            physics.dump_energy();
        }
        else if (cmd == "metrics") {
            constructive.dump_metrics();
        }
        else if (cmd == "heatmap") {
            int k = 15;
            iss >> k;
            constructive.dump_plasticity_heatmap(k);
        }
        else if (cmd == "incoherence") {
            int k = 15;
            iss >> k;
            constructive.dump_incoherence(k);
        }
        else if (cmd == "telemetry_reset") {
            constructive.telemetry_reset();
        }
        else if (cmd == "fission") {
            std::string sub_cmd; iss >> sub_cmd;
            if (sub_cmd == "on")       constructive.fission_enabled = true;
            else if (sub_cmd == "off") constructive.fission_enabled = false;
            std::fprintf(stdout,
                "fission=%s I>%.3f τ>%.4f hold=%d g_max=%d\n",
                constructive.fission_enabled ? "on" : "off",
                constructive.fission_I_thresh,
                constructive.fission_tau_thresh,
                constructive.fission_hold_target,
                physics.g_max);
        }
        else if (cmd == "fission_status") {
            std::fprintf(stdout, "fissions=%zu\n", physics.fission_map.size());
            for (const auto& kv : physics.fission_map) {
                int pid = kv.first;
                int label_key = physics.alias_ledger[pid].label_key;
                std::string plabel = (label_key >= 0 &&
                                       label_key < (int)si.graph.nodes.size())
                                    ? si.graph.nodes[label_key].label
                                    : std::string("<synth>");
                std::fprintf(stdout,
                    "  %s [id=%d gen=%d] → α=%d β=%d\n",
                    plabel.c_str(), pid,
                    physics.cavities[pid].generation,
                    kv.second.first, kv.second.second);
            }
        }
        else if (cmd == "fission_ledger") {
            // Emit fission_records as JSONL to data/fission_ledger.jsonl.
            std::FILE* f = std::fopen("data/fission_ledger.jsonl", "w");
            if (!f) { std::fprintf(stderr, "cant open ledger\n"); continue; }
            for (const auto& r : physics.fission_records) {
                std::fprintf(f,
                    "{\"tick\":%d,\"parent_id\":%d,\"parent_label\":\"%s\","
                    "\"I\":%.5f,\"tau\":%.5f,\"streak\":%d,"
                    "\"alpha\":%d,\"beta\":%d,"
                    "\"inc_alpha\":%d,\"inc_beta\":%d,"
                    "\"drive_split\":%.5f}\n",
                    r.tick_index, r.parent_id, r.parent_label.c_str(),
                    r.I, r.tau, r.streak, r.child_alpha, r.child_beta,
                    r.incident_alpha, r.incident_beta, r.drive_split_factor);
            }
            std::fclose(f);
            std::fprintf(stdout, "wrote %zu records\n",
                         physics.fission_records.size());
        }
        else if (cmd == "srg") {
            std::string sub_cmd; iss >> sub_cmd;
            if (sub_cmd == "on")       physics.srg_enabled = true;
            else if (sub_cmd == "off") physics.srg_enabled = false;
            std::fprintf(stdout, "srg = %s\n",
                physics.srg_enabled ? "on" : "off");
        }
        else if (cmd == "ports") {
            std::string label; iss >> label;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) continue;
            const auto& c = physics.cavities[it->second];
            std::fprintf(stdout,
                "%s | x_E=%+.5f v_E=%+.5f  x_I=%+.5f v_I=%+.5f  "
                "observed x=%+.5f v=%+.5f\n",
                label.c_str(), c.x_E, c.v_E, c.x_I, c.v_I,
                c.displacement, c.velocity);
        }
        else if (cmd == "hub_rule") {
            std::string sub_cmd; iss >> sub_cmd;
            if (sub_cmd == "on")       constructive.hub_rule_enabled = true;
            else if (sub_cmd == "off") constructive.hub_rule_enabled = false;
            std::fprintf(stdout, "hub_rule = %s\n",
                constructive.hub_rule_enabled ? "on" : "off");
        }
        // study_recall <label> <rounds> — full instrumented study.
        // Runs a naive probe, then trains with instrumentation, then
        // dumps the heatmap, incoherence, and Structural Integrity Score
        // both before and after. Compares trained response vs naive.
        else if (cmd == "study_recall") {
            std::string label; int rounds = 5;
            if (!(iss >> label)) continue;
            iss >> rounds;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) continue;
            int nid = it->second;

            std::fprintf(stdout, "\n[study_recall %s] hub_rule=%s\n",
                label.c_str(),
                constructive.hub_rule_enabled ? "on" : "off");

            // Baseline naive probe.
            physics.bind(si);
            constructive.bind(&physics);
            rade.bind(&physics, &constructive);
            physics.drive(nid, 20.0f, 6.0f, 15);
            physics.run(60);
            float pre_peak = 0.0f;
            for (const auto& c : physics.cavities) {
                float d = std::fabs(c.displacement);
                if (d > pre_peak) pre_peak = d;
            }
            std::fprintf(stdout, "  pre-train:  peak=%.5f   ", pre_peak);
            constructive.dump_metrics();

            // Training pass with instrumentation.
            physics.bind(si);
            constructive.bind(&physics);
            constructive.telemetry_reset();
            physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
            for (int r = 0; r < rounds; ++r) {
                physics.drive(nid, 20.0f, 8.0f, 40);
                for (int chunk = 0; chunk < 10; ++chunk) {
                    physics.run(20);
                    constructive.structural_tick();
                }
            }
            physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

            // Post-training telemetry (before probe zeros the field).
            std::fprintf(stdout, "  post-train telemetry:\n");
            std::fprintf(stdout, "    plasticity_heatmap (top 8):\n");
            constructive.dump_plasticity_heatmap(8);
            std::fprintf(stdout, "    incoherence (top 8):\n");
            constructive.dump_incoherence(8);
            std::fprintf(stdout, "    ");
            constructive.dump_metrics();

            // Probe on trained substrate.
            physics.reset_field();
            physics.drive(nid, 20.0f, 6.0f, 15);
            physics.run(60);

            // Global peak (any cavity).
            float post_peak_global = 0.0f;
            for (const auto& c : physics.cavities) {
                float d = std::fabs(c.displacement);
                if (d > post_peak_global) post_peak_global = d;
            }

            // Concept-scoped aggregates: parent alias + children.
            // For unfissioned cases, this is just the parent cavity.
            bool  fissioned = physics.is_fissioned(nid);
            float p_p = std::fabs(physics.cavities[nid].displacement);
            float p_a = 0.0f, p_b = 0.0f;
            if (fissioned) {
                auto ch = physics.fission_children(nid);
                p_a = std::fabs(physics.cavities[ch.first].displacement);
                p_b = std::fabs(physics.cavities[ch.second].displacement);
            }
            float concept_peak = std::max({p_p, p_a, p_b});
            float concept_sum  = p_p + p_a + p_b;
            float rms_denom    = fissioned ? 2.0f : 1.0f;
            float concept_rms  = std::sqrt(
                (p_p*p_p + p_a*p_a + p_b*p_b) / rms_denom);

            std::fprintf(stdout,
                "  RESULT %s | naive=%.5f  trained=%.5f  gain=%.3fx"
                " | concept peak=%.5f (gain=%.3fx)"
                " sum=%.5f (gain=%.3fx)"
                " rms=%.5f (gain=%.3fx)"
                "%s\n",
                label.c_str(), pre_peak, post_peak_global,
                pre_peak > 1e-9f ? post_peak_global / pre_peak : 0.0f,
                concept_peak, pre_peak > 1e-9f ? concept_peak / pre_peak : 0.0f,
                concept_sum,  pre_peak > 1e-9f ? concept_sum  / pre_peak : 0.0f,
                concept_rms,  pre_peak > 1e-9f ? concept_rms  / pre_peak : 0.0f,
                fissioned ? " [fissioned]" : "");
        }
        // study_recall_seeds <label> <rounds> <n_seeds>
        // Runs the recall protocol under N distinct RNG seeds. For each
        // seed, executes both fission-OFF (baseline) and fission-ON
        // (RB+G) passes and records gain / concept peak-sum-rms /
        // fission count. Emits per-seed rows plus mean ± std across
        // seeds. Uses the same training regime as study_recall.
        else if (cmd == "study_recall_seeds") {
            std::string label; int rounds = 5; int n_seeds = 5;
            if (!(iss >> label)) continue;
            iss >> rounds >> n_seeds;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) continue;

            struct Row {
                unsigned seed;
                float pre_baseline, post_baseline, gain_baseline;
                float pre_rbg, post_rbg, gain_rbg;
                float concept_peak, concept_sum, concept_rms;
                float concept_rms_gain;
                int   n_fissions;
                bool  target_fissioned;
            };
            std::vector<Row> rows;
            rows.reserve(n_seeds);

            std::fprintf(stdout,
                "\n[study_recall_seeds] label=%s rounds=%d n_seeds=%d\n",
                label.c_str(), rounds, n_seeds);
            std::fprintf(stdout,
                "seed | baseline_gain | rbg_gain | c_peak c_sum c_rms | rms_gain | fissions | target_fiss?\n");

            auto run_pass = [&](int nid, bool fission_on, unsigned seed,
                                float& pre, float& post,
                                float& concept_peak, float& concept_sum,
                                float& concept_rms, int& n_fissions,
                                bool& target_fissioned)
            {
                physics.bind_seed = seed;
                physics.bind(si);
                constructive.bind(&physics);
                rade.bind(&physics, &constructive);
                constructive.fission_enabled = fission_on;

                // Naive probe.
                physics.drive(nid, 20.0f, 6.0f, 15);
                physics.run(60);
                pre = 0.0f;
                for (const auto& c : physics.cavities) {
                    float d = std::fabs(c.displacement);
                    if (d > pre) pre = d;
                }

                // Rebind fresh, train.
                physics.bind_seed = seed;
                physics.bind(si);
                constructive.bind(&physics);
                rade.bind(&physics, &constructive);
                constructive.fission_enabled = fission_on;
                physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
                for (int r = 0; r < rounds; ++r) {
                    physics.drive(nid, 20.0f, 8.0f, 40);
                    for (int chunk = 0; chunk < 10; ++chunk) {
                        physics.run(20);
                        constructive.structural_tick();
                    }
                }
                physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

                // Probe on trained substrate.
                physics.reset_field();
                physics.drive(nid, 20.0f, 6.0f, 15);
                physics.run(60);
                post = 0.0f;
                for (const auto& c : physics.cavities) {
                    float d = std::fabs(c.displacement);
                    if (d > post) post = d;
                }

                // Concept-scoped readouts on the CURRENT nid after
                // training. If fission fired on the target concept its
                // children carry the response.
                target_fissioned = physics.is_fissioned(nid);
                float p_p = std::fabs(physics.cavities[nid].displacement);
                float p_a = 0.0f, p_b = 0.0f;
                if (target_fissioned) {
                    auto ch = physics.fission_children(nid);
                    p_a = std::fabs(physics.cavities[ch.first].displacement);
                    p_b = std::fabs(physics.cavities[ch.second].displacement);
                }
                concept_peak = std::max({p_p, p_a, p_b});
                concept_sum  = p_p + p_a + p_b;
                float rms_denom = target_fissioned ? 2.0f : 1.0f;
                concept_rms  = std::sqrt(
                    (p_p*p_p + p_a*p_a + p_b*p_b) / rms_denom);
                n_fissions = (int)physics.fission_records.size();
            };

            for (int s = 0; s < n_seeds; ++s) {
                unsigned seed = 0xCAF17E00u
                              ^ (0x9E3779B9u * (unsigned)(s + 1));
                Row row{};
                row.seed = seed;

                // Baseline pass (fission off).
                float pre, post, cp, cs, crms;
                int nfiss; bool tfiss;
                run_pass(it->second, false, seed,
                         pre, post, cp, cs, crms, nfiss, tfiss);
                row.pre_baseline  = pre;
                row.post_baseline = post;
                row.gain_baseline = pre > 1e-9f ? post / pre : 0.0f;

                // RB+G pass (fission on) — separate bind, same seed.
                run_pass(it->second, true, seed,
                         pre, post, cp, cs, crms, nfiss, tfiss);
                row.pre_rbg           = pre;
                row.post_rbg          = post;
                row.gain_rbg          = pre > 1e-9f ? post / pre : 0.0f;
                row.concept_peak      = cp;
                row.concept_sum       = cs;
                row.concept_rms       = crms;
                row.concept_rms_gain  = pre > 1e-9f ? crms / pre : 0.0f;
                row.n_fissions        = nfiss;
                row.target_fissioned  = tfiss;

                rows.push_back(row);
                std::fprintf(stdout,
                    "0x%08x | %.3fx | %.3fx | %.5f %.5f %.5f | %.3fx | %d | %s\n",
                    seed,
                    row.gain_baseline, row.gain_rbg,
                    row.concept_peak, row.concept_sum, row.concept_rms,
                    row.concept_rms_gain, row.n_fissions,
                    row.target_fissioned ? "yes" : "no");
            }

            // Aggregate.
            auto stats = [&](std::vector<float> xs) {
                float mean = 0, var = 0;
                for (float x : xs) mean += x;
                mean /= (float)xs.size();
                for (float x : xs) var += (x-mean)*(x-mean);
                var /= (float)xs.size();
                return std::make_pair(mean, std::sqrt(var));
            };
            std::vector<float> gb, gr, gr_rms; std::vector<float> nf;
            for (const auto& r : rows) {
                gb.push_back(r.gain_baseline);
                gr.push_back(r.gain_rbg);
                gr_rms.push_back(r.concept_rms_gain);
                nf.push_back((float)r.n_fissions);
            }
            auto [mgb, sgb] = stats(gb);
            auto [mgr, sgr] = stats(gr);
            auto [mgrms, sgrms] = stats(gr_rms);
            auto [mnf, snf] = stats(nf);
            std::fprintf(stdout,
                "\nAGGREGATE  %s [n=%d]:\n"
                "  baseline_gain      = %.4f ± %.4f\n"
                "  rbg_gain (global)  = %.4f ± %.4f  (Δ = %+.4f)\n"
                "  rbg_gain (RMS)     = %.4f ± %.4f\n"
                "  fissions per seed  = %.2f ± %.2f\n",
                label.c_str(), n_seeds,
                mgb, sgb, mgr, sgr, mgr - mgb,
                mgrms, sgrms, mnf, snf);
        }
        // study_recall_ablation <label> <rounds> <n_seeds>
        // Sweeps six RB+G configurations per seed and reports concept-RMS
        // gain / n_fissions for each. Configurations:
        //   BASELINE      : fission_enabled = false
        //   ABLATE_CAP    : g_max = 999 (cascade allowed)
        //   ABLATE_ALIAS  : rbg_alias_route_enabled = false (probe hits ghost)
        //   ABLATE_FANOUT : rbg_fanout_factor = 1.0 (energy doubling)
        //   ABLATE_BIND   : rbg_binding_enabled = false (no α↔β spring)
        //   FULL          : all four RB+G mechanisms on
        // Emits per-seed rows + AGGREGATE stats per config.
        else if (cmd == "study_recall_ablation") {
            std::string label; int rounds = 5; int n_seeds = 5;
            if (!(iss >> label)) continue;
            iss >> rounds >> n_seeds;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) continue;
            int nid = it->second;

            struct Cfg {
                std::string name;
                bool  fission_on;
                int   g_max;
                bool  alias_on;
                float fanout;
                bool  binding_on;
            };
            std::vector<Cfg> cfgs = {
                {"BASELINE",      false, 1,   true,  0.70710678f, true},
                {"ABLATE_CAP",    true,  999, true,  0.70710678f, true},
                {"ABLATE_ALIAS",  true,  1,   false, 0.70710678f, true},
                {"ABLATE_FANOUT", true,  1,   true,  1.00000000f, true},
                {"ABLATE_BIND",   true,  1,   true,  0.70710678f, false},
                {"FULL",          true,  1,   true,  0.70710678f, true},
            };

            std::fprintf(stdout,
                "\n[study_recall_ablation] label=%s rounds=%d n_seeds=%d\n",
                label.c_str(), rounds, n_seeds);
            std::fprintf(stdout,
                "cfg | seed | pre | post | rms_gain | n_fissions\n");

            auto run_pass = [&](int nid_local, const Cfg& cfg, unsigned seed,
                                float& pre, float& post,
                                float& concept_rms, int& n_fissions,
                                bool& target_fissioned)
            {
                physics.bind_seed = seed;
                physics.bind(si);
                constructive.bind(&physics);
                rade.bind(&physics, &constructive);
                physics.g_max                    = cfg.g_max;
                physics.rbg_alias_route_enabled  = cfg.alias_on;
                physics.rbg_fanout_factor        = cfg.fanout;
                physics.rbg_binding_enabled      = cfg.binding_on;
                constructive.fission_enabled     = cfg.fission_on;

                physics.drive(nid_local, 20.0f, 6.0f, 15);
                physics.run(60);
                pre = 0.0f;
                for (const auto& c : physics.cavities) {
                    float d = std::fabs(c.displacement);
                    if (d > pre) pre = d;
                }

                physics.bind_seed = seed;
                physics.bind(si);
                constructive.bind(&physics);
                rade.bind(&physics, &constructive);
                physics.g_max                    = cfg.g_max;
                physics.rbg_alias_route_enabled  = cfg.alias_on;
                physics.rbg_fanout_factor        = cfg.fanout;
                physics.rbg_binding_enabled      = cfg.binding_on;
                constructive.fission_enabled     = cfg.fission_on;
                physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
                for (int r = 0; r < rounds; ++r) {
                    physics.drive(nid_local, 20.0f, 8.0f, 40);
                    for (int chunk = 0; chunk < 10; ++chunk) {
                        physics.run(20);
                        constructive.structural_tick();
                    }
                }
                physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);

                physics.reset_field();
                physics.drive(nid_local, 20.0f, 6.0f, 15);
                physics.run(60);
                post = 0.0f;
                for (const auto& c : physics.cavities) {
                    float d = std::fabs(c.displacement);
                    if (d > post) post = d;
                }

                target_fissioned = physics.is_fissioned(nid_local);
                float p_p = std::fabs(physics.cavities[nid_local].displacement);
                float p_a = 0.0f, p_b = 0.0f;
                if (target_fissioned) {
                    auto ch = physics.fission_children(nid_local);
                    p_a = std::fabs(physics.cavities[ch.first].displacement);
                    p_b = std::fabs(physics.cavities[ch.second].displacement);
                }
                float rms_denom = target_fissioned ? 2.0f : 1.0f;
                concept_rms  = std::sqrt(
                    (p_p*p_p + p_a*p_a + p_b*p_b) / rms_denom);
                n_fissions = (int)physics.fission_records.size();
            };

            // cfg → vector<rms_gain>, vector<n_fiss>
            std::vector<std::vector<float>> gains(cfgs.size());
            std::vector<std::vector<float>> nfiss(cfgs.size());
            std::vector<int>                target_fiss_count(cfgs.size(), 0);
            for (int s = 0; s < n_seeds; ++s) {
                unsigned seed = 0xCAF17E00u
                              ^ (0x9E3779B9u * (unsigned)(s + 1));
                for (size_t k = 0; k < cfgs.size(); ++k) {
                    float pre, post, crms; int nf; bool tf;
                    run_pass(nid, cfgs[k], seed, pre, post, crms, nf, tf);
                    float g = (pre > 1e-9f) ? crms / pre : 0.0f;
                    gains[k].push_back(g);
                    nfiss[k].push_back((float)nf);
                    if (tf) ++target_fiss_count[k];
                    std::fprintf(stdout,
                        "%-14s | 0x%08x | %.5f | %.5f | %.3fx | %d%s\n",
                        cfgs[k].name.c_str(), seed,
                        pre, post, g, nf, tf ? " [tfiss]" : "");
                }
            }

            // Restore engine defaults after the sweep.
            physics.g_max                   = 1;
            physics.rbg_alias_route_enabled = true;
            physics.rbg_fanout_factor       = 0.70710678f;
            physics.rbg_binding_enabled     = true;
            constructive.fission_enabled    = false;

            auto stats = [](const std::vector<float>& xs){
                float m = 0.0f, v = 0.0f;
                for (float x : xs) m += x;
                m /= std::max<size_t>(1, xs.size());
                for (float x : xs) v += (x - m) * (x - m);
                v /= std::max<size_t>(1, xs.size());
                return std::make_pair(m, std::sqrt(v));
            };
            std::fprintf(stdout,
                "\nAGGREGATE %s [n=%d]:\n"
                "  cfg            | rms_gain (mean±std) | fissions (mean±std) | target_fissioned\n",
                label.c_str(), n_seeds);
            for (size_t k = 0; k < cfgs.size(); ++k) {
                auto [mg, sg] = stats(gains[k]);
                auto [mn, sn] = stats(nfiss[k]);
                std::fprintf(stdout,
                    "  %-14s | %.4f ± %.4f    | %.2f ± %.2f      | %d/%d\n",
                    cfgs[k].name.c_str(), mg, sg, mn, sn,
                    target_fiss_count[k], n_seeds);
            }
        }
        // recall_test <label> <n_train_rounds> — trains by driving <label>
        // several times with S=2 (serotonin high, Hebbian rate doubled),
        // then measures response amplitude to a SHORT probe drive.
        // Compares against the same short probe on a naive engine (before
        // training) — the diff is the memory-recall gain.
        else if (cmd == "recall_test") {
            std::string label; int rounds = 5;
            if (!(iss >> label)) continue;
            iss >> rounds;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) continue;
            int nid = it->second;

            // Baseline probe on NAIVE engine (fresh bind).
            physics.bind(si);
            constructive.bind(&physics);
            rade.bind(&physics, &constructive);
            physics.drive(nid, 20.0f, 6.0f, 15);
            physics.run(60);
            auto pre_audit = physics.audit_energy();
            float pre_peak = 0.0f;
            {
                for (const auto& c : physics.cavities) {
                    float d = std::fabs(c.displacement);
                    if (d > pre_peak) pre_peak = d;
                }
            }
            // Rebind and TRAIN with S=2.
            physics.bind(si);
            constructive.bind(&physics);
            physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
            // Interleave physics + structural ticks — plasticity
            // consolidation depends on the structural pass.
            for (int r = 0; r < rounds; ++r) {
                physics.drive(nid, 20.0f, 8.0f, 40);
                for (int chunk = 0; chunk < 10; ++chunk) {
                    physics.run(20);
                    constructive.structural_tick();
                }
            }
            // Restore neutral NT for the probe.
            physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);
            physics.reset_field();
            physics.drive(nid, 20.0f, 6.0f, 15);
            physics.run(60);
            auto post_audit = physics.audit_energy();
            float post_peak = 0.0f;
            {
                for (const auto& c : physics.cavities) {
                    float d = std::fabs(c.displacement);
                    if (d > post_peak) post_peak = d;
                }
            }
            std::fprintf(stdout,
                "recall_test %s | naive_peak=%.5f  trained_peak=%.5f  "
                "gain=%.3fx | naive_energy=%.5f  trained_energy=%.5f  "
                "plasticity=%.5f\n",
                label.c_str(),
                pre_peak, post_peak,
                pre_peak > 1e-9f ? post_peak / pre_peak : 0.0f,
                pre_audit.total_mechanical, post_audit.total_mechanical,
                post_audit.plasticity);
        }
        // baseline_luna — stream LUNA with polarity drift DISABLED
        // (g_split = 0) then again with it ENABLED. Compare peak tension.
        else if (cmd == "baseline_luna") {
            auto run_pass = [&](bool splitting_on, const char* label){
                physics.bind(si);
                constructive.bind(&physics);
                rade.bind(&physics, &constructive);
                stream.bind(&physics, si);
                float saved_split = constructive.g_split;
                if (!splitting_on) constructive.g_split = 0.0f;
                stream.open("data/luna_2022_may.csv");
                // Interleave physics with structural ticks — this is
                // what makes P5b Splitting actually act.
                for (int day = 0; day < 18; ++day) {
                    stream.step_rows(1);
                    constructive.structural_tick();
                    constructive.structural_tick();
                }
                constructive.g_split = saved_split;
                auto a = physics.audit_energy();
                std::fprintf(stdout,
                    "%s | tension=%.5f  T+U=%.5f  plasticity=%.5f\n",
                    label,
                    physics.ic.tension,
                    a.total_mechanical, a.plasticity);
                // dump polarity of luna→ust for inspection
                auto it_l = si.graph.by_label.find("luna");
                auto it_u = si.graph.by_label.find("ust");
                if (it_l != si.graph.by_label.end() && it_u != si.graph.by_label.end()) {
                    for (const auto& t : physics.tunnels) {
                        if (t.cavity_a == it_l->second && t.cavity_b == it_u->second) {
                            std::fprintf(stdout,
                                "  luna->ust polarity=%+.4f  k=%.4f\n",
                                t.polarity, t.stiffness);
                        }
                    }
                }
            };
            run_pass(false, "splitting_OFF");
            run_pass(true,  "splitting_ON ");
        }
        // struct_step [n]
        //   Runs n structural (Layer-2) ticks. Each tick applies the
        //   nine primitives + Hebbian aggregation + P5b splitting +
        //   fission check. Does NOT run physics ticks — pair with `step`.
        else if (cmd == "struct_step") {
            int n = 1;
            iss >> n;
            for (int i = 0; i < n; ++i) constructive.structural_tick();
        }
        // train_multi <lbl1,lbl2,...> [rounds]
        //   Sequentially runs the study-recall training loop on each
        //   listed label WITHOUT rebinding between them. Fissions and
        //   Hebbian plasticity accumulate across labels. Reports how
        //   many fissions fired per label.
        else if (cmd == "train_multi") {
            std::string list; int rounds = 5;
            if (!(iss >> list)) continue;
            iss >> rounds;

            // Split comma-separated list.
            std::vector<std::string> labels;
            {
                std::string cur;
                for (char c : list) {
                    if (c == ',') { if (!cur.empty()) labels.push_back(cur); cur.clear(); }
                    else cur.push_back(c);
                }
                if (!cur.empty()) labels.push_back(cur);
            }

            physics.set_neurotransmitters(1.0f, 1.0f, 2.0f);
            for (const auto& lbl : labels) {
                auto it_l = si.graph.by_label.find(lbl);
                if (it_l == si.graph.by_label.end()) {
                    std::fprintf(stdout, "unknown_label %s\n", lbl.c_str());
                    continue;
                }
                int nid_l = it_l->second;
                int fbefore = (int)physics.fission_records.size();
                for (int r = 0; r < rounds; ++r) {
                    physics.drive(nid_l, 20.0f, 8.0f, 40);
                    for (int chunk = 0; chunk < 10; ++chunk) {
                        physics.run(20);
                        constructive.structural_tick();
                    }
                }
                int fafter = (int)physics.fission_records.size();
                std::fprintf(stdout,
                    "trained %s | new_fissions=%d | total_fissions=%d\n",
                    lbl.c_str(), fafter - fbefore, fafter);
            }
            physics.set_neurotransmitters(1.0f, 1.0f, 1.0f);
        }
        // fisher_distance <label_a> <label_b>
        //   Phase 4.2 — Fisher-metric distance from corr_ema. Small
        //   value = statistically indistinguishable = a Fisher-optimal
        //   L6-A bridge candidate. Requires training first.
        else if (cmd == "fisher_distance") {
            std::string a, b;
            if (!(iss >> a >> b)) continue;
            auto it_a = si.graph.by_label.find(a);
            auto it_b = si.graph.by_label.find(b);
            if (it_a == si.graph.by_label.end() ||
                it_b == si.graph.by_label.end()) {
                std::fprintf(stderr, "unknown_label\n"); continue;
            }
            float d = constructive.fisher_distance(it_a->second, it_b->second);
            std::fprintf(stdout,
                "fisher_distance(%s, %s) = %.6f\n", a.c_str(), b.c_str(), d);
        }
        // discover_abductions <goal_label> [probe_freq]
        //   Phase 3.2 — abduction discovery. Drive the goal cavity at
        //   probe_freq, observe upstream excitatory-tunnel sources that
        //   accumulate energy, register each as a hypothesis candidate.
        else if (cmd == "discover_abductions") {
            std::string label; float freq = 3.0f;
            if (!(iss >> label)) continue;
            iss >> freq;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) {
                std::fprintf(stderr, "unknown_label %s\n", label.c_str());
                continue;
            }
            int found = constructive.discover_abductions(it->second, freq);
            std::fprintf(stdout,
                "discovered %d abduction candidates for %s (freq=%.2f)\n",
                found, label.c_str(), freq);
            constructive.dump_abductions();
        }
        // syntax_slots <N>
        //   Phase 2.1 — bind N-slot syntax macro-layer (3 = SVO, 4 =
        //   +Modifier, 5 = +Adverb). Frequencies are harmonic ratios;
        //   phases distributed as 2π k/N around the unit circle.
        else if (cmd == "syntax_slots") {
            int n = 3;
            iss >> n;
            physics.bind_syntax_slots(n);
            std::fprintf(stdout,
                "syntax_slots bound: N=%zu roles=", physics.syntax_slots.size());
            for (const auto& s : physics.syntax_slots)
                std::fprintf(stdout, "%s(ω=%.2f θ=%.2f) ",
                    s.role.c_str(), s.natural_omega, s.target_phase);
            std::fprintf(stdout, "\n");
        }
        // generate <label> [drive_freq] [ticks]
        //   Phase 2.1 — physics-driven language generation.
        //   Drives <label> at the given frequency, runs the tick loop,
        //   then samples each syntax slot at its canonical phase to
        //   pick the winning cavity. Emits an ordered sentence.
        //   Requires syntax_slots to be bound first.
        else if (cmd == "generate") {
            std::string label; float drive_freq = 20.0f; int ticks = 200;
            if (!(iss >> label)) continue;
            iss >> drive_freq >> ticks;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) {
                std::fprintf(stderr, "unknown_label %s\n", label.c_str());
                continue;
            }
            if (physics.syntax_slots.empty()) physics.bind_syntax_slots(3);
            physics.reset_field();
            physics.drive(it->second, drive_freq, 6.0f, 20);
            physics.run(ticks);
            auto winners = physics.readout_slot_winners();
            std::fprintf(stdout, "generated: ");
            for (size_t s = 0; s < winners.size(); ++s) {
                int w = winners[s];
                std::string tok = (w >= 0 && w < (int)si.graph.nodes.size())
                                ? si.graph.nodes[w].label
                                : "<empty>";
                std::fprintf(stdout, "%s%s",
                    (s > 0 ? " " : ""), tok.c_str());
            }
            std::fprintf(stdout, "\n");
            // Also dump slot state for inspection.
            std::fprintf(stdout, "  slots:");
            for (const auto& s : physics.syntax_slots) {
                std::fprintf(stdout, " [%s: A=%.3f φ=%.2f]",
                    s.role.c_str(), s.amplitude, s.phase);
            }
            std::fprintf(stdout, "\n");
        }
        // sparse_tick <on|off|status>
        //   Phase 1.1 — enable Active Wavefront Tracking. When on, the
        //   fission scan iterates only "hot" cavities (those with fresh
        //   input in the last K_idle ticks). Bit-identical results on
        //   ontologies where cold cavities are at rest.
        else if (cmd == "sparse_tick") {
            std::string sub;
            iss >> sub;
            if (sub == "on")       physics.sparse_tick_enabled = true;
            else if (sub == "off") physics.sparse_tick_enabled = false;
            std::fprintf(stdout,
                "sparse_tick=%s eps=%.5f K_idle=%d hot_count=%d / %zu\n",
                physics.sparse_tick_enabled ? "on" : "off",
                physics.wavefront_eps, physics.K_idle,
                physics.hot_count(), physics.cavities.size());
        }
        // auto_thresh <on|off|status>
        //   Phase 1.2 — enable homeostatic-energy-feedback auto-tuning of
        //   fission_I_thresh. On enable, threshold floats with EMA-smoothed
        //   system energy: I_thresh = I0 + k * E_smooth. Positive k means
        //   more energy → higher threshold → suppresses cascade.
        else if (cmd == "auto_thresh") {
            std::string sub;
            iss >> sub;
            if (sub == "on")       constructive.auto_thresh_enabled = true;
            else if (sub == "off") constructive.auto_thresh_enabled = false;
            std::fprintf(stdout,
                "auto_thresh=%s I0=%.4f k=%.5f α=%.4f E_smooth=%.5f I_thresh=%.5f\n",
                constructive.auto_thresh_enabled ? "on" : "off",
                constructive.auto_thresh_I0, constructive.auto_thresh_k,
                constructive.auto_thresh_alpha,
                constructive.E_smooth, constructive.fission_I_thresh);
        }
        // dump_analogies — print analogy_records (bridges) without
        // re-running discovery. Useful after online L6-A.
        else if (cmd == "dump_analogies") {
            std::fprintf(stdout,
                "analogy_records: %zu\n",
                constructive.analogy_records.size());
            constructive.dump_analogies();
        }
        // online_l6a <on|off|status>
        //   Phase 1.3 — enable in-tick L6-A discovery. When on, every
        //   fission event triggers discover_analogies_locked() with
        //   only_parent set to the new parent, so bridges appear in
        //   the tick they become valid rather than waiting for an
        //   explicit `discover_analogies` command.
        else if (cmd == "online_l6a") {
            std::string sub;
            iss >> sub;
            if (sub == "on")       constructive.online_l6a_enabled = true;
            else if (sub == "off") constructive.online_l6a_enabled = false;
            std::fprintf(stdout,
                "online_l6a=%s min_shared=%d k=%.3f c=%.3f\n",
                constructive.online_l6a_enabled ? "on" : "off",
                constructive.online_l6a_min_shared,
                constructive.online_l6a_rubber_k,
                constructive.online_l6a_rubber_c);
        }
        // discover_analogies [min_shared] [rubber_k] [rubber_c]
        //   Scan fissioned-parent pairs and forge a low-stiffness (rubber)
        //   analogy tunnel between α children that share ≥ min_shared
        //   excitatory downstream targets. Reports the inserted bridges.
        else if (cmd == "discover_analogies") {
            int   min_shared = 2;
            float rubber_k   = 0.05f;
            float rubber_c   = 0.15f;
            iss >> min_shared >> rubber_k >> rubber_c;
            int found = constructive.discover_analogies(
                min_shared, rubber_k, rubber_c);
            std::fprintf(stdout,
                "discovered %d analogy bridges (min_shared=%d k=%.3f c=%.3f)\n",
                found, min_shared, rubber_k, rubber_c);
            constructive.dump_analogies();
        }
        // repurpose <label> [low_freq] [top_k]
        //   Probe <label> at a low (structural) frequency and report the
        //   cavities that vibrate hardest. Low frequency + rubber-tuned
        //   analogy tunnels means the probe rides the L6 bridges and
        //   surfaces analogical partners rather than mechanistic
        //   downstream targets.
        else if (cmd == "repurpose") {
            std::string label; float low_freq = 3.0f; int top_k = 10;
            if (!(iss >> label)) continue;
            iss >> low_freq >> top_k;
            auto it = si.graph.by_label.find(label);
            if (it == si.graph.by_label.end()) continue;
            int nid = it->second;

            physics.reset_field();
            physics.drive(nid, low_freq, 6.0f, 20);
            physics.run(120);

            struct AmpRow { float amp; int cid; };
            std::vector<AmpRow> ranked;
            for (int i = 0; i < (int)physics.cavities.size(); ++i) {
                if (i == nid) continue;
                auto ch = physics.fission_children(nid);
                if (i == ch.first || i == ch.second) continue;
                float a = std::fabs(physics.cavities[i].displacement);
                ranked.push_back({a, i});
            }
            std::sort(ranked.begin(), ranked.end(),
                [](const AmpRow& a, const AmpRow& b){ return a.amp > b.amp; });

            std::fprintf(stdout,
                "\n[repurpose %s at %.1fHz]\n", label.c_str(), low_freq);
            int shown = 0;
            for (const auto& r : ranked) {
                if (shown >= top_k) break;
                if (r.amp < 1e-6f) break;
                int label_key = physics.alias_ledger[r.cid].label_key;
                std::string lbl = (label_key >= 0 && label_key < (int)si.graph.nodes.size())
                                ? si.graph.nodes[label_key].label
                                : "<unknown>";
                std::fprintf(stdout,
                    "  %-32s | x=%+.5f | cav=%d\n",
                    lbl.c_str(), physics.cavities[r.cid].displacement, r.cid);
                ++shown;
            }
        }
        else if (cmd == "reset") {
            physics.reset_field();
        }
        else if (cmd == "rebind") {
            dreamer.stop();
            physics.bind(si);
        }
        else if (cmd == "load") {
            std::string path;
            if (!(iss >> path)) continue;
            int n = load_csv(si, path);
            std::fprintf(stderr, "csv_loaded %d\n", n);
            dreamer.stop();
            physics.bind(si);
        }
        else if (cmd == "dream") {
            std::string sub;
            iss >> sub;
            if (sub == "on")  dreamer.start(&physics);
            if (sub == "off") dreamer.stop();
        }
        else if (cmd == "state") {
            std::fprintf(stderr, "nodes=%zu edges=%zu dream=%d\n",
                si.graph.nodes.size(),
                si.graph.edges.size(),
                (int)dreamer.running.load());
        }
    }
    dreamer.stop();
    return 0;
}
