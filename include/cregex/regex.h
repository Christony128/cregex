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

typedef struct {
    RegexMatch *matches;
    size_t count;
} RegexMatchList;
Regex *regex_compile(
    const char *pattern,
    RegexError *error
);

int regex_full_match(
    const Regex *regex,
    const char *text,
    int *matched,
    RegexError *error
);
int regex_search(
    const Regex *regex,
    const char *text,
    int *matched,
    RegexMatch *match,
    RegexError *error
);
int regex_find_all(
    const Regex *regex,
    const char *text,
    RegexMatchList *result,
    RegexError *error
);

void regex_match_list_free(RegexMatchList *list);
void regex_free(Regex *regex);

#endif