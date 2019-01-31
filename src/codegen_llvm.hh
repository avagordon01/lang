#include <stack>
#include <unordered_map>
#include <memory>

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

#include "ast.hh"

struct codegen_context_llvm {
    codegen_context_llvm(): builder(context) {}
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<ast::identifier, llvm::Value *> named_values;
};

llvm::Value* codegen_llvmexpression(codegen_context_llvm &context, ast::expression expression) {
    switch (expression.type) {
        case ast::expression::VARIABLE: {
            auto x = context.named_values.find(expression.variable);
            if (x != context.named_values.end()) {
                return x->second;
            } else {
                yyerror("variable used before being defined");
            }
        } break;
        case ast::expression::LITERAL: {
            switch (expression.literal->type) {
                case ast::literal::FLOAT:
                    //return llvm::ConstantFP::get(context.context, llvm::APFloat(expression.literal->_float));
                case ast::literal::INTEGER:
                    //return llvm::ConstantInt::get(context.context, expression.literal->_integer);
                case ast::literal::BOOL:
                    //return llvm::ConstantInt::get(context.context, expression.literal->_bool);
                    ;
                default: ;
            }
        } break;
        case ast::expression::UNARY_OPERATOR: {
            llvm::Value *r;
            r = codegen_llvmexpression(context, *expression.unary_operator->r);
            switch (expression.unary_operator->unary_operator) {
                case ast::unary_operator::B_NOT:
                    break;
                case ast::unary_operator::L_NOT:
                    break;
            }
        } break;
        case ast::expression::BINARY_OPERATOR: {
            llvm::Value *l, *r;
            l = codegen_llvmexpression(context, *expression.binary_operator->l);
            r = codegen_llvmexpression(context, *expression.binary_operator->r);
            switch (expression.binary_operator->binary_operator) {
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
                    ;
            }
        } break;
    }
    return NULL;
}

void codegen_llvmstatement(codegen_context_llvm &context, ast::statement statement) {
    switch (statement.type) {
        case ast::statement::S_BLOCK: {
        } break;
        case ast::statement::S_IF: {
        } break;
        case ast::statement::S_FOR: {
        } break;
        case ast::statement::S_WHILE: {
        } break;
        case ast::statement::S_FUNCTION: {
        } break;
        case ast::statement::S_ASSIGNMENT: {
            ast::assignment& assign = *(statement.assignment);
            llvm::Value* value = codegen_llvmexpression(context, *(assign.expression));
            llvm::Value* variable = context.named_values[assign.identifier];
            context.builder.CreateStore(value, variable);
        } break;
    }
}

void codegen_llvmprogram(codegen_context_llvm &context, ast::program *program) {
    printf("beginning codegen\n");
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
    for (auto& statement: program->statements) {
        codegen_llvmstatement(context, statement);
    }
    DBuilder->finalize();
    context.module->print(llvm::errs(), nullptr);
}
