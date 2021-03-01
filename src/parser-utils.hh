#pragma once

#include <sstream>
#include <tl/expected.hpp>

#include "parser.hh"

template<typename... Ts>
std::string string_error(Ts... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    return ss.str();
}

[[noreturn]] void fatal_error(std::string e) {
    std::cerr << e << std::endl;
    exit(1);
}

void parser_context::next_token() {
    buffer_loc++;
    if (buffer_loc >= buffer.size()) {
        buffer.push_back({lexer.yylex(), lexer.current_param});
        std::cerr << buffer[buffer_loc].first << std::endl;
    }
    current_token = buffer[buffer_loc].first;
    lexer.current_param = buffer[buffer_loc].second;
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

template<typename T, typename E>
std::vector<T> parser_context::parse_list(tl::expected<T, E> (parser_context::*parse)()) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(std::invoke(parse, this));
        if (res) {
            list.emplace_back(std::move(res.value()));
        } else {
            return list;
        }
    }
}
template<typename T, typename E>
std::vector<T> parser_context::parse_list_sep(tl::expected<T, E> (parser_context::*parse)(), token_type sep) {
    std::vector<T> list;
    while (true) {
        auto res = std::move((std::invoke(parse, this)));
        if (!res) {
            break;
        }
        list.emplace_back(std::move(res.value()));
        if (!accept(sep)) {
            break;
        }
    }
    return list;
}
template<typename T, typename E>
std::vector<T> parser_context::parse_list(tl::expected<T, E> (parser_context::*parse)(), token_type delim) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(std::invoke(parse, this));
        list.emplace_back(std::move(res.value()));
        if (accept(delim)) {
            break;
        }
    }
    return list;
}
template<typename T, typename E>
std::vector<T> parser_context::parse_list(tl::expected<T, E> (parser_context::*parse)(), token_type sep, token_type delim) {
    std::vector<T> list;
    while (true) {
        if (accept(delim)) {
            break;
        }
        auto res = std::move(std::invoke(parse, this));
        list.emplace_back(std::move(res.value()));
        if (accept(sep)) {
            if (accept(delim)) {
                break;
            }
        } else if (accept(delim)) {
            break;
        } else {
            //TODO
            //string_error(location, "parser expected", sep, "or", delim, "got", current_token);
        }
    }
    return list;
}
