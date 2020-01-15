#include <iostream>
#include <functional>
#include <deque>
#include <sstream>

#include "driver.hh"
#include "alt-parser.hh"

template<typename... Ts>
[[noreturn]] static void error(Ts... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    throw parse_error(ss.str());
}

parser_context::parser_context(driver& drv_) : drv(drv_) {
    buffer_loc = -1;
    next_token();
}

void parser_context::next_token() {
    buffer_loc++;
    if (buffer_loc >= buffer.size()) {
        buffer.push_back(yylex(drv));
    }
    current_token = buffer[buffer_loc];
}
bool parser_context::accept(token_type t) {
    if (current_token == t) {
        next_token();
        return true;
    } else {
        return false;
    }
}
void parser_context::expect(token_type t) {
    if (!accept(t)) {
        error("parser expected", t, "got", current_token);
    }
}

template<typename T>
bool parser_context::maybe(T parse) {
    size_t buffer_stop = buffer_loc;
    try {
        std::invoke(parse, this);
    } catch (parse_error& e) {
        buffer_loc = buffer_stop;
        current_token = buffer[buffer_loc];
        return false;
    }
    return true;
}

template<typename T>
void parser_context::parse_list(T parse) {
    while (maybe(parse)) {}
}
template<typename T>
void parser_context::parse_list_sep(T parse, token_type sep) {
    do {
        if (!maybe(parse)) {
            break;
        };
    } while (accept(sep));
}
template<typename T>
void parser_context::parse_list(T parse, token_type delim) {
    while (!accept(delim)) {
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
            error("parser expected", sep, "or", delim, "got", current_token);
        }
    }
}

void parser_context::parse_program() {
    parse_list(&parser_context::parse_top_level_statement, token_type::SEMICOLON, token_type::T_EOF);
}
void parser_context::parse_block() {
    expect(token_type::OPEN_C_BRACKET);
    parse_list(&parser_context::parse_statement, token_type::SEMICOLON, token_type::CLOSE_C_BRACKET);
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
            ctx->parse_list_sep(&parser_context::parse_literal_integer, token_type::COMMA);
            ctx->parse_block();
        }
    , token_type::CLOSE_C_BRACKET);
}
void parser_context::parse_function_def() {
    accept(token_type::EXPORT);
    expect(token_type::FUNCTION);
    maybe(&parser_context::parse_type);
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
    maybe(&parser_context::parse_type);
    expect(token_type::OP_ASSIGN);
    parse_exp();
}
void parser_context::parse_return() {
    expect(token_type::RETURN);
    maybe(&parser_context::parse_exp);
}
void parser_context::parse_break() {
    expect(token_type::BREAK);
}
void parser_context::parse_continue() {
    expect(token_type::CONTINUE);
}
void parser_context::parse_field_access() {
    expect(token_type::OP_ACCESS);
    expect(token_type::IDENTIFIER);
}
void parser_context::parse_array_access() {
    expect(token_type::OPEN_S_BRACKET);
    parse_exp();
    expect(token_type::CLOSE_S_BRACKET);
}
void parser_context::parse_access() {
    if (maybe(&parser_context::parse_field_access)) {
    } else if (maybe(&parser_context::parse_array_access)) {
    } else {
        error("parser expected accessor. got");
    }
}
void parser_context::parse_accessor() {
    expect(token_type::IDENTIFIER);
    parse_list(&parser_context::parse_access);
}
void parser_context::parse_type() {
    if (maybe(&parser_context::parse_primitive_type)) {
    } else if (maybe(&parser_context::parse_struct_type)) {
    } else {
        error("parser expected type. got", current_token);
    }
}
void parser_context::parse_primitive_type() {
    expect(token_type::PRIMITIVE_TYPE);
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
        case token_type::LITERAL_BOOL:
        case token_type::LITERAL_INTEGER:
        case token_type::LITERAL_FLOAT:
            expect(current_token);
            maybe(&parser_context::parse_type);
            break;
        default:
            error("parser expected literal. got", current_token);
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
        case token_type::RETURN:    parse_return(); break;
        case token_type::BREAK:     parse_break(); break;
        case token_type::CONTINUE:  parse_continue(); break;
        case token_type::IDENTIFIER:
            if (maybe(&parser_context::parse_assignment)) {
            } else if (maybe(&parser_context::parse_exp)) {
            } else {
                error("parser expected assignment or expression after token", current_token);
            }
            break;
        default:
            if (maybe(&parser_context::parse_exp)) {
            } else {
                error("parser expected statement. got", current_token);
            }
    }
}
void parser_context::parse_exp() {
    parse_exp_at_precedence(100);
}

void parser_context::parse_exp_atom() {
    switch (current_token) {
        case token_type::LITERAL_BOOL:
        case token_type::LITERAL_INTEGER:
        case token_type::LITERAL_FLOAT: parse_literal(); break;
        case token_type::IF:        parse_if_statement(); break;
        case token_type::SWITCH:    parse_switch_statement(); break;
        case token_type::FOR:       parse_for_loop(); break;
        case token_type::WHILE:     parse_while_loop(); break;
        case token_type::IDENTIFIER:
            if (maybe(&parser_context::parse_function_call)) {
            } else if (maybe(&parser_context::parse_accessor)) {
            } else {
                error("parser expected function call or accessor after token", current_token);
            }
            break;
        case token_type::OPEN_R_BRACKET:
            expect(token_type::OPEN_R_BRACKET);
            parse_exp();
            expect(token_type::CLOSE_R_BRACKET);
            break;
        default:
            error("parser expected expression atom. got", current_token);
    }
}

void parser_context::parse_exp_at_precedence(int current_precedence) {
    parse_exp_atom();
    while (true) {
        token_type t = current_token;
        if (!is_operator(t)) {
            break;
        }
        accept(t);
        int p = get_precedence(t);
        if (p >= current_precedence) {
            break;
        }
        auto a = get_associativity(t);
        if (a == associativity::left) {
            parse_exp_at_precedence(p + 1);
        } else {
            parse_exp_at_precedence(p);
        }
    }
}
