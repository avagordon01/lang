#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

#include "typecheck.hh"
#include "ast.hh"
#include "error.hh"

struct typecheck_fn {
    typecheck_context& context;
    ast::type operator()(ast::program& program) {
        context.scopes.push_back({});
        for (auto& statement: program.statements) {
            std::invoke(*this, statement);
        }
        context.scopes.pop_back();
        return ast::type::t_void;
    }
    ast::type operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    ast::type operator()(std::unique_ptr<ast::block>& block) {
        return std::invoke(*this, *block);
    }
    ast::type operator()(ast::block& block) {
        ast::type type = ast::type::t_void;
        context.scopes.push_back({});
        for (auto& statement: block.statements) {
            type = std::invoke(*this, statement);
        }
        context.scopes.pop_back();
        block.type = type;
        return type;
    }
    ast::type operator()(std::unique_ptr<ast::if_statement>& if_statement) {
        return std::invoke(*this, *if_statement);
    }
    ast::type operator()(ast::if_statement& if_statement) {
        for (auto& condition: if_statement.conditions) {
            if (std::invoke(*this, condition) != ast::type::t_bool) {
                error("if statement condition not a boolean");
            }
        }
        ast::type type = std::invoke(*this, if_statement.blocks.front());
        for (auto& block: if_statement.blocks) {
            ast::type t = std::invoke(*this, block);
            if (t != type) {
                error("type mismatch between if statement blocks");
            }
            type = t;
        }
        if_statement.type = type;
        return type;
    }
    ast::type operator()(std::unique_ptr<ast::for_loop>& for_loop) {
        return std::invoke(*this, *for_loop);
    }
    ast::type operator()(ast::for_loop& for_loop) {
        context.scopes.push_back({});
        std::invoke(*this, for_loop.initial);
        if (std::invoke(*this, for_loop.condition) != ast::type::t_bool) {
            error("for loop condition not a boolean");
        }
        std::invoke(*this, for_loop.block);
        std::invoke(*this, for_loop.step);
        context.scopes.pop_back();
        return ast::type::t_void;
    }
    ast::type operator()(std::unique_ptr<ast::while_loop>& while_loop) {
        return std::invoke(*this, *while_loop);
    }
    ast::type operator()(ast::while_loop& while_loop) {
        if (std::invoke(*this, while_loop.condition) != ast::type::t_bool) {
            error("while loop condition not a boolean");
        }
        std::invoke(*this, while_loop.block);
        return ast::type::t_void;
    }
    ast::type operator()(std::unique_ptr<ast::switch_statement>& switch_statement) {
        return std::invoke(*this, *switch_statement);
    }
    ast::type operator()(ast::switch_statement& switch_statement) {
        ast::type switch_type = std::invoke(*this, switch_statement.expression);
        if (!type_is_integer(switch_type)) {
            error("switch statement switch expression is not an integer");
        }
        ast::type type = ast::type::t_void;
        for (auto& case_statement: switch_statement.cases) {
            for (auto& case_exp: case_statement.cases) {
                ast::type case_type = std::invoke(*this, case_exp);
                if (!type_is_integer(case_type)) {
                    error("switch statement switch expression is not an integer");
                }
                if (case_type != switch_type) {
                    error("type mismatch between switch expression and case expression");
                }
            }
            ast::type t = std::invoke(*this, case_statement.block);
            if (t != type) {
                error("type mismatch between switch statement blocks");
            }
            type = t;
        }
        switch_statement.type = type;
        return type;
    }
    ast::type operator()(ast::function_def& function_def) {
        auto v = context.scopes.front().find(function_def.identifier);
        if (v != context.scopes.front().end()) {
            error("function already defined");
        }
        context.scopes.front()[function_def.identifier] = function_def.returntype;
        context.current_function_returntype = function_def.returntype;
        context.scopes.push_back({});
        for (auto& parameter: function_def.parameter_list) {
            context.scopes.back()[parameter.identifier] = parameter.type;
        }
        std::invoke(*this, function_def.block);
        context.scopes.pop_back();
        for (auto& parameter: function_def.parameter_list) {
            context.function_parameter_types[function_def.identifier].push_back(parameter.type);
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::struct_def& struct_def) {
        //TODO
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_return& s_return) {
        ast::type x = s_return.expression ? std::invoke(*this, *s_return.expression) : ast::type::t_void;
        if (x != context.current_function_returntype) {
            error("return type does not match defined function return type");
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_break& s_break) {
        return s_break.expression ? std::invoke(*this, *s_break.expression) : ast::type::t_void;
    }
    ast::type operator()(ast::s_continue& s_continue) {
        return ast::type::t_void;
    }
    ast::type operator()(ast::variable_def& variable_def) {
        auto v = context.scopes.back().find(variable_def.identifier);
        if (v != context.scopes.back().end()) {
            error("variable already defined in this scope");
        }
        ast::type t = std::invoke(*this, variable_def.expression);
        if (variable_def.explicit_type && variable_def.explicit_type != t) {
            error("type mismatch in variable definition");
        }
        context.scopes.back()[variable_def.identifier] = t;
        return ast::type::t_void;
    }
    ast::type operator()(ast::assignment& assignment) {
        ast::type variable;
        auto it = context.scopes.rbegin();
        for (it = context.scopes.rbegin(); it != context.scopes.rend(); ++it) {
            auto v = it->find(assignment.identifier);
            if (v != it->end()) {
                variable = v->second;
                break;
            }
        }
        if (it == context.scopes.rend()) {
            error("variable used before being defined");
        }
        ast::type value = std::invoke(*this, assignment.expression);
        if (value != variable) {
            error("type mismatch in assignment");
        }
        return ast::type::t_void;
    }

    ast::type operator()(ast::expression& expression) {
        ast::type type = std::visit(*this, expression.expression);
        expression.type = type;
        return type;
    }
    ast::type operator()(ast::identifier& identifier) {
        ast::type variable;
        auto it = context.scopes.rbegin();
        for (it = context.scopes.rbegin(); it != context.scopes.rend(); ++it) {
            auto v = it->find(identifier);
            if (v != it->end()) {
                variable = v->second;
                break;
            }
        }
        if (it == context.scopes.rend()) {
            error("variable used before being defined");
        }
        return variable;
    }
    ast::type operator()(ast::literal& literal) {
        struct literal_visitor {
            typecheck_context& context;
            std::optional<ast::type>& explicit_type;
            ast::type operator()(double& x) {
                if (!explicit_type) {
                    *explicit_type = ast::type::f32;
                    return *explicit_type;
                }
                if (ast::type_is_float(*explicit_type)) {
                    return *explicit_type;
                } else if (ast::type_is_integer(*explicit_type)) {
                    error("redundant values after decimal point in floating point literal converted to integer type");
                    assert(false);
                } else {
                    error("floating point literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::type operator()(uint64_t& x) {
                if (!explicit_type) {
                    *explicit_type = ast::type::i32;
                    return *explicit_type;
                }
                if (ast::type_is_number(*explicit_type)) {
                    return *explicit_type;
                } else {
                    error("integer literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::type operator()(bool& x) {
                if (!explicit_type) {
                    *explicit_type = ast::type::t_bool;
                    return *explicit_type;
                }
                if (ast::type_is_bool(*explicit_type)) {
                    return ast::type::t_bool;
                } else {
                    error("bool literal cannot be converted to non bool type");
                    assert(false);
                }
            }
        };
        ast::type type = std::visit(literal_visitor{context, literal.explicit_type}, literal.literal);
        literal.type = type;
        return type;
    }
    ast::type operator()(std::unique_ptr<ast::function_call>& function_call) {
        std::vector<ast::type> function_parameter_type;
        for (auto& argument: function_call->arguments) {
            function_parameter_type.push_back(std::invoke(*this, argument));
        }
        if (function_parameter_type != context.function_parameter_types[function_call->identifier]) {
            error("type mismatch between function call parameters and function definition arguments");
        }
        auto v = context.scopes.front().find(function_call->identifier);
        if (v == context.scopes.front().end()) {
            error("function called before being defined");
        }
        ast::type type = v->second;
        function_call->type = type;
        return type;
    }
    ast::type operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        ast::type l = std::invoke(*this, binary_operator->l);
        ast::type r = std::invoke(*this, binary_operator->r);
        ast::type type;
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
            case ast::binary_operator::A_SUB:
            case ast::binary_operator::A_MUL:
            case ast::binary_operator::A_DIV:
            case ast::binary_operator::A_MOD:
                if (l != r) {
                    error("LHS and RHS of arithmetic operator are not of the same type");
                }
                if (!ast::type_is_number(l)) {
                    error("LHS and RHS of arithmetic operator are not numbers");
                }
                type = l;
                break;
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
                if (!ast::type_is_integer(l)) {
                    error("LHS of shift operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error("RHS of shift operator is not an integer");
                }
                type = l;
                break;
            case ast::binary_operator::B_AND:
            case ast::binary_operator::B_XOR:
            case ast::binary_operator::B_OR:
                if (l != r) {
                    error("LHS and RHS of bitwise operator are not of the same type");
                }
                if (!ast::type_is_integer(l)) {
                    error("LHS of bitwise operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error("RHS of bitwise operator is not an integer");
                }
                type = l;
                break;
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
                if (!ast::type_is_bool(l)) {
                    error("LHS of logical operator is not a boolean");
                }
                if (!ast::type_is_bool(r)) {
                    error("RHS of logical operator is not a boolean");
                }
                type = l;
                break;
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
                if (l != r) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                type = ast::type::t_bool;
                break;
            case ast::binary_operator::C_GT:
            case ast::binary_operator::C_GE:
            case ast::binary_operator::C_LT:
            case ast::binary_operator::C_LE:
                if (l != r) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                if (!ast::type_is_number(l)) {
                    error("LHS and RHS of comparison operator are not numbers");
                }
                type = ast::type::t_bool;
                break;
        }
        binary_operator->type = type;
        return type;
    }
    ast::type operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        ast::type r = std::invoke(*this, unary_operator->r);
        ast::type type;
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                if (!ast::type_is_integer(r)) {
                    error("RHS of bitwise negation is not an integer");
                }
                type = r;
                break;
            case ast::unary_operator::L_NOT:
                if (!ast::type_is_bool(r)) {
                    error("RHS of bitwise negation is not boolean");
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
