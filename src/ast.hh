#pragma once

#include <cstdint>

#include <vector>
#include <utility>
#include <optional>

namespace ast {
    enum type {
        t_bool,
        u8, u16, u32, u64,
        i8, i16, i32, i64,
        f8, f16, f32, f64,
    };

    struct literal;
    struct binary_operator;
    struct expression {
        enum {
            VARIABLE, LITERAL, OPERATOR,
        } type;
        union {
            size_t variable;
            literal *literal;
            binary_operator *op;
        };
    };
    struct literal {
        enum {
            FLOAT, INTEGER, BOOL,
        } type;
        union {
            double _float;
            uint64_t _integer;
            bool _bool;
        };
    };
    struct binary_operator {
        enum op {
            A_ADD, A_SUB, A_MUL, A_DIV, A_MOD,
            B_SHL, B_SHR, B_AND, B_XOR, B_OR, B_NOT,
            L_AND, L_OR, L_NOT,
            C_EQ, C_NE, C_GT, C_GE, C_LT, C_LE,
        } binary_operator;
        expression *l, *r;
    };

    struct statement;
    struct block {
        std::vector<statement> statements;
    };
    using if_statement = std::pair<std::vector<ast::expression>, std::vector<ast::block>>;
    using optional_else = std::optional<ast::block>;
    using else_if_list = std::pair<std::vector<ast::expression>, std::vector<ast::block>>;
    struct for_loop {
        expression *initial;
        expression *condition;
        expression *step;
        block *block;
    };
    struct while_loop {
        expression *condition;
        block *block;
    };
    using parameter_list = std::vector<std::pair<type, size_t>>;
    struct function {
        type returntype;
        parameter_list parameter_list;
        block *block;
    };
    struct assignment {
        size_t identifier;
        expression *expression;
    };
    struct statement {
        enum {
            S_BLOCK, S_IF, S_FOR, S_WHILE, S_FUNCTION, S_ASSIGNMENT,
        } type;
        union {
            block *block;
            if_statement *if_statement;
            for_loop *for_loop;
            while_loop *while_loop;
            function *function;
            assignment *assignment;
        };
    };
    using statement_list = std::vector<statement>;
    struct program {
        statement_list statements;
    };
}
