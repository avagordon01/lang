#include "alt-parser.hh"
#include "typecheck.hh"
#include "codegen_llvm.hh"
#include "codegen_spirv.hh"
#include "error.hh"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        error("usage:", argv[0], "input.kl output.ir");
    }

    test_grammar();
    auto program_ast = parse(std::string(argv[1]));

    typecheck_context typecheck_context{};
    typecheck(typecheck_context, program_ast);

    codegen_context_llvm codegen_context_llvm{program_ast.symbols_registry};
    codegen_llvm(codegen_context_llvm, program_ast, std::string(argv[1]), std::string(argv[2]));

    exit(EXIT_SUCCESS);
}
