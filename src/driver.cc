#include "driver.hh"

#include <cstdio>

#include "error.hh"

void driver::parse() {
    location.initialize(&filename);
    scan_begin();
    parser_context parser(*this);
    program_ast = parser.parse_program();
    scan_end();
}

void driver::scan_begin() {
    yyin = fopen(filename.c_str(), "r");
    if (!yyin) {
        error("cannot open", filename, ":", std::strerror(errno));
    }
}

void driver::scan_end() {
    fclose(yyin);
}
