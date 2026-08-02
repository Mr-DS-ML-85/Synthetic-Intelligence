// router.hpp — natural-language router + human-prose renderer.
//
// The user does not type command prefixes. They type a question. This
// module decides internally which pipeline to run (describe, path,
// treatments, repurpose, analogues, compare, bisociate) and renders the
// result as flowing prose — the way a knowledgeable colleague would
// explain it, not the way a table dumps.
//
// Design principles honouring the user's request:
//
//   1. NO command palette. One entry point: route_and_answer(query).
//   2. Graph-first. If the query mentions concepts in the graph, we
//      answer from the graph. MAL abduction only fires for genuinely
//      unknown terms.
//   3. Human prose. No brackets. No [score N]. Percentages and
//      qualitative bands ("strong lead", "worth exploring") rendered
//      inline. Complete sentences.
//   4. Auto-DMN. On any query touching multiple concepts, a small
//      background wander produces an aside if a real shower thought
//      surfaces. Otherwise silent.

#pragma once

#include "activation.hpp"
#include "analogy.hpp"
#include "axiom_loader.hpp"
#include "bisociation.hpp"
#include "discourse.hpp"
#include "dispatcher.hpp"
#include "inference.hpp"
#include "mal.hpp"
#include "novelty.hpp"
#include "primitives.hpp"
#include "salience.hpp"
#include "unified.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace unified {

// ---------- text utilities ----------

inline std::string rt_lower(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (char c : s) out.push_back((char)std::tolower((unsigned char)c));
    return out;
}
inline std::string rt_strip_punct(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (char c : s) {
        if (c == '?' || c == '.' || c == '!' || c == ',' ||
            c == ';' || c == ':' || c == '"' || c == '\'') continue;
        out.push_back(c);
    }
    return out;
}
inline std::string rt_normalise(const std::string& s) {
    return rt_strip_punct(rt_lower(s));
}
inline bool rt_contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

// Canonicalise a phrase for graph lookup — turn "type 2 diabetes"
// into "type2_diabetes", spaces to underscores, keep alphanumerics
// and underscores.
inline std::string rt_canon(const std::string& phrase) {
    std::string p = rt_normalise(phrase);
    std::string out; out.reserve(p.size());
    bool prev_space = true;
    for (char c : p) {
        if (c == ' ' || c == '_' || c == '-') {
            if (!prev_space && !out.empty() && out.back() != '_') out.push_back('_');
            prev_space = true;
        } else {
            out.push_back(c);
            prev_space = false;
        }
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out;
}

// ---------- entity extraction ----------
//
// The graph itself is the lexicon. We scan the query for the longest
// substrings whose canonicalised form matches a node label. This
// handles multi-word entities ("type 2 diabetes" → type2_diabetes)
// without any hand-authored vocabulary.
// A node is a "trigger stopword" if some intent-marker points at it via
// triggered_by. These are query lexicon, not concepts, and must be
// excluded from entity extraction.
inline bool rt_is_trigger_stopword(const SI& si, int node_id) {
    for (const auto& e : si.graph.edges) {
        if (e.to == node_id && e.rel == REL_TRIGGERED_BY) return true;
    }
    return false;
}

inline std::vector<int>
rt_extract_entities(const SI& si, const std::string& query) {
    std::vector<int> out;
    std::string q = rt_normalise(query);
    // Split into words.
    std::vector<std::string> words;
    {
        std::istringstream iss(q);
        std::string w;
        while (iss >> w) words.push_back(w);
    }
    std::unordered_set<int> seen;
    // Greedy longest-match scan.
    for (size_t i = 0; i < words.size(); ) {
        int matched = -1;
        size_t matched_len = 0;
        for (size_t j = std::min(words.size(), i + 5); j > i; --j) {
            std::string phrase;
            for (size_t k = i; k < j; ++k) {
                if (k > i) phrase.push_back(' ');
                phrase += words[k];
            }
            std::string canon = rt_canon(phrase);
            // Try the exact canonical form first.
            auto it = si.graph.by_label.find(canon);
            // If no hit, try digit-glue variants — this lets natural
            // "type 2 diabetes" reach the node "type2_diabetes":
            //   (a) glue letter->digit boundaries only
            //   (b) glue digit->letter boundaries only
            //   (c) glue both (rare, but covers "cox 2" → "cox2")
            if (it == si.graph.by_label.end()) {
                auto try_glue = [&](bool ld, bool dl) -> std::string {
                    std::string glued;
                    for (size_t k = 0; k < canon.size(); ++k) {
                        if (canon[k] == '_' && k > 0 && k + 1 < canon.size()) {
                            bool la = std::isalpha((unsigned char)canon[k-1]) != 0;
                            bool ld_ = std::isdigit((unsigned char)canon[k-1]) != 0;
                            bool ra = std::isalpha((unsigned char)canon[k+1]) != 0;
                            bool rd = std::isdigit((unsigned char)canon[k+1]) != 0;
                            if (ld  && la && rd)  continue;   // letter→digit
                            if (dl  && ld_ && ra) continue;   // digit→letter
                        }
                        glued.push_back(canon[k]);
                    }
                    return glued;
                };
                for (auto flags : { std::pair<bool,bool>{true,false},
                                    std::pair<bool,bool>{false,true},
                                    std::pair<bool,bool>{true,true} }) {
                    std::string g = try_glue(flags.first, flags.second);
                    if (g == canon) continue;
                    it = si.graph.by_label.find(g);
                    if (it != si.graph.by_label.end()) break;
                }
            }
            if (it != si.graph.by_label.end()) {
                matched = it->second;
                matched_len = j - i;
                break;
            }
        }
        if (matched >= 0) {
            // Skip trigger-word nodes — they're the intent lexicon,
            // not domain entities. The intent classifier consumes them.
            if (!rt_is_trigger_stopword(si, matched)) {
                if (!seen.count(matched)) {
                    out.push_back(matched);
                    seen.insert(matched);
                }
            }
            i += matched_len;
        } else {
            ++i;
        }
    }
    return out;
}

// ---------- intent classification ----------
enum class Intent {
    Describe,          // "what is X"
    HowWork,           // "how does X work" / "how does X reach Y"
    Treatments,        // "what treats X" / "what are the treatments for X"
    Repurpose,         // "what could treat X" / "what else might work for X"
    Similar,           // "what is X like" / "similar to X" / "structural analogues"
    Compare,           // "what do X and Y share" / "compare X and Y"
    Bridge,            // "is there a connection between X and Y" / "bridge X and Y"
    YesNoIsA,          // "is X a Y"
    YesNoHas,          // "does X have Y"
    // ---- Discovery Engine intents ----
    LogicGap,          // "what is missing between X and Y" / "what would connect X and Y"
    Isolated,          // "what concepts are floating" / "what's isolated"
    Dangling,          // "what nodes are ungrounded"
    SymmetryGap,       // "any incomplete symmetries" / "any missing symmetric laws"
    Anomalies,         // "where should we look for a breakthrough" / "what are the confused zones"
    Adverse,           // "which drugs are harmful for X" / "adverse effects"
    Fallback,          // otherwise, describe first entity
};

struct RoutedIntent {
    Intent kind = Intent::Fallback;
    std::vector<int> entities;     // node ids
    int mechanism_rel = REL_INHIBITS; // default for repurposing
};

// Map an intent-marker node label to the Intent enum. This is the one
// place a mapping table is unavoidable, but it is data — a bijection
// between graph labels and enum members, not classification logic.
inline Intent intent_from_marker_label(const std::string& lbl) {
    if (lbl == "intent_describe")   return Intent::Describe;
    if (lbl == "intent_howwork")    return Intent::HowWork;
    if (lbl == "intent_treatments") return Intent::Treatments;
    if (lbl == "intent_repurpose")  return Intent::Repurpose;
    if (lbl == "intent_similar")    return Intent::Similar;
    if (lbl == "intent_compare")    return Intent::Compare;
    if (lbl == "intent_bridge")     return Intent::Bridge;
    if (lbl == "intent_logicgap")   return Intent::LogicGap;
    if (lbl == "intent_isolated")   return Intent::Isolated;
    if (lbl == "intent_symmetry")   return Intent::SymmetryGap;
    if (lbl == "intent_anomalies")  return Intent::Anomalies;
    if (lbl == "intent_adverse")    return Intent::Adverse;
    return Intent::Fallback;
}

// Split a query into tokens without doing any pattern classification —
// pure tokenisation, no rt_contains logic.
inline std::vector<std::string>
tokenise_query(const std::string& raw) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : raw) {
        char lc = (char)std::tolower((unsigned char)c);
        if (std::isalnum((unsigned char)lc)) {
            cur.push_back(lc);
        } else {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Graph-driven intent classification.
//
// Instead of an if-else chain of rt_contains() over hardcoded strings,
// we run spreading activation over the graph itself:
//
//   1. Tokenise the query into words (pure tokenisation, no regex).
//   2. For each token, spike the corresponding graph node — if the
//      graph has a node with that exact label, its voltage rises.
//   3. Propagate the wave a few ticks. Activation flows backward
//      through `triggered_by` edges to the `intent_*` marker nodes.
//   4. Whichever intent-marker node accumulates the highest voltage
//      wins.
//
// The knowledge of WHICH words map to WHICH intent lives entirely in
// `data/intent_lexicon.axioms` — data, not code.
inline RoutedIntent
rt_classify(const SI& si, const std::string& raw_query) {
    RoutedIntent r;
    r.entities = rt_extract_entities(si, raw_query);

    // Tokenise.
    auto toks = tokenise_query(raw_query);

    // Bind an activation field just for this classification, without
    // disturbing the main graph.
    ActivationField field;
    field.bind(si);
    // Restrict conductivity to the `triggered_by` relation, so token
    // activation only flows to intent-markers, not into taxonomy.
    for (auto& em : field.edge) {
        em.valence = 1.0f;   // absolute — we want activation, not signed effect
        if (em.type_id != REL_TRIGGERED_BY) em.conductivity = 0.0f;
    }

    // Spike each token that matches an exact node label.
    int spiked = 0;
    for (const auto& t : toks) {
        auto it = si.graph.by_label.find(t);
        if (it != si.graph.by_label.end()) {
            field.inject(it->second, 1.0f);
            ++spiked;
        }
    }

    // Also detect mechanism-relation hint from stems in the query.
    // This is a small remaining bit of stem inspection — it disambiguates
    // "by activation" vs "by inhibition" for the repurposing pipeline.
    // We could push this into the graph too, but valence-side selection
    // is a boolean, not really intent.
    r.mechanism_rel = REL_INHIBITS;
    for (const auto& t : toks) {
        if (t.find("activat") != std::string::npos) r.mechanism_rel = REL_ACTIVATES;
        if (t.find("agonist") != std::string::npos) r.mechanism_rel = REL_ACTIVATES;
        if (t.find("inhibit") != std::string::npos) r.mechanism_rel = REL_INHIBITS;
        if (t.find("block")   != std::string::npos) r.mechanism_rel = REL_INHIBITS;
    }

    if (spiked == 0) {
        // No trigger words at all — this is a bare-concept query.
        // If there are entities, describe them; otherwise fallback.
        r.kind = r.entities.empty() ? Intent::Fallback : Intent::Describe;
        return r;
    }

    // Reverse propagation: for each spiked token, find incoming
    // triggered_by edges and pull voltage from the token INTO the
    // intent-marker at the other end. This is a manual reverse-pass
    // since the field's tick() only propagates forward.
    std::vector<float> intent_voltage(si.graph.nodes.size(), 0.0f);
    for (size_t i = 0; i < si.graph.edges.size(); ++i) {
        const Edge& e = si.graph.edges[i];
        if (e.rel != REL_TRIGGERED_BY) continue;
        // e.from is the intent marker, e.to is the trigger word.
        float token_v = field.voltage[e.to];
        if (token_v > 0.01f)
            intent_voltage[e.from] += token_v * field.edge[i].conductivity;
    }

    // Pick the intent-marker with the highest accumulated voltage.
    int best_node = -1;
    float best_v = 0.0f;
    for (int i = 0; i < (int)intent_voltage.size(); ++i) {
        if (intent_voltage[i] > best_v) {
            best_v = intent_voltage[i];
            best_node = i;
        }
    }
    if (best_node < 0 || best_v < 0.05f) {
        r.kind = r.entities.empty() ? Intent::Fallback : Intent::Describe;
        return r;
    }
    r.kind = intent_from_marker_label(si.graph.nodes[best_node].label);
    return r;
}

// ---------- prose helpers ----------

inline std::string rt_english(const std::string& s) {
    // Only underscores turn into spaces. We DO NOT split letter/digit
    // boundaries — those are proper names of proteins (sglt2, dpp4,
    // glp1r, hmg_coa_reductase). "type2_diabetes" reads as "type2
    // diabetes" which is what a domain reader expects.
    std::string out; out.reserve(s.size());
    for (char c : s) out.push_back(c == '_' ? ' ' : c);
    return out;
}

inline std::string rt_join_and(const std::vector<std::string>& xs) {
    if (xs.empty()) return "";
    if (xs.size() == 1) return xs[0];
    std::string out;
    for (size_t i = 0; i < xs.size(); ++i) {
        out += xs[i];
        if (i + 2 < xs.size()) out += ", ";
        else if (i + 1 < xs.size())
            out += (xs.size() == 2 ? " and " : ", and ");
    }
    return out;
}

inline std::string rt_strength_band(float s) {
    if (s >= 2.5f) return "a strong lead";
    if (s >= 1.5f) return "worth exploring";
    if (s >= 0.9f) return "a weak-but-real signal";
    return "a marginal signal";
}

// ---------- pipeline renderers (return prose) ----------

// State-dump renderer. No prose. Every answer looks the same:
//   intent = <intent_name>
//   <structured readout of graph state relevant to the query>
//
// The engine no longer speaks. It shows.

inline std::string rt_english_no_underscore(const std::string& s) {
    return s;   // labels are printed verbatim — they are the identifiers
}

inline std::string
rt_render_describe(SI& si, MorphAbduction& mal, DiscourseState& disc,
                   const std::string& raw, int node_id_or_neg) {
    std::ostringstream oss;
    oss << "  intent = describe\n";
    if (node_id_or_neg < 0) {
        oss << "  subject = " << raw << "  [not in graph]\n";
        auto hyps = mal.hypothesise(raw);
        if (!hyps.empty()) {
            oss << "\n  mal_decomposition:\n";
            for (size_t i = 0; i < hyps.size() && i < 3; ++i) {
                const auto& h = hyps[i];
                oss << "    " << raw << " => "
                    << (h.expansion.empty() ? "(none)" : h.expansion)
                    << "  conf=" << (int)(h.confidence * 100) << "%\n";
            }
        }
        (void)disc;
        return oss.str();
    }
    int nid = node_id_or_neg;
    const std::string& label = si.graph.nodes[nid].label;
    oss << "  subject = " << label << "\n\n";
    // Outgoing edges — group by relation, no prose.
    std::unordered_map<int, std::vector<std::string>> by_rel;
    for (const auto& e : si.graph.edges) {
        if (e.from != nid) continue;
        by_rel[e.rel].push_back(si.graph.nodes[e.to].label);
    }
    if (by_rel.empty()) {
        oss << "  [no outgoing edges]\n";
    } else {
        for (auto& kv : by_rel) {
            oss << "  " << rel_name_ext(kv.first) << ":\n";
            for (const auto& t : kv.second)
                oss << "    " << t << "\n";
        }
    }
    return oss.str();
}

inline std::string
rt_render_how_work(SI& si, int from_id, int to_id) {
    std::string a = si.graph.nodes[from_id].label;
    std::string b = si.graph.nodes[to_id].label;
    auto paths = find_paths(si, a, b, /*max_hops=*/6, /*cap=*/4);
    std::ostringstream oss;
    oss << "  intent = howwork\n";
    oss << "  from = " << a << "\n";
    oss << "  to   = " << b << "\n\n";
    if (paths.empty()) {
        oss << "  status = no_path_within_6_hops\n";
        return oss.str();
    }
    oss << "  paths:\n";
    for (size_t i = 0; i < paths.size(); ++i) {
        oss << "    [" << (i + 1) << ", len=" << paths[i].length() << "] ";
        for (size_t k = 0; k < paths[i].edges.size(); ++k) {
            const Edge& e = si.graph.edges[paths[i].edges[k]];
            if (k == 0) oss << si.graph.nodes[e.from].label;
            oss << " --" << rel_name_ext(e.rel) << "--> "
                << si.graph.nodes[e.to].label;
        }
        oss << "\n";
    }
    return oss.str();
}

inline std::string
rt_render_treatments(SI& si, int disease_id) {
    std::ostringstream oss;
    oss << "  intent = treatments\n";
    oss << "  subject = " << si.graph.nodes[disease_id].label << "\n\n";
    std::vector<std::string> tx;
    for (const auto& e : si.graph.edges)
        if (e.rel == REL_TREATS && e.to == disease_id)
            tx.push_back(si.graph.nodes[e.from].label);
    if (tx.empty()) {
        oss << "  [no treats-edges point at this node]\n";
    } else {
        oss << "  treats:\n";
        for (const auto& t : tx) oss << "    " << t << "\n";
    }
    return oss.str();
}

// Repurposing — driven by the spreading-activation engine. The ranking
// here does NOT come from a formula. It comes from voltage — the disease
// is charged, the wave flows through mechanism edges into known
// treatments, cascades into targets, then out into other drugs. Whichever
// drug ends the wave with the highest voltage is what the engine surfaces
// most strongly. The output text comes from concatenating the labels of
// nodes that lit up, in voltage order, with only the minimum grammatical
// scaffold.
inline std::string
rt_render_repurpose(SI& si, int disease_id, int mechanism_rel) {
    std::string disease = rt_english(si.graph.nodes[disease_id].label);
    auto cands = activation_candidates_for(
                     si, si.graph.nodes[disease_id].label,
                     mechanism_rel, /*n_ticks=*/8);
    if (cands.empty()) {
        return "I charged " + disease + " and let activation propagate "
               "through the graph; no other molecule accumulated voltage "
               "through the " + std::string(rel_name_ext(mechanism_rel))
             + " mechanism axis. Either my graph does not hold enough "
               "mechanism data yet, or the known treatments hit targets "
               "no other molecule in the graph shares.";
    }
    std::ostringstream oss;
    oss << "I charged " << disease << " and let the activation wave "
           "propagate through the " << rel_name_ext(mechanism_rel)
        << " axis. These are the molecules whose voltage rose above the "
           "noise floor — ranked by the final charge they accumulated, "
           "which is proportional to how many high-endorsement targets "
           "they share with the known treatments:\n\n";
    // Cap at 5 for readability, but do not otherwise pick and choose.
    size_t max_show = std::min<size_t>(5, cands.size());
    for (size_t i = 0; i < max_show; ++i) {
        const auto& c = cands[i];
        std::string name = rt_english(si.graph.nodes[c.node_id].label);
        std::vector<std::string> tgts, refs;
        for (int t : c.resonant_targets)
            tgts.push_back(rt_english(si.graph.nodes[t].label));
        for (int r : c.reference_treatments)
            refs.push_back(rt_english(si.graph.nodes[r].label));
        // Emit: "<name> [V=voltage]  — resonates on <targets>, sharing with <refs>."
        char buf[96];
        std::snprintf(buf, sizeof(buf), " (final voltage %.2f)",
                      c.voltage);
        oss << "  · " << name << buf;
        if (!tgts.empty()) {
            oss << " — resonates on " << rt_join_and(tgts);
        }
        if (!refs.empty()) {
            oss << ", sharing that axis with " << rt_join_and(refs);
        }
        oss << ".\n";
    }
    oss << "\nEach of these is a hypothesis surfaced by the propagation "
           "wave, not a proven treatment. The engine's confidence is the "
           "voltage number itself — the higher the accumulated charge, "
           "the more the graph's mechanism topology reinforced this "
           "candidate.";
    return fix_articles(oss.str());
}

inline std::string
rt_render_similar(SI& si, int src_id) {
    std::string src = si.graph.nodes[src_id].label;
    auto matches = find_analogues(si, src, /*top_k=*/6);
    if (matches.empty()) {
        return "I don't see anything structurally close to "
             + rt_english(src) + " in the graph right now.";
    }
    std::ostringstream oss;
    oss << rt_english(src) << "'s closest structural neighbours are ";
    std::vector<std::string> names;
    for (size_t i = 0; i < matches.size() && i < 3; ++i) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s (%.0f%% signature match)",
            rt_english(si.graph.nodes[matches[i].node_id].label).c_str(),
            matches[i].iso_score * 100.0f);
        names.push_back(buf);
    }
    oss << rt_join_and(names) << ".";
    // Highlight the most surprising one (highest Phi = novelty potential).
    auto surprise = *std::max_element(matches.begin(), matches.end(),
        [](const AnalogueMatch& a, const AnalogueMatch& b){
            return a.novelty_phi < b.novelty_phi;
        });
    if (surprise.hop_distance < 0 && surprise.node_id != matches.front().node_id) {
        oss << " The most interesting of these is "
            << rt_english(si.graph.nodes[surprise.node_id].label)
            << " — it doesn't sit on a direct path from "
            << rt_english(src) << " but shares its outgoing signature, "
            "which is the kind of structural echo that hints at a "
            "cross-domain analogy.";
    }
    return fix_articles(oss.str());
}

inline std::string
rt_render_compare(SI& si, int a_id, int b_id) {
    std::string a = si.graph.nodes[a_id].label;
    std::string b = si.graph.nodes[b_id].label;
    auto sh = shared_targets(si, a, b);
    if (sh.empty()) {
        return rt_english(a) + " and " + rt_english(b) +
               " have no shared downstream targets in my current graph. "
               "They may sit in unrelated pathways, or my mechanism data "
               "is not yet dense enough here.";
    }
    std::ostringstream oss;
    oss << rt_english(a) << " and " << rt_english(b) << " overlap on ";
    std::vector<std::string> parts;
    auto pluralize = [](const std::string& verb) -> std::string {
        // Subject "both" is plural — strip the -s that singular relations
        // pick up ("activates" -> "activate", "inhibits" -> "inhibit").
        if (!verb.empty() && verb.back() == 's') {
            // Handle common cases we actually produce.
            if (verb == "activates") return "activate";
            if (verb == "inhibits")  return "inhibit";
            if (verb == "treats")    return "treat";
            if (verb == "causes")    return "cause";
            if (verb == "binds")     return "bind";
        }
        return verb;
    };
    for (const auto& s : sh) {
        std::string t = rt_english(si.graph.nodes[s.target_node].label);
        std::string va = rel_name_ext(s.rel_from_a);
        std::string vb = rel_name_ext(s.rel_from_b);
        std::string phrase = t;
        if (va == vb)
            phrase += " (both " + pluralize(va) + " it)";
        else
            phrase += " (" + rt_english(a) + " " + va + " it, " +
                      rt_english(b) + " " + vb + ")";
        parts.push_back(phrase);
    }
    oss << rt_join_and(parts) << ".";
    if (sh.size() >= 2) {
        oss << " That's a two-target convergence, which usually means "
               "the two molecules operate on the same axis of biology, "
               "not just by coincidence.";
    }
    return fix_articles(oss.str());
}

inline std::string
rt_render_bridge(SI& si, int a_id, int b_id) {
    std::string a = si.graph.nodes[a_id].label;
    std::string b = si.graph.nodes[b_id].label;
    auto r = bisociate(si, a, b, /*tau=*/2, /*max_hops=*/6, /*u_crit=*/6.0f);
    std::ostringstream oss;
    if (r.validated && r.bridge_found) {
        oss << "There is a bridge: "
            << render_path(si, DiscoveryPath{r.bridge_edges})
            << ". The hinge concept is "
            << rt_english(r.intermediate)
            << ". Treat this as a mechanistic hypothesis, not a "
               "proven link.";
    } else if (r.validated && !r.intermediate.empty()) {
        oss << rt_english(a) << " and " << rt_english(b)
            << " don't reach each other by direct hops, but they share "
            << rt_english(r.intermediate)
            << " as a hinge concept — both act on it, so structurally "
               "they belong to the same axis of biology.";
    } else {
        oss << "I couldn't find a bridge between " << rt_english(a)
            << " and " << rt_english(b)
            << " within a reasonable number of hops. If a connection "
               "exists, it isn't in my graph yet.";
    }
    return fix_articles(oss.str());
}

inline std::string
rt_render_yesno_isa(SI& si, int a_id, int b_id) {
    std::string subj = si.graph.nodes[a_id].label;
    std::string obj  = si.graph.nodes[b_id].label;
    auto q = si.query(subj, REL_IS_A, obj);
    if (!q.holds) q = si.ask_backward_adaptive(subj, REL_IS_A, obj);
    if (q.holds) {
        // Format via existing render_proof for a proof-backed answer.
        DiscourseState d;
        return render_proof(si, q, &d);
    }
    return "Not that I can prove from the graph. " + rt_english(subj)
         + " is not derivable as " + rt_english(obj) + " here.";
}

inline std::string
rt_render_yesno_has(SI& si, int a_id, int b_id) {
    std::string subj = si.graph.nodes[a_id].label;
    std::string obj  = si.graph.nodes[b_id].label;
    auto q = si.query(subj, REL_HAS, obj);
    if (!q.holds) q = si.ask_backward_adaptive(subj, REL_HAS, obj);
    if (q.holds) {
        DiscourseState d;
        return render_proof(si, q, &d);
    }
    return "Not in the current graph. " + rt_english(subj)
         + " has no derivable link to " + rt_english(obj) + " under has.";
}

// ---------- Adverse / harmful query (honest refusal when the graph lacks the data) ----------
inline std::string
rt_render_adverse(SI& si, int subj_id) {
    std::string s = rt_english(si.graph.nodes[subj_id].label);
    // Scan for anything that causes or worsens the concept via
    // causes_disease, causes, downregulates, or inhibits a known
    // therapeutic target.
    std::vector<std::string> direct_causes;
    for (const auto& e : si.graph.edges) {
        if (e.to != subj_id) continue;
        if (e.rel == REL_CAUSES_DISEASE || e.rel == REL_CAUSES)
            direct_causes.push_back(rt_english(si.graph.nodes[e.from].label));
    }
    if (!direct_causes.empty()) {
        return "In my current graph, the entities that either cause or "
               "worsen " + s + " are " + rt_join_and(direct_causes) +
               ". This is what the graph asserts — adverse-effect data "
               "for individual drugs would need to be taught explicitly "
               "with edges like 'X worsens " + s + "'.";
    }
    return "I don't have adverse-effect data for " + s +
           " in the current graph — no node is asserted to cause or "
           "worsen it via causes_disease, causes, or a downregulation "
           "edge. Rather than invent something, I'll refuse. Teach me the "
           "adverse-relationship edges (e.g. 'teach: X downregulates Y') "
           "and I will surface them.";
}

// ---------- Discovery-Engine renderers ----------

inline std::string
rt_render_logic_gap(SI& si, int a_id, int b_id) {
    std::string a = si.graph.nodes[a_id].label;
    std::string b = si.graph.nodes[b_id].label;
    LogicGap g = find_logic_gap(si, a, b, /*max_hops=*/6);
    std::ostringstream oss;
    if (g.full_path_exists) {
        oss << rt_english(a) << " already reaches " << rt_english(b)
            << " in the current graph — there is no missing bridge to "
               "propose. Ask 'how does " << rt_english(a) << " reach "
            << rt_english(b) << "' if you want the path itself.";
        return fix_articles(oss.str());
    }
    if (g.from_side_node < 0 || g.to_side_node < 0) {
        oss << rt_english(a) << " and " << rt_english(b)
            << " sit in disconnected pieces of the graph. Neither one "
               "reaches anything downstream that the other reaches from "
               "the reverse side. The missing bridge is broader than a "
               "single edge — one or both of these concepts needs more "
               "mechanism data before I can point at a specific gap.";
        return fix_articles(oss.str());
    }
    std::string from_side = rt_english(si.graph.nodes[g.from_side_node].label);
    std::string to_side   = rt_english(si.graph.nodes[g.to_side_node].label);
    if (g.hops_from_a == 0 && g.hops_from_b_reverse == 0) {
        // Both endpoints are in the graph but there is no path in
        // either direction — the specific missing edge is A → B itself.
        oss << rt_english(a) << " and " << rt_english(b)
            << " are both known concepts, but nothing in the current "
               "graph connects them — the specific missing edge is "
            << rt_english(a) << " → " << rt_english(b)
            << ". If that link is real physics, teach it in and every "
               "downstream derivation will complete. This is the exact "
               "logic-gap the engine wants a physicist to fill.";
    } else {
        oss << "The engine can reach " << from_side << " from "
            << rt_english(a)
            << " in " << g.hops_from_a << " step"
            << (g.hops_from_a == 1 ? "" : "s")
            << ", and " << to_side << " reaches " << rt_english(b)
            << " in " << g.hops_from_b_reverse << " step"
            << (g.hops_from_b_reverse == 1 ? "" : "s")
            << ". What's missing is a direct link from " << from_side
            << " to " << to_side
            << ". If you can teach me that edge, the path completes. "
               "This is the logic-gap the engine would ask a physicist "
               "to fill.";
    }
    return fix_articles(oss.str());
}

inline std::string
rt_render_isolated(SI& si) {
    auto comps = find_isolated_subgraphs(si);
    if (comps.empty()) {
        return "Every connected piece of the current graph touches at "
               "least one fundamental / axiom-tagged node. Nothing is "
               "floating.";
    }
    std::ostringstream oss;
    oss << "I see " << comps.size()
        << " connected component" << (comps.size() == 1 ? "" : "s")
        << " that never touches a fundamental-tagged node. The largest "
           "such islands are:\n\n";
    size_t shown = 0;
    for (const auto& c : comps) {
        if (shown >= 5) break;
        oss << "  · " << c.nodes.size() << " concepts, including ";
        std::vector<std::string> names;
        for (size_t i = 0; i < c.nodes.size() && i < 4; ++i)
            names.push_back(rt_english(si.graph.nodes[c.nodes[i]].label));
        oss << rt_join_and(names);
        if (c.nodes.size() > 4) oss << ", and "
             << (c.nodes.size() - 4) << " more";
        oss << ".\n";
        ++shown;
    }
    oss << "\nThese islands are the abductive frontier — concepts the "
           "graph knows something about but hasn't yet grounded in a "
           "fundamental law. Bridging them is where new synthesis happens.";
    return fix_articles(oss.str());
}

inline std::string
rt_render_dangling(SI& si) {
    auto dn = find_dangling_edges(si, /*max_hops=*/6, /*top_k=*/8);
    if (dn.empty()) {
        return "Every node with two or more relations eventually reaches "
               "a fundamental-tagged concept within six hops. No dangling "
               "clusters right now.";
    }
    std::ostringstream oss;
    oss << "The following nodes are heavily connected but do not reach "
           "any fundamental-tagged concept within six hops:\n\n";
    for (size_t i = 0; i < dn.size() && i < 6; ++i) {
        const auto& d = dn[i];
        oss << "  · " << rt_english(si.graph.nodes[d.node_id].label)
            << " (" << d.total_degree << " edge"
            << (d.total_degree == 1 ? "" : "s")
            << ", not reaching any axiom in-graph)\n";
    }
    oss << "\nThe more connected a node is without touching a "
           "fundamental law, the more it looks like a 'confused zone' — "
           "the exact place a Maxwell-style unification would want to "
           "add a new axiom.";
    return fix_articles(oss.str());
}

inline std::string
rt_render_symmetry_gaps(SI& si) {
    auto gaps = find_symmetry_gaps(si);
    if (gaps.empty()) {
        return "I do not see any incomplete symmetries — every "
               "physics-style edge whose endpoints have mirror partners "
               "also has the mirrored version asserted. Either the "
               "theory is symmetry-closed as encoded, or no mirror_of "
               "partnerships have been declared yet.";
    }
    std::ostringstream oss;
    oss << "Incomplete symmetries — each of these is a candidate for the "
           "Maxwell displacement-current move (a mirror-partner relation "
           "that the graph implies should exist but doesn't):\n\n";
    for (size_t i = 0; i < gaps.size() && i < 6; ++i) {
        const auto& g = gaps[i];
        std::string rn = rel_name_ext(g.r);
        oss << "  · The graph asserts "
            << rt_english(si.graph.nodes[g.x].label) << " "
            << rn << " "
            << rt_english(si.graph.nodes[g.y].label)
            << ", but the mirror-partner relation "
            << rt_english(si.graph.nodes[g.x_mirror].label) << " "
            << rn << " "
            << rt_english(si.graph.nodes[g.y_mirror].label)
            << " is NOT asserted. If the symmetry the mirror_of pairing "
               "expresses is real, that relation is a candidate law.\n";
    }
    oss << "\nEach missing edge above is a testable hypothesis. This is "
           "how Maxwell added the displacement current to Ampere's law: "
           "curl E depended on ∂B/∂t (Faraday), so by mirror-symmetry "
           "curl B should also depend on ∂E/∂t — and the physical "
           "consequence of writing that edge in was electromagnetic waves.";
    return fix_articles(oss.str());
}

inline std::string
rt_render_anomalies(SI& si) {
    auto an = find_anomalies(si, /*max_hops=*/6, /*top_k=*/6);
    if (an.empty()) {
        return "Nothing looks especially anomalous — either the graph is "
               "still too small, or no node combines high connectivity "
               "with low axiomatic grounding.";
    }
    std::ostringstream oss;
    oss << "Ranked by connectivity-without-grounding — where a "
           "Maxwell-scale insight would restructure the theory:\n\n";
    for (size_t i = 0; i < an.size() && i < 5; ++i) {
        const auto& a = an[i];
        oss << "  · " << rt_english(si.graph.nodes[a.node_id].label)
            << " — " << a.total_degree << " relation"
            << (a.total_degree == 1 ? "" : "s")
            << ", ";
        if (a.hops_to_fundamental < 0)
            oss << "no path to any fundamental law";
        else
            oss << a.hops_to_fundamental << " hop"
                << (a.hops_to_fundamental == 1 ? "" : "s")
                << " from the nearest fundamental law";
        char buf[48];
        std::snprintf(buf, sizeof(buf), " (anomaly score %.1f)",
                      a.anomaly_score);
        oss << buf << "\n";
    }
    oss << "\nThese are the 'confused zones' — high activity, weak "
           "grounding. They are where the theory currently strains, "
           "and where a new axiom, if introduced, would resolve the "
           "most edges into a coherent story.";
    return fix_articles(oss.str());
}

// ---------- auto DMN aside ----------
//
// After the main answer, run a quick DMN scan. If a validated bridge
// emerges between two touched concepts (or between the touched concept
// and a currently high-salience node), emit a short "Actually, this
// also connects to..." aside. Silent otherwise.
inline std::string
rt_dmn_aside(SI& si, SalienceMap& sm) {
    sm.sync_with(si);
    auto pairs = sm.high_salience_distant_pairs(si, /*min_dist=*/3,
                                                /*max_pairs=*/16);
    for (const auto& p : pairs) {
        auto r = bisociate(si,
            si.graph.nodes[p.a].label, si.graph.nodes[p.b].label,
            /*tau=*/2, /*max_hops=*/6, /*u_crit=*/6.0f);
        if (r.validated) {
            std::ostringstream oss;
            oss << "\n\nActually — while thinking about this, I noticed that "
                << rt_english(r.a_label) << " and "
                << rt_english(r.b_label)
                << " share ";
            if (r.bridge_found)
                oss << "a route through "
                    << rt_english(r.intermediate) << " — "
                    << render_path(si, DiscoveryPath{r.bridge_edges}) << ".";
            else
                oss << rt_english(r.intermediate)
                    << " as a hinge concept.";
            oss << " It's the kind of connection worth keeping in mind.";
            return oss.str();
        }
    }
    return "";
}

// ---------- top-level entry point ----------

inline std::string
route_and_answer(ExtSI& si, MorphAbduction& mal, DiscourseState& disc,
                 SalienceMap& sm, const std::string& raw_query) {
    RoutedIntent I = rt_classify(si, raw_query);

    // Spike salience on every entity we touched, so DMN has fresh fuel.
    sm.sync_with(si);
    for (int e : I.entities) sm.spike(e);
    sm.tick(si);   // decay everything else

    std::string body;
    switch (I.kind) {
        case Intent::Repurpose: {
            if (I.entities.empty()) {
                body = "Tell me which disease or condition you want me to "
                       "look for candidates for — e.g. 'what could treat "
                       "type 2 diabetes?'";
                break;
            }
            body = rt_render_repurpose(si, I.entities.front(), I.mechanism_rel);
            break;
        }
        case Intent::Treatments: {
            if (I.entities.empty()) {
                body = "Which disease's treatments do you want listed?";
                break;
            }
            body = rt_render_treatments(si, I.entities.front());
            break;
        }
        case Intent::HowWork: {
            if (I.entities.size() >= 2) {
                body = rt_render_how_work(si, I.entities[0], I.entities[1]);
            } else if (I.entities.size() == 1) {
                // Single-entity "how does X work" — walk the outgoing
                // mechanism edges (activates / inhibits / part_of_pathway
                // / binds_to / treats / causes) and render them as a
                // mechanistic story rather than a bare description.
                int nid = I.entities.front();
                std::ostringstream oss;
                std::string subj = rt_english(si.graph.nodes[nid].label);
                std::vector<std::string> parts;
                for (const auto& e : si.graph.edges) {
                    if (e.from != nid) continue;
                    std::string verb;
                    switch (e.rel) {
                        case REL_ACTIVATES:       verb = "activates"; break;
                        case REL_INHIBITS:        verb = "inhibits"; break;
                        case REL_BINDS_TO:        verb = "binds to"; break;
                        case REL_TREATS:          verb = "treats"; break;
                        case REL_CAUSES:          verb = "causes"; break;
                        case REL_PART_OF_PATHWAY: verb = "sits in the pathway of"; break;
                        case REL_HAS:             verb = "carries"; break;
                        default: continue;
                    }
                    parts.push_back(verb + " " +
                        rt_english(si.graph.nodes[e.to].label));
                }
                if (parts.empty()) {
                    body = "I don't have any mechanistic edges for "
                         + subj + " in the graph — only classification. "
                         "Teach me some of its mechanism edges "
                         "('teach: " + si.graph.nodes[nid].label +
                         " activates X') and I can walk through how it works.";
                } else {
                    oss << "Mechanistically, " << subj << " " <<
                        rt_join_and(parts) << ". That's the chain of "
                        "actions the graph currently attributes to it.";
                    body = fix_articles(oss.str());
                }
            } else {
                body = "Tell me which concept's mechanism you want walked.";
            }
            break;
        }
        case Intent::Similar: {
            if (I.entities.empty()) {
                body = "Tell me which concept you want analogues of.";
                break;
            }
            body = rt_render_similar(si, I.entities.front());
            break;
        }
        case Intent::Compare: {
            if (I.entities.size() >= 2)
                body = rt_render_compare(si, I.entities[0], I.entities[1]);
            else
                body = "Give me two concepts to compare.";
            break;
        }
        case Intent::Bridge: {
            if (I.entities.size() >= 2)
                body = rt_render_bridge(si, I.entities[0], I.entities[1]);
            else
                body = "Give me two concepts to look for a bridge between.";
            break;
        }
        case Intent::YesNoIsA: {
            if (I.entities.size() >= 2)
                body = rt_render_yesno_isa(si, I.entities[0], I.entities[1]);
            else
                body = "I need both a subject and a class to check.";
            break;
        }
        case Intent::YesNoHas: {
            if (I.entities.size() >= 2)
                body = rt_render_yesno_has(si, I.entities[0], I.entities[1]);
            else
                body = "I need both a subject and a property to check.";
            break;
        }
        case Intent::LogicGap: {
            if (I.entities.size() >= 2)
                body = rt_render_logic_gap(si, I.entities[0], I.entities[1]);
            else
                body = "Give me two concepts and I will find the missing "
                       "bridge between them.";
            break;
        }
        case Intent::Isolated: {
            body = rt_render_isolated(si);
            break;
        }
        case Intent::Dangling: {
            body = rt_render_dangling(si);
            break;
        }
        case Intent::SymmetryGap: {
            body = rt_render_symmetry_gaps(si);
            break;
        }
        case Intent::Anomalies: {
            body = rt_render_anomalies(si);
            break;
        }
        case Intent::Adverse: {
            if (I.entities.empty()) {
                body = "Which condition's adverse-effect data do you "
                       "want checked?";
                break;
            }
            body = rt_render_adverse(si, I.entities.front());
            break;
        }
        case Intent::Describe:
        case Intent::Fallback:
        default: {
            if (!I.entities.empty()) {
                body = rt_render_describe(si, mal, disc, raw_query,
                                          I.entities.front());
            } else {
                // Bug 2: extract the topic token from the query and pass
                // ONLY that to MAL — not the full "what is ML" string,
                // which MAL then can't decompose.
                std::string topic;
                {
                    std::string q = rt_normalise(raw_query);
                    // Strip common question openers.
                    for (const auto& stem : {
                        "what is ", "what's ", "what does ",
                        "tell me about ", "describe " }) {
                        std::string ss = stem;
                        if (q.compare(0, ss.size(), ss) == 0) {
                            q = q.substr(ss.size());
                            break;
                        }
                    }
                    // Take the last remaining word — typically the topic.
                    size_t sp = q.find_last_of(' ');
                    topic = (sp == std::string::npos) ? q : q.substr(sp + 1);
                    // Strip trailing punctuation.
                    while (!topic.empty() &&
                           !std::isalnum((unsigned char)topic.back()))
                        topic.pop_back();
                }
                body = rt_render_describe(si, mal, disc,
                                          topic.empty() ? raw_query : topic,
                                          -1);
            }
            break;
        }
    }

    // Auto-DMN aside — only if we have enough spike-fuel.
    std::string aside = rt_dmn_aside(si, sm);
    if (!aside.empty()) body += aside;

    return body;
}

} // namespace unified
