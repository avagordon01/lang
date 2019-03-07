#include <vector>
#include <unordered_map>

#include "ast.hh"

struct typecheck_context {
    ast::type current_function_returntype;
    std::unordered_map<ast::identifier, std::vector<ast::type>> function_parameter_types;
    using scope = std::unordered_map<ast::identifier, ast::type>;
    std::vector<scope> scopes;
};

void typecheck(typecheck_context &context, ast::program &program);
