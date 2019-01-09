%{
#include "y.tab.h"
%}

NATURAL     ([0-9_]+|0[xX][0-9a-fA-F_]+|0[oO][0-7_]+|0[bB][01_]+)

%x COMMENT

%%

[ \t\r\n]* ;
; { return SEMICOLON; }
-?{NATURAL}\.{NATURAL} { yylval = atof(yytext); return LITERAL_FLOAT; }
-?{NATURAL} { yylval = atoi(yytext); return LITERAL_INTEGER; }

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
