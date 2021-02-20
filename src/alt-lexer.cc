#include <iostream>
#include <optional>

#include "alt-lexer.hh"
#include "error.hh"
#include "ast.hh"
#include "tokens.hh"
#include "parser.hh"

bool lexer_context::lex_string(std::string s, bool word_boundary) {
    backtrack_point bp;
    std::string test (s.length(), 0);
    in.read(test.data(), s.length());
    if (test == s) {
        char c;
        in >> c;
        if (word_boundary && (std::isalnum(c) || c == '_')) {
            return false;
        }
        bp.disable();
        return true;
    } else {
        return false;
    }
}
std::optional<std::string> lexer_context::lex_word() {
    std::string s;
    bool first = true;
    char c;
    /*TODO
    std::find_if_not(
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>(),
        [](char c){return std::isalnum
    );
    */
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
std::optional<std::string> lexer_context::lex_keyword(std::string keyword) {
    if (lex_string(keyword, true)) {
        return {keyword};
    } else {
        return std::nullopt;
    }
}
std::optional<token_type> lexer_context::lex_any_keyword() {
    if (false) {
    } else if (lex_keyword("var")) {
        return token_type::VAR;
    } else if (lex_keyword("if")) {
        return token_type::IF;
    } else if (lex_keyword("elif")) {
        return token_type::ELIF;
    } else if (lex_keyword("else")) {
        return token_type::ELSE;
    } else if (lex_keyword("for")) {
        return token_type::FOR;
    } else if (lex_keyword("while")) {
        return token_type::WHILE;
    } else if (lex_keyword("fn")) {
        return token_type::FUNCTION;
    } else if (lex_keyword("return")) {
        return token_type::RETURN;
    } else if (lex_keyword("break")) {
        return token_type::BREAK;
    } else if (lex_keyword("continue")) {
        return token_type::CONTINUE;
    } else if (lex_keyword("switch")) {
        return token_type::SWITCH;
    } else if (lex_keyword("case")) {
        return token_type::CASE;
    } else if (lex_keyword("import")) {
        return token_type::IMPORT;
    } else if (lex_keyword("export")) {
        return token_type::EXPORT;
    } else if (lex_keyword("struct")) {
        return token_type::STRUCT;
    } else if (lex_keyword("type")) {
        return token_type::TYPE;
    } else {
        return std::nullopt;
    }
}
void lexer_context::lex_reserved_keyword() {
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
std::optional<std::string> lexer_context::lex_identifier() {
    return lex_word();
}
std::optional<ast::primitive_type> lexer_context::lex_primitive_type() {
    backtrack_point bp;
    std::optional<std::string> os = lex_word();
    if (!os) {
        return std::nullopt;
    }
    std::string s = os.value();
    if (false) {
    } else if (s == "void") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::t_void};
    } else if (s == "bool") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::t_bool};
    } else if (s == "u8") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::u8};
    } else if (s == "u16") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::u16};
    } else if (s == "u32") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::u32};
    } else if (s == "u64") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::u64};
    } else if (s == "i8") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::i8};
    } else if (s == "i16") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::i16};
    } else if (s == "i32") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::i32};
    } else if (s == "i64") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::i64};
    } else if (s == "f16") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::f16};
    } else if (s == "f32") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::f32};
    } else if (s == "f64") {
        bp.disable();
        return ast::primitive_type{ast::primitive_type::f64};
    } else {
        return std::nullopt;
    }
}
std::optional<ast::literal_integer> lexer_context::lex_integer() {
    backtrack_point bp;
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
        return std::nullopt;
    }
    bp.disable();
    return {ast::literal_integer{positive ? value : -value}};
}
std::optional<bool> lexer_context::lex_literal_bool() {
    backtrack_point bp;
    std::optional<std::string> os;
    if ((os = lex_keyword("true"))) {
        bp.disable();
        return {true};
    } else if ((os = lex_keyword("false"))) {
        bp.disable();
        return {false};
    }
    return std::nullopt;
}
std::optional<ast::literal_integer> lexer_context::lex_literal_integer() {
    backtrack_point bp;
    auto ol = lex_integer();
    if (ol) {
        bp.disable();
        return {ol.value()};
    }
    return std::nullopt;
}
std::optional<double> lexer_context::lex_literal_float() {
    backtrack_point bp;
    double d;
    in >> d;
    if (!in.fail()) {
        bp.disable();
        return {d};
    }
    in.clear();
    return std::nullopt;
}
std::optional<token_type> lexer_context::lex_any_char() {
    if (false) {
    } else if (lex_string(";")) {
        return {token_type::SEMICOLON};
    } else if (lex_string(",")) {
        return {token_type::COMMA};
    } else if (lex_string("(")) {
        return {token_type::OPEN_R_BRACKET};
    } else if (lex_string(")")) {
        return {token_type::CLOSE_R_BRACKET};
    } else if (lex_string("[")) {
        return {token_type::OPEN_S_BRACKET};
    } else if (lex_string("]")) {
        return {token_type::CLOSE_S_BRACKET};
    } else if (lex_string("{")) {
        return {token_type::OPEN_C_BRACKET};
    } else if (lex_string("}")) {
        return {token_type::CLOSE_C_BRACKET};
    } else if (lex_string(".")) {
        return {token_type::OP_ACCESS};
    } else if (lex_string("=")) {
        return {token_type::OP_ASSIGN};
    } else {
        return std::nullopt;
    }
}
std::optional<token_type> lexer_context::lex_operator() {
    if (false) {
    } else if (lex_string("+")) {
        return {token_type::OP_A_ADD};
    } else if (lex_string("-")) {
        return {token_type::OP_A_SUB};
    } else if (lex_string("*")) {
        return {token_type::OP_A_MUL};
    } else if (lex_string("/")) {
        return {token_type::OP_A_DIV};
    } else if (lex_string("%")) {
        return {token_type::OP_A_MOD};

    } else if (lex_string("|")) {
        return {token_type::OP_B_OR};
    } else if (lex_string("^")) {
        return {token_type::OP_B_XOR};
    } else if (lex_string("~")) {
        //TODO return {token_type::OP_B_NOT};
        return {token_type::OP_A_ADD};
    } else if (lex_string("<<")) {
        return {token_type::OP_B_SHL};
    } else if (lex_string(">>")) {
        return {token_type::OP_B_SHR};

    } else if (lex_string("&&")) {
        return {token_type::OP_L_AND};
    } else if (lex_string("||")) {
        return {token_type::OP_L_OR};

    } else if (lex_string("==")) {
        return {token_type::OP_C_EQ};
    } else if (lex_string("!=")) {
        return {token_type::OP_C_NE};
    } else if (lex_string(">=")) {
        return {token_type::OP_C_GE};
    } else if (lex_string(">")) {
        return {token_type::OP_C_GT};
    } else if (lex_string("<=")) {
        return {token_type::OP_C_LE};
    } else if (lex_string("<")) {
        return {token_type::OP_C_LT};

    } else if (lex_string("&")) {
        return {token_type::OP_B_AND};
    } else if (lex_string("!")) {
        return {token_type::OP_A_ADD};
        //TODO return {token_type::OP_L_NOT};
    } else {
        return std::nullopt;
    }
}
bool lexer_context::lex_whitespace() {
    in >> std::ws;
    return !in.fail();
}
bool lexer_context::lex_comment() {
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
bool lexer_context::lex_space() {
    bool any_space = false;
    do {
        any_space = any_space || lex_whitespace();
    } while (lex_comment());
    return any_space;
}
token_type lexer_context::yylex() {
    std::optional<std::string> s;
    std::optional<ast::primitive_type> type;
    std::optional<bool> literal_bool;
    std::optional<ast::literal_integer> literal_integer;
    std::optional<double> literal_float;
    std::optional<token_type> tok;

    lex_space();
    if (in.eof()) {
        return token_type::T_EOF;
    }
    lex_reserved_keyword();
    if (false) {
    } else if ((tok = lex_any_keyword())) {
        return tok.value();
    } else if ((type = lex_primitive_type())) {
        pc.current_param = type.value();
        return token_type::PRIMITIVE_TYPE;
    } else if ((literal_bool = lex_literal_bool())) {
        pc.current_param = literal_bool.value();
        return token_type::LITERAL_BOOL;
    } else if ((literal_integer = lex_literal_integer())) {
        pc.current_param = literal_integer.value();
        return token_type::LITERAL_INTEGER;
    } else if ((literal_float = lex_literal_float())) {
        pc.current_param = literal_float.value();
        return token_type::LITERAL_FLOAT;
    } else if ((s = lex_identifier())) {
        pc.current_param = pc.symbols_registry.insert(s.value());
        return token_type::IDENTIFIER;
    } else if ((tok = lex_operator())) {
        return tok.value();
    } else if ((tok = lex_any_char())) {
        return tok.value();
    } else {
        std::string s;
        in >> s;
        error("error: unknown input", s);
    }
}

#ifdef LEXER_TEST
int main() {
    parser_context pc;
    lexer_context lexer(pc);
    while (lexer.yylex() != token_type::T_EOF) {
    }
    return 0;
}
#endif
