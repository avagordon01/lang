#include <cstdio>
#include <cstring>
#include <string>

#include "ast.hh"
#include "alt-parser.hh"
#include "error.hh"
#define YY_DECL token_type yylex(driver& drv)
YY_DECL;
extern FILE *yyin;

struct driver {
    ast::program program_ast;
    yy::location location;
    param_type current_param;
    std::unordered_map<std::string, ast::identifier> symbols_map;
    std::vector<std::string> symbols_list;
    std::string filename;
    void parse() {
        location.initialize(&filename);
        scan_begin();
        parser_context parser(*this);
        parser.parse_program();
        scan_end();
    }

    void scan_begin() {
        yyin = fopen(filename.c_str(), "r");
        if (!yyin) {
            error("cannot open", filename, ":", std::strerror(errno));
        }
    }

    void scan_end() {
        fclose(yyin);
    }
};
