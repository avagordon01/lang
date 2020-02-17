#include <string>

#include "ast.hh"
#include "parser.hh"
#define YY_DECL token_type yylex(driver& drv)
YY_DECL;
extern FILE *yyin;

struct driver {
    ast::program program_ast;
    yy::location location;
    param_type current_param;
    bi_registry<ast::identifier, std::string> symbols_registry;
    std::string filename;

    void parse();
    void scan_begin();
    void scan_end();
};
