#include "cregex/ast.h"
#include "cregex/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    ParserError error;
    AstNode *root;

    if (argc != 2) {
        fprintf(
            stderr,
            "Usage: %s <regex-pattern>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    root = parser_parse(argv[1], &error);

    if (root == NULL) {
        fprintf(
            stderr,
            "Regex error at position %lu: %s\n",
            (unsigned long) error.position,
            error.message != NULL
                ? error.message
                : "unknown parser error"
        );

        return EXIT_FAILURE;
    }

    ast_print(stdout, root);
    ast_free(root);

    return EXIT_SUCCESS;
}