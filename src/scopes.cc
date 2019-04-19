#include "scopes.hh"
#include "ast.hh"

int main() {
    scopes<ast::identifier, ast::type_id> s;
    s.push_item(0, ast::type_id::t_void);
    s.push_item(1, ast::type_id::t_bool);
    s.push_item(2, ast::type_id::u8);
    s.push_scope();
    s.push_item(0, ast::type_id::u16);
    s.push_item(1, ast::type_id::u32);
    assert(*s.find_item(0) == ast::type_id::u16);
    assert(*s.find_item(1) == ast::type_id::u32);
    assert(s.find_item_current_scope(2) == std::nullopt);
    s.pop_scope();
    assert(*s.find_item(0) == ast::type_id::t_void);
    assert(*s.find_item(1) == ast::type_id::t_bool);
    assert(*s.find_item(2) == ast::type_id::u8);
    s.pop_scope();
    return 0;
}
