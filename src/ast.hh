#pragma once

#include <cstdint>

#include <vector>
#include <utility>
#include <optional>
#include <memory>
#include <variant>

namespace ast {
    using identifier = size_t;

    enum type {
        t_bool,
        u8, u16, u32, u64,
        i8, i16, i32, i64,
        f8, f16, f32, f64,
    };

    struct literal {
        std::variant<double, uint64_t, bool> literal;
    };
    struct binary_operator;
    struct unary_operator;
    struct expression {
        std::variant<
            size_t,
            literal,
            std::unique_ptr<binary_operator>,
            std::unique_ptr<unary_operator>
        > expression;
    };
    struct binary_operator {
        enum op {
            A_ADD, A_SUB, A_MUL, A_DIV, A_MOD,
            B_SHL, B_SHR, B_AND, B_XOR, B_OR,
            L_AND, L_OR,
            C_EQ, C_NE, C_GT, C_GE, C_LT, C_LE,
        } binary_operator;
        expression l, r;
    };
    struct unary_operator {
        enum op {
            B_NOT, L_NOT,
        } unary_operator;
        expression r;
    };

    struct statement;
    struct block {
        std::unique_ptr<std::vector<statement>> statements;
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
        expression initial;
        expression condition;
        expression step;
        block block;
    };
    struct while_loop {
        expression condition;
        block block;
    };
    using parameter_list = std::vector<std::pair<type, size_t>>;
    struct function {
        type returntype;
        parameter_list parameter_list;
        block block;
    };
    struct assignment {
        size_t identifier;
        expression expression;
    };
    struct statement {
        std::variant<
            block,
            if_statement,
            for_loop,
            while_loop,
            function,
            assignment
        > statement;
    };
    using statement_list = std::vector<statement>;
    struct program {
        statement_list statements;
    };
}
