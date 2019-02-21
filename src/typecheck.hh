#include <vector>
#include <unordered_map>

#include "ast.hh"

struct typecheck_context {
    ast::type current_function_returntype;
    ast::type current_loop_returntype;
    std::unordered_map<ast::identifier, ast::function_def> functions;
    using scope = std::unordered_map<ast::identifier, ast::type>;
    std::vector<scope> scopes;
    std::vector<std::string> symbols;
};

void typecheck(typecheck_context &context, ast::program &program);
