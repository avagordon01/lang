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
    //TODO create allocas for aggregate types (structs, arrays)
    llvm::AllocaInst* a = context.builder.CreateAlloca(type.to_llvm_type(context.context), 0, context.symbols_list[identifier.value].c_str());
    context.builder.SetInsertPoint(saved_bb);
    return a;
}

llvm::Value* accessor_access(codegen_context_llvm& context, ast::accessor& accessor) {
    return *context.variable_scopes.find_item(accessor.identifier);
}

struct llvm_codegen_fn {
    codegen_context_llvm& context;
    llvm::Value* operator()(ast::program& program) {
        context.variable_scopes.push_scope();
        for (auto& statement: program.statements) {
            std::invoke(*this, statement);
        }
        context.variable_scopes.pop_scope();
        return NULL;
    }
    llvm::Value* operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    llvm::Value* operator()(std::unique_ptr<ast::block>& block) {
        return std::invoke(*this, *block);
    }
    llvm::Value* operator()(ast::block& block) {
        llvm::Value* ret = NULL;
        context.variable_scopes.push_scope();
        for (auto& statement: block.statements) {
            ret = std::invoke(*this, statement);
        }
        context.variable_scopes.pop_scope();
        return ret;
    }
    llvm::Value* operator()(std::unique_ptr<ast::if_statement>& if_statement) {
        return std::invoke(*this, *if_statement);
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

        context.builder.SetInsertPoint(merge_block);
        ast::named_type type = if_statement.blocks.front().type;
        llvm::PHINode* phi = nullptr;
        if (!type.is_void()) {
            phi = context.builder.CreatePHI(
                type.to_llvm_type(context.context),
                if_statement.blocks.size(), "phi");
        }

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
                llvm::Value* v = std::invoke(*this, if_statement.blocks[i]);
                if (!type.is_void()) {
                    phi->addIncoming(v, basic_blocks[i]);
                }
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
                llvm::Value* v = std::invoke(*this, if_statement.blocks[i]);
                if (!type.is_void()) {
                    phi->addIncoming(v, basic_blocks[i]);
                }
                context.builder.CreateBr(merge_block);
            }
        }
        if (if_statement.blocks.size() > if_statement.conditions.size()) {
            //else
            context.builder.SetInsertPoint(basic_blocks.back());
            llvm::Value* v = std::invoke(*this, if_statement.blocks.back());
            if (!type.is_void()) {
                phi->addIncoming(v, basic_blocks.back());
            }
            context.builder.CreateBr(merge_block);
        }

        context.builder.SetInsertPoint(merge_block);

        return phi;
    }
    llvm::Value* operator()(std::unique_ptr<ast::for_loop>& for_loop) {
        return std::invoke(*this, *for_loop);
    }
    llvm::Value* operator()(ast::for_loop& for_loop) {
        llvm::Function* f = context.builder.GetInsertBlock()->getParent();
        context.variable_scopes.push_scope();
        std::invoke(*this, for_loop.initial);
        llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(context.context, "forloop", f);
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context.context, "formerge", f);
        context.current_loop_entry = loop_bb;
        context.current_loop_exit = merge_bb;
        context.builder.CreateBr(loop_bb);

        ast::named_type type = for_loop.block.type;
        llvm::PHINode* phi = nullptr;
        if (!type.is_void()) {
            context.builder.SetInsertPoint(merge_bb);
            phi = context.builder.CreatePHI(
                type.to_llvm_type(context.context),
                0, "forphi");
            context.current_loop_phi = phi;
        }

        context.builder.SetInsertPoint(loop_bb);
        llvm::Value* v = std::invoke(*this, for_loop.block);
        if (!type.is_void()) {
            phi->addIncoming(v, context.builder.GetInsertBlock());
        }
        std::invoke(*this, for_loop.step);
        llvm::Value* cond = std::invoke(*this, for_loop.condition);
        context.variable_scopes.pop_scope();
        context.builder.CreateCondBr(cond, loop_bb, merge_bb);
        context.builder.SetInsertPoint(merge_bb);

        return phi;
    }
    llvm::Value* operator()(std::unique_ptr<ast::while_loop>& while_loop) {
        return std::invoke(*this, *while_loop);
    }
    llvm::Value* operator()(ast::while_loop& while_loop) {
        llvm::Function* f = context.builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(context.context, "whileloop", f);
        llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context.context, "whilemerge", f);
        context.current_loop_entry = loop_bb;
        context.current_loop_exit = merge_bb;
        context.builder.CreateBr(loop_bb);

        ast::named_type type = while_loop.block.type;
        llvm::PHINode* phi = nullptr;
        if (!type.is_void()) {
            context.builder.SetInsertPoint(merge_bb);
            phi = context.builder.CreatePHI(
                type.to_llvm_type(context.context),
                0, "whilephi");
            context.current_loop_phi = phi;
        }

        context.builder.SetInsertPoint(loop_bb);
        llvm::Value* block_value = std::invoke(*this, while_loop.block);
        llvm::Value* cond = std::invoke(*this, while_loop.condition);
        context.builder.CreateCondBr(cond, loop_bb, merge_bb);
        context.builder.SetInsertPoint(merge_bb);

        if (!type.is_void()) {
            phi->addIncoming(block_value, loop_bb);
        }
        return phi;
    }
    llvm::Value* operator()(std::unique_ptr<ast::switch_statement>& switch_statement) {
        return std::invoke(*this, *switch_statement);
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

        context.builder.SetInsertPoint(merge_bb);
        ast::named_type type = switch_statement.cases.front().block.type;
        llvm::PHINode* phi {};
        if (!type.is_void()) {
            phi = context.builder.CreatePHI(
                type.to_llvm_type(context.context),
                num_basic_cases, "phi");
        }

        size_t i = 0;
        for (auto& case_statement: switch_statement.cases) {
            for (auto& basic_case: case_statement.cases) {
                ast::literal l{basic_case};
                llvm::Value* v = std::invoke(*this, l);
                if (!type.is_void()) {
                    phi->addIncoming(v, blocks[i]);
                }
                switch_inst->addCase(static_cast<llvm::ConstantInt*>(v), blocks[i]);
            }
            context.builder.SetInsertPoint(blocks[i]);
            std::invoke(*this, case_statement.block);
            context.builder.CreateBr(merge_bb);
            i++;
        }

        context.builder.SetInsertPoint(merge_bb);

        if (!type.is_void()) {
            return phi;
        } else {
            return NULL;
        }
    }
    llvm::Value* operator()(ast::function_def& function_def) {
        //prototype
        std::vector<llvm::Type*> parameter_types;
        for (auto& param: function_def.parameter_list) {
            ast::named_type type = param.type;
            parameter_types.push_back(type.to_llvm_type(context.context));
        }
        llvm::FunctionType* ft = llvm::FunctionType::get(
            ast::type{function_def.returntype}.to_llvm_type(context.context),
            parameter_types,
            false);
        llvm::Function* f = llvm::Function::Create(
            ft,
            function_def.to_export ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage,
            context.symbols_list[function_def.identifier.value], context.module.get());
        size_t i = 0;
        for (auto& arg: f->args()) {
            arg.setName(context.symbols_list[function_def.parameter_list[i++].identifier.value]);
        }

        //body
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(context.context, "entry", f);
        context.current_function_entry = bb;
        context.builder.SetInsertPoint(bb);

        context.variable_scopes.push_scope();
        size_t j = 0;
        for (auto& arg: f->args()) {
            ast::parameter param = function_def.parameter_list[j++];
            llvm::AllocaInst* alloca = CreateEntryBlockAlloca(context, param.identifier, param.type);
            context.builder.CreateStore(&arg, alloca);
            context.variable_scopes.push_item(param.identifier, std::move(alloca));
        }

        std::invoke(*this, function_def.block);
        context.variable_scopes.pop_scope();

        llvm::verifyFunction(*f);

        return f;
    }
    llvm::Value* operator()(ast::type_def& type_def) {
        //TODO
        return NULL;
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
            error(s_break.loc, "cannot call break statement outside of a loop body");
        }
        if (s_break.expression) {
            llvm::Value* v = std::invoke(*this, *s_break.expression);
            context.current_loop_phi->addIncoming(v, context.builder.GetInsertBlock());
        }
        context.builder.CreateBr(context.current_loop_exit);
        return NULL;
    }
    llvm::Value* operator()(ast::s_continue& s_continue) {
        if (!context.current_loop_entry) {
            error(s_continue.loc, "cannot call continue statement outside of a loop body");
        }
        context.builder.CreateBr(context.current_loop_entry);
        return NULL;
    }
    llvm::Value* operator()(ast::variable_def& variable_def) {
        llvm::Value* value = std::invoke(*this, variable_def.expression);
        llvm::AllocaInst* alloca = CreateEntryBlockAlloca(context, variable_def.identifier, variable_def.expression.type);
        context.builder.CreateStore(value, alloca);
        context.variable_scopes.push_item(variable_def.identifier, std::move(alloca));
        return NULL;
    }
    llvm::Value* operator()(ast::assignment& assignment) {
        llvm::Value* access = accessor_access(context, assignment.accessor);
        llvm::Value* value = std::invoke(*this, assignment.expression);
        context.builder.CreateStore(value, access);
        return NULL;
    }

    llvm::Value* operator()(ast::expression& expression) {
        return std::visit(*this, expression.expression);
    }
    llvm::Value* operator()(ast::identifier& identifier) {
        llvm::Value* variable = *context.variable_scopes.find_item(identifier);
        return context.builder.CreateLoad(variable, context.symbols_list[identifier.value].c_str());
    }
    llvm::Value* operator()(ast::literal& literal) {
        struct literal_visitor {
            codegen_context_llvm& context;
            ast::named_type type;
            llvm::Value* operator()(double& x) {
                assert(type.is_float());
                llvm::Type* llvm_type = type.to_llvm_type(context.context);
                return llvm::ConstantFP::get(llvm_type, x);
            }
            llvm::Value* operator()(ast::literal_integer& x) {
                assert(type.is_number());
                llvm::Type* llvm_type = type.to_llvm_type(context.context);
                if (type.is_integer()) {
                    return llvm::ConstantInt::get(llvm_type, x.data);
                } else {
                    return llvm::ConstantFP::get(llvm_type, static_cast<double>(x.data));
                }
            }
            llvm::Value* operator()(bool& x) {
                return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context.context), x);
            }
        };
        return std::visit(literal_visitor{context, literal.type}, literal.literal);
    }
    llvm::Value* operator()(ast::accessor& accessor) {
        llvm::Value* access = accessor_access(context, accessor);
        llvm::Value* value = context.builder.CreateLoad(access);
        return value;
    }
    llvm::Value* operator()(std::unique_ptr<ast::accessor>& accessor) {
        return std::invoke(*this, *accessor);
    }
    llvm::Value* operator()(std::unique_ptr<ast::function_call>& function_call) {
        llvm::Function* function = context.module->getFunction(context.symbols_list[function_call->identifier.value]);
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
                    if (binary_operator->type.is_unsigned_integer()) {
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
                    if (binary_operator->type.is_unsigned_integer()) {
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
                    if (binary_operator->type.is_unsigned_integer()) {
                        return context.builder.CreateICmpUGT(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSGT(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpUGT(l, r, "gttmp");
                }
            case ast::binary_operator::C_GE:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->type.is_unsigned_integer()) {
                        return context.builder.CreateICmpUGE(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSGE(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpUGE(l, r, "getmp");
                }
            case ast::binary_operator::C_LT:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->type.is_unsigned_integer()) {
                        return context.builder.CreateICmpULT(l, r, "getmp");
                    } else {
                        return context.builder.CreateICmpSLT(l, r, "getmp");
                    }
                } else {
                    return context.builder.CreateFCmpULT(l, r, "lttmp");
                }
            case ast::binary_operator::C_LE:
                if (l->getType()->isIntegerTy()) {
                    if (binary_operator->type.is_unsigned_integer()) {
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
        error(Error);
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
        error("couldn't open file", EC.message());
    }
    context.module->print(dest, nullptr);
    dest.flush();
}
