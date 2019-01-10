%{
#include <stdio.h>
int yylex (void);
void print_symbols();

int yyerror(char *s) {
    fprintf(stderr, "parse error: %s\n", s);
}
%}

%token LITERAL_FLOAT
%token LITERAL_INTEGER
%token IDENTIFIER
%token OP_A_ADD OP_A_SUB OP_A_MUL OP_A_DIV OP_A_MOD
%token OP_B_AND OP_B_OR OP_B_XOR OP_B_NOT
%token OP_B_SHR OP_B_SHL
%token OP_L_AND OP_L_OR OP_L_NOT
%token OP_C_EQ OP_C_NE
%token OP_C_GT OP_C_GE
%token OP_C_LT OP_C_LE
%token OPEN_R_BRACKET CLOSE_R_BRACKET
%token OPEN_S_BRACKET CLOSE_S_BRACKET
%token OPEN_C_BRACKET CLOSE_C_BRACKET
%token K_IF K_ELSE
%token K_FOR K_WHILE
%token K_FUNCTION K_RETURN
%token TYPE_BOOL
%token TYPE_U8 TYPE_U16 TYPE_U32 TYPE_U64
%token TYPE_I8 TYPE_I16 TYPE_I32 TYPE_I64
%token TYPE_F8 TYPE_F16 TYPE_F32 TYPE_F64
%token SEMICOLON

%%

statement:
         | statement expr_10 SEMICOLON { printf("= %d\n", $2); }
         ;

expr_10: expr_20
       | expr_10 OP_A_ADD expr_10 { $$ = $1 + $3; }
       | expr_10 OP_A_SUB expr_20 { $$ = $1 - $3; }
       ;

expr_20: expr_30
       | expr_20 OP_A_MUL expr_30 { $$ = $1 * $3; }
       | expr_20 OP_A_DIV expr_30 { $$ = $1 / $3; }
       | expr_20 OP_A_MOD expr_30 { $$ = $1 % $3; }
       ;

expr_30: NUMBER
       | OPEN_R_BRACKET expr_10 CLOSE_R_BRACKET { $$ = $2; }
       ;

NUMBER: IDENTIFIER
      | LITERAL_FLOAT
      | LITERAL_INTEGER
      ;

%%

int main() {
    yyparse();
    print_symbols();
    return 0;
}
