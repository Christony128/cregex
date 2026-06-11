#include "cregex/ast.h"

#include <ctype.h>
#include <stdlib.h>

static AstNode *ast_allocate(
    AstType type,
    size_t position
)
{
    AstNode *node;

    node = (AstNode *) calloc(1, sizeof(AstNode));

    if (node == NULL) {
        return NULL;
    }

    node->type = type;
    node->position = position;

    return node;
}

const char *ast_type_name(AstType type)
{
    switch (type) {
        case AST_EMPTY:
            return "EMPTY";

        case AST_LITERAL:
            return "LITERAL";

        case AST_DOT:
            return "DOT";

        case AST_CHARACTER_CLASS:
            return "CHARACTER_CLASS";

        case AST_CONCATENATION:
            return "CONCATENATION";

        case AST_ALTERNATION:
            return "ALTERNATION";

        case AST_STAR:
            return "STAR";

        case AST_PLUS:
            return "PLUS";

        case AST_OPTIONAL:
            return "OPTIONAL";

        case AST_ASSERT_START:
            return "ASSERT_START";

        case AST_ASSERT_END:
            return "ASSERT_END";
    }

    return "UNKNOWN";
}

AstNode *ast_make_empty(size_t position)
{
    return ast_allocate(AST_EMPTY, position);
}

AstNode *ast_make_literal(
    unsigned char character,
    size_t position
)
{
    AstNode *node;

    node = ast_allocate(AST_LITERAL, position);

    if (node == NULL) {
        return NULL;
    }

    node->value.character = character;

    return node;
}

AstNode *ast_make_dot(size_t position)
{
    return ast_allocate(AST_DOT, position);
}

AstNode *ast_make_character_class(
    const CharSet *character_class,
    size_t position
)
{
    AstNode *node;

    if (character_class == NULL) {
        return NULL;
    }

    node = ast_allocate(
        AST_CHARACTER_CLASS,
        position
    );

    if (node == NULL) {
        return NULL;
    }

    node->value.character_class =
        *character_class;

    return node;
}

AstNode *ast_make_assertion(
    AstType type,
    size_t position
)
{
    if (
        type != AST_ASSERT_START &&
        type != AST_ASSERT_END
    ) {
        return NULL;
    }

    return ast_allocate(type, position);
}

AstNode *ast_make_unary(
    AstType type,
    AstNode *child,
    size_t position
)
{
    AstNode *node;

    if (
        type != AST_STAR &&
        type != AST_PLUS &&
        type != AST_OPTIONAL
    ) {
        return NULL;
    }

    if (child == NULL) {
        return NULL;
    }

    node = ast_allocate(type, position);

    if (node == NULL) {
        return NULL;
    }

    node->value.child = child;

    return node;
}

AstNode *ast_make_binary(
    AstType type,
    AstNode *left,
    AstNode *right,
    size_t position
)
{
    AstNode *node;

    if (
        type != AST_CONCATENATION &&
        type != AST_ALTERNATION
    ) {
        return NULL;
    }

    if (left == NULL || right == NULL) {
        return NULL;
    }

    node = ast_allocate(type, position);

    if (node == NULL) {
        return NULL;
    }

    node->value.binary.left = left;
    node->value.binary.right = right;

    return node;
}

static void ast_print_indent(
    FILE *output,
    unsigned int depth
)
{
    unsigned int index;

    for (index = 0; index < depth; index++) {
        fputs("  ", output);
    }
}

static void ast_print_recursive(
    FILE *output,
    const AstNode *node,
    unsigned int depth
)
{
    if (node == NULL) {
        ast_print_indent(output, depth);
        fputs("<null>\n", output);
        return;
    }

    ast_print_indent(output, depth);

    switch (node->type) {
        case AST_LITERAL:
            if (isprint((int) node->value.character)) {
                fprintf(
                    output,
                    "LITERAL '%c'\n",
                    node->value.character
                );
            } else {
                fprintf(
                    output,
                    "LITERAL 0x%02X\n",
                    (unsigned int) node->value.character
                );
            }

            return;

        case AST_CHARACTER_CLASS:
            fprintf(
                output,
                "CHARACTER_CLASS%s\n",
                node->value.character_class.negated
                    ? " (negated)"
                    : ""
            );

            return;

        case AST_CONCATENATION:
        case AST_ALTERNATION:
            fprintf(
                output,
                "%s\n",
                ast_type_name(node->type)
            );

            ast_print_recursive(
                output,
                node->value.binary.left,
                depth + 1U
            );

            ast_print_recursive(
                output,
                node->value.binary.right,
                depth + 1U
            );

            return;

        case AST_STAR:
        case AST_PLUS:
        case AST_OPTIONAL:
            fprintf(
                output,
                "%s\n",
                ast_type_name(node->type)
            );

            ast_print_recursive(
                output,
                node->value.child,
                depth + 1U
            );

            return;

        case AST_EMPTY:
        case AST_DOT:
        case AST_ASSERT_START:
        case AST_ASSERT_END:
            fprintf(
                output,
                "%s\n",
                ast_type_name(node->type)
            );

            return;
    }

    fputs("UNKNOWN\n", output);
}

void ast_print(
    FILE *output,
    const AstNode *node
)
{
    if (output == NULL) {
        return;
    }

    ast_print_recursive(output, node, 0);
}

void ast_free(AstNode *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case AST_CONCATENATION:
        case AST_ALTERNATION:
            ast_free(node->value.binary.left);
            ast_free(node->value.binary.right);
            break;

        case AST_STAR:
        case AST_PLUS:
        case AST_OPTIONAL:
            ast_free(node->value.child);
            break;

        case AST_EMPTY:
        case AST_LITERAL:
        case AST_DOT:
        case AST_CHARACTER_CLASS:
        case AST_ASSERT_START:
        case AST_ASSERT_END:
            break;
    }

    free(node);
}