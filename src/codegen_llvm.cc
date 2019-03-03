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

#include <iostream>
#include "ast.hh"
#include "codegen_llvm.hh"
#include "error.hh"

static llvm::AllocaInst *
CreateEntryBlockAlloca(
codegen_context_llvm& context, const ast::identifier identifier, ast::type type
) {
    llvm::BasicBlock* saved_bb = context.builder.GetInsertBlock();
    llvm::BasicBlock* entry_bb = context.current_function_entry;
    context.builder.SetInsertPoint(entry_bb, entry_bb->begin());
    llvm::AllocaInst* a = context.builder.CreateAlloca(ast::type_to_llvm_type(context.context, type), 0, context.symbols[identifier].c_str());
    context.builder.SetInsertPoint(saved_bb);
    return a;
}

struct llvm_codegen_fn {
    codegen_context_llvm& context;
    llvm::Value* operator()(ast::program& program) {
        context.scopes.push_back({});
        for (auto& function_def: program.function_defs) {
            std::invoke(*this, function_def);
        }
        return NULL;
    }
    llvm::Value* operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    llvm::Value* operator()(ast::block& block) {
        llvm::Value* ret = NULL;
        for (auto& statement: block.statements) {
            ret = std::invoke(*this, statement);
        }
        return ret;
    }
    llvm::Value* operator()(ast::if_statement& if_statement) {
        std::vector<llvm::Value*> conditions;
        for (auto& condition: if_statement.conditions) {
            conditions.push_back(std::invoke(*this, condition));
        }

        llvm::Function* f = context.builder.GetInsertBlock()->getParent();
        std::vector<llvm::BasicBlock*> condition_blocks;
        for (size_t i = 0; i < if_statement.conditions.size(); i++) {
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(context.context, "condblock", f);
            condition_blocks.push_back(bb);
        }
        std::vector<llvm::BasicBlock*> basic_blocks;
        for (size_t i = 0; i < if_statement.blocks.size(); i++) {
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(context.context, "doblock", f);
            basic_blocks.push_back(bb);
        }
        llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(context.context, "mergeblock", f);

        context.builder.CreateBr(condition_blocks[0]);

        assert(if_statement.blocks.size() >= 1);
        assert(if_statement.conditions.size() >= 1);
        assert(if_statement.blocks.size() == if_statement.conditions.size() ||
            if_statement.blocks.size() == if_statement.conditions.size() + 1);

        for (size_t i = 0; i < if_statement.conditions.size(); i++) {
            if (i != if_statement.conditions.size()-1) {
                //if/else if
                context.builder.SetInsertPoint(condition_blocks[i]);
                context.builder.CreateCondBr(conditions[i], basic_blocks[i], condition_blocks[i + 1]);
                context.builder.SetInsertPoint(basic_blocks[i]);
                context.scopes.push_back({});
                std::invoke(*this, if_statement.blocks[i]);
                context.scopes.pop_back();
                context.builder.CreateBr(merge_block);
            } else {
                //final if/else if
                context.builder.SetInsertPoint(condition_blocks[i]);
                if (if_statement.blocks.size() > if_statement.conditions.size()) {
                    context.builder.CreateCondBr(conditions[i], basic_blocks[i], basic_blocks.back());
                } else {
                    context.builder.CreateCondBr(conditions[i], basic_blocks[i], merge_block);
                }
                context.builder.SetInsertPoint(basic_blocks[i]);
                context.scopes.push_back({});
                std::invoke(*this, if_statement.blocks[i]);
                context.scopes.pop_back();
                context.builder.CreateBr(merge_block);
            }
        }
        if (if_statement.blocks.size() > if_statement.conditions.size()) {
            //else
            context.builder.SetInsertPoint(basic_blocks.back());
            context.scopes.push_back({});
            std::invoke(*this, if_statement.blocks.back());
            context.scopes.pop_back();
            context.builder.CreateBr(merge_block);
        }

        context.builder.SetInsertPoint(merge_block);

        return NULL;
    }
    llvm::Value* operator()(ast::for_loop& for_loop) {
        llvm::Function* f = context.builder.GetInsertBlock()->getParent();
        context.scopes.push_back({});
        std::invoke(*this, for_loop.initial);
        llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(context.context, "forloop", f);
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context.context, "formerge", f);
        context.current_loop_entry = loop_bb;
        context.current_loop_exit = merge_bb;
        context.builder.CreateBr(loop_bb);
        context.builder.SetInsertPoint(loop_bb);
        std::invoke(*this, for_loop.block);
        std::invoke(*this, for_loop.step);
        llvm::Value* cond = std::invoke(*this, for_loop.condition);
        context.scopes.pop_back();
        context.builder.CreateCondBr(cond, loop_bb, merge_bb);
        context.builder.SetInsertPoint(merge_bb);
        return NULL;
    }
    llvm::Value* operator()(ast::while_loop& while_loop) {
        llvm::Function* f = context.builder.GetInsertBlock()->getParent();
        context.scopes.push_back({});
        llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(context.context, "whileloop", f);
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context.context, "whilemerge", f);
        context.current_loop_entry = loop_bb;
        context.current_loop_exit = merge_bb;
        context.builder.CreateBr(loop_bb);
        context.builder.SetInsertPoint(loop_bb);
        std::invoke(*this, while_loop.block);
        llvm::Value* cond = std::invoke(*this, while_loop.condition);
        context.scopes.pop_back();
        context.builder.CreateCondBr(cond, loop_bb, merge_bb);
        context.builder.SetInsertPoint(merge_bb);
        return NULL;
    }
    llvm::Value* operator()(ast::switch_statement& switch_statement) {
        llvm::Function* f = context.builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context.context, "switchmerge", f);
        std::vector<llvm::BasicBlock*> blocks;
        size_t num_basic_cases = 0;
        for (auto& case_statement: switch_statement.cases) {
            blocks.push_back(llvm::BasicBlock::Create(context.context, "case", f));
            num_basic_cases += case_statement.cases.size();
        }
        llvm::SwitchInst* switch_inst = context.builder.CreateSwitch(
            std::invoke(*this, switch_statement.expression),
            merge_bb,
            num_basic_cases);
        size_t i = 0;
        for (auto& case_statement: switch_statement.cases) {
            for (auto& basic_case: case_statement.cases) {
                switch_inst->addCase(static_cast<llvm::ConstantInt*>(std::invoke(*this, basic_case)), blocks[i]);
            }
            context.builder.SetInsertPoint(blocks[i]);
            context.scopes.push_back({});
            std::invoke(*this, case_statement.block);
            context.builder.CreateBr(merge_bb);
            context.scopes.pop_back();
            i++;
        }
        context.builder.SetInsertPoint(merge_bb);
        return NULL;
    }
    llvm::Value* operator()(ast::function_def& function_def) {
        //prototype
        std::vector<llvm::Type*> parameter_types;
        for (auto& param: function_def.parameter_list) {
            ast::type type = param.type;
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
            arg.setName(context.symbols[function_def.parameter_list[i++].identifier]);
        }

        //body
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(context.context, "entry", f);
        context.current_function_entry = bb;
        context.builder.SetInsertPoint(bb);

        context.scopes.push_back({});
        size_t j = 0;
        for (auto& arg: f->args()) {
            ast::parameter param = function_def.parameter_list[j++];
            llvm::AllocaInst* alloca = CreateEntryBlockAlloca(context, param.identifier, param.type);
            context.builder.CreateStore(&arg, alloca);
            context.scopes.back()[param.identifier] = alloca;
        }

        std::invoke(*this, function_def.block);
        context.scopes.pop_back();

        llvm::verifyFunction(*f);

        return f;
    }
    llvm::Value* operator()(ast::s_return& s_return) {
        if (s_return.expression) {
            context.builder.CreateRet(std::invoke(*this, *s_return.expression));
        } else {
            context.builder.CreateRetVoid();
        }
        return NULL;
    }
    llvm::Value* operator()(ast::s_break& s_break) {
        if (!context.current_loop_exit) {
            error("cannot call break statement outside of a loop body");
        }
        context.builder.CreateBr(context.current_loop_exit);
        return NULL;
    }
    llvm::Value* operator()(ast::s_continue& s_continue) {
        if (!context.current_loop_entry) {
            error("cannot call continue statement outside of a loop body");
        }
        context.builder.CreateBr(context.current_loop_entry);
        return NULL;
    }
    llvm::Value* operator()(ast::variable_def& variable_def) {
        llvm::Value* value = std::invoke(*this, variable_def.expression);
        llvm::AllocaInst* alloca = CreateEntryBlockAlloca(context, variable_def.identifier, *variable_def.type);
        context.builder.CreateStore(value, alloca);
        context.scopes.back()[variable_def.identifier] = alloca;
        return NULL;
    }
    llvm::Value* operator()(ast::assignment& assignment) {
        llvm::Value* variable;
        auto it = context.scopes.rbegin();
        for (it = context.scopes.rbegin(); it != context.scopes.rend(); ++it) {
            auto v = it->find(assignment.identifier);
            if (v != it->end()) {
                variable = v->second;
                break;
            }
        }
        llvm::Value* value = std::invoke(*this, assignment.expression);
        context.builder.CreateStore(value, variable);
        return NULL;
    }

    llvm::Value* operator()(ast::expression& expression) {
        return std::visit(*this, expression.expression);
    }
    llvm::Value* operator()(ast::identifier& identifier) {
        llvm::Value* variable;
        auto it = context.scopes.rbegin();
        for (it = context.scopes.rbegin(); it != context.scopes.rend(); ++it) {
            auto v = it->find(identifier);
            if (v != it->end()) {
                variable = v->second;
                break;
            }
        }
        return context.builder.CreateLoad(variable, context.symbols[identifier].c_str());
    }
    llvm::Value* operator()(ast::literal& literal) {
        struct literal_visitor {
            codegen_context_llvm& context;
            std::optional<ast::type> type;
            llvm::Value* operator()(double& x) {
                llvm::Type* llvm_type;
                if (!type) {
                    type = ast::type::f32;
                }
                switch (*type) {
                    case ast::type::f16:
                        llvm_type = llvm::Type::getHalfTy(context.context);
                        break;
                    case ast::type::f32:
                        llvm_type = llvm::Type::getFloatTy(context.context);
                        break;
                    case ast::type::f64:
                        llvm_type = llvm::Type::getDoubleTy(context.context);
                        break;
                    default:
                        assert(false);
                }
                return llvm::ConstantFP::get(llvm_type, x);
            }
            llvm::Value* operator()(uint64_t& x) {
                llvm::Type* llvm_type;
                if (!type) {
                    type = ast::type::i32;
                }
                switch (*type) {
                    case ast::type::u8:
                        llvm_type = llvm::Type::getInt8Ty(context.context);
                        break;
                    case ast::type::u16:
                        llvm_type = llvm::Type::getInt16Ty(context.context);
                        break;
                    case ast::type::u32:
                        llvm_type = llvm::Type::getInt32Ty(context.context);
                        break;
                    case ast::type::u64:
                        llvm_type = llvm::Type::getInt64Ty(context.context);
                        break;
                    case ast::type::i8:
                        llvm_type = llvm::Type::getInt8Ty(context.context);
                        break;
                    case ast::type::i16:
                        llvm_type = llvm::Type::getInt16Ty(context.context);
                        break;
                    case ast::type::i32:
                        llvm_type = llvm::Type::getInt32Ty(context.context);
                        break;
                    case ast::type::i64:
                        llvm_type = llvm::Type::getInt64Ty(context.context);
                        break;
                    case ast::type::f16:
                        llvm_type = llvm::Type::getHalfTy(context.context);
                        break;
                    case ast::type::f32:
                        llvm_type = llvm::Type::getFloatTy(context.context);
                        break;
                    case ast::type::f64:
                        llvm_type = llvm::Type::getDoubleTy(context.context);
                        break;
                    default:
                        assert(false);
                }
                if (ast::type_is_integer(*type)) {
                    return llvm::ConstantInt::get(llvm_type, x);
                } else {
                    return llvm::ConstantFP::get(llvm_type, static_cast<double>(x));
                }
            }
            llvm::Value* operator()(bool& x) {
                return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context.context), x);
            }
        };
        return std::visit(literal_visitor{context, literal.type}, literal.literal);
    }
    llvm::Value* operator()(std::unique_ptr<ast::function_call>& function_call) {
        llvm::Function* function = context.module->getFunction(context.symbols[function_call->identifier]);
        assert(function);
        std::vector<llvm::Value*> arguments;
        for (auto& arg: function_call->arguments) {
            arguments.push_back(std::invoke(*this, arg));
        }
        return context.builder.CreateCall(function, arguments, "calltmp");
    }
    llvm::Value* operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        llvm::Value* l = std::invoke(*this, binary_operator->l);
        llvm::Value* r = std::invoke(*this, binary_operator->r);
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
                    if (binary_operator->is_unsigned) {
                        return context.builder.CreateUDiv(l, r, "divtmp");
                    } else {
                        return context.builder.CreateSDiv(l, r, "divtmp");
                    }
                } else {
                    return context.builder.CreateFDiv(l, r, "divtmp");
                }
                break;
            case ast::binary_operator::A_MOD:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->is_unsigned) {
                        return context.builder.CreateURem(l, r, "modtmp");
                    } else {
                        return context.builder.CreateSRem(l, r, "modtmp");
                    }
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
                    if (binary_operator->is_unsigned) {
                        return context.builder.CreateICmpUGT(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSGT(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpUGT(l, r, "gttmp");
                }
            case ast::binary_operator::C_GE:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->is_unsigned) {
                        return context.builder.CreateICmpUGE(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSGE(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpUGE(l, r, "getmp");
                }
            case ast::binary_operator::C_LT:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->is_unsigned) {
                        return context.builder.CreateICmpULT(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSLT(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpULT(l, r, "lttmp");
                }
            case ast::binary_operator::C_LE:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->is_unsigned) {
                        return context.builder.CreateICmpULE(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSLE(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpULE(l, r, "letmp");
                }
        }
        assert(false);
    }
    llvm::Value* operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        llvm::Value* r = std::invoke(*this, unary_operator->r);
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

void codegen_llvm(codegen_context_llvm &context, ast::program &program, const std::string& src_filename, const std::string& ir_filename) {
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
    llvm::raw_fd_ostream dest(ir_filename, EC, llvm::sys::fs::OpenFlags::F_None);
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        exit(EXIT_FAILURE);
    }
    context.module->print(dest, nullptr);
    dest.flush();
}
