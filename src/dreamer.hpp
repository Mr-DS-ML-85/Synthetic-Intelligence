// dreamer.hpp — Discovery Daemon for the PRM substrate.
//
// A background thread that continuously drives a random cavity at a
// random frequency, lets the standing-wave field settle, and records
// any UNDRIVEN cavities that ended up vibrating hard. Those are the
// emergent resonance points — where the topology of the tunnel network
// forced two independent wave fronts to constructively interfere.
//
// Emergent resonances are written to data/mutations.axioms as
//
//     <driven_label> resonates_with <emergent_label>
//
// The line format is fixed. No prose. Human validates before promoting
// to real edges.

#pragma once

#include "physics.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace unified {

class Dreamer {
public:
    PhysicsEngine* engine = nullptr;
    std::atomic<bool> running{false};
    std::thread th;
    std::string ledger_path    = "data/mutations.axioms";
    unsigned seed              = 0xDEEDu;
    int   idle_ms_min          = 200;
    int   idle_ms_max          = 900;
    // Drive parameters — sample uniformly from these ranges.
    float drive_amp_min        = 6.0f;
    float drive_amp_max        = 12.0f;
    float drive_freq_min       = 3.0f;    // low-freq probes rubber
    float drive_freq_max       = 45.0f;   // high-freq probes steel
    int   drive_ticks          = 12;
    int   settle_ticks         = 80;
    // Emergent-resonance detection.
    float emergent_amp_floor   = 0.015f;  // undriven cavity must exceed this
    int   min_cluster          = 2;       // at least N undriven co-vibrators

    void start(PhysicsEngine* e) {
        engine = e;
        if (running.exchange(true)) return;
        th = std::thread([this]{ this->loop(); });
    }
    void stop() {
        if (!running.exchange(false)) return;
        if (th.joinable()) th.join();
    }
    ~Dreamer() { stop(); }

    void loop() {
        std::mt19937 rng(seed);
        while (running.load()) {
            std::uniform_int_distribution<int> sleep_dist(idle_ms_min, idle_ms_max);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dist(rng)));
            if (!engine || !engine->graph_ptr) continue;
            int N = (int)engine->cavities.size();
            if (N == 0) continue;

            // Reset the field, pick a random node + frequency, drive it,
            // let the standing wave settle.
            engine->reset_field();
            std::uniform_int_distribution<int>   node_dist(0, N - 1);
            std::uniform_real_distribution<float> amp_dist(drive_amp_min, drive_amp_max);
            std::uniform_real_distribution<float> freq_dist(drive_freq_min, drive_freq_max);
            int   who  = node_dist(rng);
            float amp  = amp_dist(rng);
            float freq = freq_dist(rng);
            engine->drive(who, freq, amp, drive_ticks);
            for (int t = 0; t < settle_ticks && running.load(); ++t)
                engine->tick();

            // Collect undriven cavities vibrating above the floor.
            std::vector<int> emergent;
            {
                std::lock_guard<std::mutex> lk(engine->mtx);
                for (int i = 0; i < N; ++i) {
                    if (i == who) continue;
                    if (engine->cavities[i].was_driven) continue;
                    if (std::fabs(engine->cavities[i].displacement) > emergent_amp_floor)
                        emergent.push_back(i);
                }
                if ((int)emergent.size() >= min_cluster) {
                    std::ofstream out(ledger_path, std::ios::app);
                    if (out) {
                        const std::string& src =
                            engine->graph_ptr->graph.nodes[who].label;
                        for (int i : emergent) {
                            out << src << " resonates_with "
                                << engine->graph_ptr->graph.nodes[i].label
                                << "\n";
                        }
                    }
                }
            }
        }
    }
};

} // namespace unified
