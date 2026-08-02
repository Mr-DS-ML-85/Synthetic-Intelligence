// discover — the novelty CLI. Loads an ontology, then runs discovery
// queries: path tracing between two nodes, intersection of shared
// mechanisms, and drug-repurposing abduction on a disease.
//
// Usage:
//   ./discover data/insulin_pathway.axioms path metformin glucose_uptake
//   ./discover data/insulin_pathway.axioms intersect metformin berberine
//   ./discover data/insulin_pathway.axioms repurpose type2_diabetes inhibits
//   ./discover data/insulin_pathway.axioms repurpose type2_diabetes activates
//
// All output is explicitly framed as HYPOTHESES where appropriate —
// the engine never asserts a novel treatment as fact. It surfaces
// mechanism-based candidates that a wet-lab or clinical group can then
// evaluate.

#include "axiom_loader.hpp"
#include "novelty.hpp"
#include "primitives.hpp"
#include "unified.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace unified;

static int mechanism_rel_from_token(const std::string& t) {
    if (t == "inhibits")     return REL_INHIBITS;
    if (t == "activates")    return REL_ACTIVATES;
    if (t == "binds_to")     return REL_BINDS_TO;
    if (t == "metabolizes")  return REL_METABOLIZES;
    if (t == "upregulates")  return REL_UPREGULATES;
    if (t == "downregulates")return REL_DOWNREGULATES;
    return REL_INHIBITS;
}

static void usage(const char* prog) {
    std::fprintf(stderr,
        "usage:\n"
        "  %s <axioms_file> path <A> <B> [max_hops]\n"
        "  %s <axioms_file> intersect <A> <B>\n"
        "  %s <axioms_file> repurpose <disease> [mechanism_rel]\n"
        "\n"
        "mechanism_rel ∈ {inhibits, activates, binds_to, metabolizes,\n"
        "                 upregulates, downregulates} — default: inhibits\n",
        prog, prog, prog);
}

int main(int argc, char** argv) {
    if (argc < 3) { usage(argv[0]); return 1; }
    const std::string axioms_path = argv[1];
    const std::string mode = argv[2];

    ExtSI si;
    int loaded = load_axiom_file(si, axioms_path);
    if (loaded == 0) {
        std::fprintf(stderr, "discover: no axioms loaded from %s\n",
                     axioms_path.c_str());
        return 2;
    }
    si.reason_ext();

    if (mode == "path") {
        if (argc < 5) { usage(argv[0]); return 1; }
        std::string a = argv[3], b = argv[4];
        int max_hops = (argc >= 6) ? std::atoi(argv[5]) : 6;
        auto paths = find_paths(si, a, b, max_hops, /*cap=*/16);
        std::printf("Paths from %s to %s (max %d hops):\n",
                    a.c_str(), b.c_str(), max_hops);
        if (paths.empty()) {
            std::printf("  (none found; the two concepts are not "
                        "reachable through the current ontology)\n");
        } else {
            for (size_t i = 0; i < paths.size(); ++i) {
                std::printf("  [%zu, len=%d] %s\n", i + 1,
                            paths[i].length(),
                            render_path(si, paths[i]).c_str());
            }
        }
        return 0;
    }
    if (mode == "intersect") {
        if (argc < 5) { usage(argv[0]); return 1; }
        std::string a = argv[3], b = argv[4];
        auto sh = shared_targets(si, a, b);
        std::printf("%s\n", render_shared(si, a, b, sh).c_str());
        return 0;
    }
    if (mode == "repurpose") {
        if (argc < 4) { usage(argv[0]); return 1; }
        std::string disease = argv[3];
        int mech = (argc >= 5) ? mechanism_rel_from_token(argv[4])
                               : REL_INHIBITS;
        auto cands = repurposing_candidates(si, disease, mech);
        std::printf("%s\n",
                    render_repurposing(si, disease, cands, mech).c_str());
        return 0;
    }
    usage(argv[0]);
    return 1;
}
