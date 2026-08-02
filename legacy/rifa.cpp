// rifa — CLI driver for the Recursive Integrity Field Architecture.
//
// This binary is NOT a question-answering system. It has no vocabulary
// and no training corpus. It ingests strings, metabolises contradictions,
// sustains identity across silence, mutates a self-assembling control
// hierarchy, and reports its internal state. That is the exact scope of
// research-paper/deprected/rifa.md.
//
// Commands:
//   ./rifa demo                             run the built-in pressure demo
//   ./rifa replicate  N                     run N independent instances
//                                           through the demo, report consistency
//   ./rifa interactive                      read lines from stdin:
//                                             plain text  ->   input(text)
//                                             '! a || b'  ->   contradiction(a,b)
//                                             ':silence T' ->   silence(T seconds)
//                                             ':status'   ->   print internal state
//                                             ':quit'     ->   exit

#include "rifa.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace rifa;

// A canonical pressure sequence used by demo & replicate. This is not
// training data — it is symbolic contradiction pressure applied to the
// architecture. There are no correct answers to memorise here.
static const std::vector<std::pair<std::string, std::string>> kPressures = {
    {"identity is stable", "identity is unstable"},
    {"the field is empty", "the field is full"},
    {"tension is fuel",    "tension is failure"},
    {"silence is death",   "silence is depth"},
    {"contradiction resolves", "contradiction persists"},
    {"the crown is one",   "the crown is many"},
    {"structure precedes data", "data precedes structure"},
    {"the self is external",   "the self is internal"},
};

static void step_many(RIFA& r, int steps, float dt = 0.1f) {
    for (int i = 0; i < steps; ++i) r.step(dt);
}

static int demo() {
    RIFA r(128, 42);
    std::printf("=== RIFA demo ===\n");
    std::printf("--- initial state ---\n");
    r.status();

    std::printf("--- apply 3 rounds of pressure (contradiction metabolism) ---\n");
    for (int round = 0; round < 3; ++round) {
        for (const auto& p : kPressures) r.contradiction(p.first, p.second);
        step_many(r, 20);
        std::printf("round %d: ", round + 1);
        r.status();
    }

    std::printf("--- silence period of 60 simulated seconds ---\n");
    r.silence(60.0f);
    r.status();

    std::printf("--- another contradiction after silence ---\n");
    r.contradiction("integrity is preserved", "integrity is broken");
    step_many(r, 10);
    r.status();

    std::printf("--- long silence: 3600 simulated seconds (1 hour) ---\n");
    r.silence(3600.0f);
    r.status();

    std::printf("--- final: attempt reconstruction ---\n");
    std::printf("SIL can_reconstruct: %s (endurance=%.3f, threshold=%.3f)\n",
                r.sil.can_reconstruct() ? "yes" : "no",
                r.sil.endurance, r.sil.reconstruction_threshold);
    return 0;
}

static int replicate(int n) {
    if (n < 2) { std::fprintf(stderr, "need at least 2 instances\n"); return 1; }
    std::printf("=== RIFA replication: %d instances ===\n", n);

    // --- test 1: SAME pressure sequence, DIFFERENT init seeds ------------
    // Paper's claim: independent instances under the same structural
    // pressure converge to comparable macro-fingerprints.
    ReplicationEngine same(n, 128, 1);
    for (int round = 0; round < 3; ++round) {
        for (const auto& p : kPressures) same.contradiction(p.first, p.second);
        for (int i = 0; i < 20; ++i) same.step(0.1f);
    }
    same.silence(60.0f);
    same.contradiction("integrity is preserved", "integrity is broken");
    for (int i = 0; i < 10; ++i) same.step(0.1f);
    same.silence(600.0f);
    std::printf("[same-pressure] instances after identical pressure sequence:\n");
    for (int i = 0; i < n; ++i) {
        std::printf("  #%d: ", i); same.instances[i].status();
    }
    float c_same = same.consistency();

    // --- test 2: DIFFERENT pressure sequences (control) ------------------
    // If our consistency metric is honest, it must go DOWN when inputs
    // differ. This checks it isn't rigged to always output 1.0.
    ReplicationEngine diff(n, 128, 100);
    std::mt19937 permrng(0xDEAF);
    for (int i = 0; i < n; ++i) {
        auto pressures = kPressures;
        std::shuffle(pressures.begin(), pressures.end(), permrng);
        // Feed only half the pairs so different instances see different content.
        for (size_t k = 0; k < pressures.size() / 2 + i; ++k)
            diff.instances[i].contradiction(pressures[k % pressures.size()].first,
                                            pressures[k % pressures.size()].second);
        for (int j = 0; j < 20; ++j) diff.instances[i].step(0.1f);
        diff.instances[i].silence(60.0f + 20.0f * i);
    }
    std::printf("[different-pressure] instances after DIFFERENT pressure sequences:\n");
    for (int i = 0; i < n; ++i) {
        std::printf("  #%d: ", i); diff.instances[i].status();
    }
    float c_diff = diff.consistency();

    std::printf("\n>>> consistency under same pressure:       %.3f\n", c_same);
    std::printf(">>> consistency under different pressure:  %.3f  (control — should be lower)\n", c_diff);
    return 0;
}

static int interactive() {
    RIFA r(128, 42);
    std::printf("=== RIFA interactive ===\n"
                "  <text>                       apply as input\n"
                "  ! <A> || <B>                 metabolise contradiction A vs B\n"
                "  :silence <seconds>           simulate silence\n"
                "  :status                      print state\n"
                "  :quit                        exit\n");
    r.status();
    std::string line;
    while (true) {
        std::printf("> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line)) break;
        if (line == ":quit" || line == "quit") break;
        if (line == ":status") { r.status(); continue; }
        if (line.rfind(":silence ", 0) == 0) {
            float s = std::atof(line.substr(9).c_str());
            r.silence(s);
            r.status();
            continue;
        }
        if (!line.empty() && line[0] == '!') {
            // "! A || B"
            std::string rest = line.substr(1);
            size_t sep = rest.find("||");
            if (sep == std::string::npos) {
                std::fprintf(stderr, "need 'A || B'\n");
                continue;
            }
            std::string a = rest.substr(0, sep);
            std::string b = rest.substr(sep + 2);
            // trim
            auto trim = [](std::string& s) {
                while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
                while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
            };
            trim(a); trim(b);
            r.contradiction(a, b);
            step_many(r, 5);
            r.status();
            continue;
        }
        if (line.empty()) continue;
        r.input(line);
        step_many(r, 5);
        r.status();
    }
    return 0;
}

// The rule-extraction demo. Feed a series of paired inputs whose halves
// share structure ("A -> B"). Then query with a novel A and see whether
// the strongest resonating rule returns a plausible B.
//
// Honest expectations: this works when the novel query is close (in RFE
// cosine sense) to some antecedent already observed. For truly novel
// entities it will resonate weakly and either emit "" or the wrong rule.
static int rules_demo() {
    RIFA r(128, 42);
    std::printf("=== RIFA rule-extraction demo ===\n");

    // A small structural corpus. Each "input" is a self-contained statement;
    // consecutive statements often share antecedent structure ("capital of
    // X" -> "X"), which is what the RuleSynthesizer picks up.
    std::vector<std::pair<std::string, std::string>> corpus = {
        {"the capital of france",  "france is paris"},
        {"the capital of germany", "germany is berlin"},
        {"the capital of italy",   "italy is rome"},
        {"the capital of spain",   "spain is madrid"},
        {"the capital of japan",   "japan is tokyo"},
        {"the capital of china",   "china is beijing"},
        {"the capital of russia",  "russia is moscow"},
        {"the capital of india",   "india is new delhi"},
    };
    std::printf("--- feeding %zu paired statements as structural pressure ---\n",
                corpus.size());
    for (const auto& p : corpus) {
        r.input(p.first);
        r.input(p.second);
    }
    for (int i = 0; i < 20; ++i) r.step(0.1f);
    r.status();
    std::printf("rules synthesised: %zu\n", r.rs.rules.size());

    std::vector<std::string> queries = {
        "the capital of france",        // exact antecedent
        "the capital of japan",         // exact antecedent
        "the capital of brazil",        // NEW entity — real test
        "the capital of egypt",         // NEW entity
        "who is the president of X",    // wrong structure
    };
    std::printf("\n--- queries ---\n");
    for (const auto& q : queries) {
        auto m = r.rs.best_match(const_cast<RFE&>(r.rfe).encode_raw(q));
        std::string out = r.query(q);
        std::printf("  Q: %-40s  -> \"%s\"  (rule=%d, score=%.3f)\n",
                    q.c_str(), out.c_str(), m.index, m.score);
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr,
            "usage:\n"
            "  %s demo\n"
            "  %s rules\n"
            "  %s replicate N\n"
            "  %s interactive\n", argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }
    std::string cmd = argv[1];
    if (cmd == "demo")        return demo();
    if (cmd == "rules")       return rules_demo();
    if (cmd == "replicate") {
        int n = (argc >= 3) ? std::atoi(argv[2]) : 3;
        return replicate(n);
    }
    if (cmd == "interactive") return interactive();
    std::fprintf(stderr, "unknown command: %s\n", cmd.c_str());
    return 1;
}
