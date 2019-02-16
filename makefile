LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXX = g++
CPPFLAGS = -Isrc -Iout
CXXFLAGS = -g -std=c++17 -MMD -MP \
	-Werror -Wall -Wextra -Wpedantic \
	-Wno-unused-function -Wno-unused-parameter
LDFLAGS =
LDLIBS = -lLLVM-7

all: out/compiler

objects := out/lexer.o out/parser.o out/main.o
depends := $(objects:.o=.d)

out/lexer.cc out/lexer.hh: src/lexer.ll | dirs
	$(LEX) $(LFLAGS) --header-file=out/lexer.hh -o out/lexer.cc src/lexer.ll
out/parser.cc out/parser.hh: src/parser.yy | dirs
	$(YACC) $(YFLAGS) -o out/parser.cc src/parser.yy
out/%.o: out/%.cc out/lexer.hh out/parser.hh | dirs
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/%.o: src/%.cc out/lexer.hh out/parser.hh | dirs
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/compiler: $(objects) | dirs
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf out

test: parse-test codegen-link-test

parse-test: tests/parse-test.lang | out/compiler
	out/compiler $< out/parse-test.o out/parse-test.ir

codegen-link-test: tests/codegen-link-test.lang | out/compiler
	out/compiler $< out/codegen-link-test.o out/codegen-link-test.ir
	g++ tests/codegen-link-test.cc out/codegen-link-test.o -o out/codegen-link-test
	out/codegen-link-test

.PHONY: all dirs clean test parse-test codegen-link-test
dirs:
	@mkdir -p out

-include $(depends)
