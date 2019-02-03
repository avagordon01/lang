LEX = flex
LFLAGS =
YACC = bison -y
YFLAGS = -d -Werror=all
CXX = clang++
CPPFLAGS = -Isrc -Iout
CXXFLAGS = -g -std=c++17 -MMD -MP \
	-Werror -Wall -Wextra -Wpedantic \
	-Wno-unused-function -Wno-unused-parameter
LDFLAGS =
LDLIBS = -lLLVM-7

all: dirs out/compiler

objects := out/lexer.o out/parser.o out/main.o
depends := $(objects:.o=.d)

out/lexer.cc out/lexer.hh: src/lexer.ll
	$(LEX) $(LFLAGS) --header-file=out/lexer.hh -o out/lexer.cc src/lexer.ll
out/parser.cc out/parser.hh: src/parser.yy
	$(YACC) $(YFLAGS) -o out/parser.cc src/parser.yy
out/%.o: out/%.cc out/lexer.hh out/parser.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/%.o: src/%.cc out/lexer.hh out/parser.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
out/compiler: $(objects)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf out

.PHONY: dirs
dirs:
	@mkdir -p out

-include $(depends)
