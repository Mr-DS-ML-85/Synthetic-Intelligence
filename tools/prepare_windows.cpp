// Dataset preprocessor: JSONL -> compact "text window" binary.
//
// Reads {"text": "..."} lines (Pile / C4 / Wikipedia format) and emits
// fixed-width byte windows of coherent text. NO labels, no (Q,A) pairs.
// The trainer will use each window as a positive example and synthesise
// negatives (random / shuffled / spliced with another window). That is
// what teaches Phi to be an energy of *coherence*, not a memorized map.
//
// Output binary format (little-endian):
//   [u32 magic 'WNDW']
//   [u32 win_bytes][u32 n_windows]
//   [n_windows * win_bytes raw bytes]

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static bool extract_text_field(const std::string& line, std::string& out) {
    const char* key = "\"text\"";
    size_t p = line.find(key);
    if (p == std::string::npos) return false;
    p += 6;
    while (p < line.size() && (line[p] == ' ' || line[p] == ':')) ++p;
    if (p >= line.size() || line[p] != '"') return false;
    ++p;
    out.clear();
    while (p < line.size()) {
        char c = line[p];
        if (c == '"') return true;
        if (c == '\\' && p + 1 < line.size()) {
            char e = line[p + 1];
            switch (e) {
                case 'n': out.push_back(' '); break;   // collapse whitespace
                case 't': out.push_back(' '); break;
                case 'r': break;
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'u': out.push_back('?'); p += 4; break;
                default: out.push_back(e); break;
            }
            p += 2;
        } else {
            out.push_back(c == '\n' ? ' ' : c);
            ++p;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    std::vector<std::string> inputs;
    std::string out_path = "windows.bin";
    size_t win = 512;
    size_t stride = 256;
    size_t max_windows = 200000;

    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", k.c_str()); std::exit(1); }
            return argv[++i];
        };
        if (k == "-o" || k == "--out") out_path = next();
        else if (k == "--win") win = (size_t)std::atoi(next().c_str());
        else if (k == "--stride") stride = (size_t)std::atoi(next().c_str());
        else if (k == "--max") max_windows = (size_t)std::atoll(next().c_str());
        else inputs.push_back(k);
    }
    if (inputs.empty()) {
        std::fprintf(stderr,
            "usage: prepare_windows <in.jsonl>... [-o windows.bin] [--win B] [--stride B] [--max N]\n");
        return 1;
    }

    std::ofstream out(out_path, std::ios::binary);
    if (!out) { std::fprintf(stderr, "cannot open %s\n", out_path.c_str()); return 1; }
    uint32_t magic = 0x57444E57u; // 'WNDW' (little-endian)
    uint32_t w = (uint32_t)win, nw = 0;
    out.write((const char*)&magic, 4);
    out.write((const char*)&w, 4);
    std::streampos nw_pos = out.tellp();
    out.write((const char*)&nw, 4);

    size_t docs = 0, lines = 0;
    for (const auto& path : inputs) {
        std::ifstream f(path);
        if (!f) { std::fprintf(stderr, "skipping %s (open failed)\n", path.c_str()); continue; }
        std::string line, text;
        while (std::getline(f, line) && nw < max_windows) {
            ++lines;
            if (!extract_text_field(line, text)) continue;
            ++docs;
            // Collapse repeated spaces; only keep printable ASCII + space.
            std::string clean;
            clean.reserve(text.size());
            char prev = 0;
            for (char c : text) {
                if (c < 32 || c > 126) c = ' ';
                if (c == ' ' && prev == ' ') continue;
                clean.push_back(c);
                prev = c;
            }
            for (size_t off = 0; off + win <= clean.size() && nw < max_windows;
                 off += stride) {
                out.write(clean.data() + off, (std::streamsize)win);
                ++nw;
                if (nw % 50000 == 0) std::printf("  ... %u windows\n", nw);
            }
        }
        std::printf("[%s] lines=%zu docs=%zu windows=%u\n", path.c_str(), lines, docs, nw);
    }
    out.seekp(nw_pos);
    out.write((const char*)&nw, 4);
    std::printf("wrote %u windows of %zu bytes to %s\n", nw, win, out_path.c_str());
    return 0;
}
