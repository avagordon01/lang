#include "driver.hh"
#include "alt-parser.hh"

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
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
    >,
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
    star<top_level_statement, ignore, one<';'>, ignore>,
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
namespace {
    enum class precedence: int {
    };
    enum class associativity {
        left, right,
    };
    struct op {
        precedence prec;
        associativity assoc;
    };
    const static std::map<std::string, op> ops = {
        {"||", {precedence{0}, associativity::left}},
        {"&&", {precedence{1}, associativity::left}},

        {"==", {precedence{2}, associativity::left}},
        {"!=", {precedence{2}, associativity::left}},
        {">",  {precedence{2}, associativity::left}},
        {">=", {precedence{2}, associativity::left}},
        {"<",  {precedence{2}, associativity::left}},
        {"<=", {precedence{2}, associativity::left}},

        {"|",  {precedence{3}, associativity::left}},
        {"^",  {precedence{4}, associativity::left}},
        {"&",  {precedence{5}, associativity::left}},
        {">>", {precedence{6}, associativity::left}},
        {"<<", {precedence{6}, associativity::left}},

        {"+", {precedence{7}, associativity::left}},
        {"-", {precedence{7}, associativity::left}},

        {"*", {precedence{8}, associativity::left}},
        {"/", {precedence{8}, associativity::left}},
        {"%", {precedence{8}, associativity::left}},

        {"~", {precedence{9}, associativity::right}},
        {"!", {precedence{9}, associativity::right}},
    };
    constexpr size_t max_operator_length = 2;
}
struct operators {
    using rule_t = operators;
    using subs_t = empty_list;

    template<typename ParseInput>
    static bool match(ParseInput& in) {
        if (!in.empty()) {
            std::string buf {};
            for (size_t i = 0; i < max_operator_length; i++) {
                buf += in.peek_char(i);
            }
            while (!buf.empty()) {
                if (ops.find(buf) != ops.end()) {
                    in.bump(buf.size());
                    return true;
                }
                buf.resize(buf.size() - 1);
            }
        }
        return false;
    }

    /*
    template<typename ParseInput>
    static bool _match(ParseInput& in, std::string buf) {
        if (in.size(buf.size() + 1) > buf.size()) {
            buf += in.peek_char(buf.size());
            const auto i = std::find(std::begin(ops), std::end(ops), buf);
            if (i != std::end(ops)) {
                return true;
            }
            if (i->str == buf) {
                in.bump(buf.size());
                return true;
            }
        }
        return false;
    }
    */
};
namespace TAO_PEGTL_NAMESPACE {
template<typename Name>
struct analyze_traits<Name, operators>: analyze_any_traits<> {};
}
struct exp_atom: sor<
    literal,
    if_statement,
    switch_statement,
    for_loop,
    while_loop,
    function_call,
    accessor,
    seq<one<'('>, expr, one<')'>>,
    operators
> {};
struct expr: plus<exp_atom, ignore> {};


struct transformer: std::true_type {
    static void transform(std::unique_ptr<parse_tree::node>& node) {
        if (!node) {
            return;
        }
        if (node->type == "block") {
            std::cerr << "visiting block node!" << std::endl;
        } else {
            std::cerr << "visiting other node!" << std::endl;
        }
    }
};

template<typename Rule>
using selector = parse_tree::selector<
    Rule,
    parse_tree::apply<transformer>::on<
        block
    >,
    parse_tree::remove_content::on<
        program,
        top_level_statement
    >,
    parse_tree::store_content::on<
        function_def,
        variable_def,
        type_def,
        statement,
        expr
    >
>;

void visitor(std::unique_ptr<parse_tree::node>& node) {
    if (!node) {
        return;
    }
    if (node->type == "block") {
        std::cerr << "visiting block node!" << std::endl;
    } else {
        std::cerr << "visiting other node!" << std::endl;
    }
    for (std::unique_ptr<parse_tree::node>& child_node: node->children) {
        visitor(child_node);
    }
}

ast::program alt_parser_context::parse_program() {
    ast::program p {};
    file_input in(drv.filename);
    try {
        std::unique_ptr<parse_tree::node> root = parse_tree::parse<program, selector>(in);
        if (root) {
            parse_tree::print_dot(std::cout, *root);
        }
        visitor(root);
    } catch (parse_error& e) {
        const auto p = e.positions().front();
        std::cerr << e.what() << std::endl
            << in.line_at(p) << std::endl
            << std::setw(p.column) << '^' << std::endl;
        exit(1);
    }
    return p;
}
void alt_parser_context::test_grammar() {
    if (analyze<program>() != 0) {
        std::cerr << "cycles without progress detected!" << std::endl;
        exit(1);
    }
}
