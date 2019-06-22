#pragma once

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

using param_type = std::variant<ast::type_id, bool, uint64_t, double>;

struct driver;

struct parser_context {
    driver& drv;
    parser_context(driver& drv_): drv(drv_) {};

    token_type current_token;

    void next_token();
    bool accept(token_type t);
    bool expect(token_type t);

    std::optional<int> get_precedence();

    template<typename T>
    bool parse_list(T parse);
    template<typename T>
    bool parse_list(T parse, token_type sep, token_type delim);

    bool parse_program();
    bool parse_if_statement();
    bool parse_for_loop();
    bool parse_while_loop();
    bool parse_switch_statement();
    bool parse_function_def();
    bool parse_function_call();
    bool parse_type_def();
    bool parse_assignment();
    bool parse_variable_def();
    bool parse_return();
    bool parse_break();
    bool parse_continue();
    bool parse_block();
    bool parse_access();
    bool parse_accessor();
    bool parse_type();
    bool parse_struct_type();
    bool parse_array_type();
    bool parse_literal();
    bool parse_literal_integer();
    bool parse_statement();
    bool parse_exp();
    bool parse_exp_paren();
    bool parse_exp_primary();
    bool parse_exp_inner(int min_precedence);
};
