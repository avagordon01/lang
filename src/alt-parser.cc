#include <iostream>
#include <functional>
#include <deque>
#include <sstream>

#include "driver.hh"
#include "alt-parser.hh"

template<typename ... Ts>
static void perror(Ts ... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    throw parse_error(ss.str());
}

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
bool parser_context::is_operator(token_type t) {
    return t >= token_type::OP_L_OR && t <= token_type::OP_L_NOT;
}
int parser_context::get_precedence(token_type t) {
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
                perror("no precedence for operator", t);
        }
    } else {
        perror("token", t, "is not an operator");
    }
    assert(false);
}
associativity parser_context::get_associativity(token_type t) {
    if (is_operator(t)) {
        switch (t) {
            case token_type::OP_B_NOT:
            case token_type::OP_L_NOT:
                return associativity::right;
            default:
                return associativity::left;
        }
    } else {
        perror("token", t, "is not an operator");
    }
    assert(false);
}

parser_context::parser_context(driver& drv_) : drv(drv_) {
    next_token();
}

void parser_context::next_token() {
    if (!buffering) {
        if (buffer.empty()) {
            current_token = yylex(drv);
        } else {
            current_token = buffer.front();
            buffer.pop_front();
        }
    } else {
        current_token = yylex(drv);
        buffer.push_back(current_token);
    }
    std::cerr << current_token << " ";
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
        if (!buffering) {
            perror("parser: expected", t, "got", current_token);
            assert(false);
        } else {
            return false;
        }
    }
}

template<typename T>
bool parser_context::maybe(T parse) {
    buffering = true;
    try {
        std::invoke(parse, this);
    } catch (parse_error& e) {
        return false;
    }
    buffering = false;
    buffer.clear();
    return true;
}

template<typename T>
void parser_context::parse_list(T parse) {
    while (std::invoke(parse, this, false)) {}
}
template<typename T>
void parser_context::parse_list_sep(T parse, token_type sep) {
    do {
        std::invoke(parse, this);
    } while (accept(sep));
    //TODO optional trailing separator
    //TODO possibly empty lists
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
            perror("parser: expected", sep, "or", delim, "got", current_token);
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
void parser_context::parse_access() {
    expect(token_type::OP_ACCESS);
    expect(token_type::OPEN_S_BRACKET);
    parse_exp();
    expect(token_type::CLOSE_S_BRACKET);
}
void parser_context::parse_accessor() {
    expect(token_type::IDENTIFIER);
    //TODO
    //parse_list(&parser_context::parse_access, token_type::T_EOF);
}
void parser_context::parse_type() {
    switch (current_token) {
        case token_type::PRIMITIVE_TYPE: parse_primitive_type(); break;
        //TODO
        //case token_type::IDENTIFIER: assert(false); break;
        case token_type::STRUCT: parse_struct_type(); break;
        case token_type::OPEN_S_BRACKET: parse_array_type(); break;
        default:
            if (!buffering) {
                perror("parser expected type");
            } else {
                return;
            }
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
        case token_type::LITERAL_BOOL:
        case token_type::LITERAL_INTEGER:
        case token_type::LITERAL_FLOAT: accept(current_token); break;
        default: perror("parser expected literal");
    }
    maybe(&parser_context::parse_type);
}
void parser_context::parse_literal_integer() {
    expect(token_type::LITERAL_INTEGER);
}
void parser_context::parse_top_level_statement() {
    switch (current_token) {
        case token_type::FUNCTION:  parse_function_def(); break;
        case token_type::TYPE:      parse_type_def(); break;
        case token_type::VAR:       parse_variable_def(); break;
        default: perror("parser expected top level statement: one of function def, type def, or variable def. got", current_token);
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
                perror("parser expected assignment or expression after token", current_token);
            }
            break;
        default:
            parse_exp();
            //TODO
            //perror("parser expected statement. got", current_token);
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
                perror("parser expected function call or accessor after token", current_token);
            }
            break;
        case token_type::OPEN_R_BRACKET: parse_exp_atom(); break;
    }
}

token_type parser_context::parse_operator() {
    auto t = current_token;
    if (is_operator(t)) {
        accept(t);
        return t;
    } else {
        perror("parser expected operator. got", t);
    }
    assert(false);
}

void parser_context::parse_exp_at_precedence(int current_precedence) {
    parse_exp_atom();
    while (true) {
        if (!is_operator(current_token)) {
            break;
        }
        token_type t = parse_operator();
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
