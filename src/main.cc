#include "driver.hh"
#include "codegen_llvm.hh"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " input output" << std::endl;
        exit(EXIT_FAILURE);
    }

    driver driver;
    driver.parse(std::string(argv[1]));
    codegen_context_llvm codegen_context_llvm;
    codegen_llvm(codegen_context_llvm, driver.program_ast, std::string(argv[2]));

    exit(EXIT_SUCCESS);
}
