#pragma once

#include <tl/expected.hpp>

#include "parser.hh"

void parser_context::next_token() {
    current_token = lexer.yylex();
    lexer.current_param = lexer.current_param;
    std::cerr << current_token << std::endl;
}
bool parser_context::accept(token_type t) {
    if (current_token == t) {
        next_token();
        return true;
    } else {
        return false;
    }
}
tl::expected<std::monostate, std::string> parser_context::expect(token_type t) {
    if (!accept(t)) {
        return tl::unexpected(string_error(location, "parser expected", t, "got", current_token));
    }
    return {};
}

#define TRY(X) ({ auto&& e = (X); if (!e) return tl::unexpected(e.error()); std::move(*e); })

template<typename T>
tl::expected<T, std::string> parser_context::expectp(token_type t) {
    auto p = lexer.current_param;
    TRY(expect(t));
    return std::get<T>(p);
}

template<typename T, typename E>
std::optional<T> to_optional(tl::expected<T, E> ex) {
    if (ex) {
        return std::optional{std::move(ex.value())};
    } else {
        return std::nullopt;
    }
}

using namespace std::string_literals;

template<typename T, typename B>
tl::expected<std::vector<T>, std::string> parser_context::parse_list_fn(B body) {
    std::vector<T> list;
    while (true) {
        auto r = body();
        if (r) {
            list.emplace_back(std::move(r.value()));
        } else {
            return list;
        }
    }
}

template<typename T, typename B, typename C>
tl::expected<std::vector<T>, std::string> parser_context::parse_list_fn(B body, C sep) {
    std::vector<T> list;
    while (true) {
        auto r = body();
        if (r) {
            list.emplace_back(std::move(r.value()));
            TRY(sep());
        } else {
            return list;
        }
    }
}

template<typename T, typename A, typename B, typename C, typename D>
tl::expected<std::vector<T>, std::string> parser_context::parse_list_fn(A begin, B body, C sep, D delim) {
    std::vector<T> list;
    TRY(begin());
    while (true) {
        list.emplace_back(TRY(body()));
        if (sep()) {
            if (delim()) {
                break;
            }
        } else if (delim()) {
            break;
        } else {
            return tl::unexpected("error"s);
        }
    }
    return list;
}

template<typename T, typename A, typename B, typename D>
tl::expected<std::vector<T>, std::string> parser_context::parse_list_fn(A begin, B body, D delim) {
    std::vector<T> list;
    TRY(begin());
    while (true) {
        list.emplace_back(TRY(body()));
        if (delim()) {
            break;
        }
    }
    return list;
}
