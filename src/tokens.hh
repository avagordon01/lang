#pragma once
#include "ast.hh"

#include <ostream>

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
std::ostream& operator<<(std::ostream& os, token_type& t);
bool is_operator(token_type t);
ast::binary_operator::op get_binary_operator(token_type t);
ast::unary_operator::op get_unary_operator(token_type t);
int get_precedence(token_type t);
enum class associativity {
    left, right,
};
associativity get_associativity(token_type t);
