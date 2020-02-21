#include <iostream>
#include <optional>

#include "error.hh"

std::istream& in = std::cin;

std::ios::pos_type lex_backtrack() {
    return in.tellg();
}
void lex_backtrack(std::ios::pos_type p) {
    in.clear();
    in.seekg(p);
    if (in.tellg() != p) {
        error("error: failed to backtrack in input stream");
    }
}
bool lex_string(std::string s) {
    auto pos = lex_backtrack();
    std::string test (s.length(), 0);
    in.read(test.data(), s.length());
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
    char c;
    while (in >> c) {
        if (std::isalpha(c) || (!first && std::isdigit(c))) {
            first = false;
            s += c;
        } else {
            in.unget();
            if (in.fail()) {
                error("error: failed to unget on input stream");
            }
            break;
        }
    }
    if (s.empty()) {
        return std::nullopt;
    } else {
        return {s};
    }
}
std::optional<std::string> lex_keyword(std::string keyword) {
    if (lex_string(keyword)) {
        return {keyword};
    } else {
        return std::nullopt;
    }
}
std::optional<std::string> lex_any_keyword() {
    std::optional<std::string> os;
    if (
        (os = lex_keyword("var")) ||
        (os = lex_keyword("if")) ||
        (os = lex_keyword("elif")) ||
        (os = lex_keyword("else")) ||
        (os = lex_keyword("for")) ||
        (os = lex_keyword("while")) ||
        (os = lex_keyword("fn")) ||
        (os = lex_keyword("return")) ||
        (os = lex_keyword("break")) ||
        (os = lex_keyword("continue")) ||
        (os = lex_keyword("switch")) ||
        (os = lex_keyword("case")) ||
        (os = lex_keyword("import")) ||
        (os = lex_keyword("export")) ||
        (os = lex_keyword("struct")) ||
        (os = lex_keyword("type"))
    ) {
        return os;
    } else {
        return std::nullopt;
    }
}
void lex_reserved_keyword() {
    std::optional<std::string> os;
    if (
        (os = lex_keyword("const")) ||
        (os = lex_keyword("auto")) ||
        (os = lex_keyword("sizeof")) ||
        (os = lex_keyword("offsetof")) ||
        (os = lex_keyword("typeof")) ||
        (os = lex_keyword("static")) ||
        (os = lex_keyword("repl")) ||
        (os = lex_keyword("cpu")) ||
        (os = lex_keyword("simd")) ||
        (os = lex_keyword("gpu")) ||
        (os = lex_keyword("fpga"))
    ) {
        error("error, use of reserved keyword", os.value());
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
        error("error, use of reserved keyword", s);
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
    return
        lex_char(';') ||
        lex_char(',') ||
        lex_char('(') ||
        lex_char(')') ||
        lex_char('[') ||
        lex_char(']') ||
        lex_char('{') ||
        lex_char('}') ||
        lex_char('.') ||
        lex_char('=');
}
bool lex_operator() {
    return
        lex_char('+') ||
        lex_char('-') ||
        lex_char('*') ||
        lex_char('/') ||
        lex_char('%') ||

        lex_char('|') ||
        lex_char('^') ||
        lex_char('~') ||
        lex_string("<<") ||
        lex_string(">>") ||

        lex_string("&&") ||
        lex_string("||") ||

        lex_string("==") ||
        lex_string("!=") ||
        lex_string(">=") ||
        lex_char('>') ||
        lex_string("<=") ||
        lex_char('<') ||

        lex_char('&') ||
        lex_char('!') ||
    false;
}
bool lex_whitespace() {
    in >> std::ws;
    return in.fail();
}
bool lex_comment() {
    if (lex_string("//")) {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return true;
    } else if (lex_string("/*")) {
        while (true) {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '*');
            if (lex_char('/')) {
                return true;
            }
        }
    }
    return false;
}

int main() {
    in >> std::noskipws;
    while (!in.eof()) {
        std::optional<std::string> s;
        if (s = lex_primitive_type()) {
            std::cout << "type: " << s.value();
        } else if (lex_comment()) {
            std::cout << "COMMENT";
        } else if (s = lex_any_keyword()) {
            std::cout << "keyword: " << s.value();
        } else if (lex_reserved_keyword(), false) {
        } else if (s = lex_literal()) {
            std::cout << "literal: " << s.value();
        } else if (s = lex_identifier()) {
            std::cout << "identifier: " << s.value();
        } else if (lex_any_char()) {
            std::cout << "ANY CHAR";
        } else if (lex_operator()) {
            std::cout << "OPERATOR";
        } else {
            std::string s;
            in >> s;
            error("error: unknown input", s);
        }
        std::cout << std::endl;
        lex_whitespace();
    }
    std::cout << std::endl;
}
