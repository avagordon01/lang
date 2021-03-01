#pragma once

#include <iostream>
#include <fstream>
#include <optional>
#include <tl/expected.hpp>

#include "error.hh"
#include "ast.hh"
#include "tokens.hh"

struct backtrack_point {
    std::optional<std::ios::pos_type> p;
    std::ifstream& in;
    backtrack_point(std::ifstream& in_): in(in_) {
        p = {in.tellg()};
    }
    void disable() {
        p = std::nullopt;
    }
    ~backtrack_point() {
        if (p) {
            in.clear();
            in.seekg(*p);
            if (in.tellg() != p) {
                error("error: failed to backtrack in input stream");
            }
        }
    }
};

struct lexer_context {
    using param_type = std::variant<ast::primitive_type, ast::identifier, bool, ast::literal_integer, double>;
    std::ifstream in;
    bi_registry<ast::identifier, std::string> symbols_registry;
    param_type current_param {};

    lexer_context(std::string filename): in(std::ifstream(filename, std::ios::binary)) {
        in >> std::noskipws;
        in.exceptions(std::istream::badbit);
    }

    tl::expected<std::monostate, std::string> lex_string(std::string s, bool word_boundary = false);
    tl::expected<std::string, std::string> lex_word();
    tl::expected<std::monostate, std::string> lex_keyword(std::string keyword);
    std::optional<token_type> lex_any_keyword();
    void lex_reserved_keyword();
    tl::expected<std::string, std::string> lex_identifier();
    std::optional<ast::primitive_type> lex_primitive_type();
    std::optional<bool> lex_literal_bool();
    std::optional<ast::literal_integer> lex_literal_integer();
    std::optional<double> lex_literal_float();
    std::optional<token_type> lex_any_char();
    std::optional<token_type> lex_operator();
    bool lex_whitespace();
    tl::expected<void, std::string> lex_comment();
    void lex_space();
    token_type yylex();
};
