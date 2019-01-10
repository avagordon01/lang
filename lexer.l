%{
#include "y.tab.h"

int parse_natural(char *s, size_t base) {
    int value = 0;
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
    if (base == 10 || base == 2 || base == 8) {
        while (*s != '\0') {
            value *= base;
            value += *s - '0';
            s++;
        }
    } else if (base == 16) {
        while (*s != '\0') {
            value *= base;
            if (value >= 'a' && value <= 'f' || value >= 'A' && value <= 'F') {
                value += *s - 'a';
            } else {
                value += *s - '0';
            }
            s++;
        }
    }
    return sign * value;
}
%}

NATURAL     ([0-9_]+|0[xX][0-9a-fA-F_]+|0[oO][0-7_]+|0[bB][01_]+)

%x COMMENT

%%

[ \t\r\n]* ;
; { return SEMICOLON; }
[+-]?{NATURAL}\.{NATURAL} { yylval = atof(yytext); return LITERAL_FLOAT; }
[+-]?[0-9_]+ { yylval = parse_natural(yytext, 10); return LITERAL_INTEGER; }
[+-]?0[xX][0-9a-fA-F_]+ { yylval = parse_natural(yytext, 16); return LITERAL_INTEGER; }
[+-]?0[oO][0-7_]+ { yylval = parse_natural(yytext, 8); return LITERAL_INTEGER; }
[+-]?0[bB][0-1_]+ { yylval = parse_natural(yytext, 2); return LITERAL_INTEGER; }

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

if { return K_IF; }
else { return K_ELSE; }
for { return K_FOR; }
while { return K_WHILE; }
function { return K_FUNCTION; }
return { return K_RETURN; }

bool { return TYPE_BOOL; }
u8 { return TYPE_U8; }
u16 { return TYPE_U16; }
u32 { return TYPE_U32; }
u64 { return TYPE_U64; }
i8 { return TYPE_I8; }
i16 { return TYPE_I16; }
i32 { return TYPE_I32; }
i64 { return TYPE_I64; }
f8 { return TYPE_F8; }
f16 { return TYPE_F16; }
f32 { return TYPE_F32; }
f64 { return TYPE_F64; }

[a-zA-Z_][a-zA-Z0-9_]* { /*yylval = yytext; */return IDENTIFIER/*symbol_table[yytext]*/; }

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

%%
