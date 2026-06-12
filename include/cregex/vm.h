#ifndef CREGEX_VM_H
#define CREGEX_VM_H

#include "cregex/nfa.h"

#include <stddef.h>

typedef struct {
    const char *message;
} VmError;

typedef struct {
    size_t start;
    size_t end;
} NfaMatch;

/*
 * Requires the complete input string to match.
 *
 * Returns:
 *   1 if execution completed successfully
 *   0 if an internal VM error occurred
 *
 * `matched` receives:
 *   1 if the complete input matched
 *   0 otherwise
 */
int nfa_vm_full_match(
    const NfaProgram *program,
    const char *text,
    int *matched,
    VmError *error
);

/*
 * Finds the leftmost-longest match anywhere in `text`.
 */
int nfa_vm_search(
    const NfaProgram *program,
    const char *text,
    int *matched,
    NfaMatch *match,
    VmError *error
);

/*
 * Finds the leftmost-longest match whose start position is greater than or
 * equal to `search_start`. Anchors are still evaluated against the original
 * complete input string.
 *
 * `search_start` may equal strlen(text), allowing a zero-length match at the
 * end of the input.
 */
int nfa_vm_search_from(
    const NfaProgram *program,
    const char *text,
    size_t search_start,
    int *matched,
    NfaMatch *match,
    VmError *error
);

#endif