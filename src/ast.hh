#pragma once

#include <cstdint>

#include <vector>
#include <optional>
#include <memory>
#include <variant>

#include "types.hh"

namespace ast {
    using identifier = size_t;

    struct literal {
        std::variant<double, uint64_t, bool> literal;
    };
    struct binary_operator;
    struct unary_operator;
    struct expression {
        std::variant<
            ast::identifier,
            ast::literal,
            std::unique_ptr<ast::binary_operator>,
            std::unique_ptr<ast::unary_operator>
        > expression;
    };
    struct binary_operator {
        enum op {
            A_ADD, A_SUB, A_MUL, A_DIV, A_MOD,
            B_SHL, B_SHR, B_AND, B_XOR, B_OR,
            L_AND, L_OR,
            C_EQ, C_NE, C_GT, C_GE, C_LT, C_LE,
        } binary_operator;
        ast::expression l, r;
    };
    struct unary_operator {
        enum op {
            B_NOT, L_NOT,
        } unary_operator;
        ast::expression r;
    };

    struct statement;
    struct block {
        std::vector<ast::statement> statements;
    };
    struct if_statement {
        std::vector<ast::expression> conditions;
        std::vector<ast::block> blocks;
    };
    using optional_else = std::optional<ast::block>;
    struct else_if_list {
        std::vector<ast::expression> conditions;
        std::vector<ast::block> blocks;
    };
    struct for_loop {
        ast::expression initial;
        ast::expression condition;
        ast::expression step;
        ast::block block;
    };
    struct while_loop {
        ast::expression condition;
        ast::block block;
    };
    using parameter_list = std::vector<std::pair<ast::type, ast::identifier>>;
    struct function {
        ast::type returntype;
        ast::parameter_list parameter_list;
        ast::block block;
    };
    struct assignment {
        ast::identifier identifier;
        ast::expression expression;
    };
    struct s_return {
        ast::expression expression;
    };
    struct s_break {
        ast::expression expression;
    };
    struct s_continue {};
    struct statement {
        std::variant<
            ast::block,
            ast::if_statement,
            ast::for_loop,
            ast::while_loop,
            ast::function,
            ast::assignment,
            ast::s_return,
            ast::s_break,
            ast::s_continue
        > statement;
    };
    using statement_list = std::vector<ast::statement>;
    struct program {
        ast::statement_list statements;
    };
}
