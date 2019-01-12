%error-verbose

%{

#include <stdio.h>
extern int yylineno;
int yylex (void);

#include <vector>
#include <string>

void yyerror(std::string s) {
    fprintf(stderr, "line %i: %s\n", yylineno, s.c_str());
    exit(1);
}

std::vector<int> values;
void assign(size_t i, int x) {
    if (i >= values.size()) {
        values.resize(i + 1);
    }
    values[i] = x;
};

int read(size_t i) {
    if (i >= values.size()) {
        yyerror("variable used before being defined");
    }
    return values[i];
};

%}

%token LITERAL_FLOAT
%token LITERAL_INTEGER
%token LITERAL_BOOL_T LITERAL_BOOL_F
%token IDENTIFIER
%token OP_A_ADD OP_A_SUB OP_A_MUL OP_A_DIV OP_A_MOD
%token OP_B_AND OP_B_OR OP_B_XOR OP_B_NOT
%token OP_B_SHR OP_B_SHL
%token OP_L_AND OP_L_OR OP_L_NOT
%token OP_C_EQ OP_C_NE
%token OP_C_GT OP_C_GE
%token OP_C_LT OP_C_LE
%token OPEN_R_BRACKET CLOSE_R_BRACKET
%token OPEN_C_BRACKET CLOSE_C_BRACKET
%token OPEN_S_BRACKET CLOSE_S_BRACKET
%token IF ELSE
%token FOR WHILE
%token FUNCTION RETURN
%token TYPE_BOOL
%token TYPE_U8 TYPE_U16 TYPE_U32 TYPE_U64
%token TYPE_I8 TYPE_I16 TYPE_I32 TYPE_I64
%token TYPE_F8 TYPE_F16 TYPE_F32 TYPE_F64
%token SEMICOLON
%token COMMA

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

program: statement_list
       ;

statement_list: /*empty*/
              | statement_list statement
              ;

statement: exp SEMICOLON { printf("%d\n", $1); }
         | IF OPEN_R_BRACKET exp CLOSE_R_BRACKET block
           else_if_list
           optional_else
         | FOR OPEN_R_BRACKET exp SEMICOLON exp SEMICOLON exp CLOSE_R_BRACKET block
         | WHILE OPEN_R_BRACKET exp CLOSE_R_BRACKET block
         | FUNCTION type IDENTIFIER OPEN_R_BRACKET parameter_list CLOSE_R_BRACKET block
         ;

optional_else: /*empty*/
             | ELSE block
             ;

else_if_list: /*empty*/
            | else_if_list ELSE IF OPEN_R_BRACKET exp CLOSE_R_BRACKET block
            ;

parameter_list: /*empty*/
              | type IDENTIFIER
              | parameter_list COMMA type IDENTIFIER
              ;

block: OPEN_C_BRACKET statement_list CLOSE_C_BRACKET;

type: TYPE_BOOL
    | TYPE_U8 | TYPE_U16 | TYPE_U32 | TYPE_U64
    | TYPE_I8 | TYPE_I16 | TYPE_I32 | TYPE_I64
    | TYPE_F8 | TYPE_F16 | TYPE_F32 | TYPE_F64
    ;

literal: LITERAL_FLOAT
       | LITERAL_INTEGER
       | LITERAL_BOOL_T
       | LITERAL_BOOL_F
       ;

exp: IDENTIFIER { $$ = read($1); }
   | type IDENTIFIER OP_ASSIGN exp { assign($2, $4); $$ = $4; }

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
   | literal { $$ = $1; }
   ;

%%

int main() {
    yyparse();
    return 0;
}
