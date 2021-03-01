#pragma once

#include <iostream>
#include <functional>
#include <deque>
#include <exception>
#include <tl/expected.hpp>

#include "ast.hh"
#include "tokens.hh"
#include "lexer.hh"

using param_type = std::variant<ast::primitive_type, ast::identifier, bool, ast::literal_integer, double>;

struct parser_context {
    yy::location location;
    lexer_context& lexer;

    size_t buffer_loc = -1;
    std::deque<std::pair<token_type, param_type>> buffer {};
    token_type current_token {};

    parser_context(lexer_context& lexer_): lexer(lexer_) {}

    void next_token();
    bool accept(token_type t);
    tl::expected<std::monostate, std::string> new_expect(token_type t);
    template<typename T>
    tl::expected<T, std::string> expectp(token_type t);

    template<typename T, typename E>
    std::vector<T> parse_list(tl::expected<T, E> (parser_context::*parse)());
    template<typename T, typename E>
    std::vector<T> parse_list(tl::expected<T, E> (parser_context::*parse)(), token_type delim);
    template<typename T, typename E>
    std::vector<T> parse_list_sep(tl::expected<T, E> (parser_context::*parse)(), token_type sep);
    template<typename T, typename E>
    std::vector<T> parse_list(tl::expected<T, E> (parser_context::*parse)(), token_type sep, token_type delim);

    ast::program parse_program(std::string filename);
    tl::expected<ast::if_statement, std::string> parse_if_statement();
    tl::expected<ast::for_loop, std::string> parse_for_loop();
    tl::expected<ast::while_loop, std::string> parse_while_loop();
    tl::expected<ast::case_statement, std::string> parse_case();
    tl::expected<ast::switch_statement, std::string> parse_switch_statement();
    tl::expected<ast::identifier, std::string> parse_identifier();
    tl::expected<ast::function_def, std::string> parse_function_def();
    tl::expected<ast::function_call, std::string> parse_function_call();
    tl::expected<ast::type_def, std::string> parse_type_def();
    tl::expected<ast::assignment, std::string> parse_assignment();
    tl::expected<ast::variable_def, std::string> parse_variable_def();
    tl::expected<ast::s_return, std::string> parse_return();
    tl::expected<ast::s_break, std::string> parse_break();
    tl::expected<ast::s_continue, std::string> parse_continue();
    tl::expected<ast::block, std::string> parse_block();
    tl::expected<ast::field_access, std::string> parse_field_access();
    tl::expected<ast::array_access, std::string> parse_array_access();
    tl::expected<ast::access, std::string> parse_access();
    tl::expected<ast::accessor, std::string> parse_accessor();
    tl::expected<ast::type, std::string> parse_type();
    tl::expected<ast::named_type, std::string> parse_named_type();
    tl::expected<ast::primitive_type, std::string> parse_primitive_type();
    tl::expected<ast::named_type, std::string> parse_primitive_type_as_named_type();
    tl::expected<ast::field, std::string> parse_field();
    tl::expected<ast::struct_type, std::string> parse_struct_type();
    tl::expected<ast::array_type, std::string> parse_array_type();
    tl::expected<ast::literal, std::string> parse_literal();
    tl::expected<ast::literal, std::string> parse_literal_integer();
    tl::expected<ast::statement, std::string> parse_top_level_statement();
    tl::expected<ast::statement, std::string> parse_statement();
    tl::expected<ast::expression, std::string> parse_exp();
    tl::expected<ast::expression, std::string> parse_exp_atom();
    tl::expected<ast::expression, std::string> parse_exp_at_precedence(int current_precedence);
};
