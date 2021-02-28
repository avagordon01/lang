#include <iostream>

#include "ast.hh"
#include "driver.hh"
#include "parser.hh"
#include "parser-utils.hh"

ast::program parser_context::parse_program(std::string filename) {
    location.initialize(&filename);
    ast::program program_ast {};
    next_token();
    try {
        program_ast.statements = parse_list(&parser_context::parse_top_level_statement, token_type::SEMICOLON, token_type::T_EOF);
    } catch (parse_error& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    program_ast.symbols_registry = lexer.symbols_registry;
    return program_ast;
}
tl::expected<ast::block, std::string> parser_context::parse_block() {
    TRY(new_expect(token_type::OPEN_C_BRACKET));
    ast::block b {};
    b.statements = parse_list(&parser_context::parse_statement, token_type::SEMICOLON, token_type::CLOSE_C_BRACKET);
    return b;
}
tl::expected<ast::if_statement, std::string> parser_context::parse_if_statement() {
    TRY(new_expect(token_type::IF));
    ast::if_statement s {};
    s.conditions.emplace_back(parse_exp());
    s.blocks.emplace_back(must(parse_block()));
    while (accept(token_type::ELIF)) {
        s.conditions.emplace_back(parse_exp());
        s.blocks.emplace_back(must(parse_block()));
    }
    if (accept(token_type::ELSE)) {
        s.blocks.emplace_back(must(parse_block()));
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
    s.block = must(parse_block());
    return s;
}
ast::while_loop parser_context::parse_while_loop() {
    expect(token_type::WHILE);
    ast::while_loop s {};
    s.condition = parse_exp();
    s.block = must(parse_block());
    return s;
}
ast::case_statement parser_context::parse_case() {
    expect(token_type::CASE);
    ast::case_statement c {};
    c.cases = parse_list_sep(&parser_context::parse_literal_integer, token_type::COMMA);
    c.block = must(parse_block());
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
    return std::get<ast::identifier>(expectp(token_type::IDENTIFIER));
}
ast::function_def parser_context::parse_function_def() {
    ast::function_def f {};
    f.to_export = accept(token_type::EXPORT);
    expect(token_type::FUNCTION);
    auto t = maybe(&parser_context::parse_primitive_type);
    if (t) {
        f.returntype = {std::move(t.value())};
    }
    f.identifier = parse_identifier();
    expect(token_type::OPEN_R_BRACKET);
    f.parameter_list = parse_list(&parser_context::parse_field, token_type::COMMA, token_type::CLOSE_R_BRACKET);
    f.block = must(parse_block());
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
    maybe_void([&v, this]() {
        auto t = parse_named_type();
        auto i = parse_identifier();
        v.explicit_type = t;
        v.identifier = i;
    });
    maybe_void([&v, this]() {
        v.identifier = parse_identifier();
    });
    //FIXME
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
    p_error(location, "parser expected accessor. got");
}
ast::accessor parser_context::parse_accessor() {
    ast::accessor a {};
    a.identifier = parse_identifier();
    a.fields = parse_list(&parser_context::parse_access);
    return a;
}
ast::named_type parser_context::parse_named_type() {
    switch (current_token) {
        case token_type::PRIMITIVE_TYPE:
            return {parse_primitive_type()};
        case token_type::IDENTIFIER:
            //TODO
            return {ast::user_type {
                std::get<ast::identifier>(expectp(token_type::IDENTIFIER)).value
            }};
        default:
            p_error(location, "parser expected named type. got", current_token);
    }
}
ast::type parser_context::parse_type() {
    ast::type t {};
    auto n = maybe(&parser_context::parse_named_type);
    if (n) {
        return ast::type{n.value()};
    }
    auto s = maybe(&parser_context::parse_struct_type);
    if (s) {
        return ast::type{s.value()};
    }
    auto a = maybe(&parser_context::parse_array_type);
    if (a) {
        return ast::type{a.value()};
    }
    p_error(location, "parser expected type. got", current_token);
}
ast::primitive_type parser_context::parse_primitive_type() {
    return std::get<ast::primitive_type>(expectp(token_type::PRIMITIVE_TYPE));
}
ast::named_type parser_context::parse_primitive_type_as_named_type() {
    return ast::named_type{parse_primitive_type()};
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
    a.element_type = parse_named_type();
    a.length = std::get<ast::literal_integer>(parse_literal_integer().literal).data;
    expect(token_type::CLOSE_S_BRACKET);
    return a;
}
ast::literal parser_context::parse_literal() {
    ast::literal l {};
    switch (current_token) {
        case token_type::LITERAL_BOOL:
            l.literal = std::get<bool>(expectp(current_token));
            l.explicit_type = maybe(&parser_context::parse_primitive_type_as_named_type);
            break;
        case token_type::LITERAL_INTEGER:
            l.literal = std::get<ast::literal_integer>(expectp(current_token));
            l.explicit_type = maybe(&parser_context::parse_primitive_type_as_named_type);
            break;
        case token_type::LITERAL_FLOAT:
            l.literal = std::get<double>(expectp(current_token));
            l.explicit_type = maybe(&parser_context::parse_primitive_type_as_named_type);
            break;
        default:
            p_error(location, "parser expected literal. got", current_token);
    }
    return l;
}
ast::literal parser_context::parse_literal_integer() {
    return ast::literal{std::get<ast::literal_integer>(expectp(token_type::LITERAL_INTEGER))};
}
ast::statement parser_context::parse_top_level_statement() {
    ast::statement s;
    switch (current_token) {
        case token_type::EXPORT:
        case token_type::FUNCTION:  s.statement = parse_function_def(); break;
        case token_type::TYPE:      s.statement = parse_type_def(); break;
        case token_type::VAR:       s.statement = parse_variable_def(); break;
        default: p_error(location, "parser expected top level statement: one of function def, type def, or variable def. got", current_token);
    }
    return s;
}
ast::statement parser_context::parse_statement() {
    ast::statement s;
    auto v = choose(
        &parser_context::parse_exp,
        &parser_context::parse_function_def,
        &parser_context::parse_variable_def,
        &parser_context::parse_type_def,
        &parser_context::parse_assignment,
        &parser_context::parse_return,
        &parser_context::parse_break,
        &parser_context::parse_continue
    );
    if (!v) {
        p_error(location, "parser expected statement. got", current_token);
    }
    s.statement = std::move(v.value());
    return s;
}
ast::expression parser_context::parse_exp() {
    return parse_exp_at_precedence(0);
}

ast::expression parser_context::parse_exp_atom() {
    ast::expression e {};
    switch (current_token) {
        case token_type::LITERAL_BOOL:
        case token_type::LITERAL_INTEGER:
        case token_type::LITERAL_FLOAT: e.expression = parse_literal(); break;
        case token_type::IF:        e.expression = std::make_unique<ast::if_statement>(must(parse_if_statement())); break;
        case token_type::SWITCH:    e.expression = std::make_unique<ast::switch_statement>(parse_switch_statement()); break;
        case token_type::FOR:       e.expression = std::make_unique<ast::for_loop>(parse_for_loop()); break;
        case token_type::WHILE:     e.expression = std::make_unique<ast::while_loop>(parse_while_loop()); break;
        case token_type::OPEN_C_BRACKET: {
            auto b = must(parse_block());
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
                    p_error(location, "parser expected function call or accessor after token", current_token);
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
            p_error(location, "parser expected expression atom. got", current_token);
    }
    return e;
}

ast::expression parser_context::parse_exp_at_precedence(int current_precedence) {
    //FIXME parsing unary operators
    ast::expression el = parse_exp_atom();
    std::optional<ast::expression> er {};
    token_type op {};
    while (true) {
        if (!is_operator(current_token)) {
            break;
        }
        op = current_token;
        auto p = get_precedence(current_token);
        if (p < current_precedence) {
            break;
        }
        auto a = get_associativity(current_token);
        accept(current_token);
        er = {parse_exp_at_precedence(
            a == associativity::left ? p + 1 : p
        )};
    }
    if (er) {
        ast::binary_operator b {};
        b.l = std::move(el);
        b.r = std::move(er.value());
        b.binary_operator = get_binary_operator(op);
        ast::expression e {};
        e.expression = std::make_unique<ast::binary_operator>(std::move(b));
        return e;
    } else {
        return el;
    }
}
