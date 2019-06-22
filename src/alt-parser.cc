#include <iostream>
#include <functional>
#include <map>
#include <optional>

#include "error.hh"
#include "driver.hh"
#include "alt-parser.hh"

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
        os << "EOF";
    } else {
        assert(static_cast<int>(t) >= static_cast<int>(token_type::OP_L_OR) && static_cast<int>(t) <= static_cast<int>(token_type::IDENTIFIER));
        os << token_type_names[static_cast<int>(t) - static_cast<int>(token_type::OP_L_OR)];
    }
    return os;
}

void parser_context::next_token() {
    current_token = yylex(drv);
}
bool parser_context::accept(token_type t) {
    if (current_token == t) {
        next_token();
        return true;
    } else {
        return false;
    }
}
bool parser_context::expect(token_type t) {
    if (accept(t)) {
        return true;
    } else {
        error("parser: expected", t, "got", current_token);
        return false;
    }
}

std::optional<int> parser_context::get_precedence() {
    auto it = operator_precedence.find(current_token);
    if (it == operator_precedence.end()) {
        return std::make_optional(it->second);
    } else {
        return std::nullopt;
    }
}

template<typename T>
bool parser_context::parse_list(T parse) {
    while (std::invoke(parse, this)) {}
    return true;
}
template<typename T>
bool parser_context::parse_list(T parse, token_type sep, token_type delim) {
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

bool parser_context::parse_program() {
    next_token();
    parse_list(&parser_context::parse_statement);
    return true;
}
#define try(x) {if (!(x)) {return false;}};
bool parser_context::parse_if_statement() {
    if (accept(token_type::IF)) {
        try(parse_exp())
        try(parse_block())
        while (accept(token_type::ELIF)) {
            try(parse_exp())
            try(parse_block())
        }
        if (accept(token_type::ELSE)) {
            try(parse_block())
        }
        return true;
    }
    return false;
}
bool parser_context::parse_for_loop() {
    return
        accept(token_type::FOR) ||
        parse_variable_def() &&
        expect(token_type::SEMICOLON) &&
        parse_exp() &&
        expect(token_type::SEMICOLON) &&
        parse_assignment() &&
        parse_block();
}
bool parser_context::parse_while_loop() {
    return
        accept(token_type::WHILE) || 
        parse_exp() &&
        parse_block();
}
bool parser_context::parse_switch_statement() {
    return
        accept(token_type::SWITCH) ||
        parse_exp() &&
        expect(token_type::OPEN_C_BRACKET) &&
        parse_list(
            [](parser_context* ctx) -> bool {
                ctx->accept(token_type::CASE);
                ctx->parse_list(&parser_context::parse_literal_integer);
                ctx->parse_block();
                return true;
            }
        ) &&
        expect(token_type::CLOSE_C_BRACKET);
}
bool parser_context::parse_function_def() {
    return
        accept(token_type::EXPORT) ||
        accept(token_type::FUNCTION) ||
        parse_type() &&
        expect(token_type::IDENTIFIER) &&
        expect(token_type::OPEN_R_BRACKET) &&
        parse_list(
            [](parser_context* ctx) -> bool {
                ctx->parse_type();
                ctx->expect(token_type::IDENTIFIER);
                return true;
            }
        , token_type::COMMA, token_type::CLOSE_R_BRACKET) &&
        parse_block();
}
bool parser_context::parse_function_call() {
    return
        accept(token_type::IDENTIFIER) ||
        accept(token_type::OPEN_R_BRACKET) &&
        parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_R_BRACKET);
}
bool parser_context::parse_type_def() {
    return
        accept(token_type::TYPE) ||
        expect(token_type::IDENTIFIER) &&
        expect(token_type::OP_ASSIGN) &&
        parse_type();
}
bool parser_context::parse_assignment() {
    return
        parse_accessor() ||
        expect(token_type::OP_ASSIGN) &&
        parse_exp();
}
bool parser_context::parse_variable_def() {
    return
        accept(token_type::VAR) ||
        parse_type() ||
        expect(token_type::IDENTIFIER) &&
        expect(token_type::OP_ASSIGN) &&
        parse_exp();
}
bool parser_context::parse_return() {
    return accept(token_type::RETURN);
}
bool parser_context::parse_break() {
    return accept(token_type::BREAK);
}
bool parser_context::parse_continue() {
    return accept(token_type::CONTINUE);
}
bool parser_context::parse_block() {
    return
        accept(token_type::OPEN_C_BRACKET) ||
        parse_list(&parser_context::parse_statement) &&
        expect(token_type::CLOSE_C_BRACKET);
}
bool parser_context::parse_access() {
    return
        accept(token_type::OP_ACCESS) ||
        accept(token_type::OPEN_S_BRACKET) &&
        parse_exp() &&
        expect(token_type::CLOSE_S_BRACKET);
}
bool parser_context::parse_accessor() {
    return
        accept(token_type::IDENTIFIER) &&
        parse_list(&parser_context::parse_access);
}
bool parser_context::parse_type() {
    return
        accept(token_type::PRIMITIVE_TYPE) ||
        accept(token_type::IDENTIFIER) ||
        parse_struct_type() ||
        parse_array_type();
}
bool parser_context::parse_struct_type() {
    return
        accept(token_type::STRUCT) ||
        expect(token_type::OPEN_C_BRACKET) &&
        parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_C_BRACKET);
}
bool parser_context::parse_array_type() {
    return
        accept(token_type::OPEN_S_BRACKET) ||
        parse_type() &&
        parse_literal_integer() &&
        expect(token_type::CLOSE_S_BRACKET);
}
bool parser_context::parse_literal() {
    return
        accept(token_type::LITERAL_BOOL) ||
        accept(token_type::LITERAL_INTEGER) ||
        accept(token_type::LITERAL_FLOAT);
}
bool parser_context::parse_literal_integer() {
    return
        accept(token_type::LITERAL_INTEGER);
}
bool parser_context::parse_statement() {
    return
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
        parse_continue();
}
bool parser_context::parse_exp() {
    auto LHS = parse_exp_primary();
    auto RHS = parse_exp_inner(0);
    return true;
}
bool parser_context::parse_exp_paren() {
    return
        accept(token_type::OPEN_R_BRACKET) ||
        parse_exp_primary() &&
        expect(token_type::CLOSE_R_BRACKET);
}
bool parser_context::parse_exp_primary() {
    return
        parse_function_call() ||
        parse_accessor() ||
        parse_literal() ||
        parse_exp_paren();
}
bool parser_context::parse_exp_inner(int min_precedence) {
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
