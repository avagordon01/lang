%option noyywrap nodefault yylineno

%{

#include "ast.hh"
#include <vector>
#include "y.tab.h"
#include <optional>
#include <utility>
#include <string>
#include <unordered_map>

void yyerror(std::string s);

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

%}

%x COMMENT

%%

[ \t\r\n]* ;
; { return SEMICOLON; }
, { return COMMA; }

[+-]?[0-9]+\.[0-9]+ { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::FLOAT; yylval.literal->_float = atof(yytext); return LITERAL_FLOAT; }
[+-]?[0-9_]+ { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::INTEGER; yylval.literal->_integer = parse_integer(yytext, 10); return LITERAL_INTEGER; }
[+-]?0[xX][0-9a-fA-F_]+ { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::INTEGER; yylval.literal->_integer = parse_integer(yytext, 16); return LITERAL_INTEGER; }
[+-]?0[oO][0-7_]+ { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::INTEGER; yylval.literal->_integer = parse_integer(yytext, 8); return LITERAL_INTEGER; }
[+-]?0[bB][0-1_]+ { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::INTEGER; yylval.literal->_integer = parse_integer(yytext, 2); return LITERAL_INTEGER; }
true { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::BOOL; yylval.literal->_bool = 1; return LITERAL_BOOL_T; }
false { yylval.literal = new ast::_literal; yylval.literal->type = ast::_literal::BOOL; yylval.literal->_bool = 0; return LITERAL_BOOL_F; }

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

if { return IF; }
else { return ELSE; }
for { return FOR; }
while { return WHILE; }
fn { return FUNCTION; }
return { return RETURN; }

bool { yylval.type = ast::_type::t_bool; return TYPE; }
u8   { yylval.type = ast::_type::u8;     return TYPE; }
u16  { yylval.type = ast::_type::u16;    return TYPE; }
u32  { yylval.type = ast::_type::u32;    return TYPE; }
u64  { yylval.type = ast::_type::u64;    return TYPE; }
i8   { yylval.type = ast::_type::i8;     return TYPE; }
i16  { yylval.type = ast::_type::i16;    return TYPE; }
i32  { yylval.type = ast::_type::i32;    return TYPE; }
i64  { yylval.type = ast::_type::i64;    return TYPE; }
f8   { yylval.type = ast::_type::f8;     return TYPE; }
f16  { yylval.type = ast::_type::f16;    return TYPE; }
f32  { yylval.type = ast::_type::f32;    return TYPE; }
f64  { yylval.type = ast::_type::f64;    return TYPE; }

[a-zA-Z_][a-zA-Z0-9_]* { yylval.id = lookup_or_insert(yytext); return IDENTIFIER; }

"(" { return OPEN_R_BRACKET; }
")" { return CLOSE_R_BRACKET; }
"[" { return OPEN_S_BRACKET; }
"]" { return CLOSE_S_BRACKET; }
"{" { return OPEN_C_BRACKET; }
"}" { return CLOSE_C_BRACKET; }

"//".* {}

"/*" { BEGIN(COMMENT); }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>. {}

. { yyerror("unexpected token: " + std::string(yytext)); }

%%
