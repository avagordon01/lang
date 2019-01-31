%require "3.2"
%language "c++"

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%error-verbose

%code requires {
#include "ast.hh"
}

%{
#include "utils.hh"
#include "lexer.hh"
%}

%left OP_L_OR
%left OP_L_AND
%left OP_C_EQ OP_C_NE OP_C_GT OP_C_LT OP_C_GE OP_C_LE
%left OP_B_OR
%left OP_B_XOR
%left OP_B_AND
%left OP_B_SHL OP_B_SHR
%left OP_A_ADD OP_A_SUB
%left OP_A_MUL OP_A_DIV OP_A_MOD
%precedence OP_B_NOT OP_L_NOT

%token OP_ASSIGN
%token OPEN_R_BRACKET CLOSE_R_BRACKET
%token OPEN_C_BRACKET CLOSE_C_BRACKET
%token OPEN_S_BRACKET CLOSE_S_BRACKET
%token IF ELSE
%token FOR WHILE
%token FUNCTION RETURN

%token SEMICOLON
%token COMMA
%token T_EOF 0

%token <ast::type> TYPE
%token <ast::literal> LITERAL_FLOAT LITERAL_INTEGER LITERAL_BOOL_T LITERAL_BOOL_F
%token <ast::identifier> IDENTIFIER

%type <ast::program> program
%type <ast::literal> literal
%type <ast::expression> exp
%type <ast::statement> statement
%type <ast::block> block
%type <ast::optional_else> optional_else
%type <ast::else_if_list> else_if_list
%type <ast::statement_list> statement_list
%type <ast::parameter_list> parameter_list
%type <ast::assignment> assignment
%type <ast::if_statement> if_statement
%type <ast::for_loop> for_loop
%type <ast::while_loop> while_loop
%type <ast::function> function

%%

program: statement_list { *program_ast = $$; };

statement_list: %empty { }
              | statement_list statement {
              $1.push_back($2);
              }
              ;

statement: block
         { $$.type = ast::statement::S_BLOCK; *$$.block = $1; }
         | assignment
         { $$.type = ast::statement::S_ASSIGNMENT; *$$.assignment = $1; }
         | if_statement
         { $$.type = ast::statement::S_IF; *$$.if_statement = $1; }
         | for_loop
         { $$.type = ast::statement::S_FOR; *$$.for_loop = $1; }
         | while_loop
         { $$.type = ast::statement::S_WHILE; *$$.while_loop = $1; }
         | function
         { $$.type = ast::statement::S_FUNCTION; *$$.function = $1; }
         ;
if_statement: IF OPEN_R_BRACKET exp CLOSE_R_BRACKET block else_if_list optional_else {
            $$.conditions->push_back($3);
            $$.blocks->push_back($5);
            $$.conditions->insert($$.conditions->end(), $6.conditions->begin(), $6.conditions->end());
            $$.blocks->insert($$.blocks->end(), $6.blocks->begin(), $6.blocks->end());
            if ($7.has_value()) {
                $$.blocks->push_back($7.value());
            }
            }
for_loop: FOR OPEN_R_BRACKET exp SEMICOLON exp SEMICOLON exp CLOSE_R_BRACKET block {
        $$.initial = &$3;
        $$.condition = &$5;
        $$.step = &$7;
        $$.block = &$9;
        }
while_loop: WHILE OPEN_R_BRACKET exp CLOSE_R_BRACKET block {
          $$.condition = &$3;
          $$.block = &$5;
          }
function: FUNCTION TYPE IDENTIFIER OPEN_R_BRACKET parameter_list CLOSE_R_BRACKET block {
        $$.returntype = $2;
        $$.parameter_list = $5;
        }
assignment: TYPE IDENTIFIER OP_ASSIGN exp SEMICOLON {
          $$.identifier = $1;
          $$.expression = &$4;
          }

optional_else: %empty {
             *$$ = std::nullopt;
             }
             | ELSE block {
             *$$ = $2;
             }
             ;

else_if_list: %empty { }
            | else_if_list ELSE IF OPEN_R_BRACKET exp CLOSE_R_BRACKET block {
            $1.conditions->push_back($5);
            $1.blocks->push_back($7);
            }
            ;

parameter_list: %empty { }
              | TYPE IDENTIFIER {
              $$.push_back(std::make_pair($1, $2));
              }
              | parameter_list COMMA TYPE IDENTIFIER {
              $1.push_back(std::make_pair($3, $4));
              }
              ;

block: OPEN_C_BRACKET statement_list CLOSE_C_BRACKET {
     $$.statements = $2;
     }
     ;

literal: LITERAL_FLOAT
       | LITERAL_INTEGER
       | LITERAL_BOOL_T
       | LITERAL_BOOL_F
       ;

exp: IDENTIFIER {
   $$.type = ast::expression::VARIABLE;
   $$.variable = $1;
   }
   | literal {
   $$.type = ast::expression::LITERAL;
   *$$.literal = $1; }
   | exp OP_A_ADD exp { $$ = new_bin_op($1, $3, ast::binary_operator::A_ADD); }
   | exp OP_A_SUB exp { $$ = new_bin_op($1, $3, ast::binary_operator::A_SUB); }
   | exp OP_A_MUL exp { $$ = new_bin_op($1, $3, ast::binary_operator::A_MUL); }
   | exp OP_A_DIV exp { $$ = new_bin_op($1, $3, ast::binary_operator::A_DIV); }
   | exp OP_A_MOD exp { $$ = new_bin_op($1, $3, ast::binary_operator::A_MOD); }

   | exp OP_B_AND exp { $$ = new_bin_op($1, $3, ast::binary_operator::B_AND); }
   | exp OP_B_OR  exp { $$ = new_bin_op($1, $3, ast::binary_operator::B_OR ); }
   | exp OP_B_XOR exp { $$ = new_bin_op($1, $3, ast::binary_operator::B_XOR); }
   | OP_B_NOT exp { $$ = new_unary_op($2, ast::unary_operator::B_NOT); }
   | exp OP_B_SHL exp { $$ = new_bin_op($1, $3, ast::binary_operator::B_SHL); }
   | exp OP_B_SHR exp { $$ = new_bin_op($1, $3, ast::binary_operator::B_SHR); }

   | exp OP_C_EQ  exp { $$ = new_bin_op($1, $3, ast::binary_operator::C_EQ ); }
   | exp OP_C_NE  exp { $$ = new_bin_op($1, $3, ast::binary_operator::C_NE ); }
   | exp OP_C_GT  exp { $$ = new_bin_op($1, $3, ast::binary_operator::C_GT ); }
   | exp OP_C_GE  exp { $$ = new_bin_op($1, $3, ast::binary_operator::C_GE ); }
   | exp OP_C_LT  exp { $$ = new_bin_op($1, $3, ast::binary_operator::C_LT ); }
   | exp OP_C_LE  exp { $$ = new_bin_op($1, $3, ast::binary_operator::C_LE ); }

   | exp OP_L_AND exp { $$ = new_bin_op($1, $3, ast::binary_operator::L_AND); }
   | exp OP_L_OR  exp { $$ = new_bin_op($1, $3, ast::binary_operator::L_OR ); }
   | OP_L_NOT exp     { $$ = new_unary_op($2, ast::unary_operator::L_NOT); }

   | OPEN_R_BRACKET exp CLOSE_R_BRACKET { $$ = $2; }
   ;
