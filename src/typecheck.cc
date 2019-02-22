#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

#include "typecheck.hh"
#include "ast.hh"

#include <string>
#include <iostream>
static void error(std::string s) {
    std::cerr << s << std::endl;
    exit(EXIT_FAILURE);
}

struct typecheck_fn {
    typecheck_context& context;
    ast::type operator()(ast::program& program) {
        context.scopes.push_back({});
        for (auto& statement: program.statements) {
            std::invoke(*this, statement);
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    ast::type operator()(ast::block& block) {
        for (auto& statement: block.statements) {
            std::invoke(*this, statement);
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::if_statement& if_statement) {
        for (auto& condition: if_statement.conditions) {
            if (std::invoke(*this, condition) != ast::type::t_bool) {
                error("if statement condition not a boolean");
            }
        }
        for (auto& block: if_statement.blocks) {
            context.scopes.push_back({});
            std::invoke(*this, block);
            context.scopes.pop_back();
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::for_loop& for_loop) {
        context.scopes.push_back({});
        std::invoke(*this, for_loop.initial);
        if (std::invoke(*this, for_loop.condition) != ast::type::t_bool) {
            error("for loop condition not a boolean");
        }
        std::invoke(*this, for_loop.block);
        context.scopes.pop_back();
        return ast::type::t_void;
    }
    ast::type operator()(ast::while_loop& while_loop) {
        if (std::invoke(*this, while_loop.condition) != ast::type::t_bool) {
            error("while loop condition not a boolean");
        }
        context.scopes.push_back({});
        std::invoke(*this, while_loop.block);
        context.scopes.pop_back();
        return ast::type::t_void;
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
        //TODO put the signature somewhere to typecheck function calls
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_return& s_return) {
        ast::type x = std::invoke(*this, s_return.expression);
        if (x != context.current_function_returntype) {
            error("return type does not match defined function return type");
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_break& s_break) {
        return ast::type::t_void;
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
        if (variable_def.type && variable_def.type != t) {
            error("type mismatch in variable definition");
        }
        variable_def.type = t;
        context.scopes.back()[variable_def.identifier] = *variable_def.type;
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
        return std::visit(*this, expression.expression);
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
            std::optional<ast::type>& type;
            ast::type operator()(double& x) {
                if (!type) {
                    *type = ast::type::f32;
                    return *type;
                }
                if (ast::type_is_float(*type)) {
                    return *type;
                } else if (ast::type_is_integer(*type)) {
                    error("redundant values after decimal point in floating point literal converted to integer type");
                    assert(false);
                } else {
                    error("floating point literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::type operator()(uint64_t& x) {
                if (!type) {
                    *type = ast::type::i32;
                    return *type;
                }
                if (ast::type_is_number(*type)) {
                    return *type;
                } else {
                    error("integer literal cannot be converted to non number type");
                    assert(false);
                }
            }
            ast::type operator()(bool& x) {
                if (!type) {
                    *type = ast::type::t_bool;
                    return *type;
                }
                if (ast::type_is_bool(*type)) {
                    return ast::type::t_bool;
                } else {
                    error("bool literal cannot be converted to non bool type");
                    assert(false);
                }
            }
        };
        return std::visit(literal_visitor{context, literal.type}, literal.literal);
    }
    ast::type operator()(std::unique_ptr<ast::function_call>& function_call) {
        //TODO check the function signature
        auto v = context.scopes.front().find(function_call->identifier);
        if (v == context.scopes.front().end()) {
            error("function called before being defined");
        }
        return v->second;
    }
    ast::type operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        ast::type l = std::invoke(*this, binary_operator->l);
        ast::type r = std::invoke(*this, binary_operator->r);
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
                return l;
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
                if (!ast::type_is_integer(l)) {
                    error("LHS of shift operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error("RHS of shift operator is not an integer");
                }
                return l;
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
                return l;
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
                if (!ast::type_is_bool(l)) {
                    error("LHS of logical operator is not a boolean");
                }
                if (!ast::type_is_bool(r)) {
                    error("RHS of logical operator is not a boolean");
                }
                return l;
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
                if (l != r) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                return ast::type::t_bool;
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
                return ast::type::t_bool;
        }
        assert(false);
    }
    ast::type operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        ast::type r = std::invoke(*this, unary_operator->r);
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                if (!ast::type_is_integer(r)) {
                    error("RHS of bitwise negation is not an integer");
                }
                return r;
            case ast::unary_operator::L_NOT:
                if (!ast::type_is_bool(r)) {
                    error("RHS of bitwise negation is not boolean");
                }
                return r;
        }
        assert(false);
    }
};

void typecheck(typecheck_context &context, ast::program &program) {
    std::invoke(typecheck_fn{context}, program);
}
