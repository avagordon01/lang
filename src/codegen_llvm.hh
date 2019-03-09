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
    using scope = std::unordered_map<ast::identifier, llvm::AllocaInst*>;
    std::vector<scope> scopes;
    std::vector<std::string> symbols_list;
    llvm::BasicBlock* current_function_entry = NULL;
    llvm::BasicBlock* current_loop_exit = NULL;
    llvm::BasicBlock* current_loop_entry = NULL;
    llvm::PHINode* current_loop_phi = NULL;
};

void codegen_llvm(codegen_context_llvm &context, ast::program &program, const std::string& src_filename, const std::string& ir_filename);
