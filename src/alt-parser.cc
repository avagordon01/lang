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
        os << "EOF";
    } else {
        assert(static_cast<int>(t) >= static_cast<int>(token_type::OP_L_OR) && static_cast<int>(t) <= static_cast<int>(token_type::IDENTIFIER));
        os << token_type_names[static_cast<int>(t) - static_cast<int>(token_type::OP_L_OR)];
    }
    return os;
}

parser_context::parser_context(driver& drv_) : drv(drv_) {
    current_token = yylex(drv);
    lookahead_token = yylex(drv);
}

void parser_context::next_token() {
    current_token = lookahead_token;
    lookahead_token = yylex(drv);
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
void parser_context::parse_list(T parse, token_type delim) {
    while (current_token != delim) {
        std::invoke(parse, this);
    }
}
template<typename T>
void parser_context::parse_list(T parse, token_type sep, token_type delim) {
    while (!accept(delim)) {
        std::invoke(parse, this);
        if (accept(sep)) {
            if (accept(delim)) {
                break;
            }
        } else if (accept(delim)) {
            break;
        } else {
            error("parser: expected", sep, "or", delim, "got", current_token);
        }
    }
}

void parser_context::parse_program() {
    parse_list(&parser_context::parse_statement, token_type::SEMICOLON, token_type::T_EOF);
}
void parser_context::parse_if_statement() {
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
}
void parser_context::parse_for_loop() {
    expect(token_type::FOR);
    parse_variable_def();
    expect(token_type::SEMICOLON);
    parse_exp();
    expect(token_type::SEMICOLON);
    parse_assignment();
    parse_block();
}
void parser_context::parse_while_loop() {
    expect(token_type::WHILE);
    parse_exp();
    parse_block();
}
void parser_context::parse_switch_statement() {
    expect(token_type::SWITCH);
    parse_exp();
    expect(token_type::OPEN_C_BRACKET);
    parse_list(
        [](parser_context* ctx) {
            ctx->expect(token_type::CASE);
            ctx->parse_list(&parser_context::parse_literal_integer, token_type::SEMICOLON);
            ctx->parse_block();
        }
    , token_type::CLOSE_C_BRACKET);
}
void parser_context::parse_function_def() {
    accept(token_type::EXPORT);
    expect(token_type::FUNCTION);
    parse_type();
    expect(token_type::IDENTIFIER);
    expect(token_type::OPEN_R_BRACKET);
    parse_list(
        [](parser_context* ctx) {
            ctx->parse_type();
            ctx->expect(token_type::IDENTIFIER);
        }
    , token_type::COMMA, token_type::CLOSE_R_BRACKET);
    parse_block();
}
void parser_context::parse_function_call() {
    expect(token_type::IDENTIFIER);
    expect(token_type::OPEN_R_BRACKET);
    parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_R_BRACKET);
}
void parser_context::parse_type_def() {
    expect(token_type::TYPE);
    expect(token_type::IDENTIFIER);
    expect(token_type::OP_ASSIGN);
    parse_type();
}
void parser_context::parse_assignment() {
    parse_accessor();
    expect(token_type::OP_ASSIGN);
    parse_exp();
}
void parser_context::parse_variable_def() {
    expect(token_type::VAR);
    expect(token_type::IDENTIFIER);
    expect(token_type::IDENTIFIER);
    expect(token_type::OP_ASSIGN);
    parse_exp();
}
void parser_context::parse_return() {
    expect(token_type::RETURN);
}
void parser_context::parse_break() {
    expect(token_type::BREAK);
}
void parser_context::parse_continue() {
    expect(token_type::CONTINUE);
}
void parser_context::parse_block() {
    expect(token_type::OPEN_C_BRACKET);
    parse_list(&parser_context::parse_statement, token_type::CLOSE_C_BRACKET);
}
void parser_context::parse_access() {
    expect(token_type::OP_ACCESS);
    expect(token_type::OPEN_S_BRACKET);
    parse_exp();
    expect(token_type::CLOSE_S_BRACKET);
}
void parser_context::parse_accessor() {
    expect(token_type::IDENTIFIER);
    parse_list(&parser_context::parse_access, token_type::T_EOF);
    //TODO
    assert(false);
}
void parser_context::parse_type() {
    switch (current_token) {
        case token_type::PRIMITIVE_TYPE: parse_primitive_type(); break;
        case token_type::IDENTIFIER: /*TODO*/ assert(false); break;
        case token_type::STRUCT: parse_struct_type(); break;
        case token_type::OPEN_S_BRACKET: parse_array_type(); break;
        default: assert(false);
    }
}
void parser_context::parse_primitive_type() {
    accept(token_type::PRIMITIVE_TYPE);
}
void parser_context::parse_field() {
    parse_type();
    expect(token_type::IDENTIFIER);
}
void parser_context::parse_struct_type() {
    expect(token_type::STRUCT);
    expect(token_type::OPEN_C_BRACKET);
    parse_list(&parser_context::parse_field, token_type::COMMA, token_type::CLOSE_C_BRACKET);
}
void parser_context::parse_array_type() {
    expect(token_type::OPEN_S_BRACKET);
    parse_type();
    parse_literal_integer();
    expect(token_type::CLOSE_S_BRACKET);
}
void parser_context::parse_literal() {
    switch (current_token) {
        case token_type::LITERAL_BOOL: break;
        case token_type::LITERAL_INTEGER: break;
        case token_type::LITERAL_FLOAT: break;
    }
}
void parser_context::parse_literal_integer() {
    expect(token_type::LITERAL_INTEGER);
}
void parser_context::parse_top_level_statement() {
    switch (current_token) {
        case token_type::FUNCTION:  parse_function_def(); break;
        case token_type::TYPE:      parse_type_def(); break;
        case token_type::VAR:       parse_variable_def(); break;
        default: error("parser expected top level statement: one of function def, type def, or variable def. got", current_token);
    }
}
void parser_context::parse_statement() {
    switch (current_token) {
        case token_type::FUNCTION:  parse_function_def(); break;
        case token_type::TYPE:      parse_type_def(); break;
        case token_type::VAR:       parse_variable_def(); break;
        case token_type::OPEN_C_BRACKET: parse_block(); break;
        case token_type::IF:        parse_if_statement(); break;
        case token_type::SWITCH:    parse_switch_statement(); break;
        case token_type::FOR:       parse_for_loop(); break;
        case token_type::WHILE:     parse_while_loop(); break;
        case token_type::RETURN:    parse_return(); break;
        case token_type::BREAK:     parse_break(); break;
        case token_type::CONTINUE:  parse_continue(); break;
        case token_type::IDENTIFIER:
            parse_accessor();
            switch (current_token) {
                case token_type::OP_ASSIGN:
                    parse_assignment();
                    break;
                case token_type::OPEN_R_BRACKET:
                    parse_function_call();
                    break;
                default:
                    error("parser expected either function call or assignment. got", current_token);
                    break;
            }
            break;
        default: error("parser expected statement. got", current_token);
    }
}
void parser_context::parse_exp() {
    //TODO
    assert(false);
}
