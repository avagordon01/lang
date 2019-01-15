LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXXFLAGS = \
	-std=c++17 \
	-g \
	-Isrc -Iout \
	-Werror -Wall -Wextra -Wpedantic \
	-Wno-unused-function -Wno-unused-parameter \
	$(shell llvm-config --cxxflags) -std=c++17
LDFLAGS = $(shell llvm-config --ldflags)
LDLIBS = $(shell llvm-config --libs)

all: out/compiler

out/lex.yy.c: src/lexer.l
	@mkdir -p out
	$(LEX) $(LFLAGS) -o out/lex.yy.c src/lexer.l
out/y.tab.c out/y.tab.h: src/parser.y
	@mkdir -p out
	$(YACC) $(YFLAGS) -o out/y.tab.c src/parser.y

out/%.o: out/%.c out/y.tab.h
	@mkdir -p out
	$(CXX) $(CXXFLAGS) -c -o $@ $<
out/%.o: src/%.cc out/y.tab.h
	@mkdir -p out
	$(CXX) $(CXXFLAGS) -c -o $@ $<

sources := $(wildcard src/*.cc)
cxx_objects := $(addprefix out/,$(notdir $(sources:.cc=.o)))
out/compiler: out/lex.yy.o out/y.tab.o $(cxx_objects)
	@mkdir -p out
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf out
