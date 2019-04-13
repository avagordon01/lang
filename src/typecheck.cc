#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

#include "typecheck.hh"
#include "ast.hh"
#include "error.hh"

ast::type_id accessor_access(typecheck_context& context, ast::accessor& accessor) {
    std::optional<ast::type_id> v = context.variable_scopes.find_item(accessor.identifier);
    if (!v.has_value()) {
        error(accessor.loc, "variable used before being defined");
    }
    return *v;
}

struct typecheck_fn {
    typecheck_context& context;
    ast::type_id operator()(ast::program& program) {
        context.variable_scopes.push_scope();
        for (auto& statement: program.statements) {
            std::invoke(*this, statement);
        }
        context.variable_scopes.pop_scope();
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    ast::type_id operator()(std::unique_ptr<ast::block>& block) {
        return std::invoke(*this, *block);
    }
    ast::type_id operator()(ast::block& block) {
        ast::type_id type = {ast::primitive_type::t_void};
        context.variable_scopes.push_scope();
        for (auto& statement: block.statements) {
            type = std::invoke(*this, statement);
        }
        context.variable_scopes.pop_scope();
        block.type = type;
        return type;
    }
    ast::type_id operator()(std::unique_ptr<ast::if_statement>& if_statement) {
        return std::invoke(*this, *if_statement);
    }
    ast::type_id operator()(ast::if_statement& if_statement) {
        for (auto& condition: if_statement.conditions) {
            if (std::invoke(*this, condition) != ast::primitive_type{ast::primitive_type::t_bool}) {
                error(if_statement.loc, "if statement condition not a boolean");
            }
        }
        ast::type_id type = std::invoke(*this, if_statement.blocks.front());
        for (auto& block: if_statement.blocks) {
            ast::type_id t = std::invoke(*this, block);
            if (t != type) {
                error(if_statement.loc, "type mismatch between if statement blocks");
            }
            type = t;
        }
        if_statement.type = type;
        return type;
    }
    ast::type_id operator()(std::unique_ptr<ast::for_loop>& for_loop) {
        return std::invoke(*this, *for_loop);
    }
    ast::type_id operator()(ast::for_loop& for_loop) {
        context.variable_scopes.push_scope();
        std::invoke(*this, for_loop.initial);
        if (std::invoke(*this, for_loop.condition) != ast::primitive_type{ast::primitive_type::t_bool}) {
            error(for_loop.loc, "for loop condition not a boolean");
        }
        std::invoke(*this, for_loop.block);
        std::invoke(*this, for_loop.step);
        context.variable_scopes.pop_scope();
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(std::unique_ptr<ast::while_loop>& while_loop) {
        return std::invoke(*this, *while_loop);
    }
    ast::type_id operator()(ast::while_loop& while_loop) {
        if (std::invoke(*this, while_loop.condition) != ast::primitive_type{ast::primitive_type::t_bool}) {
            error(while_loop.loc, "while loop condition not a boolean");
        }
        std::invoke(*this, while_loop.block);
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(std::unique_ptr<ast::switch_statement>& switch_statement) {
        return std::invoke(*this, *switch_statement);
    }
    ast::type_id operator()(ast::switch_statement& switch_statement) {
        ast::type_id switch_type = std::invoke(*this, switch_statement.expression);
        if (!type_is_integer(switch_type)) {
            error(switch_statement.loc, "switch statement switch expression is not an integer");
        }
        ast::type_id type = {ast::primitive_type::t_void};
        for (auto& case_statement: switch_statement.cases) {
            for (auto& case_exp: case_statement.cases) {
                ast::type_id case_type = std::invoke(*this, case_exp);
                if (!type_is_integer(case_type)) {
                    error(switch_statement.loc, "switch statement switch expression is not an integer");
                }
                if (case_type != switch_type) {
                    error(switch_statement.loc, "type mismatch between switch expression and case expression");
                }
            }
            ast::type_id t = std::invoke(*this, case_statement.block);
            if (t != type) {
                error(switch_statement.loc, "type mismatch between switch statement blocks");
            }
            type = t;
        }
        switch_statement.type = type;
        return type;
    }
    ast::type_id operator()(ast::function_def& function_def) {
        auto v = context.variable_scopes.find_item(function_def.identifier);
        if (v.has_value()) {
            error(function_def.loc, "function already defined");
        }
        context.variable_scopes.push_item(function_def.identifier, function_def.returntype);
        context.current_function_returntype = function_def.returntype;
        context.variable_scopes.push_scope();
        for (auto& parameter: function_def.parameter_list) {
            context.variable_scopes.push_item(parameter.identifier, parameter.type);
        }
        std::invoke(*this, function_def.block);
        context.variable_scopes.pop_scope();
        for (auto& parameter: function_def.parameter_list) {
            context.function_parameter_types[function_def.identifier].push_back(parameter.type);
        }
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::type_def& type_def) {
        //TODO
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::s_return& s_return) {
        ast::type_id x = s_return.expression ? std::invoke(*this, *s_return.expression) : ast::type_id{ast::primitive_type::t_void};
        if (x != context.current_function_returntype) {
            error(s_return.loc, "return type does not match defined function return type");
        }
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::s_break& s_break) {
        return s_break.expression ? std::invoke(*this, *s_break.expression) : ast::type_id{ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::s_continue& s_continue) {
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::variable_def& variable_def) {
        auto v = context.variable_scopes.find_item_current_scope(variable_def.identifier);
        if (v.has_value()) {
            error(variable_def.loc, "variable already defined in this scope");
        }
        ast::type_id t = std::invoke(*this, variable_def.expression);
        if (variable_def.explicit_type && variable_def.explicit_type != t) {
            error(variable_def.loc, "type mismatch in variable definition");
        }
        context.variable_scopes.push_item(variable_def.identifier, t);
        return {ast::primitive_type::t_void};
    }
    ast::type_id operator()(ast::assignment& assignment) {
        ast::type_id access = accessor_access(context, assignment.accessor);
        ast::type_id value = std::invoke(*this, assignment.expression);
        if (value != access) {
            error(assignment.loc, "type mismatch in assignment");
        }
        return {ast::primitive_type::t_void};
    }

    ast::type_id operator()(ast::expression& expression) {
        ast::type_id type = std::visit(*this, expression.expression);
        expression.type = type;
        return type;
    }
    ast::type_id operator()(ast::identifier& identifier) {
        auto v = context.variable_scopes.find_item(identifier);
        if (!v.has_value()) {
            error("variable used before being defined");
        }
        return *v;
    }
    ast::type_id operator()(ast::literal& literal) {
        struct literal_visitor {
            typecheck_context& context;
            std::optional<ast::type_id>& explicit_type;
            yy::location& loc;
            ast::type_id operator()(double& x) {
                if (!explicit_type) {
                    *explicit_type = ast::primitive_type::f32;
                    return *explicit_type;
                }
                if (ast::type_is_float(*explicit_type)) {
                    return *explicit_type;
                } else if (ast::type_is_integer(*explicit_type)) {
                    error(loc, "redundant values after decimal point in floating point literal converted to integer type");
                    assert(false);
                } else {
                    error(loc, "floating point literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::type_id operator()(uint64_t& x) {
                if (!explicit_type) {
                    *explicit_type = ast::primitive_type::i32;
                    return *explicit_type;
                }
                if (ast::type_is_number(*explicit_type)) {
                    return *explicit_type;
                } else {
                    error(loc, "integer literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::type_id operator()(bool& x) {
                if (!explicit_type) {
                    *explicit_type = {ast::primitive_type::t_bool};
                    return *explicit_type;
                }
                if (ast::type_is_bool(*explicit_type)) {
                    return {ast::primitive_type::t_bool};
                } else {
                    error(loc, "bool literal cannot be converted to non bool type");
                    assert(false);
                }
            }
        };
        ast::type_id type = std::visit(literal_visitor{context, literal.explicit_type, literal.loc}, literal.literal);
        literal.type = type;
        return type;
    }
    ast::type_id operator()(ast::accessor& accessor) {
        ast::type_id type = accessor_access(context, accessor);
        accessor.type = type;
        return type;
    }
    ast::type_id operator()(std::unique_ptr<ast::accessor>& accessor) {
        return std::invoke(*this, *accessor);;
    }
    ast::type_id operator()(std::unique_ptr<ast::function_call>& function_call) {
        std::vector<ast::type_id> function_parameter_type;
        for (auto& argument: function_call->arguments) {
            function_parameter_type.push_back(std::invoke(*this, argument));
        }
        if (function_parameter_type != context.function_parameter_types[function_call->identifier]) {
            error(function_call->loc, "type mismatch between function call parameters and function definition arguments");
        }
        auto v = context.variable_scopes.find_item(function_call->identifier);
        if (!v.has_value()) {
            error(function_call->loc, "function called before being defined");
        }
        ast::type_id type = *v;
        function_call->type = type;
        return type;
    }
    ast::type_id operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        ast::type_id l = std::invoke(*this, binary_operator->l);
        ast::type_id r = std::invoke(*this, binary_operator->r);
        ast::type_id type;
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
            case ast::binary_operator::A_SUB:
            case ast::binary_operator::A_MUL:
            case ast::binary_operator::A_DIV:
            case ast::binary_operator::A_MOD:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of arithmetic operator are not of the same type");
                }
                if (!ast::type_is_number(l)) {
                    error(binary_operator->loc, "LHS and RHS of arithmetic operator are not numbers");
                }
                type = l;
                break;
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
                if (!ast::type_is_integer(l)) {
                    error(binary_operator->loc, "LHS of shift operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error(binary_operator->loc, "RHS of shift operator is not an integer");
                }
                type = l;
                break;
            case ast::binary_operator::B_AND:
            case ast::binary_operator::B_XOR:
            case ast::binary_operator::B_OR:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of bitwise operator are not of the same type");
                }
                if (!ast::type_is_integer(l)) {
                    error(binary_operator->loc, "LHS of bitwise operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error(binary_operator->loc, "RHS of bitwise operator is not an integer");
                }
                type = l;
                break;
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
                if (!ast::type_is_bool(l)) {
                    error(binary_operator->loc, "LHS of logical operator is not a boolean");
                }
                if (!ast::type_is_bool(r)) {
                    error(binary_operator->loc, "RHS of logical operator is not a boolean");
                }
                type = l;
                break;
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of comparison operator are not of the same type. have", ast::type_to_string(l), "and", ast::type_to_string(r));
                }
                type = {ast::primitive_type::t_bool};
                break;
            case ast::binary_operator::C_GT:
            case ast::binary_operator::C_GE:
            case ast::binary_operator::C_LT:
            case ast::binary_operator::C_LE:
                if (l != r) {
                    error(binary_operator->loc, "LHS and RHS of comparison operator are not of the same type. have", ast::type_to_string(l), "and", ast::type_to_string(r));
                }
                if (!ast::type_is_number(l)) {
                    error(binary_operator->loc, "LHS and RHS of comparison operator are not numbers");
                }
                type = {ast::primitive_type::t_bool};
                break;
        }
        binary_operator->type = type;
        return type;
    }
    ast::type_id operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        ast::type_id r = std::invoke(*this, unary_operator->r);
        ast::type_id type;
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                if (!ast::type_is_integer(r)) {
                    error(unary_operator->loc, "RHS of bitwise negation is not an integer");
                }
                type = r;
                break;
            case ast::unary_operator::L_NOT:
                if (!ast::type_is_bool(r)) {
                    error(unary_operator->loc, "RHS of bitwise negation is not boolean");
                }
                type = r;
                break;
        }
        unary_operator->type = type;
        return type;
    }
};

void typecheck(typecheck_context &context, ast::program &program) {
    std::invoke(typecheck_fn{context}, program);
}
