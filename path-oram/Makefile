# definitions

SDIR=src
TDIR=test
HDIR=benchmark
IDIR=include
ODIR=obj
LDIR=lib
BDIR=bin

LDFLAGS=-L $(LDIR) -L /usr/local/opt/openssl/lib
LDLIBS=-l gtest -l boost_system -l ssl -l crypto -l pthread -l benchmark
INCLUDES=-I $(IDIR) -I /usr/local/opt/openssl/include
CPPFLAGS= --std=c++17 $(INCLUDES) -Wall -Wno-unknown-pragmas -fPIC
CC=g++
RM=rm -rf

ENTITIES = storage-adapter position-map-adapter utility oram stash-adapter

_DEPS = definitions.h $(addsuffix .hpp, $(ENTITIES))
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

_OBJ = $(addsuffix .o, $(ENTITIES))
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TESTS = $(ENTITIES)
TESTBIN = $(addprefix $(BDIR)/test-, $(TESTS))

BENCHMARKS = storage-adapter oram utility
BENCHMARKSBIN = $(addprefix $(BDIR)/benchmark-, $(BENCHMARKS))

INTEGRATION = oram-big
INTEGRATIONBIN = $(addprefix $(BDIR)/test-, $(INTEGRATION))

ENTRYPOINTCC = $(CC) -o $@ $^ $(CPPFLAGS) $(LDLIBS) $(LDFLAGS)

# flags-setting commands

all: shared docs

binaries: $(TESTBIN) $(INTEGRATIONBIN) $(BENCHMARKSBIN)
cleandebug: clean debug

debug: CPPFLAGS += -g -DTESTING
debug: binaries

profile: CPPFLAGS += -fprofile-arcs -ftest-coverage -fPIC -O0
profile: clean run-tests

# executables

.SECONDEXPANSION:
$(TESTBIN): $(OBJ) $$(subst $$(BDIR), $(TDIR), $$@).cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(BENCHMARKSBIN): $(OBJ) $$(subst $$(BDIR), $(HDIR), $$@).cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(INTEGRATIONBIN): $(OBJ) $$(subst $$(BDIR), $(TDIR), $$@).cpp
	$(ENTRYPOINTCC)

shared: CPPFLAGS += -DSHARED
shared: clean $(OBJ)
	$(CC) -shared -Wl -o $(BDIR)/libpathoram.so $(OBJ)

# objects

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

# commands

run-tests: CPPFLAGS += -DTESTING
run-tests: $(TESTBIN)
	$(addsuffix ;, $(TESTBIN))

run-benchmarks: $(BENCHMARKSBIN)
	$(addsuffix ;, $(BENCHMARKSBIN))

run-integration: CPPFLAGS += -DTESTING
run-integration: $(INTEGRATIONBIN)
	$(addsuffix ;, $(INTEGRATIONBIN))

coverage: profile
	gcovr -r . $(addprefix -f $(SDIR)/, $(addsuffix .cpp, $(ENTITIES))) --exclude-unreachable-branches
	mkdir -p coverage-html/
	gcovr -r . --html --html-details -o coverage-html/index.html

docs: clean-docs
	doxygen ../Doxyfile

clean: clean-docs clean-binaries
	$(RM) $(ODIR)/* *~ *.dSYM *.gcov *.gcda *.gcno *.bin coverage-html

clean-docs:
	$(RM) ../docs

clean-binaries:
	$(RM) $(BDIR)/*

# phony

.PHONY: docs clean clean-docs clean-binaries run-tests coverage
.PHONY: profile debug cleandebug
.PHONY: binaries all shared
