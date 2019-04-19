#include "scopes.hh"
#include "ast.hh"

int main() {
    scopes<ast::identifier, ast::type_id> vars;
    vars.push_item(0, ast::type_id::t_void);
    vars.push_item(1, ast::type_id::t_bool);
    vars.push_item(2, ast::type_id::u8);
    vars.push_scope();
    vars.push_item(0, ast::type_id::u16);
    vars.push_item(1, ast::type_id::u32);
    assert(*vars.find_item(0) == ast::type_id::u16);
    assert(*vars.find_item(1) == ast::type_id::u32);
    assert(vars.find_item_current_scope(2) == std::nullopt);
    vars.pop_scope();
    assert(*vars.find_item(0) == ast::type_id::t_void);
    assert(*vars.find_item(1) == ast::type_id::t_bool);
    assert(*vars.find_item(2) == ast::type_id::u8);
    vars.pop_scope();

    scopes<ast::type_id, ast::type> types;
    types.push_item(ast::type_id{0}, ast::type{ast::type_id{0}});
    types.find_item(ast::type_id{0});
    return 0;
}
