LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXXFLAGS = -std=c++17 \
		   -g \
		   -Isrc -Iout \
		   -Werror -Wall -Wextra -Wpedantic \
		   -Wno-unused-function -Wno-unused-parameter \
		   $(llvm-config --libs core jit native --cxxflags)
LDFLAGS = $(llvm-config --libs core jit native --ldflags)

sources := src/lexer.l src/parser.y src/*.cc src/*.hh
c_files := out/*.c src/*.cc

all: out/compiler

out/compiler: $(sources) makefile
	@mkdir -p out
	$(LEX) $(LFLAGS) -o out/lex.yy.c src/lexer.l
	$(YACC) $(YFLAGS) -o out/y.tab.c src/parser.y
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o out/compiler $(c_files)

clean:
	rm -rf out
