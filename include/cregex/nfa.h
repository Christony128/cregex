#ifndef CREGEX_NFA_H
#define CREGEX_NFA_H

#include "cregex/charset.h"

#include <stddef.h>
#include <stdio.h>

#define NFA_INVALID_STATE ((size_t) -1)

typedef enum {
    NFA_STATE_CHAR,
    NFA_STATE_ANY,
    NFA_STATE_CLASS,
    NFA_STATE_SPLIT,
    NFA_STATE_ASSERT_START,
    NFA_STATE_ASSERT_END,
    NFA_STATE_MATCH
} NfaStateType;

typedef struct {
    NfaStateType type;
    size_t out1;
    size_t out2;
    unsigned char character;
    CharSet character_class;
} NfaState;

typedef struct {
    NfaState *states;
    size_t count;
    size_t capacity;
//Compiled program entry point
    size_t start;
} NfaProgram;

const char *nfa_state_type_name(NfaStateType type);

void nfa_program_init(NfaProgram *program);

void nfa_program_free(NfaProgram *program);

size_t nfa_program_add_char(
    NfaProgram *program,
    unsigned char character
);

size_t nfa_program_add_any(NfaProgram *program);

size_t nfa_program_add_class(
    NfaProgram *program,
    const CharSet *character_class
);

size_t nfa_program_add_split(NfaProgram *program);

size_t nfa_program_add_assert_start(NfaProgram *program);

size_t nfa_program_add_assert_end(NfaProgram *program);

size_t nfa_program_add_match(NfaProgram *program);

int nfa_program_patch_out1(
    NfaProgram *program,
    size_t state_index,
    size_t destination
);

int nfa_program_patch_out2(
    NfaProgram *program,
    size_t state_index,
    size_t destination
);

int nfa_program_set_start(
    NfaProgram *program,
    size_t state_index
);

void nfa_program_print(
    FILE *output,
    const NfaProgram *program
);

#endif