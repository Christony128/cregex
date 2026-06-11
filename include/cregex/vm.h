#ifndef CREGEX_VM_H
#define CREGEX_VM_H

#include "cregex/nfa.h"

typedef struct {
    const char *message;
} VmError;

/*
 * Matches the entire input string against a compiled NFA.
 *
 * Returns:
 *   1 if execution completed successfully
 *   0 if an internal/runtime error occurred
 *
 * The match result is written to `matched`:
 *   1 = input matches
 *   0 = input does not match
 */
int nfa_vm_full_match(
    const NfaProgram *program,
    const char *text,
    int *matched,
    VmError *error
);

#endif