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

    struct literal {
        std::variant<double, uint64_t, bool> literal;
        std::optional<ast::primitive_type> explicit_type;
        ast::primitive_type type;
    };
    struct block;
    struct if_statement;
    struct for_loop;
    struct while_loop;
    struct switch_statement;
    struct accessor;
    struct expression {
        std::variant<
            std::unique_ptr<ast::block>,
            std::unique_ptr<ast::if_statement>,
            std::unique_ptr<ast::for_loop>,
            std::unique_ptr<ast::while_loop>,
            std::unique_ptr<ast::switch_statement>,
            std::unique_ptr<ast::accessor>,
            ast::identifier,
            ast::literal,
            std::unique_ptr<ast::function_call>,
            std::unique_ptr<ast::binary_operator>,
            std::unique_ptr<ast::unary_operator>
        > expression;
        ast::type type;
    };
    struct binary_operator {
        ast::expression l, r;
        enum op {
            A_ADD, A_SUB, A_MUL, A_DIV, A_MOD,
            B_SHL, B_SHR, B_AND, B_XOR, B_OR,
            L_AND, L_OR,
            C_EQ, C_NE, C_GT, C_GE, C_LT, C_LE,
        } binary_operator;
        ast::type type;
    };
    struct unary_operator {
        ast::expression r;
        enum op {
            B_NOT, L_NOT,
        } unary_operator;
        ast::type type;
    };

    using statement_list = std::vector<ast::statement>;
    struct block {
        statement_list statements;
        ast::type type;
    };
    struct if_statement {
        std::vector<ast::expression> conditions;
        std::vector<ast::block> blocks;
        ast::type type;
    };
    using optional_else = std::optional<ast::block>;
    struct else_if_list {
        std::vector<ast::expression> conditions;
        std::vector<ast::block> blocks;
    };
    using literal_list = std::vector<ast::literal>;
    struct case_statement {
        literal_list cases;
        ast::block block;
        ast::type type;
    };
    using cases_list = std::vector<ast::case_statement>;
    struct switch_statement {
        ast::expression expression;
        cases_list cases;
        ast::type type;
    };
    struct variable_def {
        std::optional<ast::primitive_type> explicit_type;
        ast::identifier identifier;
        ast::expression expression;
    };
    struct while_loop {
        ast::expression condition;
        ast::block block;
        ast::type type;
    };
    struct parameter {
        ast::type type;
        ast::identifier identifier;
    };
    using parameter_list = std::vector<ast::parameter>;
    using expression_list = std::vector<ast::expression>;
    struct function_call {
        ast::identifier identifier;
        ast::expression_list arguments;
        ast::type type;
    };
    struct function_def {
        bool to_export;
        ast::identifier identifier;
        ast::type returntype;
        ast::parameter_list parameter_list;
        ast::block block;
    };
    struct type_def {
        ast::identifier identifier;
        ast::constructed_type type;
    };
    using field_access = ast::identifier;
    using array_access = ast::expression;
    using access = std::variant<field_access, array_access>;
    struct accessor {
        ast::identifier identifier;
        std::vector<ast::access> fields;
        ast::type type;
    };
    struct assignment {
        ast::accessor accessor;
        ast::expression expression;
    };
    struct for_loop {
        ast::variable_def initial;
        ast::expression condition;
        ast::assignment step;
        ast::block block;
        ast::type type;
    };
    struct s_return {
        std::optional<ast::expression> expression;
    };
    struct s_break {
        std::optional<ast::expression> expression;
    };
    struct s_continue {};
    struct statement {
        std::variant<
            ast::expression,
            ast::function_def,
            ast::variable_def,
            ast::type_def,
            ast::assignment,
            ast::s_return,
            ast::s_break,
            ast::s_continue
        > statement;
    };
    struct program {
        statement_list statements;
    };
}
