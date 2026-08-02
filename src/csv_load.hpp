// csv_load.hpp — the string→integer boundary. Loads CSV rows of the
// form
//     subject,relation,object
// into the graph. Header rows are auto-skipped (any row whose relation
// token is not a known relation name is silently ignored).
//
// The string-to-integer mapping happens ONLY here and inside
// axiom_loader.hpp. No downstream code parses natural language.

#pragma once

#include "axiom_loader.hpp"
#include "unified.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace unified {

inline std::string csv_trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b - a);
}

inline int load_csv(ExtSI& si, const std::string& path) {
    std::ifstream in(path);
    if (!in) return 0;
    std::string line;
    int accepted = 0;
    while (std::getline(in, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        std::vector<std::string> cols;
        std::string cur;
        for (char c : line) {
            if (c == ',') { cols.push_back(csv_trim(cur)); cur.clear(); }
            else cur.push_back(c);
        }
        if (!cur.empty()) cols.push_back(csv_trim(cur));
        if (cols.size() < 3) continue;
        int rel = rel_from_token(cols[1]);
        if (rel < 0) continue;
        auto r = teach_axiom(si, cols[0], rel, cols[2], "csv");
        if (r.accepted) ++accepted;
    }
    return accepted;
}

} // namespace unified
