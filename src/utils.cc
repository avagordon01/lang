#include "utils.hh"
#include "ast.hh"

#include <string>
#include <stdio.h>
#include <unordered_map>

extern int yylineno;
void yyerror(std::string s) {
    fprintf(stderr, "line %i: %s\n", yylineno, s.c_str());
    exit(1);
}

static std::unordered_map<std::string, ast::identifier> symbols;
ast::identifier lookup_or_insert(char* c) {
    auto str = std::string(c);
    auto s = symbols.find(str);
    if (s != symbols.end()) {
        return s->second;
    } else {
        size_t n = symbols.size();
        symbols.insert({str, n});
        return n;
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

ast::expression* new_unary_op(ast::expression* r, ast::unary_operator::op op) {
    auto x = new ast::expression;
    x->type = ast::expression::UNARY_OPERATOR;
    x->unary_operator = new ast::unary_operator;
    x->unary_operator->r = r;
    x->unary_operator->unary_operator = op;
    return x;
}
ast::expression* new_bin_op(ast::expression* l, ast::expression* r, ast::binary_operator::op op) {
    auto x = new ast::expression;
    x->type = ast::expression::BINARY_OPERATOR;
    x->binary_operator = new ast::binary_operator;
    x->binary_operator->l = l;
    x->binary_operator->r = r;
    x->binary_operator->binary_operator = op;
    return x;
}
