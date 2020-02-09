#include "tokens.hh"
#include "error.hh"
#include <string>
#include <cassert>

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
    "if", "elif", "else",
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
        os << "end of file";
    } else {
        assert(static_cast<int>(t) >= static_cast<int>(token_type::OP_L_OR) && static_cast<int>(t) <= static_cast<int>(token_type::IDENTIFIER));
        os << token_type_names[static_cast<int>(t) - static_cast<int>(token_type::OP_L_OR)];
    }
    return os;
}
bool is_operator(token_type t) {
    return t >= token_type::OP_L_OR && t <= token_type::OP_L_NOT;
}
ast::binary_operator::op get_binary_operator(token_type t) {
    switch (t) {
        case token_type::OP_A_ADD: return ast::binary_operator::op::A_ADD;
        case token_type::OP_A_SUB: return ast::binary_operator::op::A_SUB;
        case token_type::OP_A_MUL: return ast::binary_operator::op::A_MUL;
        case token_type::OP_A_DIV: return ast::binary_operator::op::A_DIV;
        case token_type::OP_A_MOD: return ast::binary_operator::op::A_MOD;
        case token_type::OP_B_AND: return ast::binary_operator::op::B_AND;
        case token_type::OP_B_OR: return ast::binary_operator::op::B_OR;
        case token_type::OP_B_XOR: return ast::binary_operator::op::B_XOR;
        case token_type::OP_B_SHL: return ast::binary_operator::op::B_SHL;
        case token_type::OP_B_SHR: return ast::binary_operator::op::B_SHR;
        case token_type::OP_C_EQ: return ast::binary_operator::op::C_EQ;
        case token_type::OP_C_NE: return ast::binary_operator::op::C_NE;
        case token_type::OP_C_GT: return ast::binary_operator::op::C_GT;
        case token_type::OP_C_GE: return ast::binary_operator::op::C_GE;
        case token_type::OP_C_LT: return ast::binary_operator::op::C_LT;
        case token_type::OP_C_LE: return ast::binary_operator::op::C_LE;
        default: error("token", t, "is not a binary operator");
    }
}
ast::unary_operator::op get_unary_operator(token_type t) {
    switch (t) {
        case token_type::OP_B_NOT: return ast::unary_operator::op::B_NOT;
        case token_type::OP_L_NOT: return ast::unary_operator::op::L_NOT;
        default: error("token", t, "is not a unary operator");
    }
}
int get_precedence(token_type t) {
    if (is_operator(t)) {
        switch (t) {
            case token_type::OP_L_OR:   return 0;
            case token_type::OP_L_AND:  return 1;
            case token_type::OP_C_EQ:
            case token_type::OP_C_NE:
            case token_type::OP_C_GT:
            case token_type::OP_C_LT:
            case token_type::OP_C_GE:
            case token_type::OP_C_LE:   return 2;
            case token_type::OP_B_OR:   return 3;
            case token_type::OP_B_XOR:  return 4;
            case token_type::OP_B_AND:  return 5;
            case token_type::OP_B_SHL:  return 6;
            case token_type::OP_B_SHR:
            case token_type::OP_A_ADD:
            case token_type::OP_A_SUB:  return 7;
            case token_type::OP_A_MUL:
            case token_type::OP_A_DIV:
            case token_type::OP_A_MOD:  return 8;
            case token_type::OP_B_NOT:
            case token_type::OP_L_NOT:  return 9;
            default:
                error("no precedence for operator", t);
        }
    } else {
        error("token", t, "is not an operator");
    }
}
associativity get_associativity(token_type t) {
    if (is_operator(t)) {
        switch (t) {
            case token_type::OP_B_NOT:
            case token_type::OP_L_NOT:
                return associativity::right;
            default:
                return associativity::left;
        }
    } else {
        error("token", t, "is not an operator");
    }
}
