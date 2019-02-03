#include "driver.hh"
#include "codegen_llvm.hh"

int main() {
    driver driver;
    driver.parse();
    codegen_context_llvm codegen_context_llvm;
    codegen_llvm(codegen_context_llvm, driver.program_ast);
    return 0;
}
