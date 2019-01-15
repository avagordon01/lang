#include "utils.hh"

#include <string>
#include <stdio.h>
#include <unordered_map>


void yyerror(std::string s) {
    fprintf(stderr, "line %i: %s\n", yylineno, s.c_str());
    exit(1);
}

static std::unordered_map<std::string, size_t> symbols;
size_t lookup_or_insert(char* c) {
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

int parse_integer(char *s, size_t base) {
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

ast::_expression* new_bin_op(ast::_expression* l, ast::_expression* r, ast::_operator::op) {
    auto x = new ast::_expression;
    x->type = ast::_expression::OPERATOR;
    x->op = new ast::_operator;
    x->op->l = l;
    x->op->r = r;
    return x;
}
