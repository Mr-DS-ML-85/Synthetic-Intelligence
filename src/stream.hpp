// stream.hpp — Streaming CSV → drive_envelope binding.
//
// CSV format:
//   first row  : header  ("t,label1,label2,...")   — the first column is
//                a time marker (ignored by the physics; used only for
//                logging); every other column names a graph cavity.
//   data rows  : one row per streaming step. Each numeric column
//                becomes an amplitude that is fed to the corresponding
//                cavity as a driving-force envelope for `hold_ticks`.
//
// The mapping is a scalar affine transform per column:
//   amp = (value − col_mean) / (col_std + ε) · amp_gain
// (mean/std auto-tracked online with EMAs). This normalises different
// scales (price, TVL, sentiment) into comparable driving forces without
// hardcoded per-column knobs.

#pragma once

#include "physics.hpp"
#include "unified.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace unified {

class StreamIngester {
public:
    PhysicsEngine* sub = nullptr;
    const SI*      graph_ptr = nullptr;
    std::ifstream  in;

    // Column i maps to node id (may be −1 for unknown / time column).
    std::vector<int>         column_to_node;
    // Running EMA of mean and variance per column, for normalisation.
    std::vector<float>       col_mean;
    std::vector<float>       col_var;
    float ema_alpha  = 0.10f;
    float amp_gain   = 6.0f;      // driving amplitude scale
    float freq       = 8.0f;      // sine driver frequency for stream nodes
    int   hold_ticks = 40;        // each row's envelope lasts this many ticks
    int   ticks_between_rows = 30;// how many physics ticks per data row
    std::string time_col_label = "t";
    bool  header_read = false;
    long  row_count   = 0;

    void bind(PhysicsEngine* p, const SI& si) {
        sub = p;
        graph_ptr = &si;
    }

    bool open(const std::string& path) {
        in.close();
        in.open(path);
        if (!in) return false;
        std::string header;
        if (!std::getline(in, header)) return false;
        parse_header(header);
        header_read = true;
        row_count = 0;
        return true;
    }

    void close() { in.close(); header_read = false; }

    // Read and apply one row. Returns false at EOF.
    bool step_row() {
        if (!in || !header_read) return false;
        std::string line;
        if (!std::getline(in, line)) return false;
        std::vector<std::string> tokens = split_csv(line);
        int C = (int)std::min(tokens.size(), column_to_node.size());
        for (int c = 0; c < C; ++c) {
            int nid = column_to_node[c];
            if (nid < 0) continue;    // I/O guard, not physics
            float value = parse_float(tokens[c]);
            // Online EMA of mean/variance.
            float prev_mean = col_mean[c];
            col_mean[c] = prev_mean * (1.0f - ema_alpha) + ema_alpha * value;
            float dev = value - col_mean[c];
            col_var[c] = col_var[c] * (1.0f - ema_alpha) + ema_alpha * dev * dev;
            float std_ = std::sqrt(col_var[c] + 1e-6f);
            float amp  = std::tanh((value - col_mean[c]) / std_) * amp_gain;
            sub->drive(nid, freq, amp, hold_ticks);
        }
        ++row_count;
        return true;
    }

    // Read next N rows, running `ticks_between_rows` physics ticks
    // between each. Returns rows actually applied.
    int step_rows(int n) {
        int applied = 0;
        for (int i = 0; i < n; ++i) {
            if (!step_row()) break;
            for (int t = 0; t < ticks_between_rows; ++t) sub->tick();
            ++applied;
        }
        return applied;
    }

    void dump_columns(FILE* out = stdout) {
        for (size_t c = 0; c < column_to_node.size(); ++c) {
            int nid = column_to_node[c];
            const char* lbl = (nid < 0) ? "(none)"
                            : graph_ptr->graph.nodes[nid].label.c_str();
            std::fprintf(out, "col[%zu] -> %s  μ=%.4f  σ=%.4f\n",
                c, lbl, col_mean[c], std::sqrt(col_var[c] + 1e-6f));
        }
    }

private:
    void parse_header(const std::string& line) {
        std::vector<std::string> h = split_csv(line);
        column_to_node.assign(h.size(), -1);
        col_mean.assign(h.size(), 0.0f);
        col_var.assign(h.size(), 1.0f);
        for (size_t c = 0; c < h.size(); ++c) {
            std::string lbl = trim(h[c]);
            if (lbl == time_col_label) continue;  // time column stays −1
            auto it = graph_ptr->graph.by_label.find(lbl);
            if (it != graph_ptr->graph.by_label.end())
                column_to_node[c] = it->second;
        }
    }
    static std::string trim(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a])) ++a;
        while (b > a && std::isspace((unsigned char)s[b-1])) --b;
        return s.substr(a, b - a);
    }
    static std::vector<std::string> split_csv(const std::string& line) {
        std::vector<std::string> out; std::string cur;
        for (char ch : line) {
            if (ch == ',') { out.push_back(trim(cur)); cur.clear(); }
            else cur.push_back(ch);
        }
        out.push_back(trim(cur));
        return out;
    }
    static float parse_float(const std::string& s) {
        if (s.empty()) return 0.0f;
        try { return std::stof(s); } catch (...) { return 0.0f; }
    }
};

} // namespace unified
