%{
#include <stdio.h>
int yylex (void);
void yyerror (char const *);
%}

%%

input:
  %empty

%%

int main() {
    return yyparse();
}
