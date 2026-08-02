// si_state.hpp — portable substrate state format (.si)
//
// ONE FILE HOLDS THE WHOLE SUBSTRATE: architecture AND axioms. Loading
// a `.si` reconstitutes the system exactly — the concept graph it
// knows, and the physical state it reached while knowing it. No
// companion file, no config, no rebuild step.
//
// (`.neuron` is deliberately NOT used: that extension belongs to the
// retired DCHL lineage's fitted weight blobs, which were archived out
// of the tree in Phase 4. Keeping the names distinct keeps the
// provenance distinct — those were trained parameters, this is a graph
// plus the state a physics substrate arrived at.)
//
// ROADMAP Phase 1.4. Addresses a real engineering gap: until now every
// binary rebuilt the ontology from `data/*.axioms` at startup, bound a
// fresh physics field, and discarded every learned quantity on exit.
// Nothing the substrate learned survived the process. Knowledge was
// *wired into the run*, not held in an artifact.
//
// A `.neuron` file is self-contained. It carries both the concept graph
// and the learned overlay that sits on top of it:
//
//   graph          node labels, edges (from, to, relation)
//   thermal        per-cavity heat_T          (CMD habituation)
//   acoustic       per-cavity acoustic_mass   (CPME §1.3)
//   plasticity     per-tunnel plasticity_delta (ZeroGrad Hebbian)
//   gauge          per-tunnel psi_target      (CPSB §2.1)
//   sensory        byte -> concept bindings   (CMD byte-wave layer)
//   lanes          lane<->lane tunnels + learned psi (MRS §1.3)
//   fission        RB+G parent -> {alpha, beta} map
//
// Everything above is state the substrate *acquired*. None of it is
// derivable from the axiom files alone, which is exactly why it needs
// to be serialisable.
//
// Format is little-endian binary with a magic + version header, so a
// stale file is rejected rather than misread.

#pragma once

#include "physics.hpp"
#include "unified.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace unified {

static const char     SI_MAGIC[8] = {'P','C','M','A','S','I','\0','\0'};
static const uint32_t SI_VERSION  = 1;

// Canonical filename. The substrate looks for this on start and writes
// it on exit, so a user never names a file or passes a flag.
static const char* const SI_DEFAULT_PATH = "pcma.si";

// ------------------------------------------------------------- helpers

inline bool si_w(FILE* f, const void* p, size_t n) {
    return std::fwrite(p, 1, n, f) == n;
}
inline bool si_r(FILE* f, void* p, size_t n) {
    return std::fread(p, 1, n, f) == n;
}
inline bool si_wu32(FILE* f, uint32_t v) { return si_w(f, &v, 4); }
inline bool si_ru32(FILE* f, uint32_t& v) { return si_r(f, &v, 4); }
inline bool si_wf(FILE* f, float v)      { return si_w(f, &v, 4); }
inline bool si_rf(FILE* f, float& v)     { return si_r(f, &v, 4); }

inline bool si_wstr(FILE* f, const std::string& s) {
    if (!si_wu32(f, (uint32_t)s.size())) return false;
    return s.empty() || si_w(f, s.data(), s.size());
}
inline bool si_rstr(FILE* f, std::string& s) {
    uint32_t n = 0;
    if (!si_ru32(f, n)) return false;
    if (n > (1u << 20)) return false;          // sanity bound
    s.assign(n, '\0');
    return n == 0 || si_r(f, &s[0], n);
}

// ---------------------------------------------------------------- save

inline bool si_save(const ExtSI& si, const PhysicsEngine& px,
                    const std::string& path, std::string* err = nullptr) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { if (err) *err = "cannot open for write: " + path; return false; }

    bool ok = true;
    ok = ok && si_w(f, SI_MAGIC, 8);
    ok = ok && si_wu32(f, SI_VERSION);

    // --- graph ---
    ok = ok && si_wu32(f, (uint32_t)si.graph.nodes.size());
    for (const auto& n : si.graph.nodes) ok = ok && si_wstr(f, n.label);
    ok = ok && si_wu32(f, (uint32_t)si.graph.edges.size());
    for (const auto& e : si.graph.edges) {
        ok = ok && si_wu32(f, (uint32_t)e.from);
        ok = ok && si_wu32(f, (uint32_t)e.to);
        ok = ok && si_wu32(f, (uint32_t)e.rel);
        // PROVENANCE. Writing only (from,to,rel) silently reduced every
        // derived edge to an asserted one on reload: the graph still had
        // 100 extra facts but could no longer say where any of them came
        // from, and `derived` reported 0. Since "this was concluded, not
        // stored — here is the proof chain" is the load-bearing claim of
        // the whole system, dropping these four fields turned the
        // artifact into a plausible-looking lie.
        ok = ok && si_wu32(f, (uint32_t)e.derived_by_rule);
        ok = ok && si_wu32(f, (uint32_t)e.parent_a);
        ok = ok && si_wu32(f, (uint32_t)e.parent_b);
        ok = ok && si_wu32(f, (uint32_t)e.step);
    }

    // --- per-cavity learned state ---
    ok = ok && si_wu32(f, (uint32_t)px.cavities.size());
    for (const auto& c : px.cavities) {
        ok = ok && si_wf(f, c.heat_T);
        ok = ok && si_wf(f, c.acoustic_mass);
    }

    // --- per-tunnel learned state ---
    ok = ok && si_wu32(f, (uint32_t)px.tunnels.size());
    for (const auto& t : px.tunnels) {
        ok = ok && si_wf(f, t.plasticity_delta);
        ok = ok && si_wf(f, t.psi_target);
    }

    // --- byte -> concept sensory bindings ---
    uint32_t n_bind = 0;
    for (const auto& m : px.byte_bind_w) n_bind += (uint32_t)m.size();
    ok = ok && si_wu32(f, n_bind);
    for (size_t b = 0; b < px.byte_bind_w.size(); ++b) {
        for (const auto& kv : px.byte_bind_w[b]) {
            ok = ok && si_wu32(f, (uint32_t)b);
            ok = ok && si_wu32(f, (uint32_t)kv.first);
            ok = ok && si_wf(f, kv.second);
        }
    }

    // --- lane <-> lane tunnels (MRS) ---
    ok = ok && si_wu32(f, (uint32_t)px.lane_tunnels.size());
    for (const auto& lt : px.lane_tunnels) {
        ok = ok && si_wu32(f, (uint32_t)lt.lane_a);
        ok = ok && si_wu32(f, (uint32_t)lt.lane_b);
        ok = ok && si_wf(f, lt.weight);
        ok = ok && si_wf(f, lt.psi);
        ok = ok && si_wf(f, lt.psi_r);
        ok = ok && si_wf(f, lt.psi_i);
    }

    // --- RB+G fission map ---
    ok = ok && si_wu32(f, (uint32_t)px.fission_map.size());
    for (const auto& kv : px.fission_map) {
        ok = ok && si_wu32(f, (uint32_t)kv.first);
        ok = ok && si_wu32(f, (uint32_t)kv.second.first);
        ok = ok && si_wu32(f, (uint32_t)kv.second.second);
    }

    std::fclose(f);
    if (!ok && err) *err = "write failed (disk full?)";
    return ok;
}

// ---------------------------------------------------------------- load

// Rebuilds the graph, then binds physics and restores the learned
// overlay. `si` and `px` are overwritten.
inline bool si_load(ExtSI& si, PhysicsEngine& px,
                    const std::string& path, std::string* err = nullptr) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { if (err) *err = "cannot open: " + path; return false; }

    char magic[8];
    uint32_t version = 0;
    if (!si_r(f, magic, 8) || std::memcmp(magic, SI_MAGIC, 8) != 0) {
        std::fclose(f);
        if (err) *err = "not a .si file";
        return false;
    }
    if (!si_ru32(f, version) || version != SI_VERSION) {
        std::fclose(f);
        if (err) *err = "unsupported .si version";
        return false;
    }

    bool ok = true;

    // --- graph ---
    uint32_t n_nodes = 0;
    ok = ok && si_ru32(f, n_nodes);
    si.graph.nodes.clear();
    si.graph.edges.clear();
    si.graph.by_label.clear();
    for (uint32_t i = 0; ok && i < n_nodes; ++i) {
        std::string lbl;
        ok = ok && si_rstr(f, lbl);
        if (ok) si.graph.add_or_get_node(lbl);
    }
    uint32_t n_edges = 0;
    ok = ok && si_ru32(f, n_edges);
    for (uint32_t i = 0; ok && i < n_edges; ++i) {
        uint32_t a = 0, b = 0, r = 0, dr = 0, pa = 0, pb = 0, st = 0;
        ok = ok && si_ru32(f, a) && si_ru32(f, b) && si_ru32(f, r)
                && si_ru32(f, dr) && si_ru32(f, pa) && si_ru32(f, pb)
                && si_ru32(f, st);
        if (ok) {
            Edge e;
            e.from = (int)a; e.to = (int)b; e.rel = (int)r;
            // Restore provenance so a reloaded substrate can still prove
            // what it concluded. -1 (asserted) round-trips as 0xffffffff.
            e.derived_by_rule = (int)dr;
            e.parent_a        = (int)pa;
            e.parent_b        = (int)pb;
            e.step            = (int)st;
            si.graph.edges.push_back(e);
        }
    }
    // edges[] was filled directly rather than through add_edge, so the
    // O(1) lookup index has to be built from it.
    si.graph.reindex();

    // Bind physics against the restored graph before overlaying state.
    px.bind(si);

    // --- per-cavity ---
    uint32_t n_cav = 0;
    ok = ok && si_ru32(f, n_cav);
    for (uint32_t i = 0; ok && i < n_cav; ++i) {
        float h = 0, m = 0;
        ok = ok && si_rf(f, h) && si_rf(f, m);
        if (ok && i < px.cavities.size()) {
            px.cavities[i].heat_T        = h;
            px.cavities[i].acoustic_mass = m;
        }
    }

    // --- per-tunnel ---
    uint32_t n_tun = 0;
    ok = ok && si_ru32(f, n_tun);
    for (uint32_t i = 0; ok && i < n_tun; ++i) {
        float d = 0, p = 0;
        ok = ok && si_rf(f, d) && si_rf(f, p);
        if (ok && i < px.tunnels.size()) {
            px.tunnels[i].plasticity_delta = d;
            px.tunnels[i].psi_target       = p;
            px.tunnels[i].stiffness =
                px.tunnels[i].base_stiffness + d;
        }
    }

    // --- sensory bindings ---
    uint32_t n_bind = 0;
    ok = ok && si_ru32(f, n_bind);
    if (px.byte_bind_w.size() != 256)
        px.byte_bind_w.assign(256, std::unordered_map<int,float>());
    for (uint32_t i = 0; ok && i < n_bind; ++i) {
        uint32_t b = 0, c = 0; float w = 0;
        ok = ok && si_ru32(f, b) && si_ru32(f, c) && si_rf(f, w);
        if (ok && b < 256 && c < px.cavities.size())
            px.byte_bind_w[b][(int)c] = w;
    }

    // --- lane tunnels ---
    uint32_t n_lane = 0;
    ok = ok && si_ru32(f, n_lane);
    px.lane_tunnels.clear();
    px.lane_pair_index.clear();
    for (uint32_t i = 0; ok && i < n_lane; ++i) {
        uint32_t a = 0, b = 0;
        LaneTunnel lt;
        ok = ok && si_ru32(f, a) && si_ru32(f, b);
        ok = ok && si_rf(f, lt.weight) && si_rf(f, lt.psi);
        ok = ok && si_rf(f, lt.psi_r) && si_rf(f, lt.psi_i);
        if (ok) {
            lt.lane_a = (int)a; lt.lane_b = (int)b;
            px.lane_pair_index[(int)a * 256 + (int)b] =
                (int)px.lane_tunnels.size();
            px.lane_tunnels.push_back(lt);
        }
    }

    // --- fission map ---
    uint32_t n_fis = 0;
    ok = ok && si_ru32(f, n_fis);
    for (uint32_t i = 0; ok && i < n_fis; ++i) {
        uint32_t p = 0, a = 0, b = 0;
        ok = ok && si_ru32(f, p) && si_ru32(f, a) && si_ru32(f, b);
        if (ok) px.fission_map[(int)p] = {(int)a, (int)b};
    }

    std::fclose(f);
    if (!ok && err) *err = "truncated or corrupt .si file";
    return ok;
}


// ---------------------------------------------------------- overlay only
//
// Save/restore ONLY the physics overlay, without the graph. Used when
// the graph is rebuilt from axioms each run but the field's learned
// state — notably heat earned by failed experiments — must carry over.
// Indices are positional, so this is only valid against a graph built
// the same way; a size mismatch is tolerated by truncation rather than
// corrupting state.
inline bool si_save_overlay_only(const PhysicsEngine& px,
                                 const std::string& path,
                                 std::string* err = nullptr) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { if (err) *err = "cannot open for write"; return false; }
    bool ok = si_w(f, SI_MAGIC, 8) && si_wu32(f, SI_VERSION);
    ok = ok && si_wu32(f, (uint32_t)px.cavities.size());
    for (const auto& c : px.cavities) {
        ok = ok && si_wf(f, c.heat_T);
        ok = ok && si_wf(f, c.acoustic_mass);
    }
    ok = ok && si_wf(f, px.ic.tension);
    std::fclose(f);
    return ok;
}

inline bool si_load_overlay_only(PhysicsEngine& px,
                                 const std::string& path,
                                 std::string* err = nullptr) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { if (err) *err = "no prior field state"; return false; }
    char magic[8]; uint32_t version = 0;
    if (!si_r(f, magic, 8) || std::memcmp(magic, SI_MAGIC, 8) != 0 ||
        !si_ru32(f, version) || version != SI_VERSION) {
        std::fclose(f);
        if (err) *err = "bad or stale field file";
        return false;
    }
    uint32_t n = 0;
    bool ok = si_ru32(f, n);
    for (uint32_t i = 0; ok && i < n; ++i) {
        float h = 0, m = 0;
        ok = ok && si_rf(f, h) && si_rf(f, m);
        if (ok && i < px.cavities.size()) {
            px.cavities[i].heat_T        = h;
            px.cavities[i].acoustic_mass = m;
        }
    }
    float t = 0;
    if (ok && si_rf(f, t)) px.ic.tension = t;
    std::fclose(f);
    return ok;
}

}  // namespace unified
