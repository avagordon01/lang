#include "driver.hh"
#include "typecheck.hh"
#include "codegen_llvm.hh"
#include "codegen_spirv.hh"
#include "error.hh"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        error("usage:", argv[0], "input.kl output.ir");
    }

    driver driver;
    driver.filename = std::string(argv[1]);
    driver.parse();

    typecheck_context typecheck_context;
    typecheck(typecheck_context, driver.program_ast);

    codegen_context_llvm codegen_context_llvm;
    codegen_context_llvm.symbols_list = driver.symbols_list;
    codegen_llvm(codegen_context_llvm, driver.program_ast, std::string(argv[1]), std::string(argv[2]));

    exit(EXIT_SUCCESS);
}
