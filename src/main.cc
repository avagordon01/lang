#include "utils.hh"
#include "ast.hh"
#include "parser.hh"
#include "codegen_llvm.hh"

ast::program *program_ast;
int main() {
    yy::parser parse{};
    parse();
    codegen_context_llvm codegen_context_llvm;
    codegen_llvmprogram(codegen_context_llvm, program_ast);
    return 0;
}
