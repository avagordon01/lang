#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>

namespace pegtl = tao::pegtl;

#include "alt-parser.hh"
using grammar = program;

int main() {
    if (pegtl::analyze<grammar>() != 0) {
        std::cerr << "cycles without progress detected!\n";
        return 1;
    }
    return 0;
}
