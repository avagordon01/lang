#include "utils.hh"
#include "parser.hh"

#include <iostream>
#include <string>

void error(std::string s) {
    std::cerr << s << std::endl;
    exit(1);
}
void yy::parser::error(const location_type& l, const std::string& m) {
    std::cerr << "line " << l.begin.line << " column " << l.begin.column << ": " << m << std::endl;
    exit(1);
}
