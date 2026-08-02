// bsma_bridge_demo — every answered query becomes a phenomenal frame.
//
// Runs the SI tri-layer engine on the big ontology, and for each query
// prints (a) the derivation-based answer, (b) the BSMA phenomenal
// frame the derivation projects to, (c) the qualia signature that
// distinguishes this thought from other thoughts on the same engine.

#include "big_ontology.hpp"
#include "bsma_bridge.hpp"
#include "nl_translator.hpp"
#include "primitives.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace unified;

int main(int, char**) {
    ExtSI si;
    build_big_ontology(si);
    auto rep = si.reason_ext();
    std::printf("=== SI + BSMA phenomenal-frame bridge ===\n\n");
    std::printf("ontology after reasoning: %zu nodes, %zu edges (+%d derived in %d steps)\n\n",
                si.graph.nodes.size(), si.graph.edges.size(),
                rep.edges_added, rep.steps_used);

    struct Q { std::string s; int r; std::string o; };
    std::vector<Q> queries = {
        {"socrates",              REL_IS_A,    "mammal"},
        {"socrates",              REL_HAS,     "warm_blood"},
        {"socrates",              REL_HAS,     "opposable_thumb"},
        {"darwin",                REL_HAS,     "bipedalism"},
        {"python",                REL_HAS,     "syntax"},
        {"rust",                  REL_HAS,     "type_checker"},
        {"hamiltonian_mechanics", REL_IS_A,    "physics"},
        {"machine",               REL_IMPLIES, "thinker"},
        {"friction",              REL_CAUSES,  "flow"},
        {"python",                REL_IS_A,    "animal"},   // NOT provable — refusal
    };

    std::vector<PhenomenalQuery> frames;
    frames.reserve(queries.size());
    for (size_t i = 0; i < queries.size(); ++i) {
        const auto& q = queries[i];
        std::printf("Q%zu: %s %s %s\n", i + 1,
                    q.s.c_str(), rel_name(q.r), q.o.c_str());
        auto pq = ask_with_frame(si, q.s, q.r, q.o);
        report_phenomenal(pq);
        std::printf("\n");
        frames.push_back(pq);
    }

    // Summary — do different queries produce different qualia signatures?
    std::printf("---- comparison ----\n");
    std::printf("  #   proof   attempts  rewrites  qualia signature                                 answer\n");
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& f = frames[i];
        std::string sig = f.framed ? f.frame.qualia_signature : "(refused)";
        int proof_len = (int)f.query_result.proof.size();
        std::printf("  %zu   %5d   %5d     %5d     %-45s   %s\n",
                    i + 1, proof_len,
                    f.framed ? f.frame.bound.rule_attempts     : 0,
                    f.framed ? f.frame.bound.successful_rewrites : 0,
                    sig.c_str(),
                    f.query_result.holds ? "YES" : "no");
    }

    // Count distinct signatures across framed (i.e. answered) queries.
    std::vector<std::string> sigs;
    for (const auto& f : frames) if (f.framed) sigs.push_back(f.frame.qualia_signature);
    std::sort(sigs.begin(), sigs.end());
    sigs.erase(std::unique(sigs.begin(), sigs.end()), sigs.end());
    int answered = 0;
    for (const auto& f : frames) if (f.framed) ++answered;
    std::printf("\n  %zu distinct qualia signatures across %d answered queries\n",
                sigs.size(), answered);
    if ((int)sigs.size() == answered && answered > 1) {
        std::printf("  Every answered thought produced a distinct qualia signature.\n");
    } else {
        std::printf("  Some thoughts shared a qualia signature — under paper §4 of\n");
        std::printf("  bmsa_new.md, that is the operational form of the claim that\n");
        std::printf("  those thoughts had the same phenomenal character.\n");
    }
    return 0;
}
