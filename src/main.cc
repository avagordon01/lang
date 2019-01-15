#include "utils.hh"
#include "codegen_llvm.hh"

ast::_program *program_ast;
int main() {
    yyparse();
    codegen_context_llvm codegen_context_llvm;
    codegen_llvm_program(codegen_context_llvm, program_ast);
    return 0;
}
