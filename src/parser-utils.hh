#pragma once

#include <sstream>
#include <tl/expected.hpp>

#include "parser.hh"

struct parse_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

template<typename... Ts>
[[noreturn]] void p_error(Ts... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    throw parse_error(ss.str());
}
template<typename... Ts>
std::string string_error(Ts... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    return ss.str();
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
tl::expected<std::monostate, std::string> parser_context::new_expect(token_type t) {
    if (!accept(t)) {
        return tl::unexpected(string_error(location, "parser expected", t, "got", current_token));
    }
    return {};
}
void parser_context::expect(token_type t) {
    if (!accept(t)) {
        p_error(location, "parser expected", t, "got", current_token);
    }
}
param_type parser_context::expectp(token_type t) {
    auto p = lexer.current_param;
    expect(t);
    return p;
}

template<typename T, typename E>
T parser_context::must(tl::expected<T, E> ex) {
    if (ex) {
        return std::move(ex.value());
    } else {
        error(ex.error());
    }
}

#define TRY(X) ({ auto&& e = (X); if (!e) return tl::unexpected(e.error()); std::move(*e); })

template<typename T, typename E>
std::optional<T> to_optional(tl::expected<T, E> ex) {
    if (ex) {
        return std::optional{std::move(ex.value())};
    } else {
        return std::nullopt;
    }
}
template<typename T>
auto parser_context::maybe(T parse) -> std::optional<decltype(std::invoke(parse, this))> {
    size_t buffer_stop = buffer_loc;
    try {
        return std::optional<decltype(std::invoke(parse, this))>{std::move(std::invoke(parse, this))};
    } catch (parse_error& e) {
        buffer_loc = buffer_stop;
        current_token = buffer[buffer_loc].first;
        lexer.current_param = buffer[buffer_loc].second;
        return std::nullopt;
    }
}
template<typename T>
auto parser_context::maybe_void(T parse) {
    size_t buffer_stop = buffer_loc;
    try {
        std::invoke(parse);
    } catch (parse_error& e) {
        buffer_loc = buffer_stop;
        current_token = buffer[buffer_loc].first;
        lexer.current_param = buffer[buffer_loc].second;
    }
}

template<typename ...T>
auto parser_context::choose(std::optional<T>... os) -> std::optional<std::variant<T...>> {
    static_assert(sizeof...(os) > 0, "choose must have at least one parameter");
    std::variant<T...> v;
    if (!((os ? v = std::move(os.value()), true : false) || ...)) {
        return std::nullopt;
    }
    return {std::move(v)};
}

template<typename ...T>
auto parser_context::choose(T... parse) -> std::optional<std::variant<decltype(std::invoke(parse, this))...>> {
    return choose(maybe(parse)...);
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
            p_error(location, "parser expected", sep, "or", delim, "got", current_token);
        }
    }
    return list;
}
