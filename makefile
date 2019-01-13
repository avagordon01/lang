LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXXFLAGS = -std=c++17 \
		   -g \
		   -Isrc \
		   $(llvm-config --libs core jit native --cxxflags --ldflags) \
		   -Werror -Wall -Wextra -Wpedantic \
		   -Wno-unused-function -Wno-unused-parameter

out/compiler: out/y.tab.c out/lex.yy.c src/codegen.hh src/ast.hh
	@mkdir -p out
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
out/y.tab.c: src/parser.y
	@mkdir -p out
	$(YACC) $(YFLAGS) -o $@ $^
out/lex.yy.c: src/lexer.l
	@mkdir -p out
	$(LEX) $(LFLAGS) -o $@ $^

clean:
	rm -rf out
