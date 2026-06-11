#ifndef CREGEX_VM_H
#define CREGEX_VM_H

#include "cregex/nfa.h"

#include <stddef.h>

typedef struct {
    const char *message;
} VmError;

typedef struct {
    /*
     * Match covers:
     *
     *     text[start ... end - 1]
     *
     * `end` is exclusive.
     */
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
 * Returns:
 *   1 if execution completed successfully
 *   0 if an internal VM error occurred
 *
 * `matched` receives:
 *   1 if a match was found
 *   0 otherwise
 */
int nfa_vm_search(
    const NfaProgram *program,
    const char *text,
    int *matched,
    NfaMatch *match,
    VmError *error
);

#endif