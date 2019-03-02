#pragma once

#include <cstdint>

#include <vector>
#include <optional>
#include <memory>
#include <variant>

#include "types.hh"

namespace ast {
    struct function_call;
    struct binary_operator;
    struct unary_operator;
    struct statement;

    using identifier = size_t;

    using raw_literal = std::variant<double, uint64_t, bool>;
    struct literal {
        raw_literal literal;
        std::optional<ast::type> type;
    };
    struct expression {
        std::variant<
            ast::identifier,
            ast::literal,
            std::unique_ptr<ast::function_call>,
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
        bool is_unsigned;
    };
    struct unary_operator {
        enum op {
            B_NOT, L_NOT,
        } unary_operator;
        ast::expression r;
    };

    using statement_list = std::vector<ast::statement>;
    struct block {
        statement_list statements;
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
    struct variable_def {
        std::optional<ast::type> type;
        ast::identifier identifier;
        ast::expression expression;
    };
    struct assignment {
        ast::identifier identifier;
        ast::expression expression;
    };
    struct for_loop {
        ast::variable_def initial;
        ast::expression condition;
        ast::assignment step;
        ast::block block;
    };
    struct while_loop {
        ast::expression condition;
        ast::block block;
    };
    struct parameter {
        ast::type type;
        ast::identifier identifier;
    };
    using parameter_list = std::vector<ast::parameter>;
    using argument_list = std::vector<ast::expression>;
    struct function_call {
        ast::identifier identifier;
        ast::argument_list arguments;
    };
    struct function_def {
        bool to_export;
        ast::identifier identifier;
        ast::type returntype;
        ast::parameter_list parameter_list;
        ast::block block;
    };
    struct s_return {
        std::optional<ast::expression> expression;
    };
    struct s_break {};
    struct s_continue {};
    struct statement {
        std::variant<
            ast::block,
            ast::if_statement,
            ast::for_loop,
            ast::while_loop,
            ast::variable_def,
            ast::assignment,
            ast::s_return,
            ast::s_break,
            ast::s_continue
        > statement;
    };
    struct program {
        std::vector<ast::function_def> function_defs;
    };
}
