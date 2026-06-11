#include "cregex/regex.h"

#include "cregex/ast.h"
#include "cregex/compiler.h"
#include "cregex/nfa.h"
#include "cregex/parser.h"
#include "cregex/vm.h"

#include <stdlib.h>

struct Regex {
    NfaProgram program;
};

static void regex_error_clear(RegexError *error)
{
    if (error == NULL) {
        return;
    }

    error->position = REGEX_ERROR_NO_POSITION;
    error->message = NULL;
}

static void regex_error_set(
    RegexError *error,
    size_t position,
    const char *message
)
{
    if (error == NULL) {
        return;
    }

    error->position = position;
    error->message = message;
}

Regex *regex_compile(
    const char *pattern,
    RegexError *error
)
{
    ParserError parser_error;
    CompilerError compiler_error;
    AstNode *root;
    Regex *regex;

    regex_error_clear(error);

    if (pattern == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "regex pattern must not be null"
        );

        return NULL;
    }

    root = parser_parse(
        pattern,
        &parser_error
    );

    if (root == NULL) {
        regex_error_set(
            error,
            parser_error.position,
            parser_error.message != NULL
                ? parser_error.message
                : "failed to parse regex pattern"
        );

        return NULL;
    }

    regex = (Regex *) malloc(sizeof(Regex));

    if (regex == NULL) {
        ast_free(root);

        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "failed to allocate compiled regex"
        );

        return NULL;
    }

    nfa_program_init(&regex->program);

    if (
        !compiler_compile(
            root,
            &regex->program,
            &compiler_error
        )
    ) {
        regex_error_set(
            error,
            compiler_error.position,
            compiler_error.message != NULL
                ? compiler_error.message
                : "failed to compile regex pattern"
        );

        nfa_program_free(&regex->program);
        free(regex);
        ast_free(root);

        return NULL;
    }

    ast_free(root);

    return regex;
}

int regex_full_match(
    const Regex *regex,
    const char *text,
    int *matched,
    RegexError *error
)
{
    VmError vm_error;

    regex_error_clear(error);

    if (matched != NULL) {
        *matched = 0;
    }

    if (regex == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "compiled regex must not be null"
        );

        return 0;
    }

    if (text == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "input text must not be null"
        );

        return 0;
    }

    if (matched == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "match-result pointer must not be null"
        );

        return 0;
    }

    if (
        !nfa_vm_full_match(
            &regex->program,
            text,
            matched,
            &vm_error
        )
    ) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            vm_error.message != NULL
                ? vm_error.message
                : "regex VM failed during full match"
        );

        return 0;
    }

    return 1;
}

int regex_search(
    const Regex *regex,
    const char *text,
    int *matched,
    RegexMatch *match,
    RegexError *error
)
{
    VmError vm_error;
    NfaMatch nfa_match;

    regex_error_clear(error);

    if (matched != NULL) {
        *matched = 0;
    }

    if (match != NULL) {
        match->start = 0U;
        match->end = 0U;
    }

    if (regex == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "compiled regex must not be null"
        );

        return 0;
    }

    if (text == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "input text must not be null"
        );

        return 0;
    }

    if (matched == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "match-result pointer must not be null"
        );

        return 0;
    }

    if (match == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "match-offset pointer must not be null"
        );

        return 0;
    }

    if (
        !nfa_vm_search(
            &regex->program,
            text,
            matched,
            &nfa_match,
            &vm_error
        )
    ) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            vm_error.message != NULL
                ? vm_error.message
                : "regex VM failed during search"
        );

        return 0;
    }

    if (*matched) {
        match->start = nfa_match.start;
        match->end = nfa_match.end;
    }

    return 1;
}

void regex_free(Regex *regex)
{
    if (regex == NULL) {
        return;
    }

    nfa_program_free(&regex->program);
    free(regex);
}