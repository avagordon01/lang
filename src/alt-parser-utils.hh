#include <sstream>

#include "alt-parser.hh"

struct parse_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

template<typename... Ts>
[[noreturn]] void error(Ts... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    throw parse_error(ss.str());
}

void parser_context::next_token() {
    buffer_loc++;
    if (buffer_loc >= buffer.size()) {
        buffer.push_back({yylex(drv), drv.current_param});
    }
    current_token = buffer[buffer_loc].first;
    current_param = buffer[buffer_loc].second;
}
bool parser_context::accept(token_type t) {
    if (current_token == t) {
        next_token();
        return true;
    } else {
        return false;
    }
}
void parser_context::expect(token_type t) {
    if (!accept(t)) {
        error(drv.location, "parser expected", t, "got", current_token);
    }
}
param_type parser_context::expectp(token_type t) {
    auto p = current_param;
    expect(t);
    return p;
}

template<typename T>
auto parser_context::maybe(T parse) -> std::optional<decltype(std::invoke(parse, this))> {
    size_t buffer_stop = buffer_loc;
    try {
        return std::optional<decltype(std::invoke(parse, this))>{std::move(std::invoke(parse, this))};
    } catch (parse_error& e) {
        buffer_loc = buffer_stop;
        current_token = buffer[buffer_loc].first;
        current_param = buffer[buffer_loc].second;
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
        current_param = buffer[buffer_loc].second;
    }
}

template<typename T>
std::vector<T> parser_context::parse_list(T (parser_context::*parse)()) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(maybe(parse));
        if (res) {
            list.emplace_back(std::move(res.value()));
        } else {
            return list;
        }
    }
}
template<typename T>
std::vector<T> parser_context::parse_list_sep(T (parser_context::*parse)(), token_type sep) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(maybe(parse));
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
template<typename T>
std::vector<T> parser_context::parse_list(T (parser_context::*parse)(), token_type delim) {
    std::vector<T> list;
    while (true) {
        auto res = std::move(std::invoke(parse, this));
        list.emplace_back(std::move(res));
        if (accept(delim)) {
            break;
        }
    }
    return list;
}
template<typename T>
std::vector<T> parser_context::parse_list(T (parser_context::*parse)(), token_type sep, token_type delim) {
    std::vector<T> list;
    while (true) {
        if (accept(delim)) {
            break;
        }
        auto res = std::move(std::invoke(parse, this));
        list.emplace_back(std::move(res));
        if (accept(sep)) {
            if (accept(delim)) {
                break;
            }
        } else if (accept(delim)) {
            break;
        } else {
            error(drv.location, "parser expected", sep, "or", delim, "got", current_token);
        }
    }
    return list;
}
