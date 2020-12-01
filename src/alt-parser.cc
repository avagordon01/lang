#include "driver.hh"
#include "alt-parser.hh"

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <map>

using namespace tao::pegtl;


struct comment: sor<
    if_must<string<'/', '/'>, until<eolf>>,
    if_must<string<'/', '*'>, until<string<'*', '/'>>>
> {};
struct whitespace: plus<space> {};
struct ignore: star<space> {};
struct block;
struct expr;
struct literal_integer: seq<
    opt<one<'+', '-'>>,
    plus<digit>
> {};
struct literal_float: seq<
    literal_integer,
    one<'.'>,
    literal_integer
> {};
struct primitive_type;
struct literal: seq<
    sor<
        sor<
            TAO_PEGTL_KEYWORD("true"),
            TAO_PEGTL_KEYWORD("false")
        >,
        literal_float,
        literal_integer
    >, ignore,
    opt<primitive_type>
> {};
struct primitive_type: sor<
    TAO_PEGTL_KEYWORD("void"),
    TAO_PEGTL_KEYWORD("bool"),
    TAO_PEGTL_KEYWORD("u8"),
    TAO_PEGTL_KEYWORD("u16"),
    TAO_PEGTL_KEYWORD("u32"),
    TAO_PEGTL_KEYWORD("u64"),
    TAO_PEGTL_KEYWORD("i8"),
    TAO_PEGTL_KEYWORD("i16"),
    TAO_PEGTL_KEYWORD("i32"),
    TAO_PEGTL_KEYWORD("i64"),
    TAO_PEGTL_KEYWORD("f8"),
    TAO_PEGTL_KEYWORD("f16"),
    TAO_PEGTL_KEYWORD("f32"),
    TAO_PEGTL_KEYWORD("f64")
> {};
struct named_type: sor<
    primitive_type,
    identifier
> {};
struct struct_type: if_must<
    TAO_PEGTL_KEYWORD("struct"), ignore,
    one<'{'>, ignore,
    star<seq<named_type, ignore, identifier, ignore, one<','>, ignore>>,
    one<'}'>
> {};
struct array_type: seq<
    one<'['>,
    named_type,
    literal_integer,
    one<']'>
> {};
struct type: sor<
    struct_type,
    array_type,
    named_type
> {};
struct function_call: seq<
    identifier,
    one<'('>,
    list_must<expr, one<','>, whitespace>,
    one<')'>
> {};
struct type_def: if_must<
    TAO_PEGTL_KEYWORD("type"), whitespace,
    identifier, ignore,
    one<'='>, ignore,
    type
> {};
struct variable_def: if_must<
    TAO_PEGTL_KEYWORD("var"), ignore,
    sor<
        seq<named_type, whitespace, identifier>,
        identifier
    >, ignore,
    one<'='>, ignore,
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
    accessor, ignore,
    one<'='>, ignore,
    expr
> {};
struct if_statement: if_must<
    TAO_PEGTL_KEYWORD("if"), ignore, expr, ignore, block,
    star<seq<TAO_PEGTL_KEYWORD("elif"), ignore, expr, ignore, block>>,
    opt<seq<TAO_PEGTL_KEYWORD("else"), ignore, block>>
> {};
struct for_loop: if_must<
    TAO_PEGTL_KEYWORD("for"), ignore,
    variable_def, ignore, one<';'>, ignore,
    expr, ignore, one<';'>, ignore,
    assignment, ignore,
    block
> {};
struct while_loop: if_must<
    TAO_PEGTL_KEYWORD("while"), ignore,
    expr, ignore,
    block
> {};
struct switch_statement: if_must<
    TAO_PEGTL_KEYWORD("switch"), ignore,
    expr, ignore,
    one<'{'>, ignore,
    star<if_must<
        TAO_PEGTL_KEYWORD("case"), whitespace,
        list<literal_integer, one<','>, whitespace>, ignore,
        block
    >>,
    one<'}'>
> {};
struct function_def: if_must<
    seq<
    opt<TAO_PEGTL_KEYWORD("export")>, ignore,
    TAO_PEGTL_KEYWORD("fn"), whitespace
    >,
    opt<primitive_type>, whitespace,
    identifier, ignore,
    one<'('>,
    star<if_must<named_type, whitespace, identifier>, one<','>, ignore>,
    opt<if_must<named_type, whitespace, identifier>>, ignore,
    one<')'>, ignore,
    block
> {};
struct top_level_statement: sor<
    function_def,
    type_def,
    variable_def
> {};
struct program: must<
    star<top_level_statement, one<';'>>,
    eof
> {};
struct s_return: seq<
    TAO_PEGTL_KEYWORD("return"), ignore,
    opt<expr>
> {};
struct s_break: TAO_PEGTL_KEYWORD("break") {};
struct s_continue: TAO_PEGTL_KEYWORD("continue") {};
struct statement: sor<
    variable_def,
    function_def,
    type_def,
    assignment,
    s_return,
    s_break,
    s_continue,
    expr
> {};
struct block: seq<
    one<'{'>, ignore,
    star<statement, ignore, one<';'>, ignore>,
    one<'}'>, ignore
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

ast::program alt_parser_context::parse_program() {
    ast::program p {};
    file_input in(drv.filename);
    try {
        parse<program>(in);
    } catch (tao::pegtl::parse_error& e) {
        const auto p = e.positions().front();
        std::cerr << e.what() << std::endl
            << in.line_at(p) << '\n'
            << std::setw(p.column) << '^' << std::endl;
        exit(1);
    }
    return p;
}
void alt_parser_context::test_grammar() {
    if (analyze<program>() != 0) {
        std::cerr << "cycles without progress detected!\n";
        exit(1);
    }
}
