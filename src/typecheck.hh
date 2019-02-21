#include <vector>
#include <unordered_map>

#include "ast.hh"

struct typecheck_context {
    std::unordered_map<ast::identifier, ast::type> named_values;
    std::vector<std::string> symbols;
};

void typecheck(typecheck_context &context, ast::program &program);
