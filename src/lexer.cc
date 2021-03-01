#include <iostream>
#include <optional>

#include "lexer.hh"
#include "error.hh"
#include "ast.hh"
#include "tokens.hh"

tl::expected<std::monostate, std::string> lexer_context::lex_string(std::string s, bool word_boundary) {
    backtrack_point bp(in);
    if (!std::equal(
        s.begin(), s.end(),
        std::istreambuf_iterator<char>(in)
    )) {
        return tl::unexpected(string_error("expected", s));
    }
    char c = in.peek();
    if (word_boundary && (std::isalnum(c) || c == '_')) {
        return tl::unexpected(string_error("expected", s));
    }
    bp.disable();
    return {};
}
tl::expected<std::string, std::string> lexer_context::lex_word() {
    if (!std::isalpha(in.peek())) {
        return tl::unexpected(string_error("expected word got", in.peek()));
    }
    std::string s;
    for (char c = in.peek(); std::isalnum(c) || c == '_'; c = in.peek()) {
        in.get();
        s += c;
    }
    return s;
}
tl::expected<std::monostate, std::string> lexer_context::lex_keyword(std::string keyword) {
    return lex_string(keyword, true);
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
    if (false) {
    } else if (lex_keyword("const")) {
        error("error, use of reserved keyword const");
    } else if (lex_keyword("auto")) {
        error("error, use of reserved keyword auto");
    } else if (lex_keyword("sizeof")) {
        error("error, use of reserved keyword sizeof");
    } else if (lex_keyword("offsetof")) {
        error("error, use of reserved keyword offsetof");
    } else if (lex_keyword("typeof")) {
        error("error, use of reserved keyword typeof");
    } else if (lex_keyword("static")) {
        error("error, use of reserved keyword static");
    } else if (lex_keyword("repl")) {
        error("error, use of reserved keyword repl");
    } else if (lex_keyword("cpu")) {
        error("error, use of reserved keyword cpu");
    } else if (lex_keyword("simd")) {
        error("error, use of reserved keyword simd");
    } else if (lex_keyword("gpu")) {
        error("error, use of reserved keyword gpu");
    } else if (lex_keyword("fpga")) {
        error("error, use of reserved keyword fpga");
    } else if (lex_keyword("f8")) {
        error("error, use of reserved keyword f8");
    }
}
tl::expected<std::string, std::string> lexer_context::lex_identifier() {
    return lex_word();
}
std::optional<ast::primitive_type> lexer_context::lex_primitive_type() {
    if (false) {
    } else if (lex_keyword("void")) {
        return ast::primitive_type{ast::primitive_type::t_void};
    } else if (lex_keyword("bool")) {
        return ast::primitive_type{ast::primitive_type::t_bool};
    } else if (lex_keyword("u8")) {
        return ast::primitive_type{ast::primitive_type::u8};
    } else if (lex_keyword("u16")) {
        return ast::primitive_type{ast::primitive_type::u16};
    } else if (lex_keyword("u32")) {
        return ast::primitive_type{ast::primitive_type::u32};
    } else if (lex_keyword("u64")) {
        return ast::primitive_type{ast::primitive_type::u64};
    } else if (lex_keyword("i8")) {
        return ast::primitive_type{ast::primitive_type::i8};
    } else if (lex_keyword("i16")) {
        return ast::primitive_type{ast::primitive_type::i16};
    } else if (lex_keyword("i32")) {
        return ast::primitive_type{ast::primitive_type::i32};
    } else if (lex_keyword("i64")) {
        return ast::primitive_type{ast::primitive_type::i64};
    } else if (lex_keyword("f16")) {
        return ast::primitive_type{ast::primitive_type::f16};
    } else if (lex_keyword("f32")) {
        return ast::primitive_type{ast::primitive_type::f32};
    } else if (lex_keyword("f64")) {
        return ast::primitive_type{ast::primitive_type::f64};
    } else {
        return std::nullopt;
    }
}
std::optional<ast::literal_integer> lexer_context::lex_literal_integer() {
    backtrack_point bp(in);
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
    if (lex_keyword("true")) {
        return {true};
    } else if (lex_keyword("false")) {
        return {false};
    }
    return std::nullopt;
}
std::optional<double> lexer_context::lex_literal_float() {
    backtrack_point bp(in);
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
        return {token_type::OP_B_NOT};
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
        return {token_type::OP_L_NOT};
    } else {
        return std::nullopt;
    }
}
bool lexer_context::lex_whitespace() {
    std::ios::pos_type p0 = in.tellg();
    in >> std::ws;
    std::ios::pos_type p1 = in.tellg();
    return p1 > p0;
}
tl::expected<void, std::string> lexer_context::lex_comment() {
    if (lex_string("//")) {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return {};
    } else if (lex_string("/*")) {
        std::string end = "*/";
        auto end_it = std::istreambuf_iterator<char>();
        auto it = std::search(
            std::istreambuf_iterator<char>(in), end_it,
            end.begin(), end.end()
        );
        if (it == end_it) {
            return {tl::unexpected(std::string("didn't get a matching close comment */"))};
        } else {
            it++;
            return {};
        }
    }
    if (in.fail()) {
        in.clear();
        in.peek();
    }
    return {tl::unexpected(std::string("didn't parse comment"))};
}
void lexer_context::lex_space() {
    while (lex_whitespace() || lex_comment()) {}
}
token_type lexer_context::yylex() {
    tl::expected<std::string, std::string> s;
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
        current_param = type.value();
        return token_type::PRIMITIVE_TYPE;
    } else if ((literal_bool = lex_literal_bool())) {
        current_param = literal_bool.value();
        return token_type::LITERAL_BOOL;
    } else if ((literal_integer = lex_literal_integer())) {
        current_param = literal_integer.value();
        return token_type::LITERAL_INTEGER;
    } else if ((literal_float = lex_literal_float())) {
        current_param = literal_float.value();
        return token_type::LITERAL_FLOAT;
    } else if ((s = lex_identifier())) {
        current_param = symbols_registry.insert(s.value());
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
