%option noyywrap nodefault

%{
#include "driver.hh"
#include "error.hh"
%}

%{
#include "ast.hh"
#include <string>
#include <unordered_map>

void reserved_token(yy::location& loc, char* yytext) {
    error(loc, "reserved token", yytext, "cannot currently be used");
}
uint64_t parse_integer(char *s, yy::location& loc) {
    //parse sign
    bool positive = true;
    if (*s == '+') {
        s++;
    } else if (*s == '-') {
        positive = false;
        s++;
    }
    //parse base
    uint8_t base = 10;
    if (*s == '0') {
        s++;
        if (*s == 'b' || *s == 'B') {
            base =  2; s++;
        } else if (*s == 'o' || *s == 'O') {
            base =  8; s++;
        } else if (*s == 'x' || *s == 'X') {
            base = 16; s++;
        }
    }
    //parse digits
    uint64_t value = 0;
    for (; *s != 0; s++) {
        value *= base;
        if (base <= 10 && *s >= '0' && *s <= '0' + base - 1) {
            value += *s - '0';
        } else if (base <= 32 && *s >= '0' && *s <= '9') {
            value += *s - '0';
        } else if (base <= 32 && *s >= 'a' && *s <= 'a' + base - 11) {
            value += *s - 'a';
        } else if (base <= 32 && *s >= 'A' && *s <= 'A' + base - 11) {
            value += *s - 'A';
        } else if (*s == '_' || *s == ',') {
        } else {
            error(loc, "unrecognised digit in base", base, "number literal", *s);
        }
    }
    return positive ? value : -value;
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

integer     [+-]?([0-9_]+|0[bBoOxX][0-9a-zA-Z_,]+)
float       [+-]?[0-9]+\.[0-9]+
identifier  [a-zA-Z_][a-zA-Z0-9_]*

%%

%{
yy::location& loc = drv.location;
loc.step();
%}

" "+ loc.step();
\n+  loc.lines(yyleng); loc.step();
";"  return token_type::SEMICOLON;
","  return token_type::COMMA;

"\." return token_type::OP_ACCESS;
"="  return token_type::OP_ASSIGN;

"+"  return token_type::OP_A_ADD;
"-"  return token_type::OP_A_SUB;
"*"  return token_type::OP_A_MUL;
"/"  return token_type::OP_A_DIV;
"%"  return token_type::OP_A_MOD;

"&"  return token_type::OP_B_AND;
"|"  return token_type::OP_B_OR;
"^"  return token_type::OP_B_XOR;
"~"  return token_type::OP_B_NOT;
"<<" return token_type::OP_B_SHL;
">>" return token_type::OP_B_SHR;

"&&" return token_type::OP_L_AND;
"||" return token_type::OP_L_OR;
"!"  return token_type::OP_L_NOT;

"==" return token_type::OP_C_EQ;
"!=" return token_type::OP_C_NE;
">"  return token_type::OP_C_GT;
">=" return token_type::OP_C_GE;
"<"  return token_type::OP_C_LT;
"<=" return token_type::OP_C_LE;

"("  return token_type::OPEN_R_BRACKET;
")"  return token_type::CLOSE_R_BRACKET;
"["  return token_type::OPEN_S_BRACKET;
"]"  return token_type::CLOSE_S_BRACKET;
"{"  return token_type::OPEN_C_BRACKET;
"}"  return token_type::CLOSE_C_BRACKET;

var      return token_type::VAR;
if       return token_type::IF;
elif     return token_type::ELIF;
else     return token_type::ELSE;
for      return token_type::FOR;
while    return token_type::WHILE;
fn       return token_type::FUNCTION;
return   return token_type::RETURN;
break    return token_type::BREAK;
continue return token_type::CONTINUE;
switch   return token_type::SWITCH;
case     return token_type::CASE;
import   return token_type::IMPORT;
export   return token_type::EXPORT;
struct   return token_type::STRUCT;
type     return token_type::TYPE;
(const|auto|sizeof|offsetof|static|repl|cpu|simd|gpu|fpga) reserved_token(loc, yytext);

bool drv.current_param = ast::type_id::t_bool; return token_type::PRIMITIVE_TYPE;
u8   drv.current_param = ast::type_id::u8; return token_type::PRIMITIVE_TYPE;
u16  drv.current_param = ast::type_id::u16; return token_type::PRIMITIVE_TYPE;
u32  drv.current_param = ast::type_id::u32; return token_type::PRIMITIVE_TYPE;
u64  drv.current_param = ast::type_id::u64; return token_type::PRIMITIVE_TYPE;
i8   drv.current_param = ast::type_id::i8; return token_type::PRIMITIVE_TYPE;
i16  drv.current_param = ast::type_id::i16; return token_type::PRIMITIVE_TYPE;
i32  drv.current_param = ast::type_id::i32; return token_type::PRIMITIVE_TYPE;
i64  drv.current_param = ast::type_id::i64; return token_type::PRIMITIVE_TYPE;
f8   reserved_token(loc, yytext);
f16  drv.current_param = ast::type_id::f16; return token_type::PRIMITIVE_TYPE;
f32  drv.current_param = ast::type_id::f32; return token_type::PRIMITIVE_TYPE;
f64  drv.current_param = ast::type_id::f64; return token_type::PRIMITIVE_TYPE;

true        drv.current_param = true; return token_type::LITERAL_BOOL;
false       drv.current_param = false; return token_type::LITERAL_BOOL;
{integer}   drv.current_param = parse_integer(yytext, loc); return token_type::LITERAL_INTEGER;
{float}     drv.current_param = static_cast<double>(strtod(yytext, nullptr)); return token_type::LITERAL_FLOAT;

{identifier} drv.current_param = lookup_or_insert(yytext, drv); return token_type::IDENTIFIER;

"//".*

"/*" BEGIN(COMMENT);
<COMMENT>"*/" BEGIN(INITIAL);
<COMMENT>.
<COMMENT>\n+  loc.lines(yyleng); loc.step();

<<EOF>> return token_type::T_EOF;

. error(loc, "unexpected token", yytext);
