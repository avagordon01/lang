LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d
LDFLAGS = -lfl

out/compiler: src/lexer.l src/parser.y
	$(YACC) $(YFLAGS) -o out/y.tab.c src/parser.y
	$(LEX) $(LFLAGS) -o out/lex.yy.c src/lexer.l
	$(CC) $(LDFLAGS) -o $@ out/y.tab.c out/lex.yy.c

clean:
	rm -rf out
