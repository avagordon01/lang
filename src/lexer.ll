%option noyywrap nodefault

%{
#include "driver.hh"
#include "parser.hh"
%}

%{
#include "ast.hh"
#include <string>
#include <unordered_map>

void reserved_token(yy::location& loc, char* yytext) {
    throw yy::parser::syntax_error(loc, "reserved token " + std::string(yytext) + " cannot currently be used");
}
uint64_t parse_integer(char *s, size_t base) {
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

ast::identifier lookup_or_insert(char* c, driver& driver) {
    auto str = std::string(c);
    auto s = driver.symbols_map.find(str);
    if (s != driver.symbols_map.end()) {
        return s->second;
    } else {
        ast::identifier id = driver.symbols_map.size();
        driver.symbols_map.insert({str, id});
        driver.symbols_list.push_back(str);
        return id;
    }
}
%}

%x COMMENT

%{
#define YY_USER_ACTION loc.columns(yyleng);
%}

%%

%{
yy::location& loc = drv.location;
loc.step();
%}

" "+ loc.step();
\n+  loc.lines(yyleng); loc.step();
";"  return yy::parser::make_SEMICOLON(loc);
","  return yy::parser::make_COMMA(loc);

true                    return yy::parser::make_LITERAL_BOOL(true, loc);
false                   return yy::parser::make_LITERAL_BOOL(false, loc);
[+-]?[0-9_]+            return yy::parser::make_LITERAL_INTEGER(parse_integer(yytext, 10), loc);
[+-]?0[xX][0-9a-fA-F_]+ return yy::parser::make_LITERAL_INTEGER(parse_integer(yytext, 16), loc);
[+-]?0[oO][0-7_]+       return yy::parser::make_LITERAL_INTEGER(parse_integer(yytext, 8), loc);
[+-]?0[bB][0-1_]+       return yy::parser::make_LITERAL_INTEGER(parse_integer(yytext, 2), loc);
[+-]?[0-9]+\.[0-9]+     return yy::parser::make_LITERAL_FLOAT(atof(yytext), loc);

"\." return yy::parser::make_OP_ACCESS(loc);
"="  return yy::parser::make_OP_ASSIGN(loc);

"+"  return yy::parser::make_OP_A_ADD(loc);
"-"  return yy::parser::make_OP_A_SUB(loc);
"*"  return yy::parser::make_OP_A_MUL(loc);
"/"  return yy::parser::make_OP_A_DIV(loc);
"%"  return yy::parser::make_OP_A_MOD(loc);

"&"  return yy::parser::make_OP_B_AND(loc);
"|"  return yy::parser::make_OP_B_OR(loc);
"^"  return yy::parser::make_OP_B_XOR(loc);
"~"  return yy::parser::make_OP_B_NOT(loc);
"<<" return yy::parser::make_OP_B_SHL(loc);
">>" return yy::parser::make_OP_B_SHR(loc);

"&&" return yy::parser::make_OP_L_AND(loc);
"||" return yy::parser::make_OP_L_OR(loc);
"!"  return yy::parser::make_OP_L_NOT(loc);

"==" return yy::parser::make_OP_C_EQ(loc);
"!=" return yy::parser::make_OP_C_NE(loc);
">"  return yy::parser::make_OP_C_GT(loc);
">=" return yy::parser::make_OP_C_GE(loc);
"<"  return yy::parser::make_OP_C_LT(loc);
"<=" return yy::parser::make_OP_C_LE(loc);

"("  return yy::parser::make_OPEN_R_BRACKET(loc);
")"  return yy::parser::make_CLOSE_R_BRACKET(loc);
"["  return yy::parser::make_OPEN_S_BRACKET(loc);
"]"  return yy::parser::make_CLOSE_S_BRACKET(loc);
"{"  return yy::parser::make_OPEN_C_BRACKET(loc);
"}"  return yy::parser::make_CLOSE_C_BRACKET(loc);

var      return yy::parser::make_VAR(loc);
if       return yy::parser::make_IF(loc);
else     return yy::parser::make_ELSE(loc);
for      return yy::parser::make_FOR(loc);
while    return yy::parser::make_WHILE(loc);
fn       return yy::parser::make_FUNCTION(loc);
return   return yy::parser::make_RETURN(loc);
break    return yy::parser::make_BREAK(loc);
continue return yy::parser::make_CONTINUE(loc);
switch   return yy::parser::make_SWITCH(loc);
case     return yy::parser::make_CASE(loc);
import   return yy::parser::make_IMPORT(loc);
export   return yy::parser::make_EXPORT(loc);
struct   return yy::parser::make_STRUCT(loc);
type     return yy::parser::make_TYPE(loc);
(const|auto|sizeof|offsetof|static|repl|cpu|simd|gpu|fpga) reserved_token(loc, yytext);

bool return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::t_bool, loc);
u8   return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::u8, loc);
u16  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::u16, loc);
u32  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::u32, loc);
u64  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::u64, loc);
i8   return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::i8, loc);
i16  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::i16, loc);
i32  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::i32, loc);
i64  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::i64, loc);
f8   reserved_token(loc, yytext);
f16  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::f16, loc);
f32  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::f32, loc);
f64  return yy::parser::make_PRIMITIVE_TYPE(ast::primitive_type::f64, loc);

[a-zA-Z_][a-zA-Z0-9_]* return yy::parser::make_IDENTIFIER(lookup_or_insert(yytext, drv), loc);

"//".*

"/*" BEGIN(COMMENT);
<COMMENT>"*/" BEGIN(INITIAL);
<COMMENT>.
<COMMENT>\n+  loc.lines(yyleng); loc.step();

<<EOF>> return yy::parser::make_T_EOF(loc);

. throw yy::parser::syntax_error(loc, "unexpected token: " + std::string(yytext));
