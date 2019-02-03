#include "ast.hh"
#include "parser.hh"
#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
YY_DECL;

struct driver {
    ast::program program_ast;
    yy::location location;
    int parse() {
        location.initialize();
        yy::parser parser(*this);
        return parser.parse();
    }
};
