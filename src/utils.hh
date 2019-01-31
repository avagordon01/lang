#include "parser.hh"
#define YY_DECL yy::parser::symbol_type yylex ()
YY_DECL;

#include "ast.hh"
#include <string>
#include <memory>

extern ast::program program_ast;

void yyerror(std::string s);
ast::identifier lookup_or_insert(char* c);
uint64_t parse_integer(char *s, size_t base);
std::unique_ptr<ast::binary_operator> new_binary_op(ast::expression l, ast::expression r, ast::binary_operator::op op);
std::unique_ptr<ast::unary_operator> new_unary_op(ast::expression r, ast::unary_operator::op op);
