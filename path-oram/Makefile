# definitions

SDIR=src
TDIR=test
IDIR=include
ODIR=obj
LDIR=lib
BDIR=bin

LDFLAGS=-L $(LDIR) -L /usr/local/opt/openssl/lib
LDLIBS=-l gtest -l boost_system -l ssl -l crypto -l pthread
INCLUDES=-I $(IDIR) -I /usr/local/opt/openssl/include
CPPFLAGS= --std=c++17 $(INCLUDES) -Wall -fPIC
CC=g++
RM=rm -rf

ENTITIES = storage-adapter position-map-adapter utility oram

_DEPS = definitions.h $(addsuffix .hpp, $(ENTITIES))
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

_OBJ = $(addsuffix .o, $(ENTITIES))
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGETS = main
TARGETBIN = $(addprefix $(BDIR)/, $(TARGETS))

TESTS = $(ENTITIES)
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
	$(CC) -shared -Wl -o $(BDIR)/libpathoram.so $(OBJ)

tests: $(TESTBIN)

# objects

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

# commands

run-tests: tests
	$(addsuffix ;, $(TESTBIN))

coverage: profile
	gcovr -r . $(addprefix -f $(SDIR)/, $(addsuffix .cpp, $(ENTITIES))) --exclude-unreachable-branches
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
