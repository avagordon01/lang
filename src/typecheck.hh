#include <vector>
#include <unordered_map>

#include "scopes.hh"
#include "ast.hh"

struct typecheck_context {
    ast::type_id current_function_returntype;
    std::unordered_map<ast::identifier, std::vector<ast::type_id>> function_parameter_types;
    ::scopes<ast::identifier, ast::type_id> variable_scopes;
    ::scopes<ast::type_id, ast::type> type_scopes;
    std::vector<std::string> symbols_list;
};

void typecheck(typecheck_context &context, ast::program &program);
