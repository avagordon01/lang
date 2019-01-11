%{

#include <stdio.h>
int yylex (void);

#include <vector>
#include <string>
#include <cassert>

void yyerror(std::string s) {
    fprintf(stderr, "parse error: %s\n", s.c_str());
}

std::vector<int> values;
void assign(size_t i, int x) {
    if (i >= values.size()) {
        values.resize(i + 1);
    }
    values[i] = x;
};

int read(size_t i) {
    assert(i < values.size());
    return values[i];
};

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

%right OP_ASSIGN
%left OP_L_OR
%left OP_L_AND
%left OP_C_EQ OP_C_NE OP_C_GT OP_C_LT OP_C_GE OP_C_LE
%left OP_B_OR
%left OP_B_XOR
%left OP_B_AND
%left OP_B_SHL OP_B_SHR
%left OP_A_ADD OP_A_SUB
%left OP_A_MUL OP_A_DIV OP_A_MOD

%%

statement:
         | statement exp SEMICOLON { printf("= %d\n", $2); }
         ;

exp: IDENTIFIER { $$ = read($1); }
   | IDENTIFIER OP_ASSIGN exp { assign($1, $3); $$ = $3; }

   | exp OP_A_ADD exp { $$ = $1 + $3; }
   | exp OP_A_SUB exp { $$ = $1 - $3; }
   | exp OP_A_MUL exp { $$ = $1 * $3; }
   | exp OP_A_DIV exp { $$ = $1 / $3; }
   | exp OP_A_MOD exp { $$ = $1 % $3; }

   | exp OP_B_AND exp { $$ = $1 & $3; }
   | exp OP_B_OR  exp { $$ = $1 | $3; }
   | exp OP_B_XOR exp { $$ = $1 ^ $3; }
   | exp OP_B_SHL exp { $$ = $1 << $3; }
   | exp OP_B_SHR exp { $$ = $1 >> $3; }

   | exp OP_C_EQ exp { $$ = $1 == $3; }
   | exp OP_C_NE exp { $$ = $1 != $3; }
   | exp OP_C_GT exp { $$ = $1 >  $3; }
   | exp OP_C_GE exp { $$ = $1 >= $3; }
   | exp OP_C_LT exp { $$ = $1 <  $3; }
   | exp OP_C_LE exp { $$ = $1 <= $3; }

   | exp OP_L_AND exp { $$ = $1 && $3; }
   | exp OP_L_OR  exp { $$ = $1 || $3; }

   | OPEN_R_BRACKET exp CLOSE_R_BRACKET { $$ = $2; }
   | NUMBER { $$ = $1; }
   ;

NUMBER: LITERAL_FLOAT
      | LITERAL_INTEGER
      ;

%%

int main() {
    yyparse();
    return 0;
}
