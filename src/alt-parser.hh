#pragma once

#include <iostream>
#include <functional>
#include <deque>
#include <exception>

#include "ast.hh"
#include "tokens.hh"

using param_type = std::variant<ast::type_id, bool, uint64_t, double>;

struct driver;

struct parser_context {
    driver& drv;
    parser_context(driver& drv_) : drv(drv_) {
        next_token();
    };

    size_t buffer_loc = -1;
    std::deque<std::pair<token_type, param_type>> buffer {};
    token_type current_token {};
    param_type current_param {};

    void next_token();
    bool accept(token_type t);
    void expect(token_type t);
    param_type expectp(token_type t);

    template<typename T>
    auto maybe(T parse) -> std::optional<decltype(std::invoke(parse, this))>;
    template<typename T>
    auto maybe_void(T parse);
    template<typename T>
    std::vector<T> parse_list(T (parser_context::*parse)());
    template<typename T>
    std::vector<T> parse_list(T (parser_context::*parse)(), token_type delim);
    template<typename T>
    std::vector<T> parse_list_sep(T (parser_context::*parse)(), token_type sep);
    template<typename T>
    std::vector<T> parse_list(T (parser_context::*parse)(), token_type sep, token_type delim);

    ast::program parse_program();
    ast::if_statement parse_if_statement();
    ast::for_loop parse_for_loop();
    ast::while_loop parse_while_loop();
    ast::case_statement parse_case();
    ast::switch_statement parse_switch_statement();
    ast::identifier parse_identifier();
    ast::type_id parse_type_id();
    ast::function_def parse_function_def();
    ast::function_call parse_function_call();
    ast::type_def parse_type_def();
    ast::assignment parse_assignment();
    ast::variable_def parse_variable_def();
    ast::s_return parse_return();
    ast::s_break parse_break();
    ast::s_continue parse_continue();
    ast::block parse_block();
    ast::field_access parse_field_access();
    ast::array_access parse_array_access();
    ast::access parse_access();
    ast::accessor parse_accessor();
    ast::type parse_type();
    ast::type_id parse_named_type();
    ast::type_id parse_primitive_type();
    ast::field parse_field();
    ast::struct_type parse_struct_type();
    ast::array_type parse_array_type();
    ast::literal parse_literal();
    ast::literal_integer parse_literal_integer();
    ast::statement parse_top_level_statement();
    ast::statement parse_statement();
    ast::expression parse_exp();
    ast::expression parse_exp_atom();
    ast::expression parse_exp_at_precedence(int current_precedence);
};
