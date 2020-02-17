#include <memory>

#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>

#include "scopes.hh"
#include "ast.hh"

struct codegen_context_llvm {
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder{context};
    std::unique_ptr<llvm::Module> module;
    ::scopes<ast::identifier, llvm::AllocaInst*> variable_scopes;
    bi_registry<ast::identifier, std::string>& symbols_registry;
    llvm::BasicBlock* current_function_entry = NULL;
    llvm::BasicBlock* current_loop_exit = NULL;
    llvm::BasicBlock* current_loop_entry = NULL;
    llvm::PHINode* current_loop_phi = NULL;
    codegen_context_llvm(bi_registry<ast::identifier, std::string>& sr): symbols_registry(sr) {}
};

void codegen_llvm(codegen_context_llvm &context, ast::program &program, const std::string& src_filename, const std::string& ir_filename);
