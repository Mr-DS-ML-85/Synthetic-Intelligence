// axiom_loader.hpp — Ontology seeding + LTO (Live Training over
// Ontology) persistence.
//
// Purpose:
//   Bulk-load subject-relation-object triples from a plain-text file
//   into an ExtSI graph, and append user-taught axioms back out during
//   an interactive session. This is the "Manual Proxy" side of the LTO
//   architecture (research-paper/lto.md §3): sparse, high-confidence
//   corrective feedback from a human, converted into a permanent
//   perturbation of the logical potential Phi (here, the graph itself).
//
// File format (one triple per line, tokens separated by whitespace):
//
//   subject   relation   object   [# optional comment]
//
//   relation is one of: is_a | has | implies | part_of | equals | causes | not
//
// Examples:
//   linux            is_a   operating_system
//   operating_system is_a   software
//   whale            is_a   mammal
//   fire             causes heat
//
// Lines starting with '#' or blank lines are ignored. Unknown relation
// tokens are skipped with a warning to stderr. Duplicate assertions are
// silently deduped by ExtSI itself.
//
// The ALTO shadow-check (auto-proxy) uses SI::would_contradict before
// each assertion; a contradicting axiom is refused and reported.

#pragma once

#include "primitives.hpp"
#include "unified.hpp"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace unified {

// Parse a relation token to its integer id. Returns -1 if unknown.
inline int rel_from_token(const std::string& t) {
    if (t == "is_a")             return REL_IS_A;
    if (t == "has")              return REL_HAS;
    if (t == "implies")          return REL_IMPLIES;
    if (t == "part_of")          return REL_PART_OF;
    if (t == "equals")           return REL_EQUALS;
    if (t == "causes")           return REL_CAUSES;
    if (t == "not")              return REL_NOT;   // primitives.hpp extension
    // Mechanistic relations (medical / biochemistry / discovery).
    if (t == "binds_to")         return REL_BINDS_TO;
    if (t == "inhibits")         return REL_INHIBITS;
    if (t == "activates")        return REL_ACTIVATES;
    if (t == "treats")           return REL_TREATS;
    if (t == "causes_disease")   return REL_CAUSES_DISEASE;
    if (t == "part_of_pathway")  return REL_PART_OF_PATHWAY;
    if (t == "expressed_in")     return REL_EXPRESSED_IN;
    if (t == "metabolizes")      return REL_METABOLIZES;
    if (t == "upregulates")      return REL_UPREGULATES;
    if (t == "downregulates")    return REL_DOWNREGULATES;
    if (t == "is_proportional_to") return REL_PROPORTIONAL_TO;
    if (t == "mirror_of")        return REL_MIRROR_OF;
    if (t == "derives_from")     return REL_DERIVES_FROM;
    if (t == "is_fundamental")   return REL_FUNDAMENTAL;
    if (t == "triggered_by")     return REL_TRIGGERED_BY;
    // Code-structure relations (see primitives.hpp).
    if (t == "calls")            return REL_CALLS;
    if (t == "returns")          return REL_RETURNS;
    if (t == "takes_arg")        return REL_TAKES_ARG;
    if (t == "precedes")         return REL_PRECEDES;
    if (t == "imports")          return REL_IMPORTS;
    if (t == "mutates")          return REL_MUTATES;
    // Commonsense relations (ConceptNet-converted; provenance = asserted,
    // support testimonial not experimental — see
    // tools/extract_conceptnet_axioms.py).
    if (t == "used_for")         return REL_USED_FOR;
    if (t == "capable_of")       return REL_CAPABLE_OF;
    if (t == "at_location")      return REL_AT_LOCATION;
    if (t == "desires")          return REL_DESIRES;
    if (t == "created_by")       return REL_CREATED_BY;
    return -1;
}

// Result of a single teach/seed attempt.
struct AxiomWriteReport {
    bool  accepted  = false;
    bool  duplicate = false;
    bool  contradiction = false;
    std::string message;
};

// Assert one triple with the ALTO shadow-check. Returns a report.
// `source` is copied into the report message for user-facing feedback.
inline AxiomWriteReport teach_axiom(ExtSI& si,
                                    const std::string& subj,
                                    int rel,
                                    const std::string& obj,
                                    const std::string& source = "user") {
    AxiomWriteReport r;
    if (subj.empty() || obj.empty() || rel < 0) {
        r.message = "malformed triple";
        return r;
    }
    // ALTO auto-proxy: contradiction check against existing graph.
    if (si.would_contradict(subj, rel, obj)) {
        r.contradiction = true;
        r.message = "contradicts an existing axiom (from " + source +
                    "); refused to overwrite. Delete the conflicting "
                    "premise first if this is intentional.";
        return r;
    }
    bool had_before = false;
    auto it_s = si.graph.by_label.find(subj);
    auto it_o = si.graph.by_label.find(obj);
    if (it_s != si.graph.by_label.end() && it_o != si.graph.by_label.end()) {
        had_before = si.graph.has_edge(it_s->second, it_o->second, rel);
    }
    if (had_before) {
        r.duplicate = true;
        r.accepted  = true;   // idempotent — treat as success
        r.message   = "already known";
        return r;
    }
    si.assert_fact(subj, rel, obj);
    r.accepted = true;
    r.message  = "added";
    return r;
}

// Bulk-load a file of triples. Returns the number of accepted axioms.
inline int load_axiom_file(ExtSI& si, const std::string& path,
                           FILE* log = stderr) {
    std::ifstream in(path);
    if (!in) return 0;
    // Seeding an ontology is not lived experience — see SI::assert_fact.
    // Restored on exit so interactive teaching still drives Layer 1.
    const bool prev_bulk = si.bulk_load;
    si.bulk_load = true;
    struct BulkGuard {
        ExtSI& s; bool prev;
        ~BulkGuard() { s.bulk_load = prev; }
    } guard{si, prev_bulk};
    int accepted = 0, refused = 0, malformed = 0, dupes = 0;
    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        // Strip inline comment.
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        // Trim.
        size_t a = 0, b = line.size();
        while (a < b && std::isspace((unsigned char)line[a])) ++a;
        while (b > a && std::isspace((unsigned char)line[b-1])) --b;
        if (a == b) continue;
        std::string clean = line.substr(a, b - a);
        std::istringstream iss(clean);
        std::string subj, rel_tok, obj;
        if (!(iss >> subj >> rel_tok >> obj)) {
            ++malformed;
            if (log) std::fprintf(log,
                "[axiom_loader] %s:%d malformed (need 3 tokens): %s\n",
                path.c_str(), line_no, clean.c_str());
            continue;
        }
        int rel = rel_from_token(rel_tok);
        if (rel < 0) {
            ++malformed;
            if (log) std::fprintf(log,
                "[axiom_loader] %s:%d unknown relation '%s' — skipped\n",
                path.c_str(), line_no, rel_tok.c_str());
            continue;
        }
        auto r = teach_axiom(si, subj, rel, obj, path);
        if (r.contradiction) {
            ++refused;
            if (log) std::fprintf(log,
                "[axiom_loader] %s:%d %s\n",
                path.c_str(), line_no, r.message.c_str());
        } else if (r.duplicate) {
            ++dupes;
        } else if (r.accepted) {
            ++accepted;
        }
    }
    if (log) std::fprintf(log,
        "[axiom_loader] %s: +%d accepted, %d duplicates, %d refused, %d malformed\n",
        path.c_str(), accepted, dupes, refused, malformed);
    return accepted;
}

// Append one teach triple to the persistent user-axioms file so the
// next boot autoloads it (Streaming Bayesian update — lto.md §2.2).
inline bool append_axiom_line(const std::string& path,
                              const std::string& subj,
                              int rel,
                              const std::string& obj) {
    const char* rn = nullptr;
    switch (rel) {
        case REL_IS_A:              rn = "is_a"; break;
        case REL_HAS:               rn = "has"; break;
        case REL_IMPLIES:           rn = "implies"; break;
        case REL_PART_OF:           rn = "part_of"; break;
        case REL_EQUALS:            rn = "equals"; break;
        case REL_CAUSES:            rn = "causes"; break;
        case REL_NOT:               rn = "not"; break;
        case REL_BINDS_TO:          rn = "binds_to"; break;
        case REL_INHIBITS:          rn = "inhibits"; break;
        case REL_ACTIVATES:         rn = "activates"; break;
        case REL_TREATS:            rn = "treats"; break;
        case REL_CAUSES_DISEASE:    rn = "causes_disease"; break;
        case REL_PART_OF_PATHWAY:   rn = "part_of_pathway"; break;
        case REL_EXPRESSED_IN:      rn = "expressed_in"; break;
        case REL_METABOLIZES:       rn = "metabolizes"; break;
        case REL_UPREGULATES:       rn = "upregulates"; break;
        case REL_DOWNREGULATES:     rn = "downregulates"; break;
        case REL_PROPORTIONAL_TO:   rn = "is_proportional_to"; break;
        case REL_MIRROR_OF:         rn = "mirror_of"; break;
        case REL_DERIVES_FROM:      rn = "derives_from"; break;
        case REL_FUNDAMENTAL:       rn = "is_fundamental"; break;
        case REL_TRIGGERED_BY:      rn = "triggered_by"; break;
        default: return false;
    }
    std::ofstream out(path, std::ios::app);
    if (!out) return false;
    out << subj << ' ' << rn << ' ' << obj << '\n';
    return true;
}

} // namespace unified
