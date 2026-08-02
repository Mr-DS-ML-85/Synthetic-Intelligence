// dispatcher.hpp — MAA state-dependent cognitive dispatcher Delta.
//
// Implements research-paper/maa.md §4: before any inference runs, the
// engine inspects the macroscopic geometry of the active graph and
// routes execution to one of three specialised pipelines. This
// replaces the monolithic respond() loop that treats every query with
// the same rules.
//
//   State alpha (Topological Collision) : query contradicts an axiom  -> Resolution
//   State beta  (Orphan Detected)       : subject/object has deg = 0  -> Abductive
//   State gamma (Connected Query)       : otherwise                    -> Standard
//
// This header only makes the routing decision. The pipelines it names
// are implemented elsewhere (`ask_backward_adaptive`, `MAL`,
// `bisociate`, `render_answer`).

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <string>

namespace unified {

enum class DispatchState {
    Resolution,   // alpha — contradiction
    Abductive,    // beta  — orphan node
    Standard,     // gamma — normal query
};

inline const char* dispatch_state_name(DispatchState s) {
    switch (s) {
        case DispatchState::Resolution: return "Resolution (contradiction detected)";
        case DispatchState::Abductive:  return "Abductive (orphan node detected)";
        default:                        return "Standard (connected query)";
    }
}

// Decide the routing for a proposed triple (subject, rel, object).
// `rel` and `object` may be sentinels (rel < 0 means "just describe
// subject", empty object means the same). `subject` is always the
// key concept the query is about.
inline DispatchState
dispatch(const SI& si, const std::string& subject,
         int rel = -1, const std::string& object = std::string()) {
    // Alpha — direct contradiction check.
    if (rel >= 0 && !object.empty()) {
        if (si.would_contradict(subject, rel, object))
            return DispatchState::Resolution;
    }
    // Beta — orphan: either subject or object is not in the graph, or
    // is in the graph but has zero outgoing edges (isolated island).
    auto it_s = si.graph.by_label.find(subject);
    if (it_s == si.graph.by_label.end()) return DispatchState::Abductive;
    int deg = 0;
    for (const auto& e : si.graph.edges) if (e.from == it_s->second) ++deg;
    if (deg == 0) return DispatchState::Abductive;
    if (!object.empty()) {
        auto it_o = si.graph.by_label.find(object);
        if (it_o == si.graph.by_label.end()) return DispatchState::Abductive;
    }
    return DispatchState::Standard;
}

} // namespace unified
