#include "parser.hh"
#include "typecheck.hh"
#include "codegen_llvm.hh"
#include "codegen_spirv.hh"
#include "error.hh"
#include "lexer.hh"

int main(int argc, char *argv[]) {
    std::vector<std::string> args;
    args.assign(argv, argv + argc);
    if (args.size() != 3) {
        error("usage:", args[0], "input.kl output.ir");
    }

    lexer_context lexer(args[1]);

    parser_context parser(lexer);
    auto program_ast = parser.parse_program(args[1]);

    typecheck_context typecheck_context{program_ast.symbols_registry};
    typecheck(typecheck_context, program_ast);

    codegen_context_llvm codegen_context_llvm{program_ast.symbols_registry};
    codegen_llvm(codegen_context_llvm, program_ast, args[1], args[2]);

    exit(EXIT_SUCCESS);
}
