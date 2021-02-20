#include <iostream>
#include <optional>

#include "error.hh"
#include "ast.hh"

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
    in.peek();
}
bool lex_string(std::string s, bool word_boundary = false) {
    auto pos = lex_backtrack();
    std::string test (s.length(), 0);
    in.read(test.data(), s.length());
    if (test == s) {
        char c;
        in >> c;
        if (word_boundary && (std::isalnum(c) || c == '_')) {
            lex_backtrack(pos);
            return false;
        }
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
    if (lex_string(keyword, true)) {
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
        (os = lex_keyword("fpga")) ||
        (os = lex_keyword("f8"))
    ) {
        error("error, use of reserved keyword", os.value());
    }
}
std::optional<std::string> lex_identifier() {
    return lex_word();
}
std::optional<ast::primitive_type> lex_primitive_type() {
    auto pos = lex_backtrack();
    std::optional<std::string> os = lex_word();
    if (!os) {
        lex_backtrack(pos);
        return std::nullopt;
    }
    std::string s = os.value();
    if (false) {
    } else if (s == "void") {
        return ast::primitive_type{ast::primitive_type::t_void};
    } else if (s == "bool") {
        return ast::primitive_type{ast::primitive_type::t_bool};
    } else if (s == "u8") {
        return ast::primitive_type{ast::primitive_type::u8};
    } else if (s == "u16") {
        return ast::primitive_type{ast::primitive_type::u16};
    } else if (s == "u32") {
        return ast::primitive_type{ast::primitive_type::u32};
    } else if (s == "u64") {
        return ast::primitive_type{ast::primitive_type::u64};
    } else if (s == "i8") {
        return ast::primitive_type{ast::primitive_type::i8};
    } else if (s == "i16") {
        return ast::primitive_type{ast::primitive_type::i16};
    } else if (s == "i32") {
        return ast::primitive_type{ast::primitive_type::i32};
    } else if (s == "i64") {
        return ast::primitive_type{ast::primitive_type::i64};
    } else if (s == "f16") {
        return ast::primitive_type{ast::primitive_type::f16};
    } else if (s == "f32") {
        return ast::primitive_type{ast::primitive_type::f32};
    } else if (s == "f64") {
        return ast::primitive_type{ast::primitive_type::f64};
    } else {
        lex_backtrack(pos);
        return std::nullopt;
    }
}
std::optional<ast::literal_integer> lex_integer() {
    auto pos = lex_backtrack();
    //parse sign
    bool positive = true;
    char c = in.peek();
    if (c == '+') {
        in.get();
    } else if (c == '-') {
        positive = false;
        in.get();
    }
    if (!std::isdigit(in.peek())) {
        lex_backtrack(pos);
        return std::nullopt;
    }
    //parse base
    uint8_t base = 10;
    c = in.peek();
    if (c == '0') {
        in.get();
        c = in.peek();
        if (c == 'b' || c == 'B') {
            base =  2;
            in.get();
        } else if (c == 'o' || c == 'O') {
            base =  8;
            in.get();
        } else if (c == 'x' || c == 'X') {
            base = 16;
            in.get();
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
        in.get();
    }
    if (c == '.') {
        lex_backtrack(pos);
        return std::nullopt;
    }
    return {ast::literal_integer{positive ? value : -value}};
}
std::optional<ast::literal> lex_literal() {
    auto pos = lex_backtrack();
    {
        std::optional<std::string> os;
        if ((os = lex_keyword("true"))) {
            return {{true}};
        } else if ((os = lex_keyword("false"))) {
            return {{false}};
        }
    }
    lex_backtrack(pos);
    {
        auto ol = lex_integer();
        if (ol) {
            return {{ol.value()}};
        }
    }
    lex_backtrack(pos);
    {
        double d;
        in >> d;
        if (!in.fail()) {
            return {{d}};
        }
        in.clear();
    }
    lex_backtrack(pos);
    return std::nullopt;
}
bool lex_any_char() {
    return
        lex_string(";") ||
        lex_string(",") ||
        lex_string("(") ||
        lex_string(")") ||
        lex_string("[") ||
        lex_string("]") ||
        lex_string("{") ||
        lex_string("}") ||
        lex_string(".") ||
        lex_string("=");
}
std::optional<ast::binary_operator::op> lex_operator() {
    if (false) {
    } else if (lex_string("+")) {
        return {ast::binary_operator::A_ADD};
    } else if (lex_string("-")) {
        return {ast::binary_operator::A_SUB};
    } else if (lex_string("*")) {
        return {ast::binary_operator::A_MUL};
    } else if (lex_string("/")) {
        return {ast::binary_operator::A_DIV};
    } else if (lex_string("%")) {
        return {ast::binary_operator::A_MOD};

    } else if (lex_string("|")) {
        return {ast::binary_operator::B_OR};
    } else if (lex_string("^")) {
        return {ast::binary_operator::B_XOR};
    } else if (lex_string("~")) {
        //TODO return {ast::unary_operator::B_NOT};
        return {ast::binary_operator::A_ADD};
    } else if (lex_string("<<")) {
        return {ast::binary_operator::B_SHL};
    } else if (lex_string(">>")) {
        return {ast::binary_operator::B_SHR};

    } else if (lex_string("&&")) {
        return {ast::binary_operator::L_AND};
    } else if (lex_string("||")) {
        return {ast::binary_operator::L_OR};

    } else if (lex_string("==")) {
        return {ast::binary_operator::C_EQ};
    } else if (lex_string("!=")) {
        return {ast::binary_operator::C_NE};
    } else if (lex_string(">=")) {
        return {ast::binary_operator::C_GE};
    } else if (lex_string(">")) {
        return {ast::binary_operator::C_GT};
    } else if (lex_string("<=")) {
        return {ast::binary_operator::C_LE};
    } else if (lex_string("<")) {
        return {ast::binary_operator::C_LT};

    } else if (lex_string("&")) {
        return {ast::binary_operator::B_AND};
    } else if (lex_string("!")) {
        return {ast::binary_operator::A_ADD};
        //TODO return {ast::unary_operator::L_NOT};
    }
    return std::nullopt;
}
bool lex_whitespace() {
    in >> std::ws;
    return !in.fail();
}
bool lex_comment() {
    if (lex_string("//")) {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return true;
    } else if (lex_string("/*")) {
        while (true) {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '*');
            if (lex_string("/")) {
                return true;
            }
        }
    }
    if (in.fail()) {
        in.clear();
        in.peek();
    }
    return false;
}
bool lex_space() {
    bool any_space = false;
    do {
        any_space = any_space || lex_whitespace();
    } while (lex_comment());
    return any_space;
}

int main() {
    in >> std::noskipws;
    in.exceptions(std::istream::badbit);
    lex_space();
    while (!in.eof()) {
        std::optional<std::string> s;
        std::optional<ast::binary_operator::op> op;
        std::optional<ast::primitive_type> type;
        std::optional<ast::literal> literal;
        if (false) {
        } else if (lex_reserved_keyword(), false) {
        } else if (s = lex_any_keyword()) {
            std::cout << "keyword(" << s.value() << ")";
        } else if ((type = lex_primitive_type())) {
            std::cout << "type(" << type.value() << ")";
        } else if ((literal = lex_literal())) {
            std::cout << "literal()";
        } else if (s = lex_identifier()) {
            std::cout << "identifier(" << s.value() << ")";
        } else if (lex_any_char()) {
            std::cout << "any_char";
        } else if ((op = lex_operator())) {
            std::cout << "operator(" << op.value() << ")";
        } else {
            std::string s;
            in >> s;
            error("error: unknown input", s);
        }
        lex_space();
        std::cout << ", ";
    }
    std::cout << std::endl;
}
