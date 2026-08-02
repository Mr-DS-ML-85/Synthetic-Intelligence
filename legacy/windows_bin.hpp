// windows.bin loader — shared by lto.cpp (CPU) and gpu_hcma.cu.
// Format: [u32 magic 'WNDW'][u32 win_bytes][u32 n_windows][n * win bytes]

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "hcma.hpp"

namespace hcma {

struct Windows {
    uint32_t win = 0;
    uint32_t n = 0;
    std::vector<uint8_t> data;

    Vec state(size_t i) const {
        Vec c(win);
        const uint8_t* p = data.data() + (size_t)i * win;
        for (uint32_t k = 0; k < win; ++k) c[k] = byte_to_val((int)p[k]);
        return c;
    }
};

inline bool load_windows(const std::string& path, Windows& w) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    uint32_t magic = 0;
    f.read((char*)&magic, 4);
    if (magic != 0x57444E57u) return false;
    f.read((char*)&w.win, 4);
    f.read((char*)&w.n, 4);
    w.data.resize((size_t)w.win * w.n);
    f.read((char*)w.data.data(), (std::streamsize)w.data.size());
    return (bool)f;
}

} // namespace hcma
