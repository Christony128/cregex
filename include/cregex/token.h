#ifndef CREGEX_TOKEN_H
#define CREGEX_TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_LITERAL,
    TOKEN_DOT,
    TOKEN_PIPE,
    TOKEN_STAR,
    TOKEN_PLUS,
    TOKEN_QUESTION,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_CARET,
    TOKEN_DOLLAR,
    TOKEN_DIGIT_CLASS,
    TOKEN_WORD_CLASS,
    TOKEN_SPACE_CLASS,
    TOKEN_END,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    unsigned char character;
    size_t position;
    const char *error_message;
} Token;

const char *token_type_name(TokenType type);

#endif