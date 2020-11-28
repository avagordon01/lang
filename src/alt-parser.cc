#include "driver.hh"
#include "alt-parser.hh"

#include <tao/pegtl.hpp>
#include <map>

using namespace tao::pegtl;

ast::program alt_parser_context::parse_program() {
    ast::program p {};
    try {
    } catch (parse_error& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    return p;
}
struct ignore: sor<
    plus<space>,
    if_must<string<'/', '/'>, until<eolf>>,
    if_must<string<'/', '*'>, until<string<'*', '/'>>>
> {};
struct block;
struct expr;
struct literal_integer: seq<
    opt<one<'+', '-'>>,
    plus<digit>
> {};
struct primitive_type: sor<
    keyword<'b', 'o', 'o', 'l'>,
    keyword<'v', 'o', 'i', 'd'>,
    keyword<'u', '8'>,
    keyword<'u', '1', '6'>,
    keyword<'u', '3', '2'>,
    keyword<'u', '6', '4'>,
    keyword<'i', '8'>,
    keyword<'i', '1', '6'>,
    keyword<'i', '3', '2'>,
    keyword<'i', '6', '4'>,
    keyword<'f', '8'>,
    keyword<'f', '1', '6'>,
    keyword<'f', '3', '2'>,
    keyword<'f', '6', '4'>
> {};
struct named_type: sor<
    primitive_type,
    identifier
> {};
struct struct_type: if_must<
    keyword<'s', 't', 'r', 'u', 'c', 't'>,
    one<'{'>,
    list<seq<named_type, identifier>, one<','>>,
    one<'}'>
> {};
struct array_type: seq<
    one<'['>,
    named_type,
    literal_integer,
    one<']'>
> {};
struct type: sor<
    named_type,
    struct_type,
    array_type
> {};
struct function_call: seq<
    identifier,
    one<'('>,
    list_must<expr, one<','>>,
    one<')'>
> {};
struct type_def: if_must<
    keyword<'t', 'y', 'p', 'e'>,
    identifier,
    one<'='>,
    type
> {};
struct variable_def: if_must<
    keyword<'v', 'a', 'r'>,
    opt<named_type>,
    identifier,
    one<'='>,
    expr
> {};
struct field_access: seq<
    one<'.'>, identifier
> {};
struct array_access: seq<
    one<'['>, expr, one<']'>
> {};
struct accessor: seq<
    identifier,
    star<sor<field_access, array_access>>
> {};
struct assignment: seq<
    accessor,
    one<'='>,
    expr
> {};
struct if_statement: if_must<
    keyword<'i', 'f'>, expr, block,
    star<seq<keyword<'e', 'l', 'i', 'f'>, expr, block>>,
    opt<seq<keyword<'e', 'l', 's', 'e'>, block>>
> {};
struct for_loop: if_must<
    keyword<'f', 'o', 'r'>,
    variable_def, one<';'>, 
    expr, one<';'>,
    assignment,
    block
> {};
struct while_loop: if_must<
    keyword<'w', 'h', 'i', 'l', 'e'>,
    expr, block
> {};
struct switch_statement: if_must<
    keyword<'s', 'w', 'i', 't', 'c', 'h'>,
    expr,
    one<'{'>, star<seq<
        keyword<'c', 'a', 's', 'e'>,
        list<literal_integer, one<','>>,
        block
    >>, one<'}'>
> {};
struct function_def: seq<
    opt<keyword<'e', 'x', 'p', 'o', 'r', 't'>>,
    keyword<'f', 'n'>,
    opt<primitive_type>,
    identifier,
    one<'('>,
    list_must<seq<named_type, identifier>, one<','>>,
    one<')'>,
    block
> {};
struct s_return: seq<
    keyword<'r', 'e', 't', 'u', 'r', 'n'>,
    opt<expr>
> {};
struct s_break: keyword<'b', 'r', 'e', 'a', 'k'> {};
struct s_continue: keyword<'c', 'o', 'n', 't', 'i', 'n', 'u', 'e'> {};
struct literal: sor<
    sor<
        keyword<'t', 'r', 'u', 'e'>,
        keyword<'f', 'a', 'l', 's', 'e'>
    >,
    //TODO pegtl must provide primitives for parsing floats/ints
    literal_integer,
    float
> {};
struct top_level_statement: sor<
    function_def,
    type_def,
    variable_def
> {};
struct statement: sor<
    expr,
    function_def,
    variable_def,
    type_def,
    assignment,
    s_return,
    s_break,
    s_continue
> {};
struct block: seq<
    one<'{'>,
    list_must<statement, one<';'>>,
    one<'}'>
> {};
struct operators {
    enum class order: int {
    };
    operators();
    [[nodiscard]] const std::map<std::string, order> ops() const noexcept {
        return {
            {"+", order(5)},
            {"-", order(5)},
            {"*", order(5)},
            {"/", order(5)},
            {"%", order(5)},

            {"&", order(5)},
            {"|", order(5)},
            {"^", order(5)},
            {"~", order(5)},
            {"<<", order(5)},
            {">>", order(5)},

            {"&&", order(5)},
            {"||", order(5)},
            {"!",  order(5)},

            {"==", order(5)},
            {"!=", order(5)},
            {">",  order(5)},
            {">=", order(5)},
            {"<",  order(5)},
            {">=", order(5)},
        };
    }
};
struct exp_atom: sor<
    literal,
    if_statement,
    switch_statement,
    for_loop,
    while_loop,
    block,
    function_call,
    accessor,
    seq<one<'('>, expr, one<')'>>
> {};
struct expr: exp_atom {};
struct program: must<
    list_must<top_level_statement, one<';'>>,
    eof
> {};
