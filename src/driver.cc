#include "driver.hh"

#include <cstdio>

#include "error.hh"
#include "alt-parser.hh"

void driver::parse(std::string _filename) {
    filename = _filename;
    location.initialize(&filename);
    yyin = fopen(filename.c_str(), "r");
    if (!yyin) {
        error("cannot open", filename, ":", std::strerror(errno));
    }
    alt_parser_context parser(*this);
    parser.test_grammar();
    program_ast = parser.parse_program();
    fclose(yyin);
}
