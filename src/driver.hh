#pragma once

#include "tokens.hh"
#include "parser.hh"
#define YY_DECL token_type yylex(parser_context& pc)
YY_DECL;
extern FILE *yyin;
