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

out/lexer.cc out/lexer.hh: src/lexer.l
	@mkdir -p out
	$(LEX) $(LFLAGS) --header-file=out/lexer.hh -o out/lexer.cc src/lexer.l
out/parser.cc out/parser.hh: src/parser.y
	@mkdir -p out
	$(YACC) $(YFLAGS) -o out/parser.cc src/parser.y

out/%.o: out/%.cc out/parser.hh out/lexer.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/%.o: src/%.cc out/parser.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

out/compiler: out/lexer.o out/parser.o out/main.o out/utils.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf out
