%option noyywrap nodefault yylineno

%{
#include "parser.hh"
#include "utils.hh"
%}

%x COMMENT

%%

[ \t\r\n]* ;
; { return SEMICOLON; }
, { return COMMA; }

true {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::BOOL;
    yylval.literal->_bool = 1;
    return LITERAL_BOOL_T; }
false {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::BOOL;
    yylval.literal->_bool = 0;
    return LITERAL_BOOL_F; }
[+-]?[0-9_]+ {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::INTEGER;
    yylval.literal->_integer = parse_integer(yytext, 10);
    return LITERAL_INTEGER; }
[+-]?0[xX][0-9a-fA-F_]+ {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::INTEGER;
    yylval.literal->_integer = parse_integer(yytext, 16);
    return LITERAL_INTEGER; }
[+-]?0[oO][0-7_]+ {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::INTEGER;
    yylval.literal->_integer = parse_integer(yytext, 8);
    return LITERAL_INTEGER; }
[+-]?0[bB][0-1_]+ {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::INTEGER;
    yylval.literal->_integer = parse_integer(yytext, 2);
    return LITERAL_INTEGER; }
[+-]?[0-9]+\.[0-9]+ {
    yylval.literal = new ast::literal;
    yylval.literal->type = ast::literal::FLOAT;
    yylval.literal->_float = atof(yytext);
    return LITERAL_FLOAT; }

"=" { return OP_ASSIGN; }

"+" { return OP_A_ADD; }
"-" { return OP_A_SUB; }
"*" { return OP_A_MUL; }
"/" { return OP_A_DIV; }
"%" { return OP_A_MOD; }

"&" { return OP_B_AND; }
"|" { return OP_B_OR; }
"^" { return OP_B_XOR; }
"~" { return OP_B_NOT; }
"<<" { return OP_B_SHL; }
">>" { return OP_B_SHR; }

"&&" { return OP_L_AND; }
"||" { return OP_L_OR; }
"!" { return OP_L_NOT; }

"==" { return OP_C_EQ; }
"!=" { return OP_C_NE; }
">" { return OP_C_GT; }
">=" { return OP_C_GE; }
"<" { return OP_C_LT; }
"<=" { return OP_C_LE; }

"(" { return OPEN_R_BRACKET; }
")" { return CLOSE_R_BRACKET; }
"[" { return OPEN_S_BRACKET; }
"]" { return CLOSE_S_BRACKET; }
"{" { return OPEN_C_BRACKET; }
"}" { return CLOSE_C_BRACKET; }

if { return IF; }
else { return ELSE; }
for { return FOR; }
while { return WHILE; }
fn { return FUNCTION; }
return { return RETURN; }

bool { yylval.type = ast::type::t_bool; return TYPE; }
u8   { yylval.type = ast::type::u8;     return TYPE; }
u16  { yylval.type = ast::type::u16;    return TYPE; }
u32  { yylval.type = ast::type::u32;    return TYPE; }
u64  { yylval.type = ast::type::u64;    return TYPE; }
i8   { yylval.type = ast::type::i8;     return TYPE; }
i16  { yylval.type = ast::type::i16;    return TYPE; }
i32  { yylval.type = ast::type::i32;    return TYPE; }
i64  { yylval.type = ast::type::i64;    return TYPE; }
f8   { yylval.type = ast::type::f8;     return TYPE; }
f16  { yylval.type = ast::type::f16;    return TYPE; }
f32  { yylval.type = ast::type::f32;    return TYPE; }
f64  { yylval.type = ast::type::f64;    return TYPE; }

[a-zA-Z_][a-zA-Z0-9_]* { yylval.id = lookup_or_insert(yytext); return IDENTIFIER; }

"//".* {}

"/*" { BEGIN(COMMENT); }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>.|\n {}

. { yyerror("unexpected token: " + std::string(yytext)); }
