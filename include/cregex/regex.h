#ifndef CREGEX_REGEX_H
#define CREGEX_REGEX_H

#include <stddef.h>

#define REGEX_ERROR_NO_POSITION ((size_t) -1)

typedef struct Regex Regex;

typedef struct {
    size_t position;
    const char *message;
} RegexError;

typedef struct {
    size_t start;
    size_t end;
} RegexMatch;

Regex *regex_compile(
    const char *pattern,
    RegexError *error
);

/*
 * Requires the complete input string to match.
 *
 * Returns:
 *   1 if matching completed successfully
 *   0 if an invalid argument or VM error occurred
 *
 * `matched` receives 1 for a match and 0 otherwise.
 */
int regex_full_match(
    const Regex *regex,
    const char *text,
    int *matched,
    RegexError *error
);
/*
 * Returns:
 *   1 if searching completed successfully
 *   0 if an invalid argument or VM error occurred
 * `matched` receives 1 when a match is found and 0 otherwise.
 * When a match is found, `match` receives half-open byte offsets.
 */
int regex_search(
    const Regex *regex,
    const char *text,
    int *matched,
    RegexMatch *match,
    RegexError *error
);

void regex_free(Regex *regex);

#endif