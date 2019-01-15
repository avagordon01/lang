LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXXFLAGS = $(shell llvm-config --cxxflags --ldflags --libs) \
	-std=c++17 \
	-g \
	-Isrc -Iout \
	-Werror -Wall -Wextra -Wpedantic \
	-Wno-unused-function -Wno-unused-parameter

all: out/compiler

out/lex.yy.c: src/lexer.l
	@mkdir -p out
	$(LEX) $(LFLAGS) -o $@ $^
out/y.tab.c out/y.tab.h: src/parser.y
	@mkdir -p out
	$(YACC) $(YFLAGS) -o $@ $^
out/compiler: out/lex.yy.c out/y.tab.c src/*.cc src/*.hh
	@mkdir -p out
	$(CXX) $(CXXFLAGS) -o $@ out/*.c src/*.cc

clean:
	rm -rf out
