LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d
LDFLAGS = -lfl

compiler: lexer.o parser.o

clean:
	rm -rf *.c *.h *.o
