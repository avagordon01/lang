#pragma once

#include "ast.hh"
#include <string>
#include <stdio.h>

extern ast::_program *program_ast;

void yyerror(std::string s);
size_t lookup_or_insert(char* c);
int parse_integer(char *s, size_t base);
ast::_expression* new_bin_op(ast::_expression* l, ast::_expression* r, ast::_operator::op);
