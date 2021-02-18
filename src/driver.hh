#include <string>

#include "ast.hh"

struct driver {
    ast::program program_ast;
    yy::location location;
    bi_registry<ast::identifier, std::string> symbols_registry;
    std::string filename;

    void parse(std::string filename);
};
