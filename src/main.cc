#include "parser.hh"
#include "driver.hh"
#include "codegen_llvm.hh"

int main() {
    driver driver;
    yy::parser parser{driver};
    parser.parse();
    codegen_context_llvm codegen_context_llvm;
    codegen_llvm(codegen_context_llvm, driver.program_ast);
    return 0;
}
