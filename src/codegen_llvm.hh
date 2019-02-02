#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>

#include "utils.hh"
#include "ast.hh"

struct codegen_context_llvm {
    codegen_context_llvm(): builder(context) {}
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::unordered_map<ast::identifier, llvm::Value *> named_values;
};

struct llvm_codegen_fn {
    codegen_context_llvm& context;
    llvm::Value* operator()(ast::program& program) {
        for (auto& statement: program.statements) {
            printf("codegen statement\n");
            std::invoke(*this, statement);
        }
        return NULL;
    }
    llvm::Value* operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    llvm::Value* operator()(ast::block& block) {
        for (auto& statement: block.statements) {
            std::invoke(*this, statement);
        }
        return NULL;
    }
    llvm::Value* operator()(ast::if_statement& if_statement) {
        for (auto& condition: if_statement.conditions) {
            std::invoke(*this, condition);
        }
        return NULL;
    }
    llvm::Value* operator()(ast::for_loop& for_loop) {
        return NULL;
    }
    llvm::Value* operator()(ast::while_loop& while_loop) {
        return NULL;
    }
    llvm::Value* operator()(ast::function& function) {
        return NULL;
    }
    llvm::Value* operator()(ast::assignment& assignment) {
        llvm::Value* value = std::invoke(*this, assignment.expression);
        llvm::Value* variable = context.named_values[assignment.identifier];
        return context.builder.CreateStore(value, variable);
    }

    llvm::Value* operator()(ast::expression& expression) {
        return std::visit(*this, expression.expression);
    }
    llvm::Value* operator()(ast::identifier& identifier) {
        auto x = context.named_values.find(identifier);
        if (x != context.named_values.end()) {
            return x->second;
        } else {
            yyerror("variable used before being defined");
            return NULL;
        }
    }
    llvm::Value* operator()(ast::literal& literal) {
        struct literal_visitor {
            codegen_context_llvm& context;
            llvm::Value* operator()(double& x) {
                return llvm::ConstantFP::get(context.context, llvm::APFloat(x));
            }
            llvm::Value* operator()(uint64_t& x) {
                return NULL;//llvm::ConstantInt::get(context.context, x);
            }
            llvm::Value* operator()(bool& x) {
                return NULL;//llvm::ConstantInt::get(context.context, x);
            }
        };
        return std::visit(literal_visitor{context}, literal.literal);
    }
    llvm::Value* operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        llvm::Value* l = std::invoke(*this, binary_operator->l);
        llvm::Value* r = std::invoke(*this, binary_operator->r);
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
                return context.builder.CreateFAdd(l, r, "addtmp");
                break;
            case ast::binary_operator::A_SUB:
                return context.builder.CreateFSub(l, r, "subtmp");
                break;
            case ast::binary_operator::A_MUL:
                return context.builder.CreateFMul(l, r, "multmp");
                break;
            case ast::binary_operator::A_DIV:
            case ast::binary_operator::A_MOD:
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
            case ast::binary_operator::B_AND:
            case ast::binary_operator::B_XOR:
            case ast::binary_operator::B_OR:
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
            case ast::binary_operator::C_GT:
            case ast::binary_operator::C_GE:
            case ast::binary_operator::C_LT:
            case ast::binary_operator::C_LE:
                return NULL;
                ;
        }
    }
    llvm::Value* operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        llvm::Value* r = std::invoke(*this, unary_operator->r);
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                break;
            case ast::unary_operator::L_NOT:
                break;
        }
        return r;
    }
};

void codegen_llvm(codegen_context_llvm &context, ast::program &program) {
    context.module = llvm::make_unique<llvm::Module>("lang compiler", context.context);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    context.module->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target) {
        llvm::errs() << Error;
        exit(1);
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    context.module->setDataLayout(TheTargetMachine->createDataLayout());

    context.module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
    std::unique_ptr<llvm::DIBuilder> DBuilder = llvm::make_unique<llvm::DIBuilder>(*context.module);
    std::invoke(llvm_codegen_fn{context}, program);
    DBuilder->finalize();
    context.module->print(llvm::errs(), nullptr);
}
