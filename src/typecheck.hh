#include <vector>
#include <unordered_map>

#include "ast.hh"

struct typecheck_context {
    ast::type current_function_returntype;
    using scope = std::unordered_map<ast::identifier, ast::type>;
    std::vector<scope> scopes;
    std::vector<std::string> symbols;
};

void typecheck(typecheck_context &context, ast::program &program);
