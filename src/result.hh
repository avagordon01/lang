#pragma once

#include <variant>
#include <optional>

#include "error.hh"

namespace parser {

template<typename T>
struct result {
    std::variant<T, parser::parse_error> data;

    template<typename ...Args>
    result(Args&&... args) : data(std::forward<Args>(args)...) {}

    template<typename P>
    result wrap(P parse) {
        try {
            data = std::invoke(parse);
        } catch (parser::parse_error& e) {
            data = e;
        }
        return *this;
    }

    operator bool() {
        return std::holds_alternative<T>(data);
    }
    operator std::optional<T>() {
        if (*this) {
            return {value()};
        } else {
            return std::nullopt;
        }
    }
    T&& value() {
        if (*this) {
            return std::get<T>(data);
        }
    }
    parser::parse_error error() {
        if (!*this) {
            return std::get<parser::parse_error>(data);
        }
    }
    void unwrap() {
        if (!*this) {
            throw std::get<parser::parse_error>(data);
        }
    }
};

}
