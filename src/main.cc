#include "utils.hh"
#include "codegen_llvm.hh"

ast::_program *program_ast;
int main() {
    yyparse();
    return 0;
}
