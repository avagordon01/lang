#pragma once

#include <iostream>
#include <functional>
#include <deque>
#include <exception>

#include "ast.hh"

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

enum class associativity {
    left, right,
};

using param_type = std::variant<ast::type_id, bool, uint64_t, double>;

struct driver;

struct parse_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct parser_context {
    driver& drv;
    parser_context(driver& drv_);

    size_t buffer_loc;
    std::deque<token_type> buffer {};
    token_type current_token {};

    void next_token();
    bool accept(token_type t);
    void expect(token_type t);

    bool is_operator(token_type t);
    int get_precedence(token_type t);
    associativity get_associativity(token_type t);

    template<typename T>
    bool maybe(T parse);
    template<typename T>
    void parse_list(T parse);
    template<typename T>
    void parse_list(T parse, token_type delim);
    template<typename T>
    void parse_list_sep(T parse, token_type sep);
    template<typename T>
    void parse_list(T parse, token_type sep, token_type delim);

    void parse_program();
    void parse_if_statement();
    void parse_for_loop();
    void parse_while_loop();
    void parse_switch_statement();
    void parse_function_def();
    void parse_function_call();
    void parse_type_def();
    void parse_assignment();
    void parse_variable_def();
    void parse_return();
    void parse_break();
    void parse_continue();
    void parse_block();
    void parse_access();
    void parse_accessor();
    void parse_type();
    void parse_primitive_type();
    void parse_field();
    void parse_struct_type();
    void parse_array_type();
    void parse_literal();
    void parse_literal_integer();
    void parse_top_level_statement();
    void parse_statement();
    void parse_exp();
    void parse_exp_atom();
    void parse_exp_at_precedence(int current_precedence);
};
