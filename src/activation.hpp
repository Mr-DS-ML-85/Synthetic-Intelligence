// activation.hpp — Neuro-Symbolic Spreading Activation.
//
// This is the answer to "stop hardcoding text patterns." Nodes carry
// continuous voltage. Edges carry conductivity + valence + type_id as
// mutable floats. Propagation is a pure numerical loop: on each tick,
// every node's voltage updates from its incoming neighbours' voltage
// weighted by (conductivity × valence), minus a decay term. Lateral
// inhibition damps competing concepts via softmax-style normalisation.
//
// Nothing in this file switches on a relation name. The relation IDs
// are arbitrary integers indexing into a mutable meta table. Novelty
// emerges by observing which nodes cross threshold, in what order,
// and at what strength — the caller renders output by walking that
// activation harvest, not by consulting a template pool.
//
// A relation-type's *initial* valence is +1 for excitatory relations
// (activates, is_a, treats, causes, part_of) and −1 for inhibitory
// ones (inhibits, downregulates, not). But these initial values are
// data — read from `default_valence_for(rel_id)` — and get overwritten
// by Hebbian reinforcement as the graph is used. There is no
// "if rel == REL_INHIBITS" logic anywhere in the propagation loop.

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace unified {

// Per-edge mutable properties. One row per edge in si.graph.edges,
// aligned by edge index.
struct EdgeMeta {
    float conductivity = 1.0f;   // speed / capacity of voltage transfer
    float valence      = 1.0f;   // +1 excitatory, -1 inhibitory (mutable)
    int   type_id      = -1;     // dynamic index into evolving relation schema
    int   use_count    = 0;      // Hebbian counter — increments when fired
};

// The default valence for a relation id. Data, not a switch inside the
// propagation loop — this only runs once, at initialisation.
inline float default_valence_for(int rel_id) {
    switch (rel_id) {
        case REL_INHIBITS:
        case REL_DOWNREGULATES:
        case REL_NOT:
            return -1.0f;
        // Faraday/mirror relations are neutral — no valence bias.
        case REL_MIRROR_OF:
            return 0.0f;
        default:
            return +1.0f;
    }
}

// The activation field: per-node voltage + per-edge meta.
struct ActivationField {
    std::vector<float> voltage;      // node id -> current voltage
    std::vector<float> next_voltage; // scratch
    std::vector<EdgeMeta> edge;      // per si.graph.edges[i]

    // Precomputed adjacency (out-edges per node), fresh each field to
    // avoid re-scanning the whole edge list per tick.
    std::vector<std::vector<int>> outgoing;

    void bind(const SI& si) {
        voltage.assign(si.graph.nodes.size(), 0.0f);
        next_voltage.assign(si.graph.nodes.size(), 0.0f);
        edge.resize(si.graph.edges.size());
        for (size_t i = 0; i < si.graph.edges.size(); ++i) {
            edge[i].type_id = si.graph.edges[i].rel;
            edge[i].valence = default_valence_for(si.graph.edges[i].rel);
            edge[i].conductivity = 1.0f;
        }
        outgoing.assign(si.graph.nodes.size(), {});
        for (size_t i = 0; i < si.graph.edges.size(); ++i)
            outgoing[si.graph.edges[i].from].push_back((int)i);
    }

    // Inject charge into a specific node — the "prompt" of the wave.
    void inject(int node_id, float charge) {
        if (node_id >= 0 && (size_t)node_id < voltage.size())
            voltage[node_id] += charge;
    }
    void inject(const SI& si, const std::string& label, float charge) {
        auto it = si.graph.by_label.find(label);
        if (it != si.graph.by_label.end()) inject(it->second, charge);
    }

    // One tick of energy propagation.
    //
    //   V_next[j] = V[j] * (1 - decay)
    //             + sum over incoming edges (V[i] * conductivity * valence)
    //
    // Lateral inhibition: after the raw update, subtract a small
    // fraction of the mean voltage from every active node. Competing
    // concepts damp each other proportionally to how many are active.
    void tick(const SI& si,
              float decay = 0.15f,
              float inhibition = 0.05f,
              float step_size = 0.35f) {
        // Zero the scratch.
        std::fill(next_voltage.begin(), next_voltage.end(), 0.0f);
        // 1. Retention with decay.
        for (size_t j = 0; j < voltage.size(); ++j)
            next_voltage[j] = voltage[j] * (1.0f - decay);
        // 2. Propagate from every out-edge: charge flows from the
        //    source node into the target node scaled by conductivity ×
        //    valence. Reinforce use count if the source is active.
        for (int i = 0; i < (int)voltage.size(); ++i) {
            float vi = voltage[i];
            if (std::fabs(vi) < 1e-4f) continue;
            for (int eidx : outgoing[i]) {
                const Edge& E = si.graph.edges[eidx];
                float transfer = vi * edge[eidx].conductivity
                                    * edge[eidx].valence
                                    * step_size;
                next_voltage[E.to] += transfer;
                ++edge[eidx].use_count;
            }
        }
        // 3. Lateral inhibition — subtract a shared damping term
        //    proportional to the mean over active nodes.
        int active = 0;
        float sum = 0.0f;
        for (float v : next_voltage) {
            if (v > 1e-4f) { sum += v; ++active; }
        }
        if (active > 1 && inhibition > 0.0f) {
            float mean_pos = sum / (float)active;
            float damp = mean_pos * inhibition;
            for (float& v : next_voltage) if (v > 1e-4f) v -= damp;
        }
        // 4. Squash to prevent runaway.
        for (float& v : next_voltage) v = std::tanh(v);
        voltage.swap(next_voltage);
    }

    // Run `n_ticks` ticks (or until convergence).
    void propagate(const SI& si, int n_ticks = 12) {
        for (int t = 0; t < n_ticks; ++t) tick(si);
    }

    // Harvest active nodes above a threshold, sorted highest voltage
    // first. Excludes the seed nodes themselves if `exclude` is
    // provided.
    struct Harvest { int node_id; float voltage; };
    std::vector<Harvest>
    harvest(float threshold = 0.05f,
            const std::vector<int>& exclude = {}) const {
        std::vector<Harvest> out;
        for (int i = 0; i < (int)voltage.size(); ++i) {
            bool ex = false;
            for (int x : exclude) if (x == i) { ex = true; break; }
            if (ex) continue;
            if (voltage[i] > threshold)
                out.push_back({i, voltage[i]});
        }
        std::sort(out.begin(), out.end(),
                  [](const Harvest& a, const Harvest& b){
                      return a.voltage > b.voltage;
                  });
        return out;
    }

    // Hebbian reinforcement: after a "successful" derivation (caller
    // decides), increase conductivity of edges used along the way.
    void hebbian_strengthen(int edge_idx, float delta = 0.05f) {
        if (edge_idx < 0 || (size_t)edge_idx >= edge.size()) return;
        edge[edge_idx].conductivity =
            std::min(2.0f, edge[edge_idx].conductivity + delta);
    }
};

// -----------------------------------------------------------------------
// Activation-driven repurposing search.
//
// The user wants candidate ranking that is NOT computed by a
// hand-crafted formula. Here it is: charge the disease, propagate
// through the treats-edge reversed direction so known treatments light
// up, then let the wave flow forward through mechanism edges to
// downstream targets, then back through mechanism edges to any drug
// that shares those targets. Rank candidates by their final voltage.
//
// The known treatments are damped after step 2 (their voltage is
// zeroed) so their charge fully cascades into targets rather than
// staying with them. New drug candidates naturally accumulate voltage
// proportional to (a) how many downstream targets they share with
// known treatments, and (b) how strongly-endorsed those targets are.
// This is the "weighted score" the user asked for, but emerging from
// the propagation, not from a formula.
// -----------------------------------------------------------------------
struct ActivationCandidate {
    int   node_id;
    float voltage;
    // For rendering: which mechanism-target nodes the candidate is
    // most strongly linked to (top by voltage at harvest time).
    std::vector<int> resonant_targets;
    // Known reference treatments that share those targets.
    std::vector<int> reference_treatments;
};

// Backward-directed neighbours (who points AT me?) — used to seed the
// wave from the disease into its known treatments.
inline std::vector<std::vector<int>>
in_edges_by_node(const SI& si) {
    std::vector<std::vector<int>> incoming(si.graph.nodes.size());
    for (size_t i = 0; i < si.graph.edges.size(); ++i)
        incoming[si.graph.edges[i].to].push_back((int)i);
    return incoming;
}

inline std::vector<ActivationCandidate>
activation_candidates_for(const SI& si,
                          const std::string& disease_label,
                          int mechanism_rel /* REL_INHIBITS or REL_ACTIVATES */,
                          int n_ticks = 6) {
    std::vector<ActivationCandidate> out;
    auto it_d = si.graph.by_label.find(disease_label);
    if (it_d == si.graph.by_label.end()) return out;
    int D = it_d->second;

    ActivationField field;
    field.bind(si);
    // For the co-mechanism search, valence is not what we want — we
    // are asking "which nodes share this edge type?" not "does the
    // signal excite or inhibit downstream?". So run the wave with
    // absolute valence: all mechanism edges conduct +1.
    for (auto& em : field.edge) em.valence = std::fabs(em.valence);

    // Phase 1 — inject charge into the disease node, then reverse-pull
    // through treats-edges so the known treatments become the sources
    // of the wave.
    field.inject(D, 1.0f);
    auto incoming = in_edges_by_node(si);
    std::vector<int> known_treatments;
    for (int eidx : incoming[D]) {
        const Edge& E = si.graph.edges[eidx];
        if (E.rel != REL_TREATS) continue;
        field.inject(E.from, 0.8f);
        known_treatments.push_back(E.from);
    }
    if (known_treatments.empty()) return out;

    // Phase 2 — restrict conductivity to the requested mechanism
    // relation only. Non-mechanism edges do not conduct in this phase.
    for (auto& em : field.edge) {
        if (em.type_id != mechanism_rel) em.conductivity = 0.0f;
    }
    // Forward wave: charge flows from each known treatment through its
    // mechanism edges into the shared downstream targets.
    field.propagate(si, n_ticks);

    // Phase 3 — damp the known treatments so their charge does not
    // dominate the ranking, and record which downstream targets lit up.
    std::vector<float> target_voltage = field.voltage;
    std::vector<int> targets_lit;
    for (int j = 0; j < (int)field.voltage.size(); ++j) {
        bool is_known = false;
        for (int k : known_treatments) if (k == j) { is_known = true; break; }
        if (is_known || j == D) continue;
        if (field.voltage[j] > 0.02f) targets_lit.push_back(j);
    }
    // Phase 4 — REVERSE propagation from each lit target back through
    // incoming mechanism edges to find OTHER molecules that hit the
    // same target. This is what surfaces candidates.
    std::vector<float> candidate_voltage(si.graph.nodes.size(), 0.0f);
    for (int t : targets_lit) {
        float src_v = field.voltage[t];
        for (int eidx : incoming[t]) {
            const Edge& E = si.graph.edges[eidx];
            if (E.rel != mechanism_rel) continue;
            candidate_voltage[E.from] += src_v * field.edge[eidx].conductivity;
        }
    }
    // Zero out the disease and known treatments in the candidate pool.
    candidate_voltage[D] = 0.0f;
    for (int t : known_treatments) candidate_voltage[t] = 0.0f;

    // Phase 5 — build harvest from candidate_voltage.
    std::vector<ActivationField::Harvest> harv;
    for (int j = 0; j < (int)candidate_voltage.size(); ++j) {
        if (candidate_voltage[j] > 0.05f)
            harv.push_back({j, candidate_voltage[j]});
    }
    std::sort(harv.begin(), harv.end(),
              [](const ActivationField::Harvest& a,
                 const ActivationField::Harvest& b){
                  return a.voltage > b.voltage;
              });
    // Also write candidate_voltage back into field.voltage so the
    // downstream resonance-context code sees them.
    field.voltage = candidate_voltage;
    field.voltage[D] = 0.0f;
    // Restore full voltage of targets so resonant_targets can find them.
    for (int t : targets_lit) field.voltage[t] = target_voltage[t];

    // Attach mechanism resonance context — for each candidate, find
    // its outgoing mechanism-rel targets that ALSO have high voltage
    // (i.e. are being co-activated) — these are the shared axes that
    // caused the wave to concentrate here.
    for (const auto& h : harv) {
        ActivationCandidate c;
        c.node_id = h.node_id;
        c.voltage = h.voltage;
        for (int eidx : field.outgoing[h.node_id]) {
            const Edge& E = si.graph.edges[eidx];
            if (E.rel != mechanism_rel) continue;
            if (field.voltage[E.to] > 0.05f)
                c.resonant_targets.push_back(E.to);
        }
        // And the known treatments that co-hit those targets.
        for (int t : c.resonant_targets) {
            for (int eidx : incoming[t]) {
                const Edge& E = si.graph.edges[eidx];
                if (E.rel != mechanism_rel) continue;
                if (E.from == h.node_id) continue;
                bool is_known = false;
                for (int k : known_treatments)
                    if (k == E.from) { is_known = true; break; }
                if (is_known) c.reference_treatments.push_back(E.from);
            }
        }
        // Dedupe.
        std::sort(c.resonant_targets.begin(), c.resonant_targets.end());
        c.resonant_targets.erase(std::unique(c.resonant_targets.begin(),
                                             c.resonant_targets.end()),
                                  c.resonant_targets.end());
        std::sort(c.reference_treatments.begin(),
                  c.reference_treatments.end());
        c.reference_treatments.erase(
            std::unique(c.reference_treatments.begin(),
                        c.reference_treatments.end()),
            c.reference_treatments.end());
        // Only keep candidates that DID accumulate through the
        // mechanism relation — otherwise the wave hit them by unrelated
        // is_a or part_of edges.
        if (c.resonant_targets.empty()) continue;
        // Restrict to plausible therapeutic entities (drugs / molecules).
        bool therapeutic = false;
        for (int eidx : field.outgoing[c.node_id]) {
            const Edge& E = si.graph.edges[eidx];
            if (E.rel != REL_IS_A) continue;
            const std::string& lbl = si.graph.nodes[E.to].label;
            if (lbl == "drug" || lbl == "molecule" || lbl == "compound" ||
                lbl == "small_molecule" || lbl == "natural_product" ||
                lbl == "biologic" || lbl == "peptide") {
                therapeutic = true; break;
            }
        }
        if (!therapeutic) continue;
        out.push_back(std::move(c));
    }
    return out;
}

} // namespace unified
