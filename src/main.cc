#include "ast.hh"
#include "parser.hh"
#include "utils.hh"
#include "codegen_llvm.hh"

ast::program *program_ast;
int main() {
    yyparse();
    codegen_context_llvm codegen_context_llvm;
    codegen_llvmprogram(codegen_context_llvm, program_ast);
    return 0;
}
