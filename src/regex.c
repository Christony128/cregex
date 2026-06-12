#include "cregex/regex.h"

#include "cregex/ast.h"
#include "cregex/compiler.h"
#include "cregex/nfa.h"
#include "cregex/parser.h"
#include "cregex/vm.h"

#include <stdlib.h>
#include <string.h>

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

static int regex_match_list_append(
    RegexMatchList *list,
    size_t *capacity,
    size_t start,
    size_t end,
    RegexError *error
)
{
    RegexMatch *resized_matches;
    size_t new_capacity;
    size_t maximum_size;

    if (list->count < *capacity) {
        list->matches[list->count].start = start;
        list->matches[list->count].end = end;
        list->count++;

        return 1;
    }

    maximum_size = (size_t) -1;

    if (*capacity == 0U) {
        new_capacity = 8U;
    } else {
        if (*capacity > maximum_size / 2U) {
            regex_error_set(
                error,
                REGEX_ERROR_NO_POSITION,
                "too many regex matches"
            );

            return 0;
        }

        new_capacity = *capacity * 2U;
    }

    if (
        new_capacity >
        maximum_size / sizeof(RegexMatch)
    ) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "regex match list is too large"
        );

        return 0;
    }

    resized_matches = (RegexMatch *) realloc(
        list->matches,
        new_capacity * sizeof(RegexMatch)
    );

    if (resized_matches == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "failed to allocate regex match list"
        );

        return 0;
    }

    list->matches = resized_matches;
    *capacity = new_capacity;

    list->matches[list->count].start = start;
    list->matches[list->count].end = end;
    list->count++;

    return 1;
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

int regex_find_all(
    const Regex *regex,
    const char *text,
    RegexMatchList *result,
    RegexError *error
)
{
    VmError vm_error;
    NfaMatch match;
    size_t capacity;
    size_t search_start;
    size_t text_length;
    int matched;

    regex_error_clear(error);

    if (result != NULL) {
        result->matches = NULL;
        result->count = 0U;
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

    if (result == NULL) {
        regex_error_set(
            error,
            REGEX_ERROR_NO_POSITION,
            "match-list pointer must not be null"
        );

        return 0;
    }

    capacity = 0U;
    search_start = 0U;
    text_length = strlen(text);

    while (search_start <= text_length) {
        if (
            !nfa_vm_search_from(
                &regex->program,
                text,
                search_start,
                &matched,
                &match,
                &vm_error
            )
        ) {
            regex_match_list_free(result);

            regex_error_set(
                error,
                REGEX_ERROR_NO_POSITION,
                vm_error.message != NULL
                    ? vm_error.message
                    : "regex VM failed during find-all"
            );

            return 0;
        }

        if (!matched) {
            break;
        }

        if (
            !regex_match_list_append(
                result,
                &capacity,
                match.start,
                match.end,
                error
            )
        ) {
            regex_match_list_free(result);
            return 0;
        }

        if (match.end > match.start) {
            search_start = match.end;
        } else if (match.end < text_length) {
            search_start = match.end + 1U;
        } else {
            break;
        }
    }

    return 1;
}

void regex_match_list_free(RegexMatchList *list)
{
    if (list == NULL) {
        return;
    }

    free(list->matches);
    list->matches = NULL;
    list->count = 0U;
}

void regex_free(Regex *regex)
{
    if (regex == NULL) {
        return;
    }

    nfa_program_free(&regex->program);
    free(regex);
}