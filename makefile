LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d
LDFLAGS = -lfl

compiler: lexer.l parser.y
	$(YACC) $(YFLAGS) parser.y
	$(LEX) $(LFLAGS) lexer.l
	$(CC) $(LDFLAGS) -o $@ y.tab.c lex.yy.c

clean:
	git clean -Xfd
