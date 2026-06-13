#ifndef CREGEX_AST_H
#define CREGEX_AST_H

#include "cregex/charset.h"

#include <stddef.h>
#include <stdio.h>

#define CREGEX_REPEAT_UNBOUNDED ((size_t) -1)
#define CREGEX_MAX_REPEAT_COUNT ((size_t) 1000U)

typedef enum {
    AST_EMPTY,
    AST_LITERAL,
    AST_DOT,
    AST_CHARACTER_CLASS,

    AST_CONCATENATION,
    AST_ALTERNATION,

    AST_STAR,
    AST_PLUS,
    AST_OPTIONAL,
    AST_REPEAT,

    AST_ASSERT_START,
    AST_ASSERT_END
} AstType;

typedef struct AstNode AstNode;

struct AstNode {
    AstType type;
    size_t position;

    union {
        unsigned char character;
        CharSet character_class;

        struct {
            AstNode *left;
            AstNode *right;
        } binary;

        AstNode *child;

        struct {
            AstNode *child;
            size_t minimum;
            size_t maximum;
        } repetition;
    } value;
};

const char *ast_type_name(AstType type);

AstNode *ast_make_empty(size_t position);

AstNode *ast_make_literal(
    unsigned char character,
    size_t position
);

AstNode *ast_make_dot(size_t position);

AstNode *ast_make_character_class(
    const CharSet *character_class,
    size_t position
);

AstNode *ast_make_assertion(
    AstType type,
    size_t position
);

AstNode *ast_make_unary(
    AstType type,
    AstNode *child,
    size_t position
);

AstNode *ast_make_repeat(
    AstNode *child,
    size_t minimum,
    size_t maximum,
    size_t position
);

AstNode *ast_make_binary(
    AstType type,
    AstNode *left,
    AstNode *right,
    size_t position
);

void ast_print(
    FILE *output,
    const AstNode *node
);

void ast_free(AstNode *node);

#endif