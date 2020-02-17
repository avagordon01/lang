#include <vector>
#include <unordered_map>

#include "scopes.hh"
#include "ast.hh"
#include "registry.hh"

struct typecheck_context {
    ast::named_type current_function_returntype;
    ::registry<ast::identifier, std::vector<ast::named_type>> function_parameter_types;
    ::scopes<ast::identifier, ast::named_type> variable_scopes;
    ::scopes<ast::user_type, ast::type> type_scopes;
    bi_registry<ast::identifier, std::string>& symbols_registry;
    typecheck_context(bi_registry<ast::identifier, std::string>& sr): symbols_registry(sr) {}
};

void typecheck(typecheck_context &context, ast::program &program);
