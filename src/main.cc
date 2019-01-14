#include "utils.hh"
#include "y.tab.h"

ast::_program *program_ast;
int main() {
    yyparse();
    return 0;
}
