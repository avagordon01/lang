#include <vector>
#include <unordered_map>

#include "scopes.hh"
#include "ast.hh"

struct typecheck_context {
    ast::type current_function_returntype;
    std::unordered_map<ast::identifier, std::vector<ast::type>> function_parameter_types;
    ::scopes<ast::identifier, ast::type> variable_scopes;
};

void typecheck(typecheck_context &context, ast::program &program);
