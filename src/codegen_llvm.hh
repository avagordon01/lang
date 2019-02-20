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
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>

#include "ast.hh"

#include <iostream>
#include <string>
void error(std::string s) {
    std::cerr << s << std::endl;
    exit(EXIT_FAILURE);
}

struct codegen_context_llvm {
    codegen_context_llvm(): builder(context) {}
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::unordered_map<ast::identifier, llvm::Value *> named_values;
    std::vector<std::string> symbols;
};

struct llvm_codegen_fn {
    codegen_context_llvm& context;
    llvm::Value* operator()(ast::program& program) {
        for (auto& statement: program.statements) {
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
    llvm::Value* operator()(ast::function_def& function_def) {
        //prototype
        std::vector<llvm::Type*> parameter_types;
        for (auto& param: function_def.parameter_list) {
            ast::type type = param.first;
            parameter_types.push_back(ast::type_to_llvm_type(context.context, type));
        }
        llvm::FunctionType* ft = llvm::FunctionType::get(
            ast::type_to_llvm_type(context.context, function_def.returntype),
            parameter_types,
            false);
        llvm::Function* f = llvm::Function::Create(
            ft,
            function_def.to_export ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage,
            context.symbols[function_def.identifier], context.module.get());
        size_t i = 0;
        for (auto& arg: f->args()) {
            arg.setName(context.symbols[function_def.parameter_list[i++].second]);
        }

        //body
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(context.context, "entry", f);
        context.builder.SetInsertPoint(bb);

        context.named_values.clear();
        size_t j = 0;
        for (auto& arg: f->args()) {
            context.named_values[function_def.parameter_list[j++].second] = &arg;
        }

        for (auto& statement: function_def.block.statements) {
            std::invoke(*this, statement);
        }

        llvm::verifyFunction(*f);

        return f;
    }
    llvm::Value* operator()(ast::s_return& s_return) {
        llvm::Value* ret = std::invoke(*this, s_return.expression);
        return context.builder.CreateRet(ret);
    }
    llvm::Value* operator()(ast::s_break& s_break) {
        return NULL;
    }
    llvm::Value* operator()(ast::s_continue& s_continue) {
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
        llvm::Value* x = context.named_values[identifier];
        if (!x) {
            error("variable used before being defined");
            return NULL;
        }
        return x;
    }
    llvm::Value* operator()(ast::literal& literal) {
        struct literal_visitor {
            codegen_context_llvm& context;
            llvm::Value* operator()(double& x) {
                return llvm::ConstantFP::get(context.context, llvm::APFloat(x));
            }
            llvm::Value* operator()(uint64_t& x) {
                return llvm::ConstantInt::get(context.context, llvm::APInt(x, 64));
            }
            llvm::Value* operator()(bool& x) {
                return llvm::ConstantInt::get(context.context, llvm::APInt(x, 1));
            }
        };
        return std::visit(literal_visitor{context}, literal.literal);
    }
    llvm::Value* operator()(std::unique_ptr<ast::function_call>& function_call) {
        llvm::Function* function = context.module->getFunction(context.symbols[function_call->identifier]);
        if (!function) {
            error("function called before being defined");
        }
        std::vector<llvm::Value*> arguments;
        for (auto& arg: function_call->arguments) {
            arguments.push_back(std::invoke(*this, arg));
        }
        size_t i = 0;
        for (auto arg = function->arg_begin(); arg != function->arg_end(); ++arg) {
            //FIXME use our type not LLVM type
            if (arguments[i++]->getType() != arg->getType()) {
                error("function call type does not match function definition type");
            }
        }
        return context.builder.CreateCall(function, arguments, "calltmp");
    }
    llvm::Value* operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        llvm::Value* l = std::invoke(*this, binary_operator->l);
        llvm::Value* r = std::invoke(*this, binary_operator->r);
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
            case ast::binary_operator::A_SUB:
            case ast::binary_operator::A_MUL:
            case ast::binary_operator::A_DIV:
            case ast::binary_operator::A_MOD:
                if (l->getType() != r->getType()) {
                    error("LHS and RHS of arithmetic operator are not of the same type");
                }
                if (!ast::llvm_type_is_number(l->getType())) {
                    error("LHS and RHS of arithmetic operator are not numbers");
                }
                break;
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
                if (!ast::llvm_type_is_integer(l->getType())) {
                    error("LHS of shift operator is not an integer");
                }
                if (!ast::llvm_type_is_integer(r->getType())) {
                    error("RHS of shift operator is not an integer");
                }
                break;
            case ast::binary_operator::B_AND:
            case ast::binary_operator::B_XOR:
            case ast::binary_operator::B_OR:
                if (l->getType() != r->getType()) {
                    error("LHS and RHS of bitwise operator are not of the same type");
                }
                if (!ast::llvm_type_is_integer(l->getType())) {
                    error("LHS of bitwise operator is not an integer");
                }
                if (!ast::llvm_type_is_integer(r->getType())) {
                    error("RHS of bitwise operator is not an integer");
                }
                break;
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
                if (!ast::llvm_type_is_bool(l->getType())) {
                    error("LHS of logical operator is not a boolean");
                }
                if (!ast::llvm_type_is_bool(r->getType())) {
                    error("RHS of logical operator is not a boolean");
                }
                break;
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
                if (l->getType() != r->getType()) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                break;
            case ast::binary_operator::C_GT:
            case ast::binary_operator::C_GE:
            case ast::binary_operator::C_LT:
            case ast::binary_operator::C_LE:
                if (l->getType() != r->getType()) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                if (!ast::llvm_type_is_number(l->getType())) {
                    error("LHS and RHS of comparison operator are not numbers");
                }
                break;
        }
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateAdd(l, r, "addtmp");
                } else {
                    return context.builder.CreateFAdd(l, r, "addtmp");
                }
                break;
            case ast::binary_operator::A_SUB:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateSub(l, r, "subtmp");
                } else {
                    return context.builder.CreateFSub(l, r, "subtmp");
                }
                break;
            case ast::binary_operator::A_MUL:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateMul(l, r, "multmp");
                } else {
                    return context.builder.CreateFMul(l, r, "multmp");
                }
                break;
            case ast::binary_operator::A_DIV:
                if (l->getType()->isIntegerTy()) {
                    //TODO check signedness of integer llvm types
                    return context.builder.CreateUDiv(l, r, "divtmp");
                } else {
                    return context.builder.CreateFDiv(l, r, "divtmp");
                }
                break;
            case ast::binary_operator::A_MOD:
                if (l->getType()->isIntegerTy()) {
                    //TODO check signedness of integer llvm types
                    return context.builder.CreateURem(l, r, "modtmp");
                } else {
                    return context.builder.CreateFRem(l, r, "modtmp");
                }
                break;
            case ast::binary_operator::B_SHL:
                return context.builder.CreateShl(l, r, "lshifttmp");
            case ast::binary_operator::B_SHR:
                return context.builder.CreateLShr(l, r, "rshifttmp");
            case ast::binary_operator::B_AND:
                return context.builder.CreateAnd(l, r, "bandtmp");
            case ast::binary_operator::B_XOR:
                return context.builder.CreateXor(l, r, "bxortmp");
            case ast::binary_operator::B_OR:
                return context.builder.CreateOr(l, r, "bortmp");
            case ast::binary_operator::L_AND:
                return context.builder.CreateAnd(l, r, "landtmp");
            case ast::binary_operator::L_OR:
                return context.builder.CreateOr(l, r, "lortmp");
            case ast::binary_operator::C_EQ:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateICmpEQ(l, r, "eqtmp");
                } else {
                    return context.builder.CreateFCmpUEQ(l, r, "eqtmp");
                }
            case ast::binary_operator::C_NE:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateICmpNE(l, r, "netmp");
                } else {
                    return context.builder.CreateFCmpUNE(l, r, "netmp");
                }
            case ast::binary_operator::C_GT:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateICmpUGT(l, r, "gttmp");
                } else {
                    return context.builder.CreateFCmpUGT(l, r, "gttmp");
                }
            case ast::binary_operator::C_GE:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateICmpUGE(l, r, "getmp");
                } else {
                    return context.builder.CreateFCmpUGE(l, r, "getmp");
                }
            case ast::binary_operator::C_LT:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateICmpULT(l, r, "lttmp");
                } else {
                    return context.builder.CreateFCmpULT(l, r, "lttmp");
                }
            case ast::binary_operator::C_LE:
                if (l->getType()->isIntegerTy()) {
                    return context.builder.CreateICmpULE(l, r, "letmp");
                } else {
                    return context.builder.CreateFCmpULE(l, r, "letmp");
                }
        }
        assert(false);
    }
    llvm::Value* operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        llvm::Value* r = std::invoke(*this, unary_operator->r);
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                if (!ast::llvm_type_is_integer(r->getType())) {
                    error("RHS of bitwise negation is not an integer");
                }
                break;
            case ast::unary_operator::L_NOT:
                if (!ast::llvm_type_is_bool(r->getType())) {
                    error("RHS of bitwise negation is not boolean");
                }
                break;
        }
        uint64_t ones = -1;
        uint64_t one = 1;
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                return context.builder.CreateXor(r, ones, "bnottmp");
            case ast::unary_operator::L_NOT:
                return context.builder.CreateTrunc(
                    context.builder.CreateXor(r, one, "lnottmp"),
                    llvm::IntegerType::get(context.context, 1)
                );
        }
        assert(false);
    }
};

void codegen_llvm(codegen_context_llvm &context, ast::program &program, const std::string& src_filename, const std::string& obj_filename, const std::string& ir_filename = "") {
    context.module = llvm::make_unique<llvm::Module>(src_filename, context.context);

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
        std::cerr << Error << std::endl;;
        exit(EXIT_FAILURE);
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

    std::error_code EC;
    if (!ir_filename.empty()) {
        llvm::raw_fd_ostream dest(ir_filename, EC, llvm::sys::fs::OpenFlags::F_None);
        if (EC) {
            std::cerr << "Could not open file: " << EC.message() << std::endl;
            exit(EXIT_FAILURE);
        }
        context.module->print(dest, nullptr);
        dest.flush();
    }

    llvm::raw_fd_ostream dest(obj_filename, EC, llvm::sys::fs::OpenFlags::F_None);
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        exit(EXIT_FAILURE);
    }
    llvm::legacy::PassManager pass;
    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::TargetMachine::CGFT_ObjectFile)) {
        std::cerr << "TheTargetMachine can't emit a file of this type" << std::endl;
        exit(EXIT_FAILURE);
    }
    pass.run(*context.module);
    dest.flush();
}
