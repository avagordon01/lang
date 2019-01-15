#pragma once

#include <cstdint>

#include <vector>
#include <utility>
#include <optional>

namespace ast {
    enum _type {
        t_bool,
        u8, u16, u32, u64,
        i8, i16, i32, i64,
        f8, f16, f32, f64,
    };

    struct _literal;
    struct _operator;
    struct _expression {
        enum {
            VARIABLE, LITERAL, OPERATOR,
        } expression_type;
        union {
            size_t variable;
            _literal *literal;
            _operator *op;
        };
    };
    struct _literal {
        enum {
            FLOAT, INTEGER, BOOL,
        } type;
        union {
            double _float;
            uint64_t _integer;
            bool _bool;
        };
    };
    struct _operator {
        enum op {
            A_ADD, A_SUB, A_MUL, A_DIV, A_MOD,
            B_SHL, B_SHR, B_AND, B_XOR, B_OR, B_NOT,
            L_AND, L_OR, L_NOT,
            C_EQ, C_NE, C_GT, C_GE, C_LT, C_LE,
        };
        _expression *l, *r;
    };

    struct _statement;
    struct _block {
        std::vector<_statement> statements;
    };
    using _if_statement = std::pair<std::vector<ast::_expression>, std::vector<ast::_block>>;
    using _optional_else = std::optional<ast::_block>;
    using _else_if_list = std::pair<std::vector<ast::_expression>, std::vector<ast::_block>>;
    struct _for_loop {
        _expression *initial;
        _expression *condition;
        _expression *step;
        _block *block;
    };
    struct _while_loop {
        _expression *condition;
        _block *block;
    };
    using _parameter_list = std::vector<std::pair<_type, size_t>>;
    struct _function {
        _type return_type;
        _parameter_list parameter_list;
        _block *block;
    };
    struct _assignment {
        size_t identifier;
        _expression *expression;
    };
    struct _statement {
        enum {
            S_BLOCK, S_IF, S_FOR, S_WHILE, S_FUNCTION, S_ASSIGNMENT,
        } type;
        union {
            _block *block;
            _if_statement *if_statement;
            _for_loop *for_loop;
            _while_loop *while_loop;
            _function *function;
            _assignment *assignment;
        };
    };
    using _statement_list = std::vector<_statement>;
    struct _program {
        _statement_list statements;
    };
}
