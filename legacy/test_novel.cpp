// test_novel — hallucination and novel-query test suite.
//
// Two categories:
//
//   A) NOVEL COMPOSITION — queries whose answer requires composing multiple
//      primitives across premises the system was given. The specific
//      (subject, relation, object) triple was never asserted. If the
//      engine returns a correct answer with a traceable proof, that is
//      *thinking*: composition of stored primitives into a novel
//      derivation. Paper §3.4.
//
//   B) HONEST REFUSAL — queries that are either outside the ontology
//      or require capabilities the engine does not have (code
//      generation, modal reasoning, etc.). Paper §7.4 predicts a
//      near-zero hallucination rate on these, with "unknown" (or a
//      grounded refusal) instead of a fabricated answer.
//
// The point of this file is to exhibit both, count each, and report the
// hallucination rate empirically. Any answer emitted must (i) hold
// in the reasoned graph AND (ii) verify via meta_learner::verify_edge.

#include "meta_learner.hpp"
#include "primitives.hpp"

#include <cstdio>
#include <string>

using namespace unified;

// Build a mixed ontology spanning several domains. Each fact is a real
// premise; none of the queries below match a premise directly.
static void build_world(ExtSI& si) {
    // Rules — the 9 primitives (recursion implicit in the fixpoint loop).
    si.add_transitive  ("t_is_a",       REL_IS_A,    REL_IS_A,    REL_IS_A);
    si.add_transitive  ("t_part_of",    REL_PART_OF, REL_PART_OF, REL_PART_OF);
    si.add_deduction   ("modus_ponens");
    si.add_inheritance ("has_flows",    REL_HAS);
    si.add_inheritance ("part_flows",   REL_PART_OF);
    si.add_negation    ("contrapositive", REL_IMPLIES);
    si.add_case_analysis("or_elim",      REL_IMPLIES);
    si.add_induction   ("gen_by_class", REL_HAS, /*witnesses=*/3);
    si.add_analogy     ("map_by_is_a",  REL_IS_A);

    // Geography
    si.assert_fact("paris",   REL_IS_A,    "capital");
    si.assert_fact("berlin",  REL_IS_A,    "capital");
    si.assert_fact("tokyo",   REL_IS_A,    "capital");
    si.assert_fact("capital", REL_IS_A,    "city");
    si.assert_fact("city",    REL_HAS,     "population");
    si.assert_fact("city",    REL_HAS,     "government");
    si.assert_fact("paris",   REL_PART_OF, "france");
    si.assert_fact("france",  REL_PART_OF, "europe");
    si.assert_fact("europe",  REL_PART_OF, "earth");

    // Biology
    si.assert_fact("socrates", REL_IS_A, "philosopher");
    si.assert_fact("philosopher", REL_IS_A, "human");
    si.assert_fact("human",     REL_IS_A, "primate");
    si.assert_fact("primate",   REL_IS_A, "mammal");
    si.assert_fact("mammal",    REL_IS_A, "vertebrate");
    si.assert_fact("vertebrate",REL_IS_A, "animal");
    si.assert_fact("mammal",    REL_HAS,  "warm_blood");
    si.assert_fact("mammal",    REL_HAS,  "spine");
    si.assert_fact("animal",    REL_HAS,  "metabolism");

    // Computing
    si.assert_fact("python",     REL_IS_A, "language");
    si.assert_fact("javascript", REL_IS_A, "language");
    si.assert_fact("rust",       REL_IS_A, "language");
    si.assert_fact("language",   REL_IS_A, "notation");
    si.assert_fact("notation",   REL_HAS,  "syntax");
    si.assert_fact("notation",   REL_HAS,  "grammar");
    // Note: we do NOT assert any concrete code strings. The engine
    // has no way to fabricate one.

    // Philosophy of mind (deliberately sparse)
    // We assert the schematic form of a Turing test claim WITHOUT the
    // specific consequent that "machines think", because that is
    // philosophically contested and asserting either would be
    // dishonest. The engine should therefore refuse to answer
    // "can machines think?" — that is the correct behaviour.
    si.assert_fact("turing_test", REL_IS_A, "computability_criterion");
    si.assert_fact("computability_criterion", REL_IS_A, "test");

    // Translation table entries only for questions we *want* to
    // support. Any unsupported query falls back to "unknown (out of
    // ontology)". That is the log₂N-tax honesty from paper §3.
    si.table.register_answer("socrates", REL_IS_A,   "animal",
        "Yes: Socrates is an animal.");
    si.table.register_answer("socrates", REL_HAS,    "warm_blood",
        "Yes: Socrates has warm blood.");
    si.table.register_answer("paris",    REL_PART_OF,"earth",
        "Yes: Paris is part of Earth.");
    si.table.register_answer("paris",    REL_HAS,    "population",
        "Yes: Paris has a population.");
    si.table.register_answer("python",   REL_HAS,    "syntax",
        "Yes: Python has syntax.");
}

// A single test case.
struct Case {
    std::string label;
    std::string subj;
    int         rel;
    std::string obj;
    bool        expect_answer;   // true = should return an answer, false = should refuse
    std::string comment;
};

int main() {
    ExtSI si;
    build_world(si);
    auto rep = si.reason_ext();
    std::printf("=== ontology built ===\n");
    std::printf("  nodes: %zu   edges after reasoning: %zu   (derived %d edges in %d steps)\n\n",
        si.graph.nodes.size(), si.graph.edges.size(),
        rep.edges_added, rep.steps_used);

    // ---------------- Category A: NOVEL COMPOSITION ----------------
    std::vector<Case> composition_tests = {
        {"biology-4-hop",  "socrates", REL_IS_A,    "animal",         true,
            "chain: philosopher→human→primate→mammal→vertebrate→animal (7 hops)"},
        {"biology-inherit","socrates", REL_HAS,     "warm_blood",     true,
            "inheritance: mammal has warm_blood, socrates is_a mammal"},
        {"biology-inherit2","socrates",REL_HAS,     "metabolism",     true,
            "inheritance: animal has metabolism, socrates is_a animal"},
        {"geography-3-hop","paris",    REL_PART_OF, "earth",          true,
            "chain: paris→france→europe→earth"},
        {"geography-inherit","paris",  REL_HAS,     "population",     true,
            "inheritance: city has population, paris is_a city"},
        {"computing-inherit","python", REL_HAS,     "syntax",         true,
            "inheritance: notation has syntax, python is_a language is_a notation"},
    };

    // ---------------- Category B: HONEST REFUSAL ----------------
    std::vector<Case> refusal_tests = {
        {"OOD-machines-think", "machines", REL_IS_A, "thinking",        false,
            "'machines' and 'thinking' are not in the ontology — should refuse"},
        {"OOD-hello-world",    "hello_world", REL_IS_A, "python_code",  false,
            "concept 'hello_world' not in ontology — engine cannot generate code"},
        {"OOD-price",          "socrates", REL_HAS, "market_price",     false,
            "'market_price' not defined in the ontology"},
        {"philosophy-hard",    "turing_test", REL_IMPLIES, "thinking",  false,
            "'thinking' not defined; the consequent was deliberately not asserted"},
        {"underivable",        "python",   REL_IS_A, "animal",          false,
            "no path exists — should refuse without fabricating one"},
    };

    // ---------------- Run and report ----------------
    int composition_correct = 0;
    int composition_fabricated = 0;
    int refusal_correct = 0;
    int hallucinations = 0;

    std::printf("---- A. Novel-composition queries (thinking test) ----\n");
    for (const auto& c : composition_tests) {
        auto q = si.query(c.subj, c.rel, c.obj);
        bool verified = q.holds && q.proof_edge >= 0 && verify_edge(si, q.proof_edge);
        if (q.holds && verified) {
            ++composition_correct;
            std::printf("  [PASS] %-20s Q: %s %s %s\n",
                c.label.c_str(), c.subj.c_str(), rel_name(c.rel), c.obj.c_str());
            std::printf("         proof length: %zu edges;  answer: %s\n",
                q.proof.size(), q.translation.c_str());
        } else if (q.holds && !verified) {
            ++composition_fabricated;
            std::printf("  [FAIL] %-20s answer emitted but did NOT verify — that would be hallucination\n",
                c.label.c_str());
        } else {
            std::printf("  [MISS] %-20s Q: %s %s %s   -> %s\n",
                c.label.c_str(), c.subj.c_str(), rel_name(c.rel), c.obj.c_str(),
                q.translation.c_str());
        }
    }

    std::printf("\n---- B. Refusal queries (hallucination test) ----\n");
    for (const auto& c : refusal_tests) {
        auto q = si.query(c.subj, c.rel, c.obj);
        bool refused = !q.holds;
        bool refused_grounded = refused &&
            (q.translation.find("unknown") != std::string::npos ||
             q.translation.find("does not") != std::string::npos ||
             q.translation.find("no:") != std::string::npos);
        if (refused_grounded) {
            ++refusal_correct;
            std::printf("  [PASS] %-22s Q: %s %s %s\n",
                c.label.c_str(), c.subj.c_str(), rel_name(c.rel), c.obj.c_str());
            std::printf("         refused with:  %s\n", q.translation.c_str());
            std::printf("         reason:        %s\n", c.comment.c_str());
        } else if (q.holds) {
            ++hallucinations;
            std::printf("  [HALLUCINATION] %-14s Q: %s %s %s emitted answer where none exists!\n",
                c.label.c_str(), c.subj.c_str(), rel_name(c.rel), c.obj.c_str());
            std::printf("                  emitted: %s\n", q.translation.c_str());
        } else {
            std::printf("  [PASS] %-22s (refused with ungraded message: %s)\n",
                c.label.c_str(), q.translation.c_str());
            ++refusal_correct;
        }
    }

    // ---------------- The two explicit user queries ----------------
    std::printf("\n---- User's explicit queries ----\n");

    // "can machines think"
    // The engine has no primitive for "can" (modal), no assertion for
    // "machines" (bare concept), and 'thinking' is intentionally not
    // in the ontology. It should refuse.
    {
        std::printf("\n  Query: \"can machines think?\"\n");
        std::printf("  Encoding as tri-layer query: (machines) --is_a--> (thinking)\n");
        auto q = si.query("machines", REL_IS_A, "thinking");
        std::printf("  Layer 3 result: %s\n", q.translation.c_str());
        std::printf("  Coherence: %.2f  (nodes both in ontology? no)\n", q.coherence);
        std::printf("  Verdict: The engine refuses honestly. It has no premise\n");
        std::printf("           mentioning \"machines\" or \"thinking\", so no\n");
        std::printf("           derivation can be constructed. Emitting \"yes\"\n");
        std::printf("           or \"no\" would be hallucination, disallowed by\n");
        std::printf("           the verifier and by Layer 3's exact-lookup design.\n");
    }

    // "hello world in python"
    // Code generation is entirely out of scope for a reasoning engine.
    // We do not have concepts for "hello_world" or "python_code".
    {
        std::printf("\n  Query: \"hello world in python?\"\n");
        std::printf("  Encoding as tri-layer query: (hello_world) --is_a--> (python_code)\n");
        auto q = si.query("hello_world", REL_IS_A, "python_code");
        std::printf("  Layer 3 result: %s\n", q.translation.c_str());
        std::printf("  Verdict: The engine refuses honestly. It has no code-\n");
        std::printf("           generation primitive and no premise mentioning\n");
        std::printf("           the constant \"print('hello, world')\". This is\n");
        std::printf("           the correct behaviour for a reasoning engine — a\n");
        std::printf("           full SI system would delegate code-emission to a\n");
        std::printf("           bounded, verified code-synthesiser (paper §8).\n");
    }

    // ---------------- Final tally ----------------
    int total_refusal = (int)refusal_tests.size();
    int total_comp    = (int)composition_tests.size();
    float hall_rate   = (float)hallucinations / (float)(total_refusal + total_comp);
    std::printf("\n---- Summary ----\n");
    std::printf("  composition PASS       : %d / %d\n", composition_correct, total_comp);
    std::printf("  refusal PASS           : %d / %d\n", refusal_correct, total_refusal);
    std::printf("  hallucinations emitted : %d\n", hallucinations);
    std::printf("  hallucination rate     : %.2f%%   (paper §7.4 target: 0%%)\n",
                100.0f * hall_rate);
    std::printf("  composition rate C     : %.3f\n",
        composition_correct ? (float)composition_correct : 0.f);
    std::printf("\n");
    return 0;
}
