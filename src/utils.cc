#include "parser.hh"
#include "utils.hh"
#include "ast.hh"

#include <string>
#include <stdio.h>
#include <unordered_map>
#include <memory>

void error(std::string s) {
    std::cerr << s << std::endl;
    exit(1);
}
void yy::parser::error(const location_type& l, const std::string& m) {
    std::cerr << "line " << l.begin.line << " column " << l.begin.column << ": " << m << std::endl;
    exit(1);
}

static std::unordered_map<std::string, ast::identifier> symbols;
ast::identifier lookup_or_insert(char* c) {
    auto str = std::string(c);
    auto s = symbols.find(str);
    if (s != symbols.end()) {
        return s->second;
    } else {
        ast::identifier id = symbols.size();
        symbols.insert({str, id});
        return id;
    }
}

uint64_t parse_integer(char *s, size_t base) {
    size_t value = 0;
    int sign = 1;
    if (*s == '+') {
        s++;
    } else if (*s == '-') {
        sign = -1;
        s++;
    }
    if (base != 10) {
        s += 2;
    }
    while (*s != '\0') {
        value *= base;
        if (*s >= '0' && *s <= '9') {
            value += *s - '0';
        } else if (*s >= 'a' && *s <= 'z') {
            value += *s - 'a';
        } else if (*s >= 'A' && *s <= 'Z') {
            value += *s - 'A';
        }
        s++;
    }
    return sign * value;
}

std::unique_ptr<ast::unary_operator>
new_unary_op(ast::expression r, ast::unary_operator::op op) {
    auto x = std::make_unique<ast::unary_operator>();
    x->r = std::move(r);
    x->unary_operator = op;
    return x;
}
std::unique_ptr<ast::binary_operator>
new_binary_op(ast::expression l, ast::expression r, ast::binary_operator::op op) {
    auto x = std::make_unique<ast::binary_operator>();
    x->l = std::move(l);
    x->r = std::move(r);
    x->binary_operator = op;
    return x;
}
