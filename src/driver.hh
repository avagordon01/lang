#include <cstdio>
#include <cstring>
#include <string>

#include "ast.hh"
#include "parser.hh"
#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
YY_DECL;
extern FILE *yyin;

struct driver {
    ast::program program_ast;
    yy::location location;
    std::unordered_map<std::string, ast::identifier> symbols_map;
    std::vector<std::string> symbols;
    void parse(const std::string& filename) {
        location.initialize();
        scan_begin(filename);
        yy::parser parser(*this);
        parser.parse();
        scan_end();
    }

    void scan_begin(const std::string& filename) {
        yyin = fopen(filename.c_str(), "r");
        if (!yyin) {
            std::cerr << "cannot open " << filename << ": " << std::strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void scan_end() {
        fclose(yyin);
    }
};
