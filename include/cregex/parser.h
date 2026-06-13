#ifndef CREGEX_PARSER_H
#define CREGEX_PARSER_H

#include "cregex/ast.h"
#include "cregex/parse_tree.h"

#include <stddef.h>

typedef struct {
    size_t position;
    const char *message;
} ParserError;

ParseTreeNode *parser_parse_tree(
    const char *pattern,
    ParserError *error
);

AstNode *parser_parse(
    const char *pattern,
    ParserError *error
);

#endif
