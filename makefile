CXX = clang++
LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CPPFLAGS = -Isrc -Iout
CXXFLAGS = -g -std=c++17 \
	-Werror -Wall -Wextra -Wpedantic \
	-Wno-unused-function -Wno-unused-parameter
LDFLAGS =
LDLIBS = -lLLVM-7

all: out/compiler

out/lex.yy.cc out/lex.yy.hh: src/lexer.l
	@mkdir -p out
	$(LEX) $(LFLAGS) --header-file=out/lex.yy.hh -o out/lex.yy.cc src/lexer.l
out/y.tab.cc out/y.tab.hh: src/parser.y
	@mkdir -p out
	$(YACC) $(YFLAGS) -o out/y.tab.cc src/parser.y

out/%.o: out/%.cc out/y.tab.hh out/lex.yy.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/%.o: src/%.cc out/y.tab.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

out/compiler: out/lex.yy.o out/y.tab.o out/main.o out/utils.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf out
