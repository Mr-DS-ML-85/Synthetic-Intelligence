// seed_ontology — bulk-load subject/relation/object triples from one or
// more text files into the big ontology, run reason_ext(), then dump a
// summary. Useful for growing the ontology from curated domain files
// (e.g. data/networking.axioms, data/biology.axioms) without editing
// big_ontology.hpp by hand.
//
// Usage:
//   ./seed_ontology file1.axioms [file2.axioms ...]
//
// File format: see src/axiom_loader.hpp header. Lines are
//   subject   relation   object
// with relation in {is_a, has, implies, part_of, equals, causes, not}.

#include "axiom_loader.hpp"
#include "big_ontology.hpp"
#include "primitives.hpp"
#include "unified.hpp"

#include <cstdio>
#include <string>

using namespace unified;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr,
            "usage: %s file1.axioms [file2.axioms ...]\n"
            "  Loads subject-relation-object triples on top of the big\n"
            "  ontology and reports how the graph grew.\n",
            argv[0]);
        return 1;
    }
    ExtSI si;
    build_big_ontology(si);
    size_t nodes_before = si.graph.nodes.size();
    size_t edges_before = si.graph.edges.size();

    int total_accepted = 0;
    for (int i = 1; i < argc; ++i) {
        int n = load_axiom_file(si, argv[i]);
        total_accepted += n;
    }

    // Kick the rewriter — new premises may derive new transitive edges.
    auto rep = si.reason_ext();
    size_t nodes_after = si.graph.nodes.size();
    size_t edges_after = si.graph.edges.size();

    std::printf("seed_ontology summary:\n");
    std::printf("  files loaded         : %d\n", argc - 1);
    std::printf("  axioms accepted      : %d\n", total_accepted);
    std::printf("  nodes  before/after  : %zu -> %zu (+%zu)\n",
                nodes_before, nodes_after, nodes_after - nodes_before);
    std::printf("  edges  before/after  : %zu -> %zu (+%zu)\n",
                edges_before, edges_after, edges_after - edges_before);
    std::printf("  reasoning added      : %d edges over %d steps\n",
                rep.edges_added, rep.steps_used);
    return 0;
}
