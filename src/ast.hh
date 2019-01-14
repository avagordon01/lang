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

    struct _variable {
        size_t id;
    };
    struct _literal;
    struct _operator;
    struct _expression {
        enum {
            VARIABLE, LITERAL, OPERATOR,
        } expression_type;
        _variable *variable;
        _literal *literal;
        _operator *op;
    };
    struct _statement;
    struct _block {
        std::vector<_statement> statements;
    };
    using _if_statement = std::pair<std::vector<ast::_expression>, std::vector<ast::_block>>;
    using optional_else = std::optional<ast::_block>;
    using else_if_list = std::pair<std::vector<ast::_expression>, std::vector<ast::_block>>;
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
    struct _function {
        _type return_type;
        std::vector<std::pair<_type, size_t>> parameter_list;
        _block *block;
    };
    struct _assignment {
        size_t identifier;
        _expression *expression;
    };
    struct _statement {
        enum {
            S_IF, S_FOR, S_WHILE, S_FUNCTION, S_ASSIGNMENT,
        } statement_type;
        union {
            _if_statement *if_statement;
            _for_loop *for_loop;
            _while_loop *while_loop;
            _function *function;
            _assignment *assignment;
        };
    };
    struct _program {
        std::vector<_statement> statements;
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

    using statement_list = std::vector<_statement>;
    using parameter_list = std::vector<std::pair<_type, size_t>>;
}
