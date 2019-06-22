LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all -Wno-yacc
LLC = llc
CXX = g++
CPPFLAGS = -Isrc -Iout
CXXFLAGS = -g -std=c++17 -MMD -MP \
	-Werror -Wall -Wextra -Wpedantic \
	-Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-parentheses
LDFLAGS =
LDLIBS = -lLLVM-8 -lSPIRV

DEBUGGER = gdb -nx -q -ex run -ex quit --args
ifdef debug
DEBUG = $(DEBUGGER)
endif

ifdef quiet
Q := @
endif

all: out/compiler

objects := out/lexer.o out/main.o out/codegen_llvm.o out/typecheck.o out/alt-parser.o
test_objects := out/scopes.o
depends := $(objects:.o=.d) $(test_objects:.o=.d)

out/lexer.cc out/lexer.hh: src/lexer.ll | dirs
	$(Q) $(LEX) $(LFLAGS) --header-file=out/lexer.hh -o out/lexer.cc src/lexer.ll
#out/parser.cc out/parser.hh: src/parser.yy | dirs
#	$(Q) $(YACC) $(YFLAGS) -o out/parser.cc src/parser.yy
out/%.o: out/%.cc out/lexer.hh | dirs
	$(Q) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/%.o: src/%.cc out/lexer.hh | dirs
	$(Q) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/compiler: $(objects) | dirs
	$(Q) $(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	@rm -rf out

type-0-tests := scopes
type-1-tests := parse codegen
type-2-tests := link fib gcd
test: $(type-0-tests) $(type-1-tests) $(type-2-tests)

$(type-0-tests): %: src/%.cc | dirs out/compiler
	$(Q) $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -o out/$@ $<
	$(Q) $(DEBUG) out/$@

$(type-1-tests): %: out/compiler | dirs
	$(Q) $(DEBUG) out/compiler tests/$@.kl out/$@.ir
	$(Q) $(LLC) -filetype=obj out/$@.ir -o out/$@.o

$(type-2-tests): %: out/compiler | dirs
	$(Q) $(DEBUG) out/compiler tests/$@.kl out/$@.ir
	$(Q) $(LLC) -filetype=obj out/$@.ir -o out/$@.o
	$(Q) $(CXX) tests/$@.cc out/$@.o -o out/$@
	$(Q) $(DEBUG) out/$@

.PHONY: all dirs clean test $(type-1-tests) $(type-2-tests)
dirs:
	@mkdir -p out

-include $(depends)
