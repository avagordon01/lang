#pragma once

#include "ast.hh"

struct driver;

struct alt_parser_context {
    driver& drv;
    alt_parser_context(driver& drv_) : drv(drv_) {
    };
    ast::program parse_program();
    void test_grammar();
};
