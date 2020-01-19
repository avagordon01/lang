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
        buffer.push_back({yylex(drv), drv.current_param});
    }
    current_token = buffer[buffer_loc].first;
    current_param = buffer[buffer_loc].second;
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
param_type parser_context::expectp(token_type t) {
    auto p = current_param;
    expect(t);
    return p;
}

template<typename T>
std::optional<T> parser_context::maybe(T (parser_context::*parse)()) {
    size_t buffer_stop = buffer_loc;
    try {
        return std::move(std::optional<T>{std::move(std::invoke(parse, this))});
    } catch (parse_error& e) {
        buffer_loc = buffer_stop;
        current_token = buffer[buffer_loc].first;
        current_param = buffer[buffer_loc].second;
        return std::nullopt;
    }
}

template<typename T>
std::vector<T> parser_context::parse_list(T (parser_context::*parse)()) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(maybe(parse));
        if (res) {
            list.emplace_back(std::move(res.value()));
        } else {
            return list;
        }
    }
}
template<typename T>
std::vector<T> parser_context::parse_list_sep(T (parser_context::*parse)(), token_type sep) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(maybe(parse));
        if (!res) {
            break;
        }
        list.emplace_back(std::move(res.value()));
        if (!accept(sep)) {
            break;
        }
    }
    return list;
}
template<typename T>
std::vector<T> parser_context::parse_list(T (parser_context::*parse)(), token_type delim) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(std::invoke(parse, this));
        list.emplace_back(std::move(res));
        if (accept(delim)) {
            break;
        }
    }
    return list;
}
template<typename T>
std::vector<T> parser_context::parse_list(T (parser_context::*parse)(), token_type sep, token_type delim) {
    std::vector<T> list;
    while (true) {
        if (accept(delim)) {
            break;
        }
        auto res = std::move(std::invoke(parse, this));
        list.emplace_back(std::move(res));
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
    return list;
}

ast::program parser_context::parse_program() {
    ast::program p {};
    p.statements = std::move(
        parse_list(&parser_context::parse_top_level_statement, token_type::SEMICOLON, token_type::T_EOF)
    );
    return p;
}
ast::block parser_context::parse_block() {
    expect(token_type::OPEN_C_BRACKET);
    ast::block b {};
    b.statements = std::move(
        parse_list(&parser_context::parse_statement, token_type::SEMICOLON, token_type::CLOSE_C_BRACKET)
    );
    return b;
}
ast::if_statement parser_context::parse_if_statement() {
    expect(token_type::IF);
    ast::if_statement s {};
    s.conditions.emplace_back(std::move(parse_exp()));
    s.blocks.emplace_back(std::move(parse_block()));
    while (accept(token_type::ELIF)) {
        s.conditions.emplace_back(std::move(parse_exp()));
        s.blocks.emplace_back(std::move(parse_block()));
    }
    if (accept(token_type::ELSE)) {
        s.blocks.emplace_back(std::move(parse_block()));
    }
    return s;
}
ast::for_loop parser_context::parse_for_loop() {
    expect(token_type::FOR);
    ast::for_loop s {};
    s.initial = std::move(parse_variable_def());
    expect(token_type::SEMICOLON);
    s.condition = std::move(parse_exp());
    expect(token_type::SEMICOLON);
    s.step = std::move(parse_assignment());
    s.block = std::move(parse_block());
    return s;
}
ast::while_loop parser_context::parse_while_loop() {
    expect(token_type::WHILE);
    ast::while_loop s {};
    s.condition = std::move(parse_exp());
    s.block = std::move(parse_block());
    return s;
}
ast::case_statement parser_context::parse_case() {
    expect(token_type::CASE);
    ast::case_statement c {};
    c.cases = std::move(
        parse_list_sep(&parser_context::parse_literal_integer, token_type::COMMA)
    );
    c.block = std::move(parse_block());
    return c;
}
ast::switch_statement parser_context::parse_switch_statement() {
    expect(token_type::SWITCH);
    ast::switch_statement s {};
    s.expression = std::move(parse_exp());
    expect(token_type::OPEN_C_BRACKET);
    s.cases = std::move(parse_list(&parser_context::parse_case, token_type::CLOSE_C_BRACKET));
    return s;
}
ast::identifier parser_context::parse_identifier() {
    return static_cast<ast::identifier>(std::get<uint64_t>(expectp(token_type::IDENTIFIER)));
    //FIXME
    //make ast::identifier and ast::type_id different types
}
ast::type_id parser_context::parse_type_id() {
    param_type p = expectp(token_type::IDENTIFIER);
    ast::type_id t = {};//ast::type_id{ast::num_primitive_types + std::get<ast::identifier>(p)};
    //FIXME
    //sort out the type, type_id, identifier namespacing mess
    return t;
}
ast::function_def parser_context::parse_function_def() {
    ast::function_def f {};
    f.to_export = accept(token_type::EXPORT);
    expect(token_type::FUNCTION);
    auto t = std::move(maybe(&parser_context::parse_primitive_type));
    if (t) {
        f.returntype = std::move(t.value());
    }
    f.identifier = parse_identifier();
    expect(token_type::OPEN_R_BRACKET);
    f.parameter_list = parse_list(&parser_context::parse_field, token_type::COMMA, token_type::CLOSE_R_BRACKET);
    f.block = parse_block();
    return f;
}
ast::function_call parser_context::parse_function_call() {
    ast::function_call f {};
    f.identifier = std::move(parse_identifier());
    expect(token_type::OPEN_R_BRACKET);
    f.arguments = std::move(parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_R_BRACKET));
    return f;
}
ast::type_def parser_context::parse_type_def() {
    ast::type_def t {};
    expect(token_type::TYPE);
    t.type_id = std::move(parse_type_id());
    expect(token_type::OP_ASSIGN);
    t.type = std::move(parse_type());
    return t;
}
ast::assignment parser_context::parse_assignment() {
    ast::assignment a {};
    a.accessor = std::move(parse_accessor());
    expect(token_type::OP_ASSIGN);
    a.expression = std::move(parse_exp());
    return a;
}
ast::variable_def parser_context::parse_variable_def() {
    ast::variable_def v {};
    expect(token_type::VAR);
    v.explicit_type = std::move(maybe(&parser_context::parse_type_id));
    expect(token_type::OP_ASSIGN);
    v.expression = std::move(parse_exp());
    return v;
}
ast::s_return parser_context::parse_return() {
    ast::s_return r {};
    expect(token_type::RETURN);
    r.expression = std::move(maybe(&parser_context::parse_exp));
    return r;
}
ast::s_break parser_context::parse_break() {
    expect(token_type::BREAK);
    return {};
}
ast::s_continue parser_context::parse_continue() {
    expect(token_type::CONTINUE);
    return {};
}
ast::field_access parser_context::parse_field_access() {
    ast::field_access f {};
    expect(token_type::OP_ACCESS);
    f = std::move(parse_identifier());
    return f;
}
ast::array_access parser_context::parse_array_access() {
    ast::array_access a {};
    expect(token_type::OPEN_S_BRACKET);
    a = std::move(parse_exp());
    expect(token_type::CLOSE_S_BRACKET);
    return a;
}
ast::access parser_context::parse_access() {
    auto f = std::move(maybe(&parser_context::parse_field_access));
    if (f) {
        return std::move(f.value());
    }
    auto a = maybe(&parser_context::parse_array_access);
    if (a) {
        return std::move(a.value());
    }
    error("parser expected accessor. got");
}
ast::accessor parser_context::parse_accessor() {
    ast::accessor a {};
    a.identifier = std::move(parse_identifier());
    a.fields = std::move(parse_list(&parser_context::parse_access));
    return a;
}
ast::type parser_context::parse_type() {
    ast::type t {};
    auto p = std::move(maybe(&parser_context::parse_primitive_type));
    if (p) {
        t = p.value();
        return t;
    }
    auto s = std::move(maybe(&parser_context::parse_struct_type));
    if (s) {
        t = std::move(std::make_unique<ast::struct_type>(std::move(s.value())));
        return t;
    }
    auto a = std::move(maybe(&parser_context::parse_array_type));
    if (a) {
        t = std::move(std::make_unique<ast::array_type>(std::move(a.value())));
        return t;
    }
    error("parser expected type. got", current_token);
}
ast::type_id parser_context::parse_primitive_type() {
    ast::type_id t = std::get<ast::type_id>(expectp(token_type::PRIMITIVE_TYPE));
    return t;
}
ast::field parser_context::parse_field() {
    parse_type();
    expect(token_type::IDENTIFIER);
    return {};
}
ast::struct_type parser_context::parse_struct_type() {
    ast::struct_type s;
    expect(token_type::STRUCT);
    expect(token_type::OPEN_C_BRACKET);
    s.fields = parse_list(&parser_context::parse_field, token_type::COMMA, token_type::CLOSE_C_BRACKET);
    return s;
}
ast::array_type parser_context::parse_array_type() {
    expect(token_type::OPEN_S_BRACKET);
    ast::array_type a;
    a.element_type = parse_type_id();
    a.length = parse_literal_integer();
    expect(token_type::CLOSE_S_BRACKET);
    return a;
}
ast::literal parser_context::parse_literal() {
    ast::literal l {};
    switch (current_token) {
        case token_type::LITERAL_BOOL:
            l.literal = std::get<bool>(expectp(current_token));
            l.explicit_type = std::move(maybe(&parser_context::parse_primitive_type));
            break;
        case token_type::LITERAL_INTEGER:
            l.literal = std::get<uint64_t>(expectp(current_token));
            l.explicit_type = std::move(maybe(&parser_context::parse_primitive_type));
            break;
        case token_type::LITERAL_FLOAT:
            l.literal = std::get<double>(expectp(current_token));
            l.explicit_type = std::move(maybe(&parser_context::parse_primitive_type));
            break;
        default:
            error("parser expected literal. got", current_token);
    }
    return l;
}
ast::literal_integer parser_context::parse_literal_integer() {
    expect(token_type::LITERAL_INTEGER);
    return {};
}
ast::statement parser_context::parse_top_level_statement() {
    switch (current_token) {
        case token_type::EXPORT:
        case token_type::FUNCTION:  parse_function_def(); break;
        case token_type::TYPE:      parse_type_def(); break;
        case token_type::VAR:       parse_variable_def(); break;
        default: error("parser expected top level statement: one of function def, type def, or variable def. got", current_token);
    }
    return {};
}
ast::statement parser_context::parse_statement() {
    switch (current_token) {
        case token_type::EXPORT:
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
    return {};
}
ast::expression parser_context::parse_exp() {
    parse_exp_at_precedence(100);
    return {};
}

ast::expression parser_context::parse_exp_atom() {
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
    return {};
}

ast::expression parser_context::parse_exp_at_precedence(int current_precedence) {
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
    return {};
}
