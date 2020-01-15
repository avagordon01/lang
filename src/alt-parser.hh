#pragma once

#include <iostream>
#include <functional>
#include <deque>
#include <exception>

#include "ast.hh"
#include "tokens.hh"

using param_type = std::variant<ast::type_id, bool, uint64_t, double>;

struct driver;

struct parse_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct parser_context {
    driver& drv;
    parser_context(driver& drv_);

    size_t buffer_loc;
    std::deque<token_type> buffer {};
    token_type current_token {};

    void next_token();
    bool accept(token_type t);
    void expect(token_type t);

    template<typename T>
    bool maybe(T parse);
    template<typename T>
    void parse_list(T parse);
    template<typename T>
    void parse_list(T parse, token_type delim);
    template<typename T>
    void parse_list_sep(T parse, token_type sep);
    template<typename T>
    void parse_list(T parse, token_type sep, token_type delim);

    void parse_program();
    void parse_if_statement();
    void parse_for_loop();
    void parse_while_loop();
    void parse_switch_statement();
    void parse_function_def();
    void parse_function_call();
    void parse_type_def();
    void parse_assignment();
    void parse_variable_def();
    void parse_return();
    void parse_break();
    void parse_continue();
    void parse_block();
    void parse_access();
    void parse_accessor();
    void parse_type();
    void parse_primitive_type();
    void parse_field();
    void parse_struct_type();
    void parse_array_type();
    void parse_literal();
    void parse_literal_integer();
    void parse_top_level_statement();
    void parse_statement();
    void parse_exp();
    void parse_exp_atom();
    void parse_exp_at_precedence(int current_precedence);
};
