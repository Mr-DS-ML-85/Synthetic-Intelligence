// nl_translator.hpp — template-based natural-language surface realization
// for derivation chains from the tri-layer engine.
//
// Fixes limitation §3 from research-paper/how_agent_imple.md
// ("Layer 3 translator is basic markdown, not fluent text").
//
// Design: rule-specific templates turn each derivation edge into a
// short English clause; the clauses are strung into an argument
// paragraph. This is NOT an LLM. It is a bounded, verifier-checked
// surface layer — the paper's §5.3.4 verifier guarantee still holds
// because every clause corresponds to an already-derived edge.
//
// The output is intentionally plain and honest; if you want florid
// prose, wrap this with a small LM as the Layer 4 scribe (paper §8).

#pragma once

#include "unified.hpp"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace unified {

// Replace underscores with spaces for prose. Leaves the rest untouched
// (labels are canonical lowercase, so we do not case-fold either).
inline std::string english_label(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(c == '_' ? ' ' : c);
    return out;
}

// English phrase for a single edge. Uses the relation name to pick the
// right verb form so "socrates --is_a--> mortal" comes out as "Socrates
// is a mortal" and not "Socrates is_a mortal".
inline std::string english_edge(const Node& from, int rel, const Node& to) {
    std::string s = english_label(from.label);
    std::string o = english_label(to.label);
    switch (rel) {
        case REL_IS_A:    return s + " is a " + o;
        case REL_HAS:     return s + " has " + o;
        case REL_IMPLIES: return "if " + s + " then " + o;
        case REL_PART_OF: return s + " is part of " + o;
        case REL_EQUALS:  return s + " equals " + o;
        case REL_CAUSES:  return s + " causes " + o;
        default:          return s + " " + rel_name(rel) + " " + o;
    }
}

// English phrase for one derivation step. Includes the rule name in
// parenthesis so a reader can audit which primitive fired.
inline std::string english_step(const SI& si, int edge_idx) {
    const Edge& e = si.graph.edges[edge_idx];
    std::string body = english_edge(si.graph.nodes[e.from], e.rel, si.graph.nodes[e.to]);
    if (e.derived_by_rule < 0) return body + " (given)";
    const std::string& rule = si.rules[e.derived_by_rule].name;
    return body + " (by " + rule + ")";
}

// Full proof rendering. Takes the ordered proof chain from a
// QueryResult and produces a paragraph of the form:
//
//   Given that A is a B, and B has C, we obtain that A has C
//   (by class_has_flows_down). Given that A has C, and C is a D,
//   we obtain that A has D (by class_has_flows_down). Therefore
//   A has D.
//
// The final "Therefore" is the query goal; everything before it
// is a witness to it.
inline std::string english_proof(const SI& si, const SI::QueryResult& q) {
    if (!q.holds || q.proof.empty()) return "No proof is available for this claim.";
    std::ostringstream oss;
    // Group premises together, then walk derived edges in order.
    std::vector<int> premises, derived;
    for (int e : q.proof) {
        (si.graph.edges[e].derived_by_rule < 0 ? premises : derived).push_back(e);
    }
    if (!premises.empty()) {
        oss << "Premises: ";
        for (size_t i = 0; i < premises.size(); ++i) {
            const Edge& e = si.graph.edges[premises[i]];
            oss << english_edge(si.graph.nodes[e.from], e.rel, si.graph.nodes[e.to]);
            oss << (i + 1 < premises.size() ? "; " : ".\n");
        }
    }
    for (int e : derived) oss << "Then " << english_step(si, e) << ". ";
    if (!derived.empty()) {
        int last = derived.back();
        const Edge& le = si.graph.edges[last];
        oss << "Therefore, "
            << english_edge(si.graph.nodes[le.from], le.rel, si.graph.nodes[le.to])
            << ".";
    }
    return oss.str();
}

// Multi-answer summary: given a set of QueryResults, produce a short
// research-note-style paragraph. Used by the paper writer / interactive
// mode.
inline std::string english_summary(const SI& si,
                                   const std::vector<SI::QueryResult>& results) {
    std::ostringstream oss;
    int held = 0;
    for (const auto& r : results) if (r.holds) ++held;
    oss << "Of " << results.size() << " queries, "
        << held << " were derived and "
        << (results.size() - held) << " were refused as underivable.\n\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        oss << "(" << (i + 1) << ") ";
        if (r.holds) oss << english_proof(si, r);
        else         oss << "The query could not be derived from the current ontology; the engine reports \"" << r.translation << "\".";
        oss << "\n\n";
    }
    return oss.str();
}

} // namespace unified
