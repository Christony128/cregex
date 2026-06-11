#ifndef CREGEX_LEXER_H
#define CREGEX_LEXER_H

#include "cregex/token.h"

#include <stddef.h>

typedef struct {
    const char *pattern;
    size_t length;
    size_t position;
    int in_character_class;
} Lexer;

void lexer_init(
    Lexer *lexer,
    const char *pattern
);

Token lexer_next(Lexer *lexer);

Token lexer_peek(const Lexer *lexer);

#endif