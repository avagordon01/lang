WHITESPACE  [ \t\r\n]*
NATURAL     ([0-9_]+|0[xX][0-9a-fA-F_]+|0[oO][0-7_]+|0[bB][01_]+)
INTEGER     -?{NATURAL}
FLOAT       -?{NATURAL}\.{NATURAL}
IDENTIFIER  [a-zA-Z_][a-zA-Z0-9_]*
IF          if
ELSE        else
WHILE       while
FOR         for
FUNCTION    function
RETURN      return
IWORDSIZE   (8|16|32|64|128)
FWORDSIZE   (8|16|32|64)
TYPE        (bool|[ui]{IWORDSIZE}|f{FWORDSIZE})
%x COMMENT

%%

{WHITESPACE} ;
{FLOAT} { printf("float: %s\n", yytext); }
{INTEGER} { printf("integer: %s\n", yytext); }
{TYPE} { printf("type: %s\n", yytext); }
{IF} {}
{ELSE} {}
{IDENTIFIER} { printf("identifier: %s\n", yytext); }

"//".* { printf("comment\n"); }

"/*" { BEGIN(COMMENT); printf("comment\n"); }
<COMMENT>{
    "*/" { BEGIN(INITIAL); }
    . {}
}

"(" { printf("open round bracket\n"); }
")" { printf("close round bracket\n"); }
"[" { printf("open square bracket\n"); }
"]" { printf("close square bracket\n"); }
"{" { printf("open curly bracket\n"); }
"}" { printf("close curly bracket\n"); }

%%
