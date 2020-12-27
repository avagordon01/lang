#include "driver.hh"

#include <cstdio>

#include "error.hh"
#include "alt-parser.hh"

void driver::parse(std::string _filename) {
    filename = _filename;
    alt_parser_context parser(*this);
    parser.test_grammar();
    program_ast = parser.parse_program();
}
