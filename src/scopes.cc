#include "scopes.hh"
#include "ast.hh"

int main() {
    scopes<ast::identifier, ast::primitive_type> vars;
    vars.push_item({0}, {ast::primitive_type::t_void});
    vars.push_item({1}, {ast::primitive_type::t_bool});
    vars.push_item({2}, {ast::primitive_type::u8});
    vars.push_scope();
    vars.push_item({0}, {ast::primitive_type::u16});
    vars.push_item({1}, {ast::primitive_type::u32});
    //FIXME
    //assert(*vars.find_item({0}) == ast::primitive_type{ast::primitive_type::u16});
    //assert(*vars.find_item({1}) == ast::primitive_type{ast::primitive_type::u32});
    assert(vars.find_item_current_scope({2}) == std::nullopt);
    vars.pop_scope();
    //assert(*vars.find_item({0}) == ast::primitive_type{ast::primitive_type::t_void});
    //assert(*vars.find_item({1}) == ast::primitive_type{ast::primitive_type::t_bool});
    //assert(*vars.find_item({2}) == ast::primitive_type{ast::primitive_type::u8});
    vars.pop_scope();

    scopes<ast::primitive_type, ast::type> types;
    types.push_item(ast::primitive_type{ast::primitive_type::t_void}, ast::type{ast::primitive_type{ast::primitive_type::t_void}});
    types.find_item(ast::primitive_type{ast::primitive_type::t_void});
    return 0;
}
