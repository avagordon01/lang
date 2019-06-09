#include <memory>

#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvBuilder.h>

#include "scopes.hh"
#include "ast.hh"

struct codegen_context_spirv {
    spv::Builder builder;
    spv::Module module;
    ::scopes<ast::identifier, size_t> variable_scopes;
    std::vector<std::string> symbols_list;
    spv::Block* current_function_entry = NULL;
    spv::Block* current_loop_exit = NULL;
    spv::Block* current_loop_entry = NULL;
};

void codegen_spirv(codegen_context_spirv &context, ast::program &program, const std::string& src_filename, const std::string& ir_filename);
