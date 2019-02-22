#include <unordered_map>
#include <memory>

#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>

#include "ast.hh"

struct codegen_context_llvm {
    codegen_context_llvm(): builder(context) {}
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    using scope = std::unordered_map<ast::identifier, llvm::Value*>;
    std::vector<scope> scopes;
    std::vector<std::string> symbols;
};

void codegen_llvm(codegen_context_llvm &context, ast::program &program, const std::string& src_filename, const std::string& obj_filename, const std::string& ir_filename = "");
