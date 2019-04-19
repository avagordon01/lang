%require "3.2"
%language "c++"

%define api.token.constructor
%define api.value.type variant
%define api.value.automove
%define parse.assert
%define parse.error verbose

%locations

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
#include "error.hh"
void yy::parser::error(const location_type& loc, const std::string& m) {
    ::error(loc, m);
}

#include <memory>
std::unique_ptr<ast::unary_operator>
new_unary_op(ast::expression r, ast::unary_operator::op op, yy::location& loc) {
    ast::unary_operator x {std::move(r), op, ast::type_id::t_void, loc};
    return std::make_unique<ast::unary_operator>(std::move(x));
}
std::unique_ptr<ast::binary_operator>
new_binary_op(ast::expression l, ast::expression r, ast::binary_operator::op op, yy::location& loc) {
    ast::binary_operator x {std::move(l), std::move(r), op, ast::type_id::t_void, loc};
    return std::make_unique<ast::binary_operator>(std::move(x));
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

%token OP_ACCESS
%token OP_ASSIGN
%token OPEN_R_BRACKET CLOSE_R_BRACKET
%token OPEN_C_BRACKET CLOSE_C_BRACKET
%token OPEN_S_BRACKET CLOSE_S_BRACKET
%token IF ELSE
%token FOR WHILE BREAK CONTINUE
%token SWITCH CASE
%token FUNCTION RETURN
%token IMPORT EXPORT
%token VAR
%token STRUCT
%token TYPE

%token SEMICOLON
%token COMMA
%token T_EOF 0

%token <ast::type_id> PRIMITIVE_TYPE
%token <double> LITERAL_FLOAT
%token <bool> LITERAL_BOOL
//this is a bit of a hack
//if IDENTIFIER is a size_t and LITERAL_INTEGER is a uint64_t they clash
%token <ast::identifier> IDENTIFIER LITERAL_INTEGER

%type <ast::program> program
%type <ast::literal> literal
%type <ast::expression> exp
%type <ast::statement> statement
%type <ast::block> block
%type <ast::optional_else> optional_else
%type <ast::else_if_list> else_if_list
%type <ast::statement_list> statement_list
%type <ast::parameter_list> parameter_list
%type <ast::expression_list> expression_list
%type <ast::literal_list> literal_list
%type <ast::assignment> assignment
%type <ast::variable_def> variable_def
%type <ast::if_statement> if_statement
%type <ast::for_loop> for_loop
%type <ast::while_loop> while_loop
%type <ast::cases_list> cases_list
%type <ast::switch_statement> switch_statement
%type <ast::type_def> type_def
%type <ast::constructed_type> type
%type <ast::struct_type> struct_type
%type <ast::array_type> array_type
%type <ast::accessor> accessor
%type <ast::function_def> function_def
%type <ast::function_call> function_call
%type <ast::s_return> return
%type <ast::s_break> break
%type <ast::s_continue> continue
%type <bool> optional_export
%type <std::optional<ast::type_id>> optional_type
%type <std::optional<ast::expression>> optional_exp

%%

program: statement_list { drv.program_ast.statements = std::move($1); };

struct_type: STRUCT OPEN_C_BRACKET parameter_list CLOSE_C_BRACKET {
           $$.fields = $3;
           };
array_type: OPEN_S_BRACKET PRIMITIVE_TYPE LITERAL_INTEGER CLOSE_S_BRACKET {
          $$.element_type = $2;
          $$.length = $3;
          };
type: PRIMITIVE_TYPE { $$ = $1; }
    | IDENTIFIER { $$ = static_cast<ast::type_id>($1); }
    | struct_type { $$ = std::make_unique<ast::struct_type>($1); }
    | array_type { $$ = std::make_unique<ast::array_type>($1); }
    ;
type_def: TYPE IDENTIFIER OP_ASSIGN type {
        $$.type_id = static_cast<ast::type_id>($2);
        $$.type = $4;
        };

statement_list: %empty { }
              | statement_list statement SEMICOLON {
              auto& v = $$;
              v = $1;
              v.push_back($2);
              }
              ;

statement: exp              { $$.statement = $1; }
         | function_def     { $$.statement = $1; }
         | type_def         { $$.statement = $1; }
         | assignment       { $$.statement = $1; }
         | variable_def     { $$.statement = $1; }
         | return           { $$.statement = $1; }
         | break            { $$.statement = $1; }
         | continue         { $$.statement = $1; }
         ;
if_statement: IF exp block else_if_list optional_else {
            $$.loc = @$;
            $$.conditions.push_back($2);
            $$.blocks.push_back($3);
            auto else_if_list = $4;
            for (auto& t: else_if_list.conditions) {
                $$.conditions.emplace_back(std::move(t));
            }
            for (auto& t: else_if_list.blocks) {
                $$.blocks.emplace_back(std::move(t));
            }
            auto optional_else = $5;
            if (optional_else.has_value()) {
                $$.blocks.emplace_back(std::move(optional_else.value()));
            }
            }
cases_list: %empty { }
          | cases_list CASE literal_list block {
          auto& v = $$;
          v = $1;
          v.push_back({$3, $4, ast::type_id::t_void});
          }
          ;
switch_statement: SWITCH exp OPEN_C_BRACKET cases_list CLOSE_C_BRACKET {
                $$.loc = @$;
                $$.expression = $2;
                $$.cases = $4;
                }
for_loop: FOR variable_def SEMICOLON exp SEMICOLON assignment block {
        $$.loc = @$;
        $$.initial = $2;
        $$.condition = $4;
        $$.step = $6;
        $$.block = $7;
        }
while_loop: WHILE exp block {
          $$.loc = @$;
          $$.condition = $2;
          $$.block = $3;
          }
optional_export: %empty { $$ = false; }
               | EXPORT { $$ = true; }
               ;
function_def: optional_export FUNCTION optional_type IDENTIFIER OPEN_R_BRACKET parameter_list CLOSE_R_BRACKET block {
            $$.to_export = $1;
            $$.identifier = $4;
            auto optional_type = $3;
            if (optional_type) {
                $$.returntype = *optional_type;
            } else {
                $$.returntype = ast::type_id::t_void;
            }
            $$.parameter_list = $6;
            $$.block = $8;
            }
variable_def: VAR optional_type IDENTIFIER OP_ASSIGN exp {
            $$.loc = @$;
            $$.explicit_type = $2;
            $$.identifier = $3;
            $$.expression = $5;
            }
assignment: accessor OP_ASSIGN exp {
          $$.loc = @$;
          $$.accessor = $1;
          $$.expression = $3;
          }
optional_exp: %empty { $$ = std::nullopt; }
            | exp { $$ = $1; }
            ;
return: RETURN optional_exp {
      $$.loc = @$;
      $$.expression = $2;
      }
break: BREAK optional_exp {
     $$.loc = @$;
     $$.expression = $2;
     }
continue: CONTINUE {
        $$.loc = @$;
        }

optional_else: %empty {
             $$ = std::nullopt;
             }
             | ELSE block {
             $$ = $2;
             }
             ;

else_if_list: %empty { }
            | else_if_list ELSE IF exp block {
            auto& v = $$;
            v = $1;
            v.conditions.push_back($4);
            v.blocks.push_back($5);
            }
            ;

parameter_list: %empty { }
              | PRIMITIVE_TYPE IDENTIFIER {
              $$.push_back({$1, $2});
              }
              | parameter_list COMMA PRIMITIVE_TYPE IDENTIFIER {
              auto& v = $$;
              v = $1;
              v.push_back({$3, $4});
              }
              ;

expression_list: %empty { }
               | exp {
               $$.push_back($1);
               }
               | expression_list COMMA exp {
               auto& v = $$;
               v = $1;
               v.push_back($3);
               }
               ;

literal_list: %empty { }
               | literal {
               $$.push_back($1);
               }
               | literal_list COMMA literal {
               auto& v = $$;
               v = $1;
               v.push_back($3);
               }
               ;

function_call: IDENTIFIER OPEN_R_BRACKET expression_list CLOSE_R_BRACKET {
             $$.loc = @$;
             $$.identifier = $1;
             $$.arguments = $3;
             };

block: OPEN_C_BRACKET statement_list CLOSE_C_BRACKET { $$.statements = $2; };

optional_type: %empty { $$ = std::nullopt; }
             | PRIMITIVE_TYPE   { $$ = $1; }
             ;
literal: LITERAL_FLOAT optional_type   { $$ = ast::literal{$1, $2, ast::type_id::t_void, @$}; }
       | LITERAL_INTEGER optional_type { $$ = ast::literal{$1, $2, ast::type_id::t_void, @$}; }
       | LITERAL_BOOL optional_type    { $$ = ast::literal{$1, $2, ast::type_id::t_void, @$}; }
       ;

accessor: IDENTIFIER {
        $$.loc = @$;
        $$.identifier = $1;
        }
        | accessor OP_ACCESS IDENTIFIER {
        $$.loc = @$;
        auto& v = $$;
        v = $1;
        v.fields.push_back({$3});
        }
        | accessor OPEN_S_BRACKET exp CLOSE_S_BRACKET {
        $$.loc = @$;
        auto& v = $$;
        v = $1;
        v.fields.push_back({$3});
        }
        ;

exp: block            { $$.expression = std::make_unique<ast::block>($1); }
   | if_statement     { $$.expression = std::make_unique<ast::if_statement>($1); }
   | for_loop         { $$.expression = std::make_unique<ast::for_loop>($1); }
   | while_loop       { $$.expression = std::make_unique<ast::while_loop>($1); }
   | switch_statement { $$.expression = std::make_unique<ast::switch_statement>($1); }

   | accessor { $$.expression = std::make_unique<ast::accessor>($1); }
   | function_call { $$.expression = std::make_unique<ast::function_call>($1); }
   | literal { $$.expression = $1; }
   | exp OP_A_ADD exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_ADD, @$); }
   | exp OP_A_SUB exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_SUB, @$); }
   | exp OP_A_MUL exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_MUL, @$); }
   | exp OP_A_DIV exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_DIV, @$); }
   | exp OP_A_MOD exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::A_MOD, @$); }

   | exp OP_B_AND exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_AND, @$); }
   | exp OP_B_OR  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_OR , @$); }
   | exp OP_B_XOR exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_XOR, @$); }
   | OP_B_NOT exp     { $$.expression = new_unary_op($2, ast::unary_operator::B_NOT, @$); }
   | exp OP_B_SHL exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_SHL, @$); }
   | exp OP_B_SHR exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::B_SHR, @$); }

   | exp OP_C_EQ  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_EQ , @$); }
   | exp OP_C_NE  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_NE , @$); }
   | exp OP_C_GT  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_GT , @$); }
   | exp OP_C_GE  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_GE , @$); }
   | exp OP_C_LT  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_LT , @$); }
   | exp OP_C_LE  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::C_LE , @$); }

   | exp OP_L_AND exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::L_AND, @$); }
   | exp OP_L_OR  exp { $$.expression = new_binary_op($1, $3, ast::binary_operator::L_OR , @$); }
   | OP_L_NOT exp     { $$.expression = new_unary_op($2, ast::unary_operator::L_NOT, @$); }

   | OPEN_R_BRACKET exp CLOSE_R_BRACKET { $$ = $2; }
   ;
