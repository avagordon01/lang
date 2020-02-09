#include <iostream>

#include "driver.hh"
#include "alt-parser.hh"
#include "alt-parser-utils.hh"

ast::program parser_context::parse_program() {
    ast::program p {};
    try {
        p.statements = parse_list(&parser_context::parse_top_level_statement, token_type::SEMICOLON, token_type::T_EOF);
    } catch (parse_error& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    return p;
}
ast::block parser_context::parse_block() {
    expect(token_type::OPEN_C_BRACKET);
    ast::block b {};
    b.statements = parse_list(&parser_context::parse_statement, token_type::SEMICOLON, token_type::CLOSE_C_BRACKET);
    return b;
}
ast::if_statement parser_context::parse_if_statement() {
    expect(token_type::IF);
    ast::if_statement s {};
    s.conditions.emplace_back(parse_exp());
    s.blocks.emplace_back(parse_block());
    while (accept(token_type::ELIF)) {
        s.conditions.emplace_back(parse_exp());
        s.blocks.emplace_back(parse_block());
    }
    if (accept(token_type::ELSE)) {
        s.blocks.emplace_back(parse_block());
    }
    return s;
}
ast::for_loop parser_context::parse_for_loop() {
    expect(token_type::FOR);
    ast::for_loop s {};
    s.initial = parse_variable_def();
    expect(token_type::SEMICOLON);
    s.condition = parse_exp();
    expect(token_type::SEMICOLON);
    s.step = parse_assignment();
    s.block = parse_block();
    return s;
}
ast::while_loop parser_context::parse_while_loop() {
    expect(token_type::WHILE);
    ast::while_loop s {};
    s.condition = parse_exp();
    s.block = parse_block();
    return s;
}
ast::case_statement parser_context::parse_case() {
    expect(token_type::CASE);
    ast::case_statement c {};
    c.cases = parse_list_sep(&parser_context::parse_literal_integer, token_type::COMMA);
    c.block = parse_block();
    return c;
}
ast::switch_statement parser_context::parse_switch_statement() {
    expect(token_type::SWITCH);
    ast::switch_statement s {};
    s.expression = parse_exp();
    expect(token_type::OPEN_C_BRACKET);
    s.cases = parse_list(&parser_context::parse_case, token_type::CLOSE_C_BRACKET);
    return s;
}
ast::identifier parser_context::parse_identifier() {
    return static_cast<ast::identifier>(std::get<uint64_t>(expectp(token_type::IDENTIFIER)));
}
//FIXME
//make ast::identifier and ast::type_id different types
//sort out the type, type_id, identifier namespacing mess
ast::type_id parser_context::parse_type_id() {
    param_type p = expectp(token_type::IDENTIFIER);
    ast::type_id t = ast::type_id{ast::num_primitive_types + std::get<ast::identifier>(p)};
    return t;
}
ast::function_def parser_context::parse_function_def() {
    ast::function_def f {};
    f.to_export = accept(token_type::EXPORT);
    expect(token_type::FUNCTION);
    auto t = maybe(&parser_context::parse_named_type);
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
    f.identifier = parse_identifier();
    expect(token_type::OPEN_R_BRACKET);
    f.arguments = parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_R_BRACKET);
    return f;
}
ast::type_def parser_context::parse_type_def() {
    ast::type_def t {};
    expect(token_type::TYPE);
    parse_identifier();
    expect(token_type::OP_ASSIGN);
    t.type = parse_type();
    return t;
}
ast::assignment parser_context::parse_assignment() {
    ast::assignment a {};
    a.accessor = parse_accessor();
    expect(token_type::OP_ASSIGN);
    a.expression = parse_exp();
    return a;
}
ast::variable_def parser_context::parse_variable_def() {
    ast::variable_def v {};
    expect(token_type::VAR);
    accept(token_type::PRIMITIVE_TYPE);
    expect(token_type::IDENTIFIER);
    expect(token_type::OP_ASSIGN);
    v.expression = parse_exp();
    return v;
}
ast::s_return parser_context::parse_return() {
    ast::s_return r {};
    expect(token_type::RETURN);
    r.expression = maybe(&parser_context::parse_exp);
    return r;
}
ast::s_break parser_context::parse_break() {
    ast::s_break b {};
    expect(token_type::BREAK);
    return b;
}
ast::s_continue parser_context::parse_continue() {
    ast::s_continue c {};
    expect(token_type::CONTINUE);
    return c;
}
ast::field_access parser_context::parse_field_access() {
    ast::field_access f {};
    expect(token_type::OP_ACCESS);
    f = parse_identifier();
    return f;
}
ast::array_access parser_context::parse_array_access() {
    ast::array_access a {};
    expect(token_type::OPEN_S_BRACKET);
    a = parse_exp();
    expect(token_type::CLOSE_S_BRACKET);
    return a;
}
ast::access parser_context::parse_access() {
    auto f = maybe(&parser_context::parse_field_access);
    if (f) {
        return std::move(f.value());
    }
    auto a = maybe(&parser_context::parse_array_access);
    if (a) {
        return std::move(a.value());
    }
    error(drv.location, "parser expected accessor. got");
}
ast::accessor parser_context::parse_accessor() {
    ast::accessor a {};
    a.identifier = parse_identifier();
    a.fields = parse_list(&parser_context::parse_access);
    return a;
}
ast::type_id parser_context::parse_named_type() {
    switch (current_token) {
        case token_type::PRIMITIVE_TYPE:
            return parse_primitive_type();
        case token_type::IDENTIFIER:
            return static_cast<ast::type_id>(std::get<uint64_t>(expectp(token_type::IDENTIFIER)));
        default:
            error(drv.location, "parser expected named type. got", current_token);
    }
}
ast::type parser_context::parse_type() {
    ast::type t {};
    auto n = maybe(&parser_context::parse_named_type);
    if (n) {
        t = n.value();
        return t;
    }
    auto s = maybe(&parser_context::parse_struct_type);
    if (s) {
        t = std::make_unique<ast::struct_type>(std::move(s.value()));
        return t;
    }
    auto a = maybe(&parser_context::parse_array_type);
    if (a) {
        t = std::make_unique<ast::array_type>(std::move(a.value()));
        return t;
    }
    error(drv.location, "parser expected type. got", current_token);
}
ast::type_id parser_context::parse_primitive_type() {
    ast::type_id t = std::get<ast::type_id>(expectp(token_type::PRIMITIVE_TYPE));
    return t;
}
ast::field parser_context::parse_field() {
    ast::field f;
    f.type = parse_named_type();
    f.identifier = parse_identifier();
    return f;
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
            l.explicit_type = maybe(&parser_context::parse_primitive_type);
            break;
        case token_type::LITERAL_INTEGER:
            l.literal = std::get<uint64_t>(expectp(current_token));
            l.explicit_type = maybe(&parser_context::parse_primitive_type);
            break;
        case token_type::LITERAL_FLOAT:
            l.literal = std::get<double>(expectp(current_token));
            l.explicit_type = maybe(&parser_context::parse_primitive_type);
            break;
        default:
            error(drv.location, "parser expected literal. got", current_token);
    }
    return l;
}
ast::literal_integer parser_context::parse_literal_integer() {
    return std::get<uint64_t>(expectp(token_type::LITERAL_INTEGER));
}
ast::statement parser_context::parse_top_level_statement() {
    ast::statement s;
    switch (current_token) {
        case token_type::EXPORT:
        case token_type::FUNCTION:  s.statement = parse_function_def(); break;
        case token_type::TYPE:      s.statement = parse_type_def(); break;
        case token_type::VAR:       s.statement = parse_variable_def(); break;
        default: error(drv.location, "parser expected top level statement: one of function def, type def, or variable def. got", current_token);
    }
    return s;
}
ast::statement parser_context::parse_statement() {
    ast::statement s;
    switch (current_token) {
        case token_type::EXPORT:
        case token_type::FUNCTION:  s.statement = parse_function_def(); break;
        case token_type::TYPE:      s.statement = parse_type_def(); break;
        case token_type::VAR:       s.statement = parse_variable_def(); break;
        case token_type::RETURN:    s.statement = parse_return(); break;
        case token_type::BREAK:     s.statement = parse_break(); break;
        case token_type::CONTINUE:  s.statement = parse_continue(); break;
        case token_type::IDENTIFIER: {
            auto a = maybe(&parser_context::parse_assignment);
            if (a) {
                s.statement = std::move(a.value());
            } else {
                auto e = maybe(&parser_context::parse_exp);
                if (e) {
                    s.statement = std::move(e.value());
                } else {
                    error(drv.location, "parser expected assignment or expression after token", current_token);
                }
            }
            break;
            }
        default:
            auto e = maybe(&parser_context::parse_exp);
            if (e) {
                s.statement = std::move(e.value());
            } else {
                error(drv.location, "parser expected statement. got", current_token);
            }
    }
    return s;
}
ast::expression parser_context::parse_exp() {
    return parse_exp_at_precedence(0);
}

ast::expression parser_context::parse_exp_atom() {
    ast::expression e;
    switch (current_token) {
        case token_type::LITERAL_BOOL:
        case token_type::LITERAL_INTEGER:
        case token_type::LITERAL_FLOAT: e.expression = parse_literal(); break;
        case token_type::IF:        e.expression = std::make_unique<ast::if_statement>(parse_if_statement()); break;
        case token_type::SWITCH:    e.expression = std::make_unique<ast::switch_statement>(parse_switch_statement()); break;
        case token_type::FOR:       e.expression = std::make_unique<ast::for_loop>(parse_for_loop()); break;
        case token_type::WHILE:     e.expression = std::make_unique<ast::while_loop>(parse_while_loop()); break;
        case token_type::OPEN_C_BRACKET: {
            auto b = parse_block();
            auto p = std::make_unique<ast::block>(std::move(b));
            assert(p);
            e.expression = std::move(p);
            break;
            }
        case token_type::IDENTIFIER:
            {
            auto f = maybe(&parser_context::parse_function_call);
            if (f) {
                e.expression = std::make_unique<ast::function_call>(std::move(f.value()));
            } else {
                auto a = maybe(&parser_context::parse_accessor);
                if (a) {
                    e.expression = std::make_unique<ast::accessor>(std::move(a.value()));
                } else {
                    error(drv.location, "parser expected function call or accessor after token", current_token);
                }
            }
            break;
            }
        case token_type::OPEN_R_BRACKET:
            {
            expect(token_type::OPEN_R_BRACKET);
            e.expression = std::move(parse_exp().expression);
            expect(token_type::CLOSE_R_BRACKET);
            break;
            }
        default:
            error(drv.location, "parser expected expression atom. got", current_token);
    }
    return e;
}

ast::expression parser_context::parse_exp_at_precedence(int current_precedence) {
    parse_exp_atom();
    while (true) {
        if (!is_operator(current_token)) {
            break;
        }
        auto p = get_precedence(current_token);
        if (p < current_precedence) {
            break;
        }
        auto a = get_associativity(current_token);
        accept(current_token);
        parse_exp_at_precedence(
            a == associativity::left ? p + 1 : p
        );
    }
    return {};
}
