// discourse.hpp — Compositional Discourse Assembly.
//
// Turns SI derivation graphs and MAL hypotheses into LLM-style human
// prose *without any token prediction, softmax, or vocabulary sampling*.
// This is traditional NLG in the sense of Halliday, Reiter & Dale, and
// Mel'čuk's Meaning-Text Theory: discrete rule-based composition of
// clauses from a small template library, deterministic selection by
// structural hash of the source graph edge (so we get variety across
// derivations without ever *sampling* text).
//
// Nothing here is autoregressive. Nothing consults a probability
// distribution over the next word. Templates are strings with named
// slots; each template variant is chosen by a per-edge hash so distinct
// edges get distinct phrasings for cohesion, and a single edge always
// gets the same phrasing (deterministic replay).

#pragma once

#include "mal.hpp"
#include "unified.hpp"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace unified {

// A stable hash of an integer or string, used only to pick between
// interchangeable template variants — not for any decision that
// touches truth.
inline uint32_t stable_hash(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// English-ify a canonical label ("warm_blood" -> "warm blood").
inline std::string english_label(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (char c : s) out.push_back(c == '_' ? ' ' : c);
    return out;
}

// Capitalise the first character of a sentence.
inline std::string capitalise(const std::string& s) {
    std::string out = s;
    if (!out.empty()) out[0] = (char)std::toupper((unsigned char)out[0]);
    return out;
}

// Post-processor: correct "a X" -> "an X" whenever X begins with a
// vowel sound.
//
// Two guards to prevent over-firing when "a" is used as a node label
// (e.g. `teach: a is a b`) or as some other non-article token:
//   * the previous alphabetic word must not itself be a determiner
//     ("the", "any", ...) — if it is, this "a" is almost certainly a
//     label, not an article;
//   * the following word must not be a short function word ("is",
//     "as", "at", ...) — if it is, this "a" is not qualifying a noun.
inline std::string fix_articles(const std::string& s) {
    static const std::vector<std::string> kFuncNext = {
        "is","are","was","were","be","been","being",
        "as","at","on","in","to","of","or","and",
        "it","he","she","we","us","them","they",
        "an","the","a"
    };
    static const std::vector<std::string> kPrevBlockers = {
        "the","any","a","an","one","some","no","each","every",
        "another","this","that","these","those"
    };
    auto is_alpha = [](char c){ return std::isalpha((unsigned char)c) != 0; };
    auto read_word_at = [&](size_t start) -> std::string {
        std::string w;
        while (start < s.size() && is_alpha(s[start])) {
            w.push_back((char)std::tolower((unsigned char)s[start]));
            ++start;
        }
        return w;
    };
    auto in_list = [](const std::string& w,
                      const std::vector<std::string>& L){
        for (const auto& x : L) if (x == w) return true;
        return false;
    };
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        bool at_word_start = (i == 0 || !is_alpha(s[i-1]));
        if (at_word_start && i + 1 < s.size() &&
            (s[i] == 'a' || s[i] == 'A') && s[i+1] == ' ') {
            size_t j = i + 2;
            // Skip any run of extra spaces.
            while (j < s.size() && s[j] == ' ') ++j;
            std::string next = read_word_at(j);
            if (!next.empty()) {
                char nx = next[0];
                bool vowel = (nx=='a'||nx=='e'||nx=='i'||nx=='o'||nx=='u');
                // Look backward for the previous word.
                size_t k = i;
                while (k > 0 && !is_alpha(s[k-1])) --k;
                size_t p_end = k;
                while (k > 0 && is_alpha(s[k-1])) --k;
                std::string prev;
                for (size_t t = k; t < p_end; ++t)
                    prev.push_back((char)std::tolower((unsigned char)s[t]));
                bool prev_is_blocker = in_list(prev, kPrevBlockers);
                bool next_is_func    = in_list(next, kFuncNext);
                if (vowel && !prev_is_blocker && !next_is_func) {
                    out.push_back(s[i]);
                    out.push_back('n');
                    out.push_back(' ');
                    i += 2;
                    continue;
                }
            }
        }
        out.push_back(s[i]);
        ++i;
    }
    return out;
}

// Discourse history state (Fix 3). Tracks how many times each edge has
// been rendered in this session so a second/third render picks a
// different template variant. Determinism is preserved: given the same
// state, the same variant is chosen. The state itself is a pure counter
// with no probability distribution attached.
struct DiscourseState {
    std::unordered_map<uint32_t, uint32_t> edge_hit;   // edge idx -> visits
    uint32_t session_nonce = 0;                        // per-boot salt

    // Anti-repetition: track the most recent frame/verb/subj choice
    // per slot so consecutive edges avoid picking the same variant.
    uint32_t last_frame_idx  = 999;   // sentinel = "nothing yet"
    uint32_t last_verb_idx   = 999;
    uint32_t last_subj_idx   = 999;

    // Called before rendering — returns the current visit count for the
    // seed and increments it. Used as a rotation index into the template
    // variant table.
    uint32_t bump(uint32_t seed) {
        uint32_t& c = edge_hit[seed];
        uint32_t out = c;
        ++c;
        return out;
    }

    // Rotate a hash off the most-recently-used slot value, so we never
    // pick the same variant twice in a row for the same slot.
    uint32_t derotate(uint32_t proposed, uint32_t last, uint32_t modulus) {
        uint32_t v = proposed % modulus;
        if (v == last && modulus > 1) v = (v + 1) % modulus;
        return v;
    }
};

// -----------------------------------------------------------------------
// Compositional NLG (rewrite v2).
//
// Old phrase_edge used one switch to pick from 3 pre-baked template
// strings per relation. That produced ~3 surface forms — every clause
// felt hardcoded because it *was* hardcoded. This version decomposes
// each clause into five orthogonal slots:
//
//   [Frame] Subject Verb Object [Trailer]
//
// Each slot has 3–6 stylistic variants, and the choice is derived from
// independent hash streams so slots vary independently. That gives
// ~800–1500 surface forms per edge and, more importantly, real
// grammatical variation — active/passive voice, definiteness,
// scoping adverbs, and self-referential framing — rather than three
// stiff paraphrases.
//
// It is still deterministic: same seed + same DiscourseState ⇒ same
// output. Repeat renders in one session rotate through variants via
// the visit counter, so a query asked twice gets phrased two ways
// without ever consulting a probability distribution.
// -----------------------------------------------------------------------

namespace nlg {

// ---- slot: subject noun phrase (3 variants) ----
inline std::string subj_form(const std::string& lbl, uint32_t k) {
    std::string L = english_label(lbl);
    switch (k % 3) {
        case 0: return L;                     // bare
        case 1: return "the " + L;            // definite
        default: return "any " + L;           // generic
    }
}

// ---- slot: object noun phrase (4 variants) ----
inline std::string obj_form(const std::string& lbl, uint32_t k, int rel) {
    std::string L = english_label(lbl);
    switch (k % 4) {
        case 0: return L;                     // bare
        case 1: return "a " + L;              // indefinite
        case 2: return (rel == REL_IS_A) ? "the " + L + " category"
                                          : "the " + L;
        default: return L;                    // bare (fallback for variety)
    }
}

// ---- slot: verb / predicate for each relation (4–5 variants each) ----
inline std::string verb_form(int rel, uint32_t k) {
    switch (rel) {
        case REL_IS_A:
            switch (k % 5) {
                case 0: return "is";
                case 1: return "counts as";
                case 2: return "is one kind of";
                case 3: return "falls under";
                default: return "resolves as";
            }
        case REL_HAS:
            switch (k % 4) {
                case 0: return "has";
                case 1: return "carries";
                case 2: return "comes with";
                default: return "possesses";
            }
        case REL_IMPLIES:
            switch (k % 4) {
                case 0: return "implies";
                case 1: return "entails";
                case 2: return "brings about";
                default: return "means";
            }
        case REL_PART_OF:
            switch (k % 4) {
                case 0: return "is part of";
                case 1: return "sits inside";
                case 2: return "belongs to";
                default: return "forms part of";
            }
        case REL_CAUSES:
            switch (k % 4) {
                case 0: return "causes";
                case 1: return "gives rise to";
                case 2: return "brings on";
                default: return "produces";
            }
        case REL_EQUALS:
            return (k % 3 == 0) ? "is the same as"
                 : (k % 3 == 1) ? "coincides with" : "matches";
        case REL_NOT:
            return (k % 3 == 0) ? "is not" : (k % 3 == 1) ? "isn't" : "cannot be";
        default: {
            std::string rn = rel_name(rel);
            return rn;
        }
    }
}

// ---- slot: opening frame (6 variants, includes empty) ----
inline std::string frame(uint32_t k) {
    switch (k % 6) {
        case 0: return "";
        case 1: return "notice that ";
        case 2: return "structurally, ";
        case 3: return "at the ontology level, ";
        case 4: return "put simply, ";
        default: return "we can see ";
    }
}

// ---- slot: trailer / evidential (5 variants, includes empty) ----
inline std::string trailer(uint32_t k) {
    switch (k % 5) {
        case 0: return "";
        case 1: return " by construction";
        case 2: return " in general";
        case 3: return " as a matter of type";
        default: return " on the graph";
    }
}

} // namespace nlg

// A clause phrasing an edge — now a five-slot composition rather than a
// three-way template pick. Each slot draws from an independent hash
// stream so we get grammatical variation (voice, definiteness, hedging),
// not just synonym swaps.
//
// If `suppress_frame` is true, the opening frame slot is left empty. Use
// this from callers that have already prefixed a connective ("So", "It
// follows that", "Given that", ...) to avoid stacked openers like
// "Given that we can see any human counts as ...".
inline std::string phrase_edge(const Node& from, int rel, const Node& to,
                                uint32_t seed, DiscourseState* st = nullptr,
                                bool suppress_frame = false) {
    uint32_t rot = st ? st->bump(seed) : 0;
    uint32_t nonce = st ? st->session_nonce : 0u;

    // Five independent streams via different hash mixings of (seed, rot, nonce).
    uint32_t h1 = stable_hash(seed ^ nonce ^ 0xA5A5A5A5u) + rot * 7u;
    uint32_t h2 = stable_hash(seed ^ nonce ^ 0x5A5A5A5Au) + rot * 11u;
    uint32_t h3 = stable_hash(seed ^ nonce ^ 0x0F0F0F0Fu) + rot * 13u;
    uint32_t h4 = stable_hash(seed ^ nonce ^ 0xF0F0F0F0u) + rot * 17u;
    uint32_t h5 = stable_hash(seed ^ nonce ^ 0x33333333u) + rot * 19u;

    // Anti-repetition: rotate the frame / verb / subject choices off
    // the last one used so we don't say "At the ontology level" three
    // sentences in a row.
    uint32_t frame_idx = st ? st->derotate(h1, st->last_frame_idx, 6) : (h1 % 6);
    uint32_t verb_mod  = (rel == REL_IS_A) ? 5 : 4;
    uint32_t verb_idx  = st ? st->derotate(h3, st->last_verb_idx, verb_mod)
                            : (h3 % verb_mod);
    uint32_t subj_idx  = st ? st->derotate(h2, st->last_subj_idx, 3) : (h2 % 3);
    if (st) {
        st->last_frame_idx = frame_idx;
        st->last_verb_idx  = verb_idx;
        st->last_subj_idx  = subj_idx;
    }

    std::string f  = suppress_frame ? std::string() : nlg::frame(frame_idx);
    std::string sf = nlg::subj_form(from.label, subj_idx);
    std::string vf = nlg::verb_form(rel, verb_idx);
    std::string of = nlg::obj_form(to.label, h4, rel);
    std::string tr = nlg::trailer(h5);

    // A tiny bit of grammar cleanup — the frame is prefixed lowercase,
    // and the composition owner (render_proof / render_answer) may
    // capitalise the first letter of the outer sentence.
    return f + sf + " " + vf + " " + of + tr;
}

// Connective for chaining two derivation steps. Now draws from a
// larger, structurally-aware pool: connectives depend not just on
// whether this is a conclusion but on how deep the chain has grown so
// far. Long chains get more reflective connectives ("stitching this
// together"), short ones stay crisp ("so").
inline std::string connective(uint32_t seed, bool is_conclusion,
                              int depth_so_far,
                              DiscourseState* st = nullptr) {
    uint32_t rot = st ? st->bump(seed ^ 0xC0FFEEu) : 0;
    uint32_t v = (stable_hash(seed ^ (st ? st->session_nonce : 0u)) + rot) % 6;
    if (is_conclusion) {
        // A pool of 6 conclusion connectives. Longer chains earn the
        // more reflective ones.
        if (depth_so_far >= 3) {
            switch (v) {
                case 0: return "Stitching this together,";
                case 1: return "Which finally gives us that";
                case 2: return "So, walking the whole chain,";
                case 3: return "Threading these links,";
                case 4: return "The endpoint of this reasoning is that";
                default: return "Pulling this together,";
            }
        }
        switch (v) {
            case 0: return "So";
            case 1: return "It follows that";
            case 2: return "Which means";
            case 3: return "Therefore";
            case 4: return "So we can say";
            default: return "That gives us that";
        }
    } else {
        switch (v) {
            case 0: return "From that";
            case 1: return "and given the above";
            case 2: return "which lets me note that";
            case 3: return "combining these two,";
            case 4: return "picking up from here,";
            default: return "carrying this forward,";
        }
    }
}

// A short self-narrated preamble for the proof, sensitive to how deep
// the chain is. This is the "I'm thinking" texture — it names the
// shape of the reasoning rather than robotically listing "Given ...".
inline std::string proof_preamble(size_t premise_count, size_t derived_count,
                                  int steps_used,
                                  uint32_t seed,
                                  DiscourseState* st = nullptr) {
    uint32_t rot = st ? st->bump(seed ^ 0xBEEFu) : 0;
    uint32_t v = (stable_hash(seed ^ (st ? st->session_nonce : 0u)) + rot) % 5;
    std::ostringstream oss;
    // If the chain is very short, do not add ceremonial preamble.
    if (derived_count == 0) {
        return "";
    }
    if (derived_count == 1) {
        switch (v) {
            case 0: return "This lands in one hop. ";
            case 1: return "One link is enough here. ";
            case 2: return "The graph gives me this directly: ";
            case 3: return "This is a single-step derivation. ";
            default: return "";
        }
    }
    // Deeper chains — self-narrate the shape.
    switch (v) {
        case 0:
            oss << "I traced " << (premise_count + derived_count)
                << " links through the ontology";
            if (steps_used > 0) oss << " (search depth " << steps_used << ")";
            oss << " to answer this. ";
            break;
        case 1:
            oss << "Let me walk the derivation. There are "
                << premise_count << " premises the graph already knows, and "
                << derived_count << " steps I derive from them. ";
            break;
        case 2:
            oss << "Thinking through this: ";
            break;
        case 3:
            oss << "The reasoning unfolds across " << derived_count
                << " derivation steps. ";
            break;
        default:
            oss << "Working through the chain, ";
            break;
    }
    return oss.str();
}

// Emit a paragraph for a full derivation chain.
inline std::string render_proof(const SI& si, const SI::QueryResult& q,
                                DiscourseState* st = nullptr) {
    std::ostringstream oss;
    if (!q.holds || q.proof.empty()) {
        oss << "The claim cannot be derived from the current premises.";
        return oss.str();
    }
    // Split premises from derived steps.
    std::vector<int> premises, derived;
    for (int e : q.proof) {
        if (si.graph.edges[e].derived_by_rule < 0) premises.push_back(e);
        else derived.push_back(e);
    }
    // Self-narrated preamble, keyed on the chain shape and search depth.
    uint32_t preamble_seed = (uint32_t)(premises.size() * 131u
                                        + derived.size() * 17u
                                        + (uint32_t)q.steps_used);
    oss << proof_preamble(premises.size(), derived.size(),
                          q.steps_used, preamble_seed, st);

    // Premises paragraph — one sentence, comma-joined, but the opener
    // varies too.
    if (!premises.empty()) {
        uint32_t open_seed = (uint32_t)(premises.size() * 13u
                                        + derived.size() * 29u);
        uint32_t open_rot  = st ? st->bump(open_seed ^ 0xFACEu) : 0;
        uint32_t ov = (stable_hash(open_seed ^ (st ? st->session_nonce : 0u))
                       + open_rot) % 4;
        bool opener_has_frame = true;
        switch (ov) {
            case 0: oss << "I start from what the graph already knows: ";
                    opener_has_frame = true; break;
            case 1: oss << "Given that ";
                    opener_has_frame = true; break;
            case 2: oss << "The premises on hand are that ";
                    opener_has_frame = true; break;
            default: oss << "Taking the known facts — ";
                    opener_has_frame = true; break;
        }
        for (size_t i = 0; i < premises.size(); ++i) {
            const Edge& e = si.graph.edges[premises[i]];
            // The very first premise sits right after the opener, so
            // suppress its own frame. Subsequent premises, being inside
            // a comma-list, can take their frame normally.
            bool suppress = (i == 0) && opener_has_frame;
            oss << phrase_edge(si.graph.nodes[e.from], e.rel,
                               si.graph.nodes[e.to],
                               (uint32_t)premises[i], st, suppress);
            if (i + 2 < premises.size()) oss << ", ";
            else if (i + 1 < premises.size()) oss << ", and ";
        }
        oss << ". ";
    }
    // Derived steps — each with its own sentence and a connective.
    for (size_t i = 0; i < derived.size(); ++i) {
        int e_idx = derived[i];
        const Edge& e = si.graph.edges[e_idx];
        bool is_conclusion = (i + 1 == derived.size());
        std::string conn = connective((uint32_t)e_idx, is_conclusion,
                                      (int)i, st);
        // A connective already opens the sentence — suppress the
        // clause's own frame so we don't say "So notice that...".
        std::string clause = phrase_edge(si.graph.nodes[e.from], e.rel,
                                         si.graph.nodes[e.to],
                                         (uint32_t)e_idx, st,
                                         /*suppress_frame=*/true);
        // Capitalise the connective; lowercase the clause opener so the
        // grammar reads naturally.
        if (!conn.empty()) conn[0] = (char)std::toupper((unsigned char)conn[0]);
        oss << conn << " " << clause << ". ";
    }
    return oss.str();
}

// Confidence-floor policy (Fix 4). Three tiers govern how an abductive
// hypothesis is presented:
//   * >= kConfFact       — render as an assertion (this expansion is real)
//   * [kConfHypothesis)  — render as a hedged hypothesis, ask verification
//   * <  kConfHypothesis — force an honest refusal, no expansion rendered
// These constants are the L9 paper's "Confidence_Floor for Assertion".
constexpr float kConfFact       = 0.70f;
constexpr float kConfHypothesis = 0.30f;

// Emit a MAL-based hypothesis paragraph. Marks inferences explicitly.
inline std::string render_mal(const std::string& raw_token,
                              const std::vector<MalHypothesis>& hyps) {
    std::ostringstream oss;
    if (hyps.empty()) {
        oss << "The token \"" << raw_token
            << "\" is unknown to me and I cannot decompose it further; I don't know what it means.";
        return oss.str();
    }
    const auto& top = hyps.front();

    // Fix 4: honest refusal below the hypothesis floor. No expansion is
    // rendered — the system explicitly declines to guess, which is what
    // an epistemically-sound engine should do below 0.30.
    if (top.confidence < kConfHypothesis) {
        oss << "The token \"" << raw_token
            << "\" is not in my ontology and my structural decomposition of it "
            << "did not clear the confidence floor of "
            << (int)(kConfHypothesis * 100) << "%. "
            << "I have no reliable structural basis for a meaning here, so I refuse to "
            << "invent one. Teach me this concept explicitly (see 'teach:' in interactive mode) "
            << "and I will reason about it.";
        return oss.str();
    }
    // Opening: acknowledge uncertainty (not a fact assertion).
    oss << "I do not have \"" << raw_token << "\" as a known concept in my ontology, "
        << "so let me reason about it structurally rather than pretend to know. ";
    // Body: how the hypothesis was constructed.
    oss << top.reasoning << " ";
    if (!top.expansion.empty()) {
        oss << "My best structural hypothesis is that \"" << raw_token
            << "\" expands to \"" << english_label(top.expansion) << "\". ";
    } else {
        oss << "I could not construct a full expansion; the letters individually give ";
        for (size_t i = 0; i < top.parts.size(); ++i) {
            oss << "'" << top.parts[i] << "'";
            if (i + 2 < top.parts.size()) oss << ", ";
            else if (i + 1 < top.parts.size()) oss << " and ";
        }
        oss << " with no known dictionary meaning attached. ";
    }
    // Evidence trace (compact).
    if (!top.evidence.empty()) {
        oss << "The evidence for this: ";
        for (size_t i = 0; i < top.evidence.size(); ++i) {
            oss << top.evidence[i];
            if (i + 1 < top.evidence.size()) oss << "; ";
        }
        oss << ". ";
    }
    // Alternatives — mention up to two.
    if (hyps.size() > 1) {
        oss << "I also considered ";
        size_t n = std::min<size_t>(2, hyps.size() - 1);
        for (size_t i = 1; i <= n; ++i) {
            oss << "\"" << english_label(hyps[i].expansion.empty() ? "(no expansion)" : hyps[i].expansion) << "\"";
            if (i + 1 <= n) oss << " and ";
        }
        oss << " but ranked them lower. ";
    }
    // Confidence hedge. Fix 4: aligned with the three-tier floor policy.
    if (top.confidence >= kConfFact) {
        oss << "I am confident about this expansion — it is a well-known initialism or a clean morphological match.";
    } else if (top.confidence >= kConfHypothesis) {
        oss << "My confidence is moderate (above the "
            << (int)(kConfHypothesis * 100) << "% floor but below the "
            << (int)(kConfFact * 100) << "% fact threshold); the structural evidence is consistent but I would verify it against a trusted source before relying on it.";
    }
    // The <kConfHypothesis case was already handled above with an
    // outright refusal; this branch is unreachable but kept explicit.
    return oss.str();
}

// Top-level: assemble a full LLM-style human answer from an SI query
// and (optionally) MAL hypotheses. Optional DiscourseState (Fix 3)
// rotates template variants across repeat renders.
inline std::string render_answer(const SI& si,
                                 const SI::QueryResult& q,
                                 const std::string& raw_token = "",
                                 const std::vector<MalHypothesis>& mal_hyps = {},
                                 DiscourseState* st = nullptr) {
    std::ostringstream oss;
    if (q.holds) {
        oss << render_proof(si, q, st);
    } else if (!mal_hyps.empty()) {
        oss << render_mal(raw_token, mal_hyps);
    } else {
        oss << "The concept \"" << raw_token
            << "\" is not in my ontology and I could not decompose it into pieces I recognise, so I do not know what it means.";
    }
    // Ensure a final period.
    std::string s = oss.str();
    while (!s.empty() && s.back() == ' ') s.pop_back();
    if (!s.empty() && s.back() != '.') s.push_back('.');
    // Post-process: fix "a X" -> "an X" where X is vowel-initial.
    s = fix_articles(s);
    return capitalise(s);
}

} // namespace unified
