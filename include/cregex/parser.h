#ifndef CREGEX_PARSER_H
#define CREGEX_PARSER_H

#include "cregex/ast.h"

#include <stddef.h>

typedef struct {
    size_t position;
    const char *message;
} ParserError;

AstNode *parser_parse(
    const char *pattern,
    ParserError *error
);

#endif