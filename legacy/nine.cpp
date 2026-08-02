// nine — the 9-primitive validation demo (paper §4.4 + §5.3.1).
//
// Each scenario uses a distinct primitive that the base SI engine cannot
// exercise. Together they close the composition space of the paper's
// nine-primitive framework. Recursion (primitive 9) is implicit in the
// fixpoint loop of reason_ext(); rule induction (macro-rule discovery)
// via the MetaLearner is the operational analogue.

#include "meta_learner.hpp"
#include "primitives.hpp"

#include <cstdio>
#include <string>

using namespace unified;

// ---------- 1. TRANSITIVE composition (baseline) ----------
static void demo_transitive() {
    ExtSI si;
    si.add_transitive("t_is_a", REL_IS_A, REL_IS_A, REL_IS_A);
    si.assert_fact("dog", REL_IS_A, "mammal");
    si.assert_fact("mammal", REL_IS_A, "vertebrate");
    si.assert_fact("vertebrate", REL_IS_A, "animal");
    auto rep = si.reason_ext();
    std::printf("  [transitive]      dog is_a animal?  ");
    auto q = si.query("dog", REL_IS_A, "animal");
    std::printf("%s   (%d edges derived)\n", q.holds ? "YES" : "no", rep.edges_added);
}

// ---------- 2. DEDUCTION (modus ponens on IMPLIES) ----------
static void demo_deduction() {
    ExtSI si;
    si.add_deduction("modus_ponens");
    si.assert_fact("raining", REL_IMPLIES, "wet_ground");
    si.assert_fact("wet_ground", REL_IMPLIES, "slippery");
    si.assert_fact("slippery", REL_IMPLIES, "risk");
    si.reason_ext();
    std::printf("  [deduction]       raining implies risk?  ");
    auto q = si.query("raining", REL_IMPLIES, "risk");
    std::printf("%s\n", q.holds ? "YES" : "no");
}

// ---------- 3. INHERITANCE (class properties flow to members) ----------
static void demo_inheritance() {
    ExtSI si;
    // (X is_a C) + (C has Y) => (X has Y)
    si.add_inheritance("has_flows_down", REL_HAS);
    si.assert_fact("socrates", REL_IS_A, "man");
    si.assert_fact("man", REL_HAS, "mortality");
    si.assert_fact("man", REL_HAS, "language");
    si.reason_ext();
    std::printf("  [inheritance]     socrates has mortality?  ");
    auto q1 = si.query("socrates", REL_HAS, "mortality");
    std::printf("%s\n", q1.holds ? "YES" : "no");
    std::printf("  [inheritance]     socrates has language?    ");
    auto q2 = si.query("socrates", REL_HAS, "language");
    std::printf("%s\n", q2.holds ? "YES" : "no");
}

// ---------- 4. NEGATION (contrapositive) ----------
static void demo_negation() {
    ExtSI si;
    si.add_negation("contrapositive", REL_IMPLIES);
    // Rule: (has_gills) implies (aquatic)
    si.assert_fact("has_gills", REL_IMPLIES, "aquatic");
    // We observe: (socrates NOT aquatic)  (via a NOT edge)
    si.assert_fact("socrates", REL_NOT, "aquatic");
    si.reason_ext();
    std::printf("  [negation]        socrates NOT has_gills?   ");
    auto q = si.query("socrates", REL_NOT, "has_gills");
    std::printf("%s\n", q.holds ? "YES (via contrapositive)" : "no");
}

// ---------- 5. CASE ANALYSIS (disjunction elimination) ----------
static void demo_case_analysis() {
    ExtSI si;
    si.add_case_analysis("or_elim", REL_IMPLIES);
    // "the switch is (up OR down)"; each branch implies "circuit_actionable"
    si.assert_fact("switch_state", REL_OR, "up");
    si.assert_fact("switch_state", REL_OR, "down");
    si.assert_fact("up", REL_IMPLIES, "circuit_actionable");
    si.assert_fact("down", REL_IMPLIES, "circuit_actionable");
    si.reason_ext();
    std::printf("  [case-analysis]   switch_state => actionable?  ");
    auto q = si.query("switch_state", REL_IMPLIES, "circuit_actionable");
    std::printf("%s\n", q.holds ? "YES (both disjuncts imply it)" : "no");
}

// ---------- 6. INDUCTION (schematic generalization) ----------
static void demo_induction() {
    ExtSI si;
    // If ≥3 members of a class share a property, the class has it.
    si.add_induction("class_generalize", REL_HAS, /*witnesses=*/3);
    si.assert_fact("alice",   REL_IS_A, "student");
    si.assert_fact("bob",     REL_IS_A, "student");
    si.assert_fact("carol",   REL_IS_A, "student");
    si.assert_fact("dan",     REL_IS_A, "student");
    si.assert_fact("alice",   REL_HAS,  "textbook");
    si.assert_fact("bob",     REL_HAS,  "textbook");
    si.assert_fact("carol",   REL_HAS,  "textbook");
    si.assert_fact("dan",     REL_HAS,  "textbook");
    // The generalization "student has textbook" was never asserted.
    si.reason_ext();
    std::printf("  [induction]       student has textbook?      ");
    auto q = si.query("student", REL_HAS, "textbook");
    std::printf("%s   (%zu witnesses)\n", q.holds ? "YES" : "no", (size_t)4);
}

// ---------- 7. ANALOGY (structure-preserving edge transfer) ----------
static void demo_analogy() {
    ExtSI si;
    // The analog mapping: earth <-> mars  (via REL_IS_A as the mapping)
    si.add_analogy("planet_analogy", REL_IS_A);
    // Concrete relations of earth in the source domain
    si.assert_fact("earth", REL_IS_A, "planet_mapped_to_mars");
    si.assert_fact("earth", REL_HAS, "atmosphere");
    si.assert_fact("earth", REL_HAS, "core");
    si.assert_fact("earth", REL_PART_OF, "solar_system");
    // Set up the mapping in the target: mars = planet_mapped_to_mars.
    // The engine will transfer earth's relations onto mars.
    si.reason_ext();
    // Because we mapped (earth -- IS_A --> planet_mapped_to_mars) as the
    // mapping edge, analogy fires (X = earth, X' = planet_mapped_to_mars)
    // and transfers earth's outgoing edges to planet_mapped_to_mars.
    std::printf("  [analogy]         planet_mapped_to_mars part_of solar_system?  ");
    auto q = si.query("planet_mapped_to_mars", REL_PART_OF, "solar_system");
    std::printf("%s\n", q.holds ? "YES (structure transferred)" : "no");
}

// ---------- 8. RECURSION (implicit fixpoint) ----------
static void demo_recursion() {
    ExtSI si;
    // Same rule as (1) but with a deep chain — recursion is the fact
    // that reason_ext() iterates until fixpoint, so a chain of any depth
    // is closed under repeated rule application. Raise the tension-bounded
    // budget so the fixpoint can actually be reached (Axiom II: growth is
    // tension-permitted, not tension-free).
    si.base_budget = 5000.0f;
    si.max_steps = 64;
    si.add_transitive("t_is_a", REL_IS_A, REL_IS_A, REL_IS_A);
    std::vector<std::string> chain = {
        "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p"};
    for (size_t i = 0; i + 1 < chain.size(); ++i)
        si.assert_fact(chain[i], REL_IS_A, chain[i+1]);
    auto rep = si.reason_ext();
    auto q = si.query("a", REL_IS_A, "p");
    std::printf("  [recursion]       a is_a p (15-hop chain)?  %s  (%d steps, %d edges)\n",
                q.holds ? "YES" : "no", rep.steps_used, rep.edges_added);
}

// ---------- 9. RULE INDUCTION (macro discovery — the meta primitive) ----------
static void demo_rule_induction() {
    ExtSI si;
    si.add_transitive("t_is_a", REL_IS_A, REL_IS_A, REL_IS_A);
    si.add_inheritance("has_flows", REL_HAS);
    // A small ontology used repeatedly
    si.assert_fact("socrates", REL_IS_A, "man");
    si.assert_fact("plato",    REL_IS_A, "man");
    si.assert_fact("man",      REL_IS_A, "mortal");
    si.assert_fact("mortal",   REL_HAS,  "death");
    MetaLearner meta; meta.engine = &si;
    ThinkerIndex idx;
    std::vector<std::string> subs = {"socrates","plato"};
    std::vector<std::pair<int,std::string>> targets = {
        {REL_IS_A,"mortal"}, {REL_HAS,"death"}
    };
    for (int round = 0; round < 6; ++round) {
        si.reason_ext();
        for (const auto& s : subs) for (const auto& t : targets) {
            auto r = si.query(s, t.first, t.second);
            idx.record(r);
            if (r.holds) meta.observe_trace(r.proof);
        }
    }
    std::printf("  [rule-induction]  %zu macro-rules discovered from %d queries\n",
                meta.macros.size(), idx.total_queries);
    idx.report(stdout);
}

int main(int argc, char** argv) {
    std::string cmd = (argc >= 2) ? argv[1] : "all";
    std::printf("=== SI: the 9 reasoning primitives ===\n\n");
    if (cmd == "all") {
        demo_transitive();
        demo_deduction();
        demo_inheritance();
        demo_negation();
        demo_case_analysis();
        demo_induction();
        demo_analogy();
        demo_recursion();
        demo_rule_induction();
    }
    return 0;
}
