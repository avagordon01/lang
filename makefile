LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d
CXXFLAGS = -std=c++17 \
		   -Isrc \
		   $(llvm-config --libs core jit native --cxxflags --ldflags)

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
