#include "cregex/ast.h"
#include "cregex/compiler.h"
#include "cregex/nfa.h"
#include "cregex/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    ParserError parser_error;
    CompilerError compiler_error;
    AstNode *root;
    NfaProgram program;

    if (argc != 2) {
        fprintf(
            stderr,
            "Usage: %s <regex-pattern>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    root = parser_parse(
        argv[1],
        &parser_error
    );

    if (root == NULL) {
        fprintf(
            stderr,
            "Regex error at position %lu: %s\n",
            (unsigned long) parser_error.position,
            parser_error.message != NULL
                ? parser_error.message
                : "unknown parser error"
        );

        return EXIT_FAILURE;
    }

    nfa_program_init(&program);

    if (
        !compiler_compile(
            root,
            &program,
            &compiler_error
        )
    ) {
        fprintf(
            stderr,
            "Compiler error at position %lu: %s\n",
            (unsigned long) compiler_error.position,
            compiler_error.message != NULL
                ? compiler_error.message
                : "unknown compiler error"
        );

        ast_free(root);

        return EXIT_FAILURE;
    }

    puts("AST:");
    ast_print(stdout, root);

    puts("\nNFA:");
    nfa_program_print(stdout, &program);

    nfa_program_free(&program);
    ast_free(root);

    return EXIT_SUCCESS;
}