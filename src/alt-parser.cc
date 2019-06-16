#include <iostream>
#include <functional>
#include <map>
#include <optional>

#include "error.hh"

enum class token_type : int {
    T_EOF = 0,
    OP_L_OR = 258, OP_L_AND,
    OP_C_EQ, OP_C_NE, OP_C_GT, OP_C_LT, OP_C_GE, OP_C_LE,
    OP_B_OR, OP_B_XOR, OP_B_AND, OP_B_SHL, OP_B_SHR,
    OP_A_ADD, OP_A_SUB, OP_A_MUL, OP_A_DIV, OP_A_MOD,
    OP_B_NOT, OP_L_NOT,
    OP_ACCESS,
    OP_ASSIGN,
    OPEN_R_BRACKET, CLOSE_R_BRACKET,
    OPEN_C_BRACKET, CLOSE_C_BRACKET,
    OPEN_S_BRACKET, CLOSE_S_BRACKET,
    IF, ELIF, ELSE,
    FOR, WHILE,
    BREAK, CONTINUE,
    SWITCH, CASE,
    FUNCTION, RETURN,
    IMPORT, EXPORT,
    VAR, STRUCT, TYPE,
    SEMICOLON, COMMA,
    PRIMITIVE_TYPE,
    LITERAL_BOOL, LITERAL_INTEGER, LITERAL_FLOAT,
    IDENTIFIER,
};
std::map<token_type, int> operator_precedence = {
    {token_type::OP_L_OR,   0},
    {token_type::OP_L_AND,  1},
    {token_type::OP_C_EQ,   2},
    {token_type::OP_C_NE,   2},
    {token_type::OP_C_GT,   2},
    {token_type::OP_C_LT,   2},
    {token_type::OP_C_GE,   2},
    {token_type::OP_C_LE,   2},
    {token_type::OP_B_OR,   3},
    {token_type::OP_B_XOR,  4},
    {token_type::OP_B_AND,  5},
    {token_type::OP_B_SHL,  6},
    {token_type::OP_B_SHR,  6},
    {token_type::OP_A_ADD,  7},
    {token_type::OP_A_SUB,  7},
    {token_type::OP_A_MUL,  8},
    {token_type::OP_A_DIV,  8},
    {token_type::OP_A_MOD,  8},
    {token_type::OP_B_NOT,  9},
    {token_type::OP_L_NOT,  9},
};
std::string token_type_names[] = {
    "||", "&&",
    "==", "!=", ">", "<", ">=", "<=",
    "|", "^", "&", "<<", ">>",
    "+", "-", "*", "/", "%",
    "~", "!",
    ".",
    "=",
    "(", ")",
    "{", "}",
    "[", "]",
    "if", "else",
    "for", "while",
    "break", "continue",
    "switch", "case",
    "function", "return",
    "import", "export",
    "var", "struct", "type",
    ";", ",",
    "primitive type",
    "literal bool", "literal integer", "literal float",
    "identifier",
};
std::ostream& operator<<(std::ostream& os, token_type& t) {
    if (t == token_type::T_EOF) {
    } else {
        os << token_type_names[static_cast<int>(t) - static_cast<int>(token_type::OP_L_OR)];
    }
    return os;
}

token_type yylex();

struct parser_context {
    token_type current_token;
    void next_token() {
        current_token = yylex();
    }
    bool accept(token_type t) {
        if (current_token == t) {
            next_token();
            return true;
        } else {
            return false;
        }
    }
    bool expect(token_type t) {
        if (accept(t)) {
            return true;
        } else {
            error("parser: expected", t, "got", current_token);
            return false;
        }
    }

    std::optional<int> get_precedence() {
        auto it = operator_precedence.find(current_token);
        if (it == operator_precedence.end()) {
            return std::make_optional(it->second);
        } else {
            return std::nullopt;
        }
    }

    template<typename T>
    bool parse_list(T parse) {
        while (std::invoke(parse, this)) {}
        return true;
    }
    template<typename T>
    bool parse_list(T parse, token_type sep, token_type delim) {
        while (current_token != delim) {
            std::invoke(parse, this);
            if (expect(sep)) {
                if (accept(delim)) {
                    break;
                }
                continue;
            } else if (expect(delim)) {
                break;
            } else {
                error("parser: expected", sep, "or", delim, "got", current_token);
                return false;
            }
        }
        return true;
    }

    bool parse_program() {
        parse_list(&parser_context::parse_statement);
        return true;
    }
    bool parse_if_statement() {
        expect(token_type::IF);
        parse_exp();
        parse_block();
        while (accept(token_type::ELIF)) {
            parse_exp();
            parse_block();
        }
        if (accept(token_type::ELSE)) {
            parse_block();
        }
        return true;
    }
    bool parse_for_loop() {
        expect(token_type::FOR);
        parse_variable_def();
        expect(token_type::SEMICOLON);
        parse_exp();
        expect(token_type::SEMICOLON);
        parse_assignment();
        parse_block();
        return true;
    }
    bool parse_while_loop() {
        expect(token_type::WHILE);
        parse_exp();
        parse_block();
        return true;
    }
    bool parse_switch_statement() {
        expect(token_type::SWITCH);
        parse_exp();
        expect(token_type::OPEN_C_BRACKET);
        auto l = [](parser_context* ctx) -> bool {
            ctx->accept(token_type::CASE);
            ctx->parse_list(&parser_context::parse_literal_integer);
            ctx->parse_block();
            return true;
        };
        parse_list(l);
        expect(token_type::CLOSE_C_BRACKET);
        return true;
    }
    bool parse_function_def() {
        if (accept(token_type::EXPORT)) {}
        expect(token_type::FUNCTION);
        if (parse_type()) {}
        expect(token_type::IDENTIFIER);
        expect(token_type::OPEN_R_BRACKET);
        auto l = [](parser_context* ctx) -> bool {
            ctx->parse_type();
            ctx->expect(token_type::IDENTIFIER);
            return true;
        };
        parse_list(l, token_type::COMMA, token_type::CLOSE_R_BRACKET);
        parse_block();
        return true;
    }
    bool parse_function_call() {
        expect(token_type::IDENTIFIER);
        expect(token_type::OPEN_R_BRACKET);
        parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_R_BRACKET);
        return true;
    }
    bool parse_type_def() {
        expect(token_type::TYPE);
        expect(token_type::IDENTIFIER);
        expect(token_type::OP_ASSIGN);
        parse_type();
        return true;
    }
    bool parse_assignment() {
        parse_accessor();
        expect(token_type::OP_ASSIGN);
        parse_exp();
        return true;
    }
    bool parse_variable_def() {
        expect(token_type::VAR);
        if (parse_type()) {}
        expect(token_type::IDENTIFIER);
        expect(token_type::OP_ASSIGN);
        parse_exp();
        return true;
    }
    bool parse_return() {
        expect(token_type::RETURN);
        return true;
    }
    bool parse_break() {
        expect(token_type::BREAK);
        return true;
    }
    bool parse_continue() {
        expect(token_type::CONTINUE);
        return true;
    }
    bool parse_block() {
        expect(token_type::OPEN_C_BRACKET);
        parse_list(&parser_context::parse_statement);
        expect(token_type::CLOSE_C_BRACKET);
        return true;
    }
    bool parse_access() {
        if (accept(token_type::OP_ACCESS)) {
        } else if (accept(token_type::OPEN_S_BRACKET)) {
            parse_exp();
            expect(token_type::CLOSE_S_BRACKET);
        }
        return true;
    }
    bool parse_accessor() {
        expect(token_type::IDENTIFIER);
        parse_list(&parser_context::parse_access);
        return true;
    }
    bool parse_type() {
        if (
            accept(token_type::PRIMITIVE_TYPE) ||
            accept(token_type::IDENTIFIER) ||
            parse_struct_type() ||
            parse_array_type()
        ) {
            return true;
        } else {
            error("parser: expected type, got", current_token);
        }
    }
    bool parse_struct_type() {
        expect(token_type::STRUCT);
        expect(token_type::OPEN_C_BRACKET);
        parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_C_BRACKET);
        return true;
    }
    bool parse_array_type() {
        expect(token_type::OPEN_S_BRACKET);
        parse_type();
        parse_literal_integer();
        expect(token_type::CLOSE_S_BRACKET);
        return true;
    }
    bool parse_literal() {
        if (accept(token_type::LITERAL_BOOL) || accept(token_type::LITERAL_INTEGER) || accept(token_type::LITERAL_FLOAT)) {
            return true;
        } else {
            error("parser: expected literal bool, integer or float. got", current_token);
        }
    }
    bool parse_literal_integer() {
        expect(token_type::LITERAL_INTEGER);
        return true;
    }
    bool parse_statement() {
        if (
            parse_exp() ||
            parse_function_def() ||
            parse_type_def() ||
            parse_variable_def() ||
            parse_assignment() ||
            parse_block() ||
            parse_if_statement() ||
            parse_switch_statement() ||
            parse_for_loop() ||
            parse_while_loop() ||
            parse_return() ||
            parse_break() ||
            parse_continue()
        ) {
            return true;
        } else {
            error("parser: expected statement. got", current_token);
        }
    }
    bool parse_exp() {
        auto LHS = parse_exp_primary();
        auto RHS = parse_exp_inner(0);
        return true;
    }
    bool parse_exp_paren() {
        if (accept(token_type::OPEN_R_BRACKET)) {
            parse_exp_primary();
            expect(token_type::CLOSE_R_BRACKET);
            return true;
        } else {
            return false;
        }
    }
    bool parse_exp_primary() {
        if (
            parse_function_call() ||
            parse_accessor() ||
            parse_literal() ||
            parse_exp_paren()
        ) {
            return true;
        } else {
            error("parser: expected function call, accessor or literal. got", current_token);
            return false;
        }
    }
    bool parse_exp_inner(int min_precedence) {
        while (true) {
            std::optional<int> prec = get_precedence();
            if (!prec) {
                error("parser: expected binary operator. got", current_token);
                return false;
            }
            if (*prec < min_precedence) {
                return true;
            }
            token_type bin_op = current_token;
            next_token();

            bool rhs = parse_exp_primary();
            if (!rhs) {
                return false;
            }

            std::optional<int> next_prec = get_precedence();
            if (*prec < *next_prec) {
                rhs = parse_exp_inner(*prec + 1);
                if (!rhs) {
                    return false;
                }
            }
        }
        return true;
    }
};
