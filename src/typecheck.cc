#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

#include "typecheck.hh"
#include "ast.hh"
#include "error.hh"

ast::named_type accessor_access(typecheck_context& context, ast::accessor& accessor) {
    std::optional<ast::named_type> v = context.variable_scopes.find_item(accessor.identifier);
    if (!v.has_value()) {
        error(accessor.loc, "variable used before being defined");
    }
    return *v;
}

struct typecheck_fn {
    typecheck_context& context;
    ast::named_type operator()(ast::program& program) {
        context.variable_scopes.push_scope();
        for (auto& statement: program.statements) {
            std::invoke(*this, statement);
        }
        context.variable_scopes.pop_scope();
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    ast::named_type operator()(std::unique_ptr<ast::block>& block) {
        return std::invoke(*this, *block);
    }
    ast::named_type operator()(ast::block& block) {
        ast::named_type type = {ast::primitive_type{ast::primitive_type::t_void}};
        context.variable_scopes.push_scope();
        for (auto& statement: block.statements) {
            type = std::invoke(*this, statement);
        }
        context.variable_scopes.pop_scope();
        block.type = type;
        return type;
    }
    ast::named_type operator()(std::unique_ptr<ast::if_statement>& if_statement) {
        return std::invoke(*this, *if_statement);
    }
    ast::named_type operator()(ast::if_statement& if_statement) {
        for (auto& condition: if_statement.conditions) {
            if (std::invoke(*this, condition) != ast::named_type{ast::primitive_type{ast::primitive_type::t_bool}}) {
                error(if_statement.loc, "if statement condition not a boolean");
            }
        }
        ast::named_type type = std::invoke(*this, if_statement.blocks.front());
        for (auto& block: if_statement.blocks) {
            ast::named_type t = std::invoke(*this, block);
            if (t != type) {
                error(if_statement.loc, "type mismatch between if statement blocks");
            }
            type = t;
        }
        if_statement.type = type;
        return type;
    }
    ast::named_type operator()(std::unique_ptr<ast::for_loop>& for_loop) {
        return std::invoke(*this, *for_loop);
    }
    ast::named_type operator()(ast::for_loop& for_loop) {
        context.variable_scopes.push_scope();
        std::invoke(*this, for_loop.initial);
        if (std::invoke(*this, for_loop.condition) != ast::named_type{ast::primitive_type{ast::primitive_type::t_bool}}) {
            error(for_loop.loc, "for loop condition not a boolean");
        }
        std::invoke(*this, for_loop.block);
        std::invoke(*this, for_loop.step);
        context.variable_scopes.pop_scope();
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(std::unique_ptr<ast::while_loop>& while_loop) {
        return std::invoke(*this, *while_loop);
    }
    ast::named_type operator()(ast::while_loop& while_loop) {
        if (std::invoke(*this, while_loop.condition) != ast::named_type{ast::primitive_type{ast::primitive_type::t_bool}}) {
            error(while_loop.loc, "while loop condition not a boolean");
        }
        std::invoke(*this, while_loop.block);
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(std::unique_ptr<ast::switch_statement>& switch_statement) {
        return std::invoke(*this, *switch_statement);
    }
    ast::named_type operator()(ast::switch_statement& switch_statement) {
        ast::named_type switch_type = std::invoke(*this, switch_statement.expression);
        if (!switch_type.is_integer()) {
            error(switch_statement.loc, "switch statement switch expression is not an integer");
        }
        ast::named_type type = {ast::primitive_type{ast::primitive_type::t_void}};
        for (auto& case_statement: switch_statement.cases) {
            for (auto& case_exp: case_statement.cases) {
                ast::named_type case_type = std::invoke(*this, case_exp);
                if (!case_type.is_integer()) {
                    error(switch_statement.loc, "switch statement switch expression is not an integer");
                }
                if (case_type != switch_type) {
                    error(switch_statement.loc, "type mismatch between switch expression and case expression");
                }
            }
            ast::named_type t = std::invoke(*this, case_statement.block);
            if (t != type) {
                error(switch_statement.loc, "type mismatch between switch statement blocks");
            }
            type = t;
        }
        switch_statement.type = type;
        return type;
    }
    ast::named_type operator()(ast::function_def& function_def) {
        auto v = context.variable_scopes.find_item(function_def.identifier);
        if (v.has_value()) {
            error(function_def.loc, "function already defined");
        }
        context.variable_scopes.push_item(function_def.identifier, std::move(function_def.returntype));
        context.current_function_returntype = function_def.returntype;
        context.variable_scopes.push_scope();
        for (auto& parameter: function_def.parameter_list) {
            context.variable_scopes.push_item(parameter.identifier, std::move(parameter.type));
        }
        std::invoke(*this, function_def.block);
        context.variable_scopes.pop_scope();
        std::vector<ast::named_type> types;
        for (auto& parameter: function_def.parameter_list) {
            types.push_back(parameter.type);
        }
        context.function_parameter_types.insert(function_def.identifier, types);
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::type_def& type_def) {
        auto t = context.type_scopes.find_item_current_scope(type_def.user_type);
        if (t.has_value()) {
            error(type_def.loc, "type already defined in this scope");
        }
        context.type_scopes.push_item(type_def.user_type, std::move(type_def.type));
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::s_return& s_return) {
        ast::named_type x = s_return.expression ? std::invoke(*this, *s_return.expression) : ast::named_type{ast::primitive_type{ast::primitive_type::t_void}};
        if (x != context.current_function_returntype) {
            error(s_return.loc, "return type does not match defined function return type");
        }
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::s_break& s_break) {
        return s_break.expression ? std::invoke(*this, *s_break.expression) : ast::named_type{ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::s_continue& s_continue) {
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::variable_def& variable_def) {
        auto v = context.variable_scopes.find_item_current_scope(variable_def.identifier);
        if (v.has_value()) {
            error(variable_def.loc, "variable already defined in this scope");
        }
        ast::named_type t = std::invoke(*this, variable_def.expression);
        if (variable_def.explicit_type && variable_def.explicit_type != t) {
            error(variable_def.loc, "type mismatch in variable definition");
        }
        context.variable_scopes.push_item(variable_def.identifier, std::move(t));
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }
    ast::named_type operator()(ast::assignment& assignment) {
        ast::named_type access = accessor_access(context, assignment.accessor);
        ast::named_type value = std::invoke(*this, assignment.expression);
        if (value != access) {
            error(assignment.loc, "type mismatch in assignment");
        }
        return {ast::primitive_type{ast::primitive_type::t_void}};
    }

    ast::named_type operator()(ast::expression& expression) {
        ast::named_type type = std::visit(*this, expression.expression);
        expression.type = type;
        return type;
    }
    ast::named_type operator()(ast::identifier& identifier) {
        auto v = context.variable_scopes.find_item(identifier);
        if (!v.has_value()) {
            error("variable used before being defined");
        }
        return *v;
    }
    ast::named_type operator()(ast::literal& literal) {
        struct literal_visitor {
            typecheck_context& context;
            std::optional<ast::named_type> explicit_type;
            yy::location& loc;
            ast::named_type operator()(double& x) {
                if (!explicit_type) {
                    return {ast::primitive_type{ast::primitive_type::f32}};
                }
                if (explicit_type.value().is_float()) {
                    return explicit_type.value();
                } else if (explicit_type.value().is_integer()) {
                    error(loc, "redundant values after decimal point in floating point literal converted to integer type");
                    assert(false);
                } else {
                    error(loc, "floating point literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::named_type operator()(ast::literal_integer& x) {
                if (!explicit_type) {
                    return {ast::primitive_type{ast::primitive_type::i32}};
                }
                if (explicit_type.value().is_number()) {
                    return explicit_type.value();
                } else {
                    error(loc, "integer literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::named_type operator()(bool& x) {
                if (!explicit_type) {
                    return {ast::primitive_type{ast::primitive_type::t_bool}};
                }
                if (explicit_type.value().is_bool()) {
                    return {ast::primitive_type{ast::primitive_type::t_bool}};
                } else {
                    error(loc, "bool literal cannot be converted to non bool type");
                    assert(false);
                }
            }
        };
        ast::named_type type = std::visit(literal_visitor{context, literal.explicit_type, literal.loc}, literal.literal);
        literal.type = type;
        return type;
    }
    ast::named_type operator()(ast::accessor& accessor) {
        ast::named_type type = accessor_access(context, accessor);
        accessor.type = type;
        return type;
    }
    ast::named_type operator()(std::unique_ptr<ast::accessor>& accessor) {
        return std::invoke(*this, *accessor);;
    }
    ast::named_type operator()(std::unique_ptr<ast::function_call>& function_call) {
        std::vector<ast::named_type> function_parameter_type;
        for (auto& argument: function_call->arguments) {
            function_parameter_type.push_back(std::invoke(*this, argument));
        }
        if (function_parameter_type != context.function_parameter_types.get(function_call->identifier)) {
            error(function_call->loc, "type mismatch between function call parameters and function definition arguments");
        }
        auto v = context.variable_scopes.find_item(function_call->identifier);
        if (!v.has_value()) {
            error(function_call->loc, "function called before being defined");
        }
        ast::named_type type = *v;
        function_call->type = type;
        return type;
    }
    ast::named_type operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        ast::primitive_type l = std::get<ast::primitive_type>(std::invoke(*this, binary_operator->l).type);
        ast::primitive_type r = std::get<ast::primitive_type>(std::invoke(*this, binary_operator->r).type);
        //TODO
        //user defined operators on user defined types
        ast::named_type type;
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
            case ast::binary_operator::A_SUB:
            case ast::binary_operator::A_MUL:
            case ast::binary_operator::A_DIV:
            case ast::binary_operator::A_MOD:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of arithmetic operator are not of the same type");
                }
                if (!l.is_number()) {
                    error(binary_operator->loc, "LHS and RHS of arithmetic operator are not numbers");
                }
                type = {l};
                break;
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
                if (!l.is_integer()) {
                    error(binary_operator->loc, "LHS of shift operator is not an integer");
                }
                if (!r.is_integer()) {
                    error(binary_operator->loc, "RHS of shift operator is not an integer");
                }
                type = {l};
                break;
            case ast::binary_operator::B_AND:
            case ast::binary_operator::B_XOR:
            case ast::binary_operator::B_OR:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of bitwise operator are not of the same type");
                }
                if (!l.is_integer()) {
                    error(binary_operator->loc, "LHS of bitwise operator is not an integer");
                }
                if (!r.is_integer()) {
                    error(binary_operator->loc, "RHS of bitwise operator is not an integer");
                }
                type = {l};
                break;
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
                if (!l.is_bool()) {
                    error(binary_operator->loc, "LHS of logical operator is not a boolean");
                }
                if (!r.is_bool()) {
                    error(binary_operator->loc, "RHS of logical operator is not a boolean");
                }
                type = {l};
                break;
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of comparison operator are not of the same type. have", l.to_string(), "and", r.to_string());
                }
                type = {ast::primitive_type{ast::primitive_type::t_bool}};
                break;
            case ast::binary_operator::C_GT:
            case ast::binary_operator::C_GE:
            case ast::binary_operator::C_LT:
            case ast::binary_operator::C_LE:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of comparison operator are not of the same type. have", l.to_string(), "and", r.to_string());
                }
                if (!l.is_number()) {
                    error(binary_operator->loc, "LHS and RHS of comparison operator are not numbers");
                }
                type = {ast::primitive_type{ast::primitive_type::t_bool}};
                break;
        }
        binary_operator->type = type;
        return type;
    }
    ast::named_type operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        ast::primitive_type r = std::get<ast::primitive_type>(std::invoke(*this, unary_operator->r).type);
        //TODO
        //user defined operators on user defined types
        ast::named_type type;
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                if (!r.is_integer()) {
                    error(unary_operator->loc, "RHS of bitwise negation is not an integer");
                }
                type = {r};
                break;
            case ast::unary_operator::L_NOT:
                if (!r.is_bool()) {
                    error(unary_operator->loc, "RHS of bitwise negation is not boolean");
                }
                type = {r};
                break;
        }
        unary_operator->type = type;
        return type;
    }
};

void typecheck(typecheck_context &context, ast::program &program) {
    std::invoke(typecheck_fn{context}, program);
}
