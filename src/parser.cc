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
    s.conditions.emplace_back(TRY(parse_exp()));
    s.blocks.emplace_back(must(parse_block()));
    while (accept(token_type::ELIF)) {
        s.conditions.emplace_back(TRY(parse_exp()));
        s.blocks.emplace_back(must(parse_block()));
    }
    if (accept(token_type::ELSE)) {
        s.blocks.emplace_back(must(parse_block()));
    }
    return s;
}
tl::expected<ast::for_loop, std::string> parser_context::parse_for_loop() {
    TRY(new_expect(token_type::FOR));
    ast::for_loop s {};
    s.initial = TRY(parse_variable_def());
    TRY(new_expect(token_type::SEMICOLON));
    s.condition = TRY(parse_exp());
    TRY(new_expect(token_type::SEMICOLON));
    s.step = TRY(parse_assignment());
    s.block = TRY(parse_block());
    return s;
}
tl::expected<ast::while_loop, std::string> parser_context::parse_while_loop() {
    TRY(new_expect(token_type::WHILE));
    ast::while_loop s {};
    s.condition = TRY(parse_exp());
    s.block = TRY(parse_block());
    return s;
}
tl::expected<ast::case_statement, std::string> parser_context::parse_case() {
    TRY(new_expect(token_type::CASE));
    ast::case_statement c {};
    c.cases = parse_list_sep(&parser_context::parse_literal_integer, token_type::COMMA);
    c.block = TRY(parse_block());
    return c;
}
tl::expected<ast::switch_statement, std::string> parser_context::parse_switch_statement() {
    TRY(new_expect(token_type::SWITCH));
    ast::switch_statement s {};
    s.expression = TRY(parse_exp());
    TRY(new_expect(token_type::OPEN_C_BRACKET));
    s.cases = parse_list(&parser_context::parse_case, token_type::CLOSE_C_BRACKET);
    return s;
}
tl::expected<ast::identifier, std::string> parser_context::parse_identifier() {
    return expectp<ast::identifier>(token_type::IDENTIFIER);
}
tl::expected<ast::function_def, std::string> parser_context::parse_function_def() {
    ast::function_def f {};
    f.to_export = accept(token_type::EXPORT);
    TRY(new_expect(token_type::FUNCTION));
    auto t = parse_primitive_type();
    if (t) {
        f.returntype = {std::move(t.value())};
    }
    f.identifier = TRY(parse_identifier());
    TRY(new_expect(token_type::OPEN_R_BRACKET));
    f.parameter_list = parse_list(&parser_context::parse_field, token_type::COMMA, token_type::CLOSE_R_BRACKET);
    f.block = TRY(parse_block());
    return f;
}
tl::expected<ast::function_call, std::string> parser_context::parse_function_call() {
    ast::function_call f {};
    f.identifier = TRY(parse_identifier());
    TRY(new_expect(token_type::OPEN_R_BRACKET));
    f.arguments = parse_list(&parser_context::parse_exp, token_type::COMMA, token_type::CLOSE_R_BRACKET);
    return f;
}
tl::expected<ast::type_def, std::string> parser_context::parse_type_def() {
    ast::type_def t {};
    TRY(new_expect(token_type::TYPE));
    TRY(parse_identifier());
    TRY(new_expect(token_type::OP_ASSIGN));
    t.type = TRY(parse_type());
    return t;
}
tl::expected<ast::assignment, std::string> parser_context::parse_assignment() {
    ast::assignment a {};
    a.accessor = TRY(parse_accessor());
    TRY(new_expect(token_type::OP_ASSIGN));
    a.expression = TRY(parse_exp());
    return a;
}
tl::expected<ast::variable_def, std::string> parser_context::parse_variable_def() {
    ast::variable_def v {};
    TRY(new_expect(token_type::VAR));
    maybe_void([&v, this]() {
        auto t = must(parse_named_type());
        auto i = must(parse_identifier());
        v.explicit_type = t;
        v.identifier = i;
    });
    maybe_void([&v, this]() {
        v.identifier = must(parse_identifier());
    });
    //FIXME
    TRY(new_expect(token_type::OP_ASSIGN));
    v.expression = TRY(parse_exp());
    return v;
}
tl::expected<ast::s_return, std::string> parser_context::parse_return() {
    ast::s_return r {};
    TRY(new_expect(token_type::RETURN));
    r.expression = to_optional(parse_exp());
    return r;
}
tl::expected<ast::s_break, std::string> parser_context::parse_break() {
    ast::s_break b {};
    TRY(new_expect(token_type::BREAK));
    return b;
}
tl::expected<ast::s_continue, std::string> parser_context::parse_continue() {
    ast::s_continue c {};
    TRY(new_expect(token_type::CONTINUE));
    return c;
}
tl::expected<ast::field_access, std::string> parser_context::parse_field_access() {
    ast::field_access f {};
    TRY(new_expect(token_type::OP_ACCESS));
    f = TRY(parse_identifier());
    return f;
}
tl::expected<ast::array_access, std::string> parser_context::parse_array_access() {
    ast::array_access a {};
    TRY(new_expect(token_type::OPEN_S_BRACKET));
    a = TRY(parse_exp());
    TRY(new_expect(token_type::CLOSE_S_BRACKET));
    return a;
}
tl::expected<ast::access, std::string> parser_context::parse_access() {
    auto f = parse_field_access();
    if (f) {
        return std::move(f.value());
    }
    auto a = parse_array_access();
    if (a) {
        return std::move(a.value());
    }
    return tl::unexpected(string_error(location, "parser expected accessor. got"));
}
tl::expected<ast::accessor, std::string> parser_context::parse_accessor() {
    ast::accessor a {};
    a.identifier = TRY(parse_identifier());
    a.fields = parse_list(&parser_context::parse_access);
    return a;
}
tl::expected<ast::named_type, std::string> parser_context::parse_named_type() {
    auto p = parse_primitive_type();
    if (p) {
        return ast::named_type{p.value()};
    }
    if (current_token == token_type::IDENTIFIER) {
        //TODO
        return ast::named_type{ast::user_type {
            TRY(expectp<ast::identifier>(token_type::IDENTIFIER)).value
        }};
    }
    return tl::unexpected(string_error(location, "parser expected named type. got", current_token));
}
tl::expected<ast::type, std::string> parser_context::parse_type() {
    ast::type t {};
    auto n = parse_named_type();
    if (n) {
        return ast::type{n.value()};
    }
    auto s = parse_struct_type();
    if (s) {
        return ast::type{s.value()};
    }
    auto a = parse_array_type();
    if (a) {
        return ast::type{a.value()};
    }
    return tl::unexpected(string_error(location, "parser expected type. got", current_token));
}
tl::expected<ast::primitive_type, std::string> parser_context::parse_primitive_type() {
    return expectp<ast::primitive_type>(token_type::PRIMITIVE_TYPE);
}
tl::expected<ast::named_type, std::string> parser_context::parse_primitive_type_as_named_type() {
    return ast::named_type{TRY(parse_primitive_type())};
}
tl::expected<ast::field, std::string> parser_context::parse_field() {
    ast::field f;
    f.type = TRY(parse_named_type());
    f.identifier = TRY(parse_identifier());
    return f;
}
tl::expected<ast::struct_type, std::string> parser_context::parse_struct_type() {
    ast::struct_type s;
    TRY(new_expect(token_type::STRUCT));
    TRY(new_expect(token_type::OPEN_C_BRACKET));
    s.fields = parse_list(&parser_context::parse_field, token_type::COMMA, token_type::CLOSE_C_BRACKET);
    return s;
}
tl::expected<ast::array_type, std::string> parser_context::parse_array_type() {
    TRY(new_expect(token_type::OPEN_S_BRACKET));
    ast::array_type a;
    a.element_type = TRY(parse_named_type());
    a.length = std::get<ast::literal_integer>(TRY(parse_literal_integer()).literal).data;
    TRY(new_expect(token_type::CLOSE_S_BRACKET));
    return a;
}
tl::expected<ast::literal, std::string> parser_context::parse_literal() {
    ast::literal l {};
    switch (current_token) {
        case token_type::LITERAL_BOOL:
            l.literal = TRY(expectp<bool>(current_token));
            l.explicit_type = to_optional(parse_primitive_type_as_named_type());
            break;
        case token_type::LITERAL_INTEGER:
            l.literal = TRY(expectp<ast::literal_integer>(current_token));
            l.explicit_type = to_optional(parse_primitive_type_as_named_type());
            break;
        case token_type::LITERAL_FLOAT:
            l.literal = TRY(expectp<double>(current_token));
            l.explicit_type = to_optional(parse_primitive_type_as_named_type());
            break;
        default:
            return tl::unexpected(string_error(location, "parser expected literal. got", current_token));
    }
    return l;
}
tl::expected<ast::literal, std::string> parser_context::parse_literal_integer() {
    return ast::literal{TRY(expectp<ast::literal_integer>(token_type::LITERAL_INTEGER))};
}
tl::expected<ast::statement, std::string> parser_context::parse_top_level_statement() {
    ast::statement s;
    switch (current_token) {
        case token_type::EXPORT:
        case token_type::FUNCTION:  s.statement = TRY(parse_function_def()); break;
        case token_type::TYPE:      s.statement = TRY(parse_type_def()); break;
        case token_type::VAR:       s.statement = TRY(parse_variable_def()); break;
        default: return tl::unexpected(string_error(location, "parser expected top level statement: one of function def, type def, or variable def. got", current_token));
    }
    return s;
}
tl::expected<ast::statement, std::string> parser_context::parse_statement() {
    ast::statement s;
    auto e = parse_exp();
    if (e) {
        s.statement = std::move(e.value());
        return s;
    }
    auto f = parse_function_def();
    if (f) {
        s.statement = std::move(f.value());
        return s;
    }
    auto v = parse_variable_def();
    if (v) {
        s.statement = std::move(v.value());
        return s;
    }
    auto t = parse_type_def();
    if (t) {
        s.statement = std::move(t.value());
        return s;
    }
    auto a = parse_assignment();
    if (a) {
        s.statement = std::move(a.value());
        return s;
    }
    auto r = parse_return();
    if (r) {
        s.statement = std::move(r.value());
        return s;
    }
    auto b = parse_break();
    if (b) {
        s.statement = std::move(b.value());
        return s;
    }
    auto c = parse_continue();
    if (c) {
        s.statement = std::move(c.value());
        return s;
    }
    return tl::unexpected(string_error(location, "parser expected statement. got", current_token));
}
tl::expected<ast::expression, std::string> parser_context::parse_exp() {
    return parse_exp_at_precedence(0);
}

tl::expected<ast::expression, std::string> parser_context::parse_exp_atom() {
    ast::expression e {};
    switch (current_token) {
        case token_type::LITERAL_BOOL:
        case token_type::LITERAL_INTEGER:
        case token_type::LITERAL_FLOAT: e.expression = TRY(parse_literal()); break;
        case token_type::IF:        e.expression = std::make_unique<ast::if_statement>(TRY(parse_if_statement())); break;
        case token_type::SWITCH:    e.expression = std::make_unique<ast::switch_statement>(TRY(parse_switch_statement())); break;
        case token_type::FOR:       e.expression = std::make_unique<ast::for_loop>(TRY(parse_for_loop())); break;
        case token_type::WHILE:     e.expression = std::make_unique<ast::while_loop>(TRY(parse_while_loop())); break;
        case token_type::OPEN_C_BRACKET: {
            auto b = must(parse_block());
            auto p = std::make_unique<ast::block>(std::move(b));
            assert(p);
            e.expression = std::move(p);
            break;
            }
        case token_type::IDENTIFIER:
            {
            auto f = parse_function_call();
            if (f) {
                e.expression = std::make_unique<ast::function_call>(std::move(f.value()));
            } else {
                auto a = parse_accessor();
                if (a) {
                    e.expression = std::make_unique<ast::accessor>(std::move(a.value()));
                } else {
                    return tl::unexpected(string_error(location, "parser expected function call or accessor after token", current_token));
                }
            }
            break;
            }
        case token_type::OPEN_R_BRACKET:
            {
            TRY(new_expect(token_type::OPEN_R_BRACKET));
            e.expression = std::move(TRY(parse_exp()).expression);
            TRY(new_expect(token_type::CLOSE_R_BRACKET));
            break;
            }
        default:
            return tl::unexpected(string_error(location, "parser expected expression atom. got", current_token));
    }
    return e;
}

tl::expected<ast::expression, std::string> parser_context::parse_exp_at_precedence(int current_precedence) {
    //FIXME parsing unary operators
    ast::expression el = TRY(parse_exp_atom());
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
        er = to_optional(parse_exp_at_precedence(
            a == associativity::left ? p + 1 : p
        ));
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
