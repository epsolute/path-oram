# definitions

SDIR=src
TDIR=test
IDIR=include
ODIR=obj
LDIR=lib
BDIR=bin

LDFLAGS=-L $(LDIR)
LDLIBS=-l gtest -l boost_system -l boost_filesystem -l pthread # TODO
CPPFLAGS= --std=c++17 -I $(IDIR) -Wall -fPIC
CC=g++
RM=rm -rf

_DEPS = definitions.h storage-adapter.hpp
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

_OBJ = storage-adapter.o
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGETS = main
TARGETBIN = $(addprefix $(BDIR)/, $(TARGETS))

TESTS = storage-adapter
TESTBIN = $(addprefix $(BDIR)/test-, $(TESTS))

ENTRYPOINTCC = $(CC) -o $@ $^ $(CPPFLAGS) $(LDLIBS) $(LDFLAGS)

# flags-setting commands

all: shared docs

binaries: $(TARGETBIN) $(TESTBIN)
cleandebug: clean debug

debug: CPPFLAGS += -g
debug: binaries

profile: CPPFLAGS += -fprofile-arcs -ftest-coverage -fPIC -O0
profile: clean run-tests

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

tests: $(TESTBIN)

# objects

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

# commands

run-tests: tests
	$(addsuffix ;, $(TESTBIN))

coverage: profile
	gcovr -r . -f $(SDIR)/storage-adapter.cpp --exclude-unreachable-branches
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
