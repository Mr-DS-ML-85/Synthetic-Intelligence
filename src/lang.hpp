// lang.hpp — Language Governance Layer (isolated syntax clusters).
//
// THE ARGUMENT
//
// Cognition and language are different kinds of thing, and conflating
// them was a category error in this project's earlier emission work.
//
//   Cognition  discovers. It needs novelty, experiment, resonance,
//              thermodynamic exploration, oracle validation. Binding it
//              with fixed rules would cripple it.
//
//   Language   does NOT discover. Its job is to transmit a cognitive
//              state from one medium to another under an agreed
//              protocol. English SVO is not a physical law awaiting
//              discovery by resonance — it is a human convention.
//              Bengali SOV encodes the same content under a different
//              convention.
//
// So the substrate splits:
//
//   [ Core SI Physics Substrate ]  -> raw resonant concept triples
//                  |
//   [ Isolated Language Cluster ]  -> deterministic surface realization
//                  |
//            human-readable stream
//
// The core is untouched. Not one grammar rule enters the physics, so
// cognitive flexibility and novelty are unaffected. The language
// cluster is a pure constraint-satisfaction realizer: given a triple
// and a target language, it produces exactly one string.
//
// COST ACCOUNTING (Translation Capacity Lemma, SI paper Sec 3.1)
//
// This adds no hidden cost. Layer 3 already stores label strings —
// the lemma forces it. A word-order constant is ~2 bits; a relation
// lexicalisation table is one row per relation per language; a
// phonetic map is one row per word pair. All are Layer-3 protocol
// cost, explicitly named, not intelligence.
//
// WHAT THIS IS NOT
//
// No model, no probabilities, no next-token distribution, no training.
// Realization is a deterministic walk over a rule table. The same
// cognitive state always produces the same surface string, which is
// why it cannot hallucinate a form.

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace unified {

// A cognitive triple as the core substrate emits it: raw concepts and
// the relation between them. No grammar, no morphology, no language.
struct ConceptTriple {
    std::string subject;
    std::string relation;   // REL_* name, e.g. "is_a", "causes"
    std::string object;
};

// Word order is the language's transmission protocol, not a discovered
// fact. SVO covers English; SOV covers Bengali, Japanese, Turkish.
enum class WordOrder { SVO, SOV, VSO };

struct LanguageCluster {
    std::string  name;
    std::string  code;                 // "en", "bn", "bn-latin"
    WordOrder    order = WordOrder::SVO;

    // Relation -> surface form. This is the piece that makes output
    // read as language rather than as a triple dump: `part_of` must
    // surface as "is part of", not "part_of".
    std::map<std::string, std::string> lexicalize;

    // Script / phonetic mapping. For Banglish this maps the Bengali
    // form to its Latin transliteration; the relation is strict and
    // static, so `tumi` is ALWAYS `তুমি` — never a probable guess.
    std::map<std::string, std::string> transliterate;

    // Minimal morphology. Deterministic string operations, not
    // learned inflection.
    //
    // Article placement is NOT a global language property — that was
    // the first thing testing disproved, producing "is a the science".
    // Whether an object takes a determiner belongs to the CONSTRUCTION:
    // `is_a` already supplies its own article, and a proper noun takes
    // none at all. So the article is a per-relation flag, which is the
    // honest granularity. This is the general shape of the remaining
    // work: real grammar is more than word order, and each additional
    // rule is deterministic data rather than a learned parameter.
    bool  use_articles     = false;
    std::set<std::string> no_article_relations;   // construction supplies it
    std::string verb_3sg_suffix;
    std::string plural_suffix;

    // ---- realization -------------------------------------------------

    std::string surface_relation(const std::string& rel) const {
        auto it = lexicalize.find(rel);
        if (it != lexicalize.end()) return it->second;
        // Unknown relation: render readably rather than inventing a
        // form. Underscores become spaces; nothing is guessed.
        std::string s = rel;
        for (char& c : s) if (c == '_') c = ' ';
        return s;
    }

    std::string surface_concept(const std::string& c) const {
        auto it = transliterate.find(c);
        std::string s = (it != transliterate.end()) ? it->second : c;
        for (char& ch : s) if (ch == '_') ch = ' ';
        return s;
    }

    // Produce one sentence from one triple. Deterministic: the same
    // triple and cluster always yield byte-identical output.
    std::string realize(const ConceptTriple& t) const {
        std::string S = surface_concept(t.subject);
        std::string V = surface_relation(t.relation);
        std::string O = surface_concept(t.object);

        // Article only when the language uses them AND the
        // construction does not already supply one AND the object is
        // not a proper noun (capitalised in the source label).
        bool construction_supplies =
            no_article_relations.count(t.relation) > 0;
        bool proper = !t.object.empty() &&
                      std::isupper((unsigned char)t.object[0]);
        if (use_articles && !construction_supplies && !proper && !O.empty())
            O = "the " + O;

        std::string out;
        switch (order) {
            case WordOrder::SVO: out = S + " " + V + " " + O; break;
            case WordOrder::SOV: out = S + " " + O + " " + V; break;
            case WordOrder::VSO: out = V + " " + S + " " + O; break;
        }
        if (!out.empty()) out[0] = (char)std::toupper((unsigned char)out[0]);
        return out + terminator;
    }

    std::string terminator = ".";
};

// ---------------------------------------------------------- builders
//
// These tables are the entire "grammar". They are data, not code, and
// adding a language means adding a table — the core substrate is never
// touched. That is the multi-lingual swap property: one cognitive
// state, many protocols.

inline LanguageCluster make_english() {
    LanguageCluster L;
    L.name = "English";
    L.code = "en";
    L.order = WordOrder::SVO;
    L.use_articles = true;
    L.verb_3sg_suffix = "s";
    // These constructions carry their own determiner or take none.
    L.no_article_relations = {
        "is_a", "part_of", "inhibits", "activates", "treats", "causes",
        "calls", "precedes", "imports", "mutates", "returns", "implies",
        "equals", "takes_arg",
        // Commonsense constructions (ConceptNet-converted). Each already
        // carries its own preposition, so an inserted determiner would
        // produce "*is used for the nail" where "nails" is meant.
        "used_for", "capable_of", "at_location", "desires", "created_by"
    };
    L.lexicalize = {
        {"is_a",        "is a"},
        {"has",         "has"},
        {"part_of",     "is part of"},
        {"causes",      "causes"},
        {"implies",     "implies"},
        {"equals",      "equals"},
        {"treats",      "treats"},
        {"inhibits",    "inhibits"},
        {"activates",   "activates"},
        {"calls",       "calls"},
        {"returns",     "returns"},
        {"takes_arg",   "takes argument"},
        {"precedes",    "must run before"},
        {"imports",     "imports"},
        {"mutates",     "modifies"},
        // Commonsense relations. Closing the §9.2 coverage cliff for the
        // relations added when ConceptNet was converted: without these
        // rows realise() falls back to the raw identifier.
        {"used_for",    "is used for"},
        {"capable_of",  "can"},
        {"at_location", "is found at"},
        {"desires",     "wants"},
        {"created_by",  "is created by"},
    };
    return L;
}

inline LanguageCluster make_bengali() {
    LanguageCluster L;
    L.name = "Bengali";
    L.code = "bn";
    L.order = WordOrder::SOV;      // subject - object - verb
    L.use_articles = false;        // Bengali has no articles
    L.terminator = "।";            // danda, not full stop
    L.lexicalize = {
        {"is_a",      "একটি"},
        {"has",       "আছে"},
        {"part_of",   "এর অংশ"},
        {"causes",    "ঘটায়"},
        {"implies",   "বোঝায়"},
        {"treats",    "চিকিৎসা করে"},
        {"inhibits",  "বাধা দেয়"},
        {"activates", "সক্রিয় করে"},
        {"calls",     "ডাকে"},
        {"precedes",  "আগে চলে"},
        // Commonsense relations (see the English cluster).
        {"used_for",    "জন্য ব্যবহৃত হয়"},
        {"capable_of",  "পারে"},
        {"at_location", "এ থাকে"},
        {"desires",     "চায়"},
        {"created_by",  "দ্বারা তৈরি"},
    };
    return L;
}

// Banglish: Bengali syntax and lexicon, Latin script. The
// transliteration relation is STRICT and STATIC — `tumi` is always
// `তুমি`. No probability is involved, so the surface form cannot drift.
inline LanguageCluster make_banglish() {
    LanguageCluster L = make_bengali();
    L.name = "Banglish";
    L.code = "bn-latin";
    L.terminator = ".";
    L.lexicalize = {
        {"is_a",      "ekta"},
        {"has",       "ache"},
        {"part_of",   "er ongsho"},
        {"causes",    "ghotay"},
        {"implies",   "bojhay"},
        {"treats",    "chikitsa kore"},
        {"inhibits",  "badha dey"},
        {"activates", "sokriyo kore"},
    };
    L.lexicalize["precedes"] = "age chole";
    L.lexicalize["calls"]    = "dake";
    // Commonsense relations, transliterated from the Bengali cluster.
    L.lexicalize["used_for"]    = "jonno byabohrito hoy";
    L.lexicalize["capable_of"]  = "pare";
    L.lexicalize["at_location"] = "e thake";
    L.lexicalize["desires"]     = "chay";
    L.lexicalize["created_by"]  = "dara toiri";
    L.transliterate = {
        {"তুমি", "tumi"}, {"কেমন", "kemon"}, {"আছো", "aso"},
        {"আমি",  "ami"},  {"ভালো",  "bhalo"}, {"জীববিজ্ঞান", "jibobiggan"},
    };
    return L;
}

}  // namespace unified
