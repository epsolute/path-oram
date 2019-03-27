# definitions

SDIR=src
TDIR=test
IDIR=include
ODIR=obj
LDIR=lib
BDIR=bin

LDFLAGS=-L /usr/local/Cellar/boost/1.68.0_1/lib -L $(LDIR)
LDLIBS=-l gtest -l boost_system -l pthread
CPPFLAGS= --std=c++17 -I /usr/local/Cellar/boost/1.68.0_1/include -I $(IDIR) -Wall -fPIC
CC=g++
RM=rm -rf

_DEPS = definitions.h max-flow.hpp solver.hpp
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

_OBJ = max-flow.o solver.o
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGETS = main
TARGETBIN = $(addprefix $(BDIR)/, $(TARGETS))

TESTS = max-flow solver integration
TESTBIN = $(addprefix $(BDIR)/test-, $(TESTS))

ENTRYPOINTCC = $(CC) -o $@ $^ $(CPPFLAGS) $(LDLIBS) $(LDFLAGS)

# flags-setting commands

all: binaries shared docs
binaries: $(TARGETBIN) $(TESTBIN)
cleandebug: clean debug

debug: CPPFLAGS += -g
debug: binaries

profile: CPPFLAGS += -fprofile-arcs -ftest-coverage -fPIC -O0
profile: clean
profile: run-tests

# executables

.SECONDEXPANSION:
$(TARGETBIN): $(OBJ) $$(subst $$(BDIR), $(SDIR), $$@).cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(TESTBIN): $(OBJ) $$(subst $$(BDIR), $(TDIR), $$@).cpp
	$(ENTRYPOINTCC)

shared: CPPFLAGS += -DSHARED
shared: clean $(OBJ)
	$(CC) -shared -Wl -o $(BDIR)/libmaxflow.so $(OBJ)

# objects

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

# commands

run-tests: $(TESTBIN)
	$(addsuffix ;, $(TESTBIN))

coverage: profile
	gcovr -r . -f $(SDIR)/max-flow.cpp -f $(SDIR)/solver.cpp --exclude-unreachable-branches
	mkdir -p coverage-html/
	gcovr -r . --html --html-details -o coverage-html/index.html

docs: clean-docs
	doxygen ../Doxyfile

clean: clean-docs clean-binaries
	$(RM) $(ODIR)/* *~ *.dSYM *.gcov *.gcda *.gcno coverage-html

clean-docs:
	$(RM) ../docs

clean-binaries:
	$(RM) $(BDIR)/*

# phony

.PHONY: docs clean clean-docs clean-binaries run-tests coverage 
.PHONY: profile debug cleandebug
.PHONY: binaries all shared
