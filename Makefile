CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS  ?= -pthread

# `si` is the product. Everything the substrate can do is reachable
# from it with no flags and no subcommands — the input selects the
# behaviour, not the operator.
#
# The three harnesses below are NOT features. They are reproducibility
# entry points that pin one measurement each, so a reader can check a
# published number without going through the chat loop.

all: si

si: src/si_main.cpp src/lang.hpp src/si_state.hpp src/big_ontology.hpp \
    src/axiom_loader.hpp src/physics.hpp src/layer2.hpp \
    src/unified.hpp src/primitives.hpp src/meta_learner.hpp src/rifa.hpp \
    src/salience.hpp src/bisociation.hpp src/novelty.hpp src/analogy.hpp \
    src/inference.hpp src/bsma_topo.hpp
	$(CXX) $(CXXFLAGS) src/si_main.cpp -o si $(LDFLAGS)

# --- reproducibility harnesses -----------------------------------------

# Phase 2: hypothesis -> emitted source -> compiler verdict -> corrected graph.
harness-synth: src/synth.cpp src/big_ontology.hpp src/axiom_loader.hpp \
    src/physics.hpp src/unified.hpp src/primitives.hpp \
    src/meta_learner.hpp src/rifa.hpp
	$(CXX) $(CXXFLAGS) src/synth.cpp -o harness-synth $(LDFLAGS)

# The J-A-S cycle and the impossibility register.
harness-jas: src/jas.cpp src/big_ontology.hpp src/axiom_loader.hpp \
    src/physics.hpp src/unified.hpp src/primitives.hpp \
    src/meta_learner.hpp src/rifa.hpp
	$(CXX) $(CXXFLAGS) src/jas.cpp -o harness-jas $(LDFLAGS)

# Structural code queries with proof chains.
harness-codeq: src/codeq.cpp src/si_state.hpp src/big_ontology.hpp \
    src/axiom_loader.hpp src/physics.hpp src/layer2.hpp \
    src/unified.hpp src/primitives.hpp src/meta_learner.hpp src/rifa.hpp
	$(CXX) $(CXXFLAGS) src/codeq.cpp -o harness-codeq $(LDFLAGS)

harnesses: harness-synth harness-jas harness-codeq

clean:
	rm -f si harness-synth harness-jas harness-codeq

# Rebuild the substrate from the axiom files (drops learned state).
reset:
	rm -f pcma.si

.PHONY: all harnesses clean reset
