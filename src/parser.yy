%require "3.2"
%language "c++"

%define api.token.constructor
%define api.value.type variant
%define api.value.automove
%define parse.assert

%locations

%error-verbose

%code requires {
#include "ast.hh"
struct driver;
}

%param { driver& drv }

%code {
#include "driver.hh"
}

%code {

#include <iostream>
#include <string>
void yy::parser::error(const location_type& l, const std::string& m) {
    std::cerr << "line " << l.begin.line << " column " << l.begin.column << ": " << m << std::endl;
    exit(1);
}

#include <memory>
std::unique_ptr<ast::unary_operator>
new_unary_op(ast::expression r, ast::unary_operator::op op) {
    auto x = std::make_unique<ast::unary_operator>();
    x->r = std::move(r);
    x->unary_operator = op;
    return x;
}
std::unique_ptr<ast::binary_operator>
new_binary_op(ast::expression l, ast::expression r, ast::binary_operator::op op) {
    auto x = std::make_unique<ast::binary_operator>();
    x->l = std::move(l);
    x->r = std::move(r);
    x->binary_operator = op;
    return x;
}
}

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

program: statement_list { drv.program_ast = std::move($$); };

statement_list: %empty { }
              | statement_list statement { $1.push_back($2); }
              ;

statement: block         { $$.statement = $1; }
         | assignment    { $$.statement = $1; }
         | if_statement  { $$.statement = $1; }
         | for_loop      { $$.statement = $1; }
         | while_loop    { $$.statement = $1; }
         | function      { $$.statement = $1; }
         ;
if_statement: IF OPEN_R_BRACKET exp CLOSE_R_BRACKET block else_if_list optional_else {
            $$.conditions.push_back($3);
            $$.blocks.push_back($5);
            auto else_if_list = $6;
            $$.conditions = std::move(else_if_list.conditions);
            $$.blocks = std::move(else_if_list.blocks);
            auto optional_else = $7;
            if (optional_else.has_value()) {
                $$.blocks.emplace_back(std::move(optional_else.value()));
            }
            }
for_loop: FOR OPEN_R_BRACKET exp SEMICOLON exp SEMICOLON exp CLOSE_R_BRACKET block {
        $$.initial = $3;
        $$.condition = $5;
        $$.step = $7;
        $$.block = $9;
        }
while_loop: WHILE OPEN_R_BRACKET exp CLOSE_R_BRACKET block {
          $$.condition = $3;
          $$.block = $5;
          }
function: FUNCTION TYPE IDENTIFIER OPEN_R_BRACKET parameter_list CLOSE_R_BRACKET block {
        $$.returntype = $2;
        $$.parameter_list = $5;
        }
assignment: TYPE IDENTIFIER OP_ASSIGN exp SEMICOLON {
          $$.identifier = $1;
          $$.expression = $4;
          }

optional_else: %empty {
             $$ = std::nullopt;
             }
             | ELSE block {
             $$ = $2;
             }
             ;

else_if_list: %empty { }
            | else_if_list ELSE IF OPEN_R_BRACKET exp CLOSE_R_BRACKET block {
            auto else_if_list = $1;
            else_if_list.conditions.push_back($5);
            else_if_list.blocks.push_back($7);
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

block: OPEN_C_BRACKET statement_list CLOSE_C_BRACKET { $$.statements = $2; };

literal: LITERAL_FLOAT
       | LITERAL_INTEGER
       | LITERAL_BOOL_T
       | LITERAL_BOOL_F
       ;

exp: IDENTIFIER { $$.expression = $1; }
   | literal { $$.expression = $1; }
   | exp OP_A_ADD exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_ADD); }
   | exp OP_A_SUB exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_SUB); }
   | exp OP_A_MUL exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_MUL); }
   | exp OP_A_DIV exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_DIV); }
   | exp OP_A_MOD exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_MOD); }

   | exp OP_B_AND exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_AND); }
   | exp OP_B_OR  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_OR ); }
   | exp OP_B_XOR exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_XOR); }
   | OP_B_NOT exp     { $$.expression = new_unary_op($2, ast::unary_operator::B_NOT); }
   | exp OP_B_SHL exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_SHL); }
   | exp OP_B_SHR exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_SHR); }

   | exp OP_C_EQ  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_EQ ); }
   | exp OP_C_NE  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_NE ); }
   | exp OP_C_GT  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_GT ); }
   | exp OP_C_GE  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_GE ); }
   | exp OP_C_LT  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_LT ); }
   | exp OP_C_LE  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_LE ); }

   | exp OP_L_AND exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::L_AND); }
   | exp OP_L_OR  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::L_OR ); }
   | OP_L_NOT exp     { $$.expression = new_unary_op($2, ast::unary_operator::L_NOT); }

   | OPEN_R_BRACKET exp CLOSE_R_BRACKET { $$ = $2; }
   ;
