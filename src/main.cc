#include "alt-parser.hh"
#include "typecheck.hh"
#include "codegen_llvm.hh"
#include "codegen_spirv.hh"
#include "error.hh"

#include <vector>
#include <string>

int main(int argc, char *argv[]) {
    std::vector<std::string> args;
    args.assign(argv, argv + argc);
    if (args.size() != 3) {
        error("usage:", args[0], "input.kl output.ir");
    }

    auto program_ast = parse(args[1]);

    typecheck_context typecheck_context{};
    typecheck(typecheck_context, program_ast);

    codegen_context_llvm codegen_context_llvm{program_ast.symbols_registry};
    codegen_llvm(codegen_context_llvm, program_ast, args[1], args[2]);

    return 0;
}
