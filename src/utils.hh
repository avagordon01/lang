#pragma once

#include "ast.hh"
#include "y.tab.h"
#include <string>
#include <stdio.h>

extern int yylineno;
extern int yylex(void);
void yyerror(std::string s);
size_t lookup_or_insert(char* c);
int parse_integer(char *s, size_t base);
ast::_expression* new_bin_op(ast::_expression* l, ast::_expression* r, ast::_operator::op);
