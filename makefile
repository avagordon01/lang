LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXXFLAGS = -std=c++17 \
		   -g \
		   -Isrc -Iout \
		   $(llvm-config --libs core jit native --cxxflags --ldflags) \
		   -Werror -Wall -Wextra -Wpedantic \
		   -Wno-unused-function -Wno-unused-parameter

out/compiler: src/lexer.l src/parser.y src/ast.hh src/utils.hh src/codegen.hh src/utils.cc
	@mkdir -p out
	$(LEX) $(LFLAGS) -o out/lex.yy.c src/lexer.l
	$(YACC) $(YFLAGS) -o out/y.tab.c src/parser.y
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o out/compiler out/y.tab.c out/lex.yy.c src/codegen.hh src/ast.hh src/utils.cc

clean:
	rm -rf out
