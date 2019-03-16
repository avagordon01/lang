#include <vector>
#include <unordered_map>

#include "scopes.hh"
#include "ast.hh"

struct typecheck_context {
    ast::type current_function_returntype;
    std::unordered_map<ast::identifier, std::vector<ast::type>> function_parameter_types;
    ::scopes<ast::identifier, ast::type> variable_scopes;
    ::scopes<ast::identifier, ast::type> type_scopes;
    typecheck_context() {
        type_scopes.push_item(ast::primitive_type::t_void);
        type_scopes.push_item(ast::primitive_type::t_bool);
        type_scopes.push_item(ast::primitive_type::u8);
        type_scopes.push_item(ast::primitive_type::u16);
        type_scopes.push_item(ast::primitive_type::u32);
        type_scopes.push_item(ast::primitive_type::u64);
        type_scopes.push_item(ast::primitive_type::i8);
        type_scopes.push_item(ast::primitive_type::i16);
        type_scopes.push_item(ast::primitive_type::i32);
        type_scopes.push_item(ast::primitive_type::i64);
        type_scopes.push_item(ast::primitive_type::f16);
        type_scopes.push_item(ast::primitive_type::f32);
        type_scopes.push_item(ast::primitive_type::f64);
    }
};

void typecheck(typecheck_context &context, ast::program &program);
