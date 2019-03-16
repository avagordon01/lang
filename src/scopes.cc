#include "scopes.hh"
#include "ast.hh"

int main() {
    scopes<ast::identifier, ast::type> s;
    s.push_item(0, ast::primitive_type::t_void);
    s.push_item(1, ast::primitive_type::t_bool);
    s.push_item(2, ast::primitive_type::u8);
    s.push_scope();
    s.push_item(0, ast::primitive_type::u16);
    s.push_item(1, ast::primitive_type::u32);
    assert(*s.find_item(0) == ast::primitive_type::u16);
    assert(*s.find_item(1) == ast::primitive_type::u32);
    assert(s.find_item_current_scope(2) == std::nullopt);
    s.pop_scope();
    assert(*s.find_item(0) == ast::primitive_type::t_void);
    assert(*s.find_item(1) == ast::primitive_type::t_bool);
    assert(*s.find_item(2) == ast::primitive_type::u8);
    s.pop_scope();
    return 0;
}
