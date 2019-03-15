# definitions

SDIR=src
TDIR=test
IDIR=include
ODIR=obj
LDIR=lib

LDFLAGS=-L $(LDIR)
LDLIBS=-l gtest
CPPFLAGS= --std=c++17 -I $(IDIR) -Wall
CC=g++
RM=rm -rf

_DEPS = definitions.h max-flow.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = max-flow.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

TARGETS = main

TESTS = max-flow
TESTTARGETS = $(addprefix test-, $(TESTS))

ENTRYPOINTCC = $(CC) -o $@ $^ $(CPPFLAGS) $(LDLIBS) $(LDFLAGS)

# flags-setting commands

all: binaries docs
binaries: $(TARGETS) $(TESTTARGETS)
cleandebug: cleanall debug

debug: CPPFLAGS += -g
debug: binaries

profile: CPPFLAGS += -fprofile-arcs -ftest-coverage -fPIC -O0
profile: cleanall
profile: run-tests

# executables

.SECONDEXPANSION:
$(TARGETS): $(OBJ) $(SDIR)/$$@.cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(TESTTARGETS): $(OBJ) $(TDIR)/$$@.cpp
	$(ENTRYPOINTCC)

# objects

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

# commands

run-tests: $(TESTTARGETS)
	./test-max-flow

coverage: profile
	gcovr -r . -f $(SDIR)/max-flow.cpp
	mkdir -p coverage-html/
	gcovr -r . --html --html-details -o coverage-html/index.html

docs: cleandocs
	doxygen ../Doxyfile

clean:
	$(RM) $(ODIR)/* *~ *.dSYM *.gcov *.gcda *.gcno coverage-html

cleandocs:
	$(RM) ../docs

cleantests:
	$(RM) test-max-flow

cleanall: clean cleandocs cleantests
	$(RM) main

# phony

.PHONY: docs clean cleandocs cleanall cleantests run-tests coverage 
.PHONY: profile debug cleandebug
.PHONY: binaries all
