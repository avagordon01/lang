#include "scopes.hh"
#include "ast.hh"

int main() {
    scopes<ast::identifier, ast::type> s;
    s.push_item(0, ast::type::t_void);
    s.push_item(1, ast::type::t_bool);
    s.push_item(2, ast::type::u8);
    s.push_scope();
    s.push_item(0, ast::type::u16);
    s.push_item(1, ast::type::u32);
    assert(*s.find_item(0) == ast::type::u16);
    assert(*s.find_item(1) == ast::type::u32);
    assert(s.find_item_shadow(2) == std::nullopt);
    s.pop_scope();
    assert(*s.find_item(0) == ast::type::t_void);
    assert(*s.find_item(1) == ast::type::t_bool);
    assert(*s.find_item(2) == ast::type::u8);
    s.pop_scope();
    return 0;
}
