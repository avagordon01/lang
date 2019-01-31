%option noyywrap nodefault yylineno

%{
#include "utils.hh"
#include "parser.hh"
%}

%x COMMENT

%%

[ \t\r\n]*
";"  return yy::parser::make_SEMICOLON();
","  return yy::parser::make_COMMA();

true                    return yy::parser::make_LITERAL_BOOL_T(ast::literal{true});
false                   return yy::parser::make_LITERAL_BOOL_F(ast::literal{false});
[+-]?[0-9_]+            return yy::parser::make_LITERAL_INTEGER(ast::literal{parse_integer(yytext, 10)});
[+-]?0[xX][0-9a-fA-F_]+ return yy::parser::make_LITERAL_INTEGER(ast::literal{parse_integer(yytext, 16)});
[+-]?0[oO][0-7_]+       return yy::parser::make_LITERAL_INTEGER(ast::literal{parse_integer(yytext, 8)});
[+-]?0[bB][0-1_]+       return yy::parser::make_LITERAL_INTEGER(ast::literal{parse_integer(yytext, 2)});
[+-]?[0-9]+\.[0-9]+     return yy::parser::make_LITERAL_FLOAT(ast::literal{atof(yytext)});

"="  return yy::parser::make_OP_ASSIGN();

"+"  return yy::parser::make_OP_A_ADD();
"-"  return yy::parser::make_OP_A_SUB();
"*"  return yy::parser::make_OP_A_MUL();
"/"  return yy::parser::make_OP_A_DIV();
"%"  return yy::parser::make_OP_A_MOD();

"&"  return yy::parser::make_OP_B_AND();
"|"  return yy::parser::make_OP_B_OR();
"^"  return yy::parser::make_OP_B_XOR();
"~"  return yy::parser::make_OP_B_NOT();
"<<" return yy::parser::make_OP_B_SHL();
">>" return yy::parser::make_OP_B_SHR();

"&&" return yy::parser::make_OP_L_AND();
"||" return yy::parser::make_OP_L_OR();
"!"  return yy::parser::make_OP_L_NOT();

"==" return yy::parser::make_OP_C_EQ();
"!=" return yy::parser::make_OP_C_NE();
">"  return yy::parser::make_OP_C_GT();
">=" return yy::parser::make_OP_C_GE();
"<"  return yy::parser::make_OP_C_LT();
"<=" return yy::parser::make_OP_C_LE();

"("  return yy::parser::make_OPEN_R_BRACKET();
")"  return yy::parser::make_CLOSE_R_BRACKET();
"["  return yy::parser::make_OPEN_S_BRACKET();
"]"  return yy::parser::make_CLOSE_S_BRACKET();
"{"  return yy::parser::make_OPEN_C_BRACKET();
"}"  return yy::parser::make_CLOSE_C_BRACKET();

if      return yy::parser::make_IF();
else    return yy::parser::make_ELSE();
for     return yy::parser::make_FOR();
while   return yy::parser::make_WHILE();
fn      return yy::parser::make_FUNCTION();
return  return yy::parser::make_RETURN();

bool return yy::parser::make_TYPE(ast::type::t_bool);
u8   return yy::parser::make_TYPE(ast::type::u8);
u16  return yy::parser::make_TYPE(ast::type::u16);
u32  return yy::parser::make_TYPE(ast::type::u32);
u64  return yy::parser::make_TYPE(ast::type::u64);
i8   return yy::parser::make_TYPE(ast::type::i8);
i16  return yy::parser::make_TYPE(ast::type::i16);
i32  return yy::parser::make_TYPE(ast::type::i32);
i64  return yy::parser::make_TYPE(ast::type::i64);
f8   return yy::parser::make_TYPE(ast::type::f8);
f16  return yy::parser::make_TYPE(ast::type::f16);
f32  return yy::parser::make_TYPE(ast::type::f32);
f64  return yy::parser::make_TYPE(ast::type::f64);

[a-zA-Z_][a-zA-Z0-9_]* return yy::parser::make_IDENTIFIER(lookup_or_insert(yytext));

"//".*

"/*" BEGIN(COMMENT);
<COMMENT>"*/" BEGIN(INITIAL);
<COMMENT>.|\n

<<EOF>> return yy::parser::make_T_EOF();

. yyerror("unexpected token: " + std::string(yytext));
