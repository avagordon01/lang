#pragma once

#include <iostream>
#include <fstream>
#include <optional>

#include "error.hh"
#include "ast.hh"
#include "tokens.hh"

struct lexer_context {
    using param_type = std::variant<ast::primitive_type, ast::identifier, bool, ast::literal_integer, double>;
    std::ifstream in;
    bi_registry<ast::identifier, std::string> symbols_registry;
    param_type current_param {};

    lexer_context(std::string filename): in(std::ifstream(filename, std::ios::binary)) {
        in >> std::noskipws;
        in.exceptions(std::istream::badbit);
    }

    struct backtrack_point {
        std::optional<std::ios::pos_type> p;
        std::ifstream& in_;
        backtrack_point(std::ifstream& in): in_(in) {
            p = {in_.tellg()};
        }
        void disable() {
            p = std::nullopt;
        }
        ~backtrack_point() {
            if (p) {
                in_.clear();
                in_.seekg(*p);
                if (in_.tellg() != p) {
                    error("error: failed to backtrack in input stream");
                }
            }
        }
    };

    bool lex_string(std::string s, bool word_boundary = false);
    std::optional<std::string> lex_word();
    bool lex_keyword(std::string keyword);
    std::optional<token_type> lex_any_keyword();
    void lex_reserved_keyword();
    std::optional<std::string> lex_identifier();
    std::optional<ast::primitive_type> lex_primitive_type();
    std::optional<bool> lex_literal_bool();
    std::optional<ast::literal_integer> lex_literal_integer();
    std::optional<double> lex_literal_float();
    std::optional<token_type> lex_any_char();
    std::optional<token_type> lex_operator();
    bool lex_whitespace();
    bool lex_comment();
    void lex_space();
    token_type yylex();
};
