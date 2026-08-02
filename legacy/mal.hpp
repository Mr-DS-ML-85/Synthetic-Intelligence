// mal.hpp — Morphological Abduction Layer.
//
// Fixes the "unknown → refuse" failure mode of the SI graph engine
// without introducing tokens, next-token prediction, or vocabulary
// softmax. When the SI engine encounters an unknown token, MAL:
//
//   1. DECOMPOSES the token into candidate substring splits and into
//      per-letter initials. These are structural operations on
//      characters, not statistical predictions.
//   2. MATCHES each substring / letter against the concepts already in
//      the ontology (via cheap character-prefix and exact matches).
//   3. HYPOTHESISES expansions by composing matched substring meanings.
//      A curated abbreviation dictionary provides a small seed of tech /
//      science / everyday initialisms so that "RDMA" → "Remote Direct
//      Memory Access" and "sudo" → "super user do" are derivable.
//   4. SCORES hypotheses by structural consistency with the graph
//      context (frequency of matched components in the current graph
//      community, and letter-alignment quality).
//
// Every step is a discrete rule application over characters and known
// concepts. Nothing is sampled. Nothing is predicted. The output is a
// ranked list of abductive hypotheses with confidence scores that a
// downstream discourse assembler can turn into prose.

#pragma once

#include "unified.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace unified {

struct MalHypothesis {
    std::string token;             // the unknown token
    std::string expansion;         // proposed expansion, e.g. "super user do"
    std::vector<std::string> parts; // constituent parts (matched substrings or expansions)
    std::vector<std::string> evidence; // graph-side matches supporting each part
    float confidence = 0.0f;
    std::string reasoning;         // short trace of how the hypothesis was built
};

// Greek and Latin morphemes — the roots and suffixes real English words
// are actually built from. Used to decompose real words like "biology"
// (bio + logy = life + study-of) instead of forcing the token into an
// initialism-shaped hole. Hand-authored; extensible.
inline const std::unordered_map<std::string, std::string>&
morpheme_dict() {
    static const std::unordered_map<std::string, std::string> M = {
        // Greek prefixes / stems
        {"bio",    "life"},           {"geo",    "earth"},
        {"astro",  "star"},           {"chrono", "time"},
        {"psycho", "mind"},           {"neuro",  "nerve"},
        {"photo",  "light"},          {"hydro",  "water"},
        {"phil",   "love"},           {"philo",  "love"},
        {"tele",   "distant"},        {"micro",  "small"},
        {"macro",  "large"},          {"proto",  "first"},
        {"anti",   "against"},        {"auto",   "self"},
        {"hyper",  "over"},           {"hypo",   "under"},
        {"meta",   "beyond"},         {"epi",    "upon"},
        {"peri",   "around"},         {"syn",    "with"},
        {"para",   "beside"},         {"eco",    "environment"},
        {"soci",   "society"},        {"cosmo",  "universe"},
        {"anthropo","human"},         {"theo",   "god"},
        {"crypto", "hidden"},         {"morph",  "form"},
        {"phon",   "sound"},          {"graph",  "written"},
        // Greek suffixes
        {"logy",   "study of"},       {"ology",  "study of"},
        {"logist", "student of"},     {"graphy", "writing about"},
        {"metry",  "measurement of"}, {"scopy",  "viewing of"},
        {"pathy",  "suffering of"},   {"phobia", "fear of"},
        {"philia", "love of"},        {"cracy",  "rule by"},
        {"archy",  "rule of"},        {"nomy",   "law of"},
        {"sophy",  "wisdom of"},      {"istry",  "art or study of"},
        {"ist",    "practitioner of"},{"ism",    "doctrine of"},
        {"osis",   "state of"},       {"itis",   "inflammation of"},
        // Latin
        {"aqua",   "water"},          {"terra",  "earth"},
        {"vita",   "life"},           {"omni",   "all"},
        {"multi",  "many"},           {"uni",    "one"},
        {"bi",     "two"},            {"tri",    "three"},
        {"quad",   "four"},           {"penta",  "five"},
        {"deca",   "ten"},            {"cent",   "hundred"},
        {"mille",  "thousand"},       {"kilo",   "thousand"},
        {"post",   "after"},          {"pre",    "before"},
        {"trans",  "across"},         {"inter",  "between"},
        {"intra",  "within"},         {"extra",  "outside"},
        {"super",  "above"},          {"sub",    "below"},
        {"re",     "again"},          {"de",     "away"},
        // Domain stems
        {"chem",   "chemical"},       {"physic", "physical"},
        {"mathe",  "mathematical"},   {"astro",  "star"},
    };
    return M;
}

// A very small curated dictionary of high-signal initialisms and
// morphemes. Not learned — hand-authored, honest, extensible. This is
// the "seed knowledge" of the layer, the same way our 9 primitives are
// the seed knowledge of the reasoner.
inline const std::unordered_map<std::string, std::vector<std::string>>&
initialism_seed() {
    static const std::unordered_map<std::string, std::vector<std::string>> D = {
        // Common Unix / networking
        {"su",   {"super_user","substitute_user"}},
        {"do",   {"execute","perform","run"}},
        {"sudo", {"super_user do (execute as super user)"}},
        {"cd",   {"change_directory"}},
        {"ls",   {"list_segments (list directory contents)"}},
        {"cp",   {"copy"}},
        {"mv",   {"move"}},
        {"rm",   {"remove"}},
        {"pwd",  {"present_working_directory"}},
        // Networking / systems
        {"rdma", {"remote_direct_memory_access"}},
        {"tcp",  {"transmission_control_protocol"}},
        {"udp",  {"user_datagram_protocol"}},
        {"ip",   {"internet_protocol"}},
        {"dns",  {"domain_name_system"}},
        {"vpn",  {"virtual_private_network"}},
        {"api",  {"application_programming_interface"}},
        {"cpu",  {"central_processing_unit"}},
        {"gpu",  {"graphics_processing_unit"}},
        {"ram",  {"random_access_memory"}},
        {"ssd",  {"solid_state_drive"}},
        // Machine learning / AI
        {"llm",  {"large_language_model"}},
        {"si",   {"synthetic_intelligence","system_of_intelligence"}},
        {"ai",   {"artificial_intelligence"}},
        {"ml",   {"machine_learning"}},
        {"nlp",  {"natural_language_processing"}},
        {"dl",   {"deep_learning"}},
        {"cnn",  {"convolutional_neural_network"}},
        {"rnn",  {"recurrent_neural_network"}},
        // Physics / math
        {"qm",   {"quantum_mechanics"}},
        {"gr",   {"general_relativity"}},
        {"pde",  {"partial_differential_equation"}},
        {"ode",  {"ordinary_differential_equation"}},
        // Everyday
        {"eta",  {"estimated_time_of_arrival"}},
        {"faq",  {"frequently_asked_questions"}},
        {"asap", {"as_soon_as_possible"}},
    };
    return D;
}

// A domain hint chooses which category of expansions to prefer.
enum class Domain { None, Computing, Physics, Biology, Everyday };

// -----------------------------------------------------------------------
// MAL engine.
// -----------------------------------------------------------------------
struct MorphAbduction {
    const SI* si = nullptr;

    // Lower-case + strip non-alphanumerics — normalise a raw user token.
    static std::string normalise(const std::string& s) {
        std::string out;
        for (char c : s) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9')) {
                out.push_back((char)std::tolower((unsigned char)c));
            }
        }
        return out;
    }

    // Is this token in the ontology as a node label?
    bool token_known(const std::string& s) const {
        if (!si) return false;
        return si->graph.by_label.find(s) != si->graph.by_label.end();
    }

    // Structural decomposition: for a length-N token, produce all splits
    // into two nonempty substrings (N-1 candidates). Longer decompositions
    // are enumerated recursively up to `max_parts`.
    static std::vector<std::vector<std::string>>
    decompose(const std::string& s, int max_parts = 3) {
        std::vector<std::vector<std::string>> result;
        result.push_back({s});
        if ((int)s.size() < 2 || max_parts < 2) return result;
        for (size_t i = 1; i < s.size(); ++i) {
            std::string left  = s.substr(0, i);
            std::string right = s.substr(i);
            auto right_decomps = decompose(right, max_parts - 1);
            for (const auto& rd : right_decomps) {
                std::vector<std::string> combined;
                combined.push_back(left);
                for (const auto& p : rd) combined.push_back(p);
                result.push_back(std::move(combined));
            }
        }
        return result;
    }

    // Look up a substring in the initialism dictionary. Returns an empty
    // vector if nothing matches.
    static const std::vector<std::string>* dict_lookup(const std::string& s) {
        const auto& d = initialism_seed();
        auto it = d.find(s);
        return (it == d.end()) ? nullptr : &it->second;
    }

    // Try to match a substring against a graph node whose label starts
    // with or equals the substring. Returns matching labels.
    std::vector<std::string> graph_prefix_match(const std::string& s) const {
        std::vector<std::string> out;
        if (!si || s.empty()) return out;
        for (const auto& n : si->graph.nodes) {
            if (n.label.size() >= s.size() &&
                n.label.compare(0, s.size(), s) == 0) {
                out.push_back(n.label);
            }
        }
        return out;
    }

    // Score for whether a part is "understood": full dict expansion > 0.9,
    // graph prefix match > 0.6, letter-only guess (initials) > 0.3.
    struct PartMatch {
        std::string source_part;
        std::string chosen_meaning;
        float part_confidence = 0.0f;
        std::string source_kind;   // "dict" | "graph" | "initial"
    };

    PartMatch score_part(const std::string& part) const {
        PartMatch pm{part, part, 0.0f, "unknown"};
        // (a) Dictionary hit (highest signal).
        if (const auto* meanings = dict_lookup(part)) {
            pm.chosen_meaning = (*meanings).front();
            pm.part_confidence = 0.9f;
            pm.source_kind = "dict";
            return pm;
        }
        // (b) Graph prefix match — pick the shortest matching label.
        auto gm = graph_prefix_match(part);
        if (!gm.empty()) {
            std::sort(gm.begin(), gm.end(),
                      [](const std::string& a, const std::string& b){
                          return a.size() < b.size();
                      });
            pm.chosen_meaning = gm.front();
            pm.part_confidence = 0.6f;
            pm.source_kind = "graph";
            return pm;
        }
        // (c) Single-letter initial — cheap fallback (no meaning invented).
        if (part.size() == 1) {
            pm.chosen_meaning = "(letter " + part + ")";
            pm.part_confidence = 0.15f;
            pm.source_kind = "initial";
            return pm;
        }
        // (d) Unknown — meaning is the part itself.
        pm.chosen_meaning = part;
        pm.part_confidence = 0.05f;
        pm.source_kind = "unknown";
        return pm;
    }

    // Classify the token so we choose the right analysis strategy.
    // A real English word ("biology") deserves morphological analysis,
    // not initialism decomposition. A short acronym ("rdma") deserves
    // dictionary lookup + letter-part matching. Neologisms deserve an
    // honest "I do not know this word".
    enum class Kind { Known, Initialism, CompoundWord, Neologism };
    Kind classify(const std::string& tok) const {
        if (token_known(tok)) return Kind::Known;
        if (tok.size() <= 5 && dict_lookup(tok)) return Kind::Initialism;
        if (tok.size() <= 4) return Kind::Initialism;      // treat short as acronym-shaped
        return Kind::CompoundWord;                         // long token: try morphemes
    }

    // A Greek scientific suffix in the right-slot means the word is a
    // science term — e.g. "-logy", "-graphy". In that context the left
    // root's Greek sense ("bio" = life) must win over its Latin sense
    // ("bi" = two). Fix 1: Contextual Root Resolution.
    static bool is_scientific_suffix(const std::string& s) {
        return s == "logy" || s == "ology" || s == "graphy" ||
               s == "metry" || s == "sophy" || s == "istry" ||
               s == "scopy" || s == "nomy"  || s == "cracy" ||
               s == "archy" || s == "pathy" || s == "phobia";
    }

    // A Greek life-science root — prefer these when combined with a
    // scientific suffix. "bio" is Greek; "bi" is Latin (numeric).
    static bool is_greek_science_root(const std::string& s) {
        return s == "bio"    || s == "geo"    || s == "astro" ||
               s == "chrono" || s == "psycho" || s == "neuro" ||
               s == "photo"  || s == "hydro"  || s == "cosmo" ||
               s == "anthropo" || s == "theo" || s == "chem"  ||
               s == "physic"|| s == "mathe" || s == "soci"    ||
               s == "eco"    || s == "morph" || s == "phon"   ||
               s == "graph"  || s == "crypto";
    }

    // Morphological decomposition of a real word: try every split
    // point; if BOTH halves are known morphemes, that's a strong
    // hypothesis. If only one half matches, weaker. If neither, skip.
    //
    // Fix 1 (Contextual Root Resolution):
    //   * Prefer longer left roots (bio > bi) via a specificity bonus.
    //   * If the right slot is a Greek scientific suffix (-logy, -graphy...),
    //     bump Greek life-science roots and suppress Latin numeric roots
    //     so "biology" analyses as bio+logy = "study of life", not
    //     "study of two".
    std::vector<MalHypothesis> morphological_hypotheses(const std::string& tok) const {
        std::vector<MalHypothesis> out;
        if (tok.size() < 5) return out;   // too short to be a compound
        const auto& M = morpheme_dict();
        for (size_t i = 2; i < tok.size() - 1; ++i) {
            std::string left  = tok.substr(0, i);
            std::string right = tok.substr(i);
            auto itL = M.find(left);
            auto itR = M.find(right);
            if (itL == M.end() && itR == M.end()) continue;
            MalHypothesis h;
            h.token = tok;
            h.parts = {left, right};
            std::string lmean = (itL != M.end()) ? itL->second : left;
            std::string rmean = (itR != M.end()) ? itR->second : right;
            // Natural-order English gloss: "study of life" not "life study-of"
            if (itR != M.end() && is_scientific_suffix(right)) {
                h.expansion = rmean + " " + lmean;
            } else {
                h.expansion = lmean + " " + rmean;
            }
            h.evidence.push_back(left + " -> " +
                (itL != M.end() ? itL->second + " (morpheme)" : "unknown"));
            h.evidence.push_back(right + " -> " +
                (itR != M.end() ? itR->second + " (morpheme)" : "unknown"));
            float base = 0.3f;
            if (itL != M.end()) base += 0.35f;
            if (itR != M.end()) base += 0.35f;

            // Specificity bonus: longer roots are more discriminative than
            // shorter ones. Prevents "bi + ology" from tying with
            // "bio + logy" on raw morpheme count.
            float len_bonus = 0.01f * (float)left.size() +
                              0.01f * (float)right.size();
            base += len_bonus;

            // Contextual root resolution: when the suffix is a Greek
            // scientific suffix, prefer the Greek life-science reading of
            // the left root; demote the Latin numeric reading.
            if (itR != M.end() && is_scientific_suffix(right)) {
                if (is_greek_science_root(left))     base += 0.20f;
                else if (left == "bi"  || left == "tri" || left == "quad" ||
                         left == "uni" || left == "deca" || left == "penta")
                    base -= 0.30f;   // "bi + ology" style false analysis
            }

            h.confidence = std::max(0.0f, std::min(1.0f, base));
            h.reasoning = "\"" + tok + "\" is a real word; I analyse it as "
                          + left + " + " + right + ".";
            out.push_back(std::move(h));
        }
        return out;
    }

    // Main entry point: produce a ranked list of hypotheses for `token`.
    std::vector<MalHypothesis> hypothesise(const std::string& raw_token,
                                           Domain domain = Domain::None,
                                           int top_k = 5) const {
        std::vector<MalHypothesis> out;
        std::string tok = normalise(raw_token);
        if (tok.empty()) return out;

        Kind kind = classify(tok);

        // 1) Direct dictionary hit on the whole token. Highest signal.
        if (const auto* meanings = dict_lookup(tok)) {
            for (const auto& m : *meanings) {
                MalHypothesis h;
                h.token = tok;
                h.expansion = m;
                h.parts = {tok};
                h.evidence = {"initialism dictionary: " + tok + " -> " + m};
                h.confidence = 0.95f;
                h.reasoning = "The token \"" + raw_token +
                              "\" is a known initialism.";
                out.push_back(std::move(h));
            }
        }

        // 2) For compound words (real English words), try MORPHOLOGICAL
        // decomposition using Greek/Latin roots. Do NOT try initialism-
        // style letter chopping, which was the "biology → bipedalism oak
        // logic y" failure mode.
        if (kind == Kind::CompoundWord) {
            auto morph = morphological_hypotheses(tok);
            for (auto& h : morph) out.push_back(std::move(h));
            // If we got at least one morphological hypothesis above 0.5,
            // don't bother emitting the honest-refusal fallback below.
            bool have_strong = false;
            for (const auto& h : out) if (h.confidence >= 0.5f) have_strong = true;
            if (!have_strong) {
                // Honest refusal: real word we cannot analyse.
                MalHypothesis h;
                h.token = tok;
                h.parts = {tok};
                h.confidence = 0.10f;
                h.reasoning = "\"" + raw_token + "\" looks like a real English "
                              "word but its morphological roots are not in my "
                              "small root dictionary. I do not know what it means "
                              "and I refuse to guess by chopping it into letters.";
                out.push_back(std::move(h));
            }
        }

        // 3) For initialism-shaped tokens, try structural substring
        // decomposition with per-part scoring (original MAL behaviour).
        if (kind == Kind::Initialism) {
            auto decomps = decompose(tok, /*max_parts=*/4);
            for (const auto& parts : decomps) {
                if (parts.size() < 2) continue;
                MalHypothesis h;
                h.token = tok;
                h.parts = parts;
                float score_sum = 0.f;
                std::string joined_meaning;
                bool all_meaningful = true;
                for (const auto& p : parts) {
                    PartMatch pm = score_part(p);
                    score_sum += pm.part_confidence;
                    h.evidence.push_back(p + " -> " + pm.chosen_meaning +
                                         " (" + pm.source_kind + ")");
                    if (!joined_meaning.empty()) joined_meaning += " ";
                    joined_meaning += pm.chosen_meaning;
                    if (pm.source_kind != "dict" && pm.source_kind != "graph")
                        all_meaningful = false;
                }
                h.expansion = joined_meaning;
                h.confidence = score_sum / (float)parts.size();
                // Only reward decompositions where ALL parts are meaningfully
                // matched — this stops "cpp hair expansion mistry" style noise
                // from ranking above an honest refusal.
                bool used_dict = false;
                for (const auto& e : h.evidence)
                    if (e.find("dict") != std::string::npos) { used_dict = true; break; }
                if (used_dict) h.confidence += 0.15f;
                if (!all_meaningful) h.confidence *= 0.5f;
                h.reasoning = "Decomposed \"" + raw_token + "\" into " +
                              std::to_string(parts.size()) + " parts and matched each.";
                out.push_back(std::move(h));
            }
            // If nothing scored above 0.4, add an honest low-confidence
            // fallback rather than emit gibberish.
            bool have_strong = false;
            for (const auto& h : out) if (h.confidence >= 0.4f) have_strong = true;
            if (!have_strong) {
                MalHypothesis h;
                h.token = tok;
                h.parts = {tok};
                h.confidence = 0.15f;
                h.reasoning = "\"" + raw_token + "\" is short enough to be an "
                              "acronym but I cannot match any of its parts to "
                              "known concepts. I do not know what it stands for.";
                out.push_back(std::move(h));
            }
        }

        // Sort by confidence descending, keep top_k.
        std::sort(out.begin(), out.end(),
                  [](const MalHypothesis& a, const MalHypothesis& b){
                      return a.confidence > b.confidence;
                  });
        if ((int)out.size() > top_k) out.resize(top_k);
        return out;
    }
};

} // namespace unified
