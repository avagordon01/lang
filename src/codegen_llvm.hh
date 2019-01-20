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
    std::map<size_t, llvm::Value *> named_values;
};

llvm::Value* codegen_llvm_expression(codegen_context_llvm &context, ast::_expression expression) {
    switch (expression.type) {
        case ast::_expression::VARIABLE: {
            auto x = context.named_values.find(expression.variable);
            if (x != context.named_values.end()) {
                return x->second;
            } else {
                yyerror("variable used before being defined");
            }
        } break;
        case ast::_expression::LITERAL: {
            switch (expression.literal->type) {
                case ast::_literal::FLOAT:
                    //return llvm::ConstantFP::get(context.context, llvm::APFloat(expression.literal->_float));
                case ast::_literal::INTEGER:
                    //return llvm::ConstantInt::get(context.context, expression.literal->_integer);
                case ast::_literal::BOOL:
                    //return llvm::ConstantInt::get(context.context, expression.literal->_bool);
                    ;
                default: ;
            }
        } break;
        case ast::_expression::OPERATOR: {
            llvm::Value *l, *r;
            l = codegen_llvm_expression(context, *expression.op->l);
            r = codegen_llvm_expression(context, *expression.op->r);
            switch (expression.op->_operator) {
                case ast::_operator::A_ADD:
                    return context.builder.CreateFAdd(l, r, "addtmp");
                    break;
                case ast::_operator::A_SUB:
                    return context.builder.CreateFSub(l, r, "subtmp");
                    break;
                case ast::_operator::A_MUL:
                    return context.builder.CreateFMul(l, r, "multmp");
                    break;
                case ast::_operator::A_DIV:
                case ast::_operator::A_MOD:
                case ast::_operator::B_SHL:
                case ast::_operator::B_SHR:
                case ast::_operator::B_AND:
                case ast::_operator::B_XOR:
                case ast::_operator::B_OR:
                case ast::_operator::B_NOT:
                case ast::_operator::L_AND:
                case ast::_operator::L_OR:
                case ast::_operator::L_NOT:
                case ast::_operator::C_EQ:
                case ast::_operator::C_NE:
                case ast::_operator::C_GT:
                case ast::_operator::C_GE:
                case ast::_operator::C_LT:
                case ast::_operator::C_LE:
                    ;
            }
        } break;
    }
    return NULL;
}

void codegen_llvm_statement(codegen_context_llvm &context, ast::_statement statement) {
    switch (statement.type) {
        case ast::_statement::S_BLOCK: {
        } break;
        case ast::_statement::S_IF: {
        } break;
        case ast::_statement::S_FOR: {
        } break;
        case ast::_statement::S_WHILE: {
        } break;
        case ast::_statement::S_FUNCTION: {
        } break;
        case ast::_statement::S_ASSIGNMENT: {
            ast::_assignment& assign = *(statement.assignment);
            llvm::Value* value = codegen_llvm_expression(context, *(assign.expression));
            llvm::Value* variable = context.named_values[assign.identifier];
            context.builder.CreateStore(value, variable);
        } break;
    }
}

void codegen_llvm_program(codegen_context_llvm &context, ast::_program *program) {
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
        codegen_llvm_statement(context, statement);
    }
    DBuilder->finalize();
    context.module->print(llvm::errs(), nullptr);
}
