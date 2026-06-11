#include "cregex/ast.h"
#include "cregex/compiler.h"
#include "cregex/nfa.h"
#include "cregex/parser.h"
#include "cregex/vm.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    ParserError parser_error;
    CompilerError compiler_error;
    AstNode *root;
    NfaProgram program;

    if (argc != 2 && argc != 3) {
        fprintf(
            stderr,
            "Usage:\n"
            "  %s <regex-pattern>\n"
            "  %s <regex-pattern> <text>\n",
            argv[0],
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

    if (argc == 2) {
        puts("AST:");
        ast_print(stdout, root);

        puts("\nNFA:");
        nfa_program_print(stdout, &program);
    } else {
        VmError vm_error;
        int matched;

        if (
            !nfa_vm_full_match(
                &program,
                argv[2],
                &matched,
                &vm_error
            )
        ) {
            fprintf(
                stderr,
                "VM error: %s\n",
                vm_error.message != NULL
                    ? vm_error.message
                    : "unknown VM error"
            );

            nfa_program_free(&program);
            ast_free(root);

            return EXIT_FAILURE;
        }

        if (matched) {
            puts("MATCH");
        } else {
            puts("NO MATCH");
        }
    }

    nfa_program_free(&program);
    ast_free(root);

    return EXIT_SUCCESS;
}