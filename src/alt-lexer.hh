#include <iostream>
#include <optional>

#include "error.hh"
#include "ast.hh"
#include "tokens.hh"
#include "parser.hh"

struct lexer_context {
    parser_context& pc;
    constexpr static std::istream& in = std::cin;

    lexer_context(parser_context& pc_): pc(pc_) {
        in >> std::noskipws;
        in.exceptions(std::istream::badbit);
    }

    struct backtrack_point {
        std::optional<std::ios::pos_type> p;
        backtrack_point() {
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

    bool lex_string(std::string s, bool word_boundary = false);
    std::optional<std::string> lex_word();
    std::optional<std::string> lex_keyword(std::string keyword);
    std::optional<token_type> lex_any_keyword();
    void lex_reserved_keyword();
    std::optional<std::string> lex_identifier();
    std::optional<ast::primitive_type> lex_primitive_type();
    std::optional<ast::literal_integer> lex_integer();
    std::optional<bool> lex_literal_bool();
    std::optional<ast::literal_integer> lex_literal_integer();
    std::optional<double> lex_literal_float();
    std::optional<token_type> lex_any_char();
    std::optional<token_type> lex_operator();
    bool lex_whitespace();
    bool lex_comment();
    bool lex_space();
    token_type yylex();
};
