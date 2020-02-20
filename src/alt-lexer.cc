#include <iostream>
#include <optional>
#include <cassert>

std::istream& in = std::cin;

std::ios::pos_type lex_backtrack() {
    assert(in.good());
    return in.tellg();
    assert(in.good());
}
void lex_backtrack(std::ios::pos_type pos) {
    assert(in.good());
    in.seekg(pos);
    assert(in.good());
}
std::optional<std::string> lex_numeric() {
    std::string s;
    bool first = true;
    while (!in.eof()) {
        char c = in.peek();
        if (std::isdigit(c)) {
            first = false;
            in >> c;
            s += c;
        } else if (first) {
            return std::nullopt;
        } else {
            break;
        }
    }
    return {s};
}
bool lex_string(std::string s) {
    auto pos = lex_backtrack();
    std::string test (s.length(), 0);
    in.read(test.data(), s.length());
    assert(in.good());
    if (test == s) {
        return true;
    } else {
        lex_backtrack(pos);
        return false;
    }
}
std::optional<std::string> lex_word() {
    std::string s;
    bool first = true;
    while (!in.eof()) {
        char c = in.peek();
        if (std::isalpha(c) || (!first && std::isdigit(c))) {
            first = false;
            in >> c;
            s += c;
        } else if (first) {
            return std::nullopt;
        } else {
            break;
        }
    }
    return {s};
}
std::optional<std::string> lex_keyword(std::string keyword) {
    auto pos = lex_backtrack();
    //TODO use lex_string
    std::optional<std::string> os = lex_word();
    if (!os) {
        lex_backtrack(pos);
        return std::nullopt;
    }
    std::string s = os.value();
    if (s != keyword) {
        lex_backtrack(pos);
        return std::nullopt;
    }
    return {s};
}
std::optional<std::string> lex_any_keyword() {
    std::optional<std::string> os;
    if (
        (os = lex_keyword("var"), os) ||
        (os = lex_keyword("if"), os) ||
        (os = lex_keyword("elif"), os) ||
        (os = lex_keyword("else"), os) ||
        (os = lex_keyword("for"), os) ||
        (os = lex_keyword("while"), os) ||
        (os = lex_keyword("fn"), os) ||
        (os = lex_keyword("return"), os) ||
        (os = lex_keyword("break"), os) ||
        (os = lex_keyword("continue"), os) ||
        (os = lex_keyword("switch"), os) ||
        (os = lex_keyword("case"), os) ||
        (os = lex_keyword("import"), os) ||
        (os = lex_keyword("export"), os) ||
        (os = lex_keyword("struct"), os) ||
        (os = lex_keyword("type"), os)
    ) {
        return os;
    } else {
        return std::nullopt;
    }
}
std::optional<std::string> lex_reserved_keyword() {
    std::optional<std::string> os;
    if (
        (os = lex_keyword("const"), os) ||
        (os = lex_keyword("auto"), os) ||
        (os = lex_keyword("sizeof"), os) ||
        (os = lex_keyword("offsetof"), os) ||
        (os = lex_keyword("typeof"), os) ||
        (os = lex_keyword("static"), os) ||
        (os = lex_keyword("repl"), os) ||
        (os = lex_keyword("cpu"), os) ||
        (os = lex_keyword("simd"), os) ||
        (os = lex_keyword("gpu"), os) ||
        (os = lex_keyword("fpga"), os)
    ) {
        return os;
    } else {
        return std::nullopt;
    }
}
std::optional<std::string> lex_identifier() {
    return lex_word();
}
std::optional<std::string> lex_primitive_type() {
    auto pos = lex_backtrack();
    std::optional<std::string> os = lex_word();
    if (!os) {
        lex_backtrack(pos);
        return std::nullopt;
    }
    std::string s = os.value();
    if (s == "void") {
    } else if (s == "bool") {
    } else if (s == "u8") {
    } else if (s == "u16") {
    } else if (s == "u32") {
    } else if (s == "u64") {
    } else if (s == "i8") {
    } else if (s == "i16") {
    } else if (s == "i32") {
    } else if (s == "i64") {
    } else if (s == "f8") {
        assert(false);
    } else if (s == "f16") {
    } else if (s == "f32") {
    } else if (s == "f64") {
    } else {
        lex_backtrack(pos);
        return std::nullopt;
    }
    return {s};
}
std::optional<std::string> lex_integer() {
    auto pos = lex_backtrack();
    //parse sign
    bool positive = true;
    char c = in.peek();
    if (c == '+') {
        in >> c;
    } else if (c == '-') {
        positive = false;
        in >> c;
    }
    if (!std::isdigit(in.peek())) {
        lex_backtrack(pos);
        return std::nullopt;
    }
    //parse base
    uint8_t base = 10;
    c = in.peek();
    if (c == '0') {
        in >> c;
        c = in.peek();
        if (c == 'b' || c == 'B') {
            base =  2;
            in >> c;
        } else if (c == 'o' || c == 'O') {
            base =  8;
            in >> c;
        } else if (c == 'x' || c == 'X') {
            base = 16;
            in >> c;
        }
    }
    //parse digits
    uint64_t value = 0;
    while (!in.eof()) {
        c = in.peek();
        if (base <= 10 && c >= '0' && c <= '0' + base - 1) {
            value *= base;
            value += c - '0';
        } else if (base <= 32 && c >= '0' && c <= '9') {
            value *= base;
            value += c - '0';
        } else if (base <= 32 && c >= 'a' && c <= 'a' + base - 11) {
            value *= base;
            value += c - 'a';
        } else if (base <= 32 && c >= 'A' && c <= 'A' + base - 11) {
            value *= base;
            value += c - 'A';
        } else if (c == '_' || c == ',') {
        } else {
            break;
        }
        in >> c;
    }
    return {std::to_string(positive ? value : -value)};
}
std::optional<std::string> lex_literal() {
    auto pos = lex_backtrack();
    std::optional<std::string> os = lex_word();
    if (!os) {
        lex_backtrack(pos);
        return lex_integer();
    }
    std::string s = os.value();
    if (s == "true") {
    } else if (s == "false") {
    } else {
        lex_backtrack(pos);
        return lex_integer();
    }
    return {s};
}
bool lex_char(char c) {
    if (in.peek() == c) {
        in >> c;
        return true;
    } else {
        return false;
    }
}
bool lex_any_char() {
    if (
        lex_char(';') ||
        lex_char(',') ||
        lex_char('(') ||
        lex_char(')') ||
        lex_char('[') ||
        lex_char(']') ||
        lex_char('{') ||
        lex_char('}') ||
        lex_char('.') ||
        lex_char('=')
    ) {
        return true;
    } else {
        return false;
    }
}
bool lex_operator() {
    if (
        //FIXME && is broken
        lex_char('+') ||
        lex_char('-') ||
        lex_char('*') ||
        lex_char('/') ||
        lex_char('%') ||

        lex_char('|') ||
        lex_char('^') ||
        lex_char('~') ||
        (lex_char('<') && lex_char('<')) ||
        (lex_char('>') && lex_char('>')) ||

        (lex_char('&') && lex_char('&')) ||
        (lex_char('|') && lex_char('|')) ||

        (lex_char('=') && lex_char('=')) ||
        (lex_char('!') && lex_char('=')) ||
        (lex_char('>') && lex_char('=')) ||
        lex_char('>') ||
        (lex_char('<') && lex_char('=')) ||
        lex_char('<') ||

        lex_char('&') ||
        lex_char('!') ||
    true) {
        return true;
    } else {
        return false;
    }
}
bool lex_whitespace() {
    in >> std::ws;
    return in.fail();
}
bool lex_comment() {
    if (lex_string("//")) {
        in.ignore(1000, '\n');
        return true;
    } else if (lex_string("/*")) {
        while (true) {
            in.ignore(1000, '*');
            if (lex_char('/')) {
                break;
            }
        }
        return true;
    }
    return false;
}

int main() {
    in >> std::noskipws;
    while (!in.eof()) {
        std::optional<std::string> s;
        if (s = lex_primitive_type(), s) {
            std::cout << "type: " << s.value();
        } else if (lex_comment()) {
            std::cout << "COMMENT";
        } else if (s = lex_any_keyword(), s) {
            std::cout << "keyword: " << s.value();
        } else if (s = lex_reserved_keyword(), s) {
            std::cout << "reserved: " << s.value();
        } else if (s = lex_literal(), s) {
            std::cout << "literal: " << s.value();
        } else if (s = lex_identifier(), s) {
            std::cout << "identifier: " << s.value();
        } else if (lex_any_char()) {
            std::cout << "ANY CHAR";
        } else if (lex_operator()) {
            std::cout << "OPERATOR";
        } else {
            assert(false);
        }
        std::cout << std::endl;
        lex_whitespace();
    }
    std::cout << std::endl;
}