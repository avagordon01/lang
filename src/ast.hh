#pragma once

#include <cstdint>

#include <vector>
#include <optional>
#include <memory>
#include <variant>

#include "types.hh"
#include "location.hh"

namespace ast {
    struct function_call;
    struct binary_operator;
    struct unary_operator;
    struct statement;

    struct literal_integer {
        uint64_t data;
    };

    struct literal {
        std::variant<double, literal_integer, bool> literal;
        std::optional<ast::named_type> explicit_type;
        ast::named_type type;
        yy::location loc;
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
        ast::named_type type;
        yy::location loc;
    };
    struct binary_operator {
        ast::expression l, r;
        enum op {
            A_ADD, A_SUB, A_MUL, A_DIV, A_MOD,
            B_SHL, B_SHR, B_AND, B_XOR, B_OR,
            L_AND, L_OR,
            C_EQ, C_NE, C_GT, C_GE, C_LT, C_LE,
        } binary_operator;
        ast::named_type type;
        yy::location loc;
    };
    struct unary_operator {
        ast::expression r;
        enum op {
            B_NOT, L_NOT,
        } unary_operator;
        ast::named_type type;
        yy::location loc;
    };

    using statement_list = std::vector<ast::statement>;
    struct block {
        statement_list statements;
        ast::named_type type;
    };
    struct if_statement {
        std::vector<ast::expression> conditions;
        std::vector<ast::block> blocks;
        ast::named_type type;
        yy::location loc;
    };
    using optional_else = std::optional<ast::block>;
    struct elif_list {
        std::vector<ast::expression> conditions;
        std::vector<ast::block> blocks;
    };
    struct case_statement {
        std::vector<literal> cases;
        ast::block block;
        ast::named_type type;
    };
    using cases_list = std::vector<ast::case_statement>;
    struct switch_statement {
        ast::expression expression;
        cases_list cases;
        ast::named_type type;
        yy::location loc;
    };
    struct variable_def {
        std::optional<ast::named_type> explicit_type;
        ast::identifier identifier;
        ast::expression expression;
        yy::location loc;
    };
    struct while_loop {
        ast::expression condition;
        ast::block block;
        ast::named_type type;
        yy::location loc;
    };
    using parameter = field;
    using parameter_list = field_list;
    using expression_list = std::vector<ast::expression>;
    struct function_call {
        ast::identifier identifier;
        ast::expression_list arguments;
        ast::named_type type;
        yy::location loc;
    };
    struct function_def {
        bool to_export;
        ast::identifier identifier;
        ast::named_type returntype;
        ast::parameter_list parameter_list;
        ast::block block;
        yy::location loc;
    };
    struct type_def {
        ast::user_type user_type;
        ast::type type;
        yy::location loc;
    };
    using field_access = ast::identifier;
    using array_access = ast::expression;
    using access = std::variant<field_access, array_access>;
    struct accessor {
        ast::identifier identifier;
        std::vector<ast::access> fields;
        ast::named_type type;
        yy::location loc;
    };
    struct assignment {
        ast::accessor accessor;
        ast::expression expression;
        yy::location loc;
    };
    struct for_loop {
        ast::variable_def initial;
        ast::expression condition;
        ast::assignment step;
        ast::block block;
        ast::named_type type;
        yy::location loc;
    };
    struct s_return {
        std::optional<ast::expression> expression;
        yy::location loc;
    };
    struct s_break {
        std::optional<ast::expression> expression;
        yy::location loc;
    };
    struct s_continue {
        yy::location loc;
    };
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
        bi_registry<ast::identifier, std::string> symbols_registry;
        statement_list statements;
    };
}
