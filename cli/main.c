#include "cregex/ast.h"
#include "cregex/compiler.h"
#include "cregex/nfa.h"
#include "cregex/parser.h"
#include "cregex/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    CLI_MODE_INSPECT,
    CLI_MODE_FULL_MATCH,
    CLI_MODE_SEARCH,
    CLI_MODE_FIND_ALL,
    CLI_MODE_PARSE_TREE
} CliMode;

static void print_usage(const char *program_name)
{
    fprintf(
        stderr,
        "Usage:\n"
        "  %s <pattern>\n"
        "  %s <pattern> <text>\n"
        "  %s --full <pattern> <text>\n"
        "  %s --search <pattern> <text>\n"
        "  %s --find-all <pattern> <text>\n"
        "  %s --tree <pattern>\n",
        program_name,
        program_name,
        program_name,
        program_name,
        program_name,
        program_name
    );
}

static void print_match_value(
    const char *text,
    const NfaMatch *match
)
{
    size_t length;

    length = match->end - match->start;

    printf(
        "MATCH start=%lu end=%lu value=\"",
        (unsigned long) match->start,
        (unsigned long) match->end
    );

    fwrite(
        text + match->start,
        sizeof(char),
        length,
        stdout
    );

    puts("\"");
}

static void print_indexed_match_value(
    size_t index,
    const char *text,
    const NfaMatch *match
)
{
    size_t length;

    length = match->end - match->start;

    printf(
        "MATCH %lu start=%lu end=%lu value=\"",
        (unsigned long) index,
        (unsigned long) match->start,
        (unsigned long) match->end
    );

    fwrite(
        text + match->start,
        sizeof(char),
        length,
        stdout
    );

    puts("\"");
}

int main(int argc, char **argv)
{
    CliMode mode;
    const char *pattern;
    const char *text;

    ParserError parser_error;
    CompilerError compiler_error;

    AstNode *root;
    NfaProgram program;

    text = NULL;

    if (
        argc == 3 &&
        strcmp(argv[1], "--tree") == 0
    ) {
        mode = CLI_MODE_PARSE_TREE;
        pattern = argv[2];
    } else if (argc == 2) {
        mode = CLI_MODE_INSPECT;
        pattern = argv[1];
    } else if (argc == 3) {
        mode = CLI_MODE_FULL_MATCH;
        pattern = argv[1];
        text = argv[2];
    } else if (
        argc == 4 &&
        strcmp(argv[1], "--full") == 0
    ) {
        mode = CLI_MODE_FULL_MATCH;
        pattern = argv[2];
        text = argv[3];
    } else if (
        argc == 4 &&
        strcmp(argv[1], "--search") == 0
    ) {
        mode = CLI_MODE_SEARCH;
        pattern = argv[2];
        text = argv[3];
    } else if (
        argc == 4 &&
        strcmp(argv[1], "--find-all") == 0
    ) {
        mode = CLI_MODE_FIND_ALL;
        pattern = argv[2];
        text = argv[3];
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (mode == CLI_MODE_PARSE_TREE) {
        ParseTreeNode *tree;

        tree = parser_parse_tree(
            pattern,
            &parser_error
        );

        if (tree == NULL) {
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

        parse_tree_print(stdout, tree);
        parse_tree_free(tree);

        return EXIT_SUCCESS;
    }

    root = parser_parse(
        pattern,
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

    if (mode == CLI_MODE_INSPECT) {
        puts("AST:");
        ast_print(stdout, root);

        puts("\nNFA:");
        nfa_program_print(stdout, &program);
    } else if (mode == CLI_MODE_FULL_MATCH) {
        VmError vm_error;
        int matched;

        if (
            !nfa_vm_full_match(
                &program,
                text,
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

        puts(matched ? "MATCH" : "NO MATCH");
    } else if (mode == CLI_MODE_SEARCH) {
        VmError vm_error;
        NfaMatch match;
        int matched;

        if (
            !nfa_vm_search(
                &program,
                text,
                &matched,
                &match,
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
            print_match_value(text, &match);
        } else {
            puts("NO MATCH");
        }
    } else {
        VmError vm_error;
        NfaMatch match;
        size_t match_index;
        size_t search_start;
        size_t text_length;
        int matched;

        match_index = 0U;
        search_start = 0U;
        text_length = strlen(text);

        while (search_start <= text_length) {
            if (
                !nfa_vm_search_from(
                    &program,
                    text,
                    search_start,
                    &matched,
                    &match,
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

            if (!matched) {
                break;
            }

            print_indexed_match_value(
                match_index,
                text,
                &match
            );

            match_index++;

            if (match.end > match.start) {
                search_start = match.end;
            } else if (match.end < text_length) {
                search_start = match.end + 1U;
            } else {
                break;
            }
        }

        if (match_index == 0U) {
            puts("NO MATCH");
        }
    }

    nfa_program_free(&program);
    ast_free(root);

    return EXIT_SUCCESS;
}