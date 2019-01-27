#include "ast.hh"
#include <string>

extern ast::program *program_ast;

void yyerror(std::string s);
ast::identifier lookup_or_insert(char* c);
uint64_t parse_integer(char *s, size_t base);
ast::expression* new_bin_op(ast::expression* l, ast::expression* r, ast::binary_operator::op op);
ast::expression* new_unary_op(ast::expression* r, ast::unary_operator::op op);
