#include "cregex/nfa.h"

#include <stdlib.h>

#define NFA_INITIAL_CAPACITY 16U

static NfaState nfa_make_state(NfaStateType type)
{
    NfaState state;

    state.type = type;
    state.out1 = NFA_INVALID_STATE;
    state.out2 = NFA_INVALID_STATE;
    state.character = '\0';

    charset_clear(&state.character_class);

    return state;
}

static int nfa_program_grow(NfaProgram *program)
{
    NfaState *new_states;
    size_t new_capacity;

    if (program == NULL) {
        return 0;
    }

    if (program->capacity == 0) {
        new_capacity = NFA_INITIAL_CAPACITY;
    } else {
        new_capacity = program->capacity * 2U;
    }

    new_states = (NfaState *) realloc(
        program->states,
        new_capacity * sizeof(NfaState)
    );

    if (new_states == NULL) {
        return 0;
    }

    program->states = new_states;
    program->capacity = new_capacity;

    return 1;
}

static size_t nfa_program_append(
    NfaProgram *program,
    NfaState state
)
{
    size_t index;

    if (program == NULL) {
        return NFA_INVALID_STATE;
    }

    if (program->count == program->capacity) {
        if (!nfa_program_grow(program)) {
            return NFA_INVALID_STATE;
        }
    }

    index = program->count;
    program->states[index] = state;
    program->count++;

    return index;
}

const char *nfa_state_type_name(NfaStateType type)
{
    switch (type) {
        case NFA_STATE_CHAR:
            return "CHAR";

        case NFA_STATE_ANY:
            return "ANY";

        case NFA_STATE_CLASS:
            return "CLASS";

        case NFA_STATE_SPLIT:
            return "SPLIT";

        case NFA_STATE_ASSERT_START:
            return "ASSERT_START";

        case NFA_STATE_ASSERT_END:
            return "ASSERT_END";

        case NFA_STATE_MATCH:
            return "MATCH";
    }

    return "UNKNOWN";
}

void nfa_program_init(NfaProgram *program)
{
    if (program == NULL) {
        return;
    }

    program->states = NULL;
    program->count = 0;
    program->capacity = 0;
    program->start = NFA_INVALID_STATE;
}

void nfa_program_free(NfaProgram *program)
{
    if (program == NULL) {
        return;
    }

    free(program->states);

    program->states = NULL;
    program->count = 0;
    program->capacity = 0;
    program->start = NFA_INVALID_STATE;
}

size_t nfa_program_add_char(
    NfaProgram *program,
    unsigned char character
)
{
    NfaState state;

    state = nfa_make_state(NFA_STATE_CHAR);
    state.character = character;

    return nfa_program_append(program, state);
}

size_t nfa_program_add_any(NfaProgram *program)
{
    return nfa_program_append(
        program,
        nfa_make_state(NFA_STATE_ANY)
    );
}

size_t nfa_program_add_class(
    NfaProgram *program,
    const CharSet *character_class
)
{
    NfaState state;

    if (character_class == NULL) {
        return NFA_INVALID_STATE;
    }

    state = nfa_make_state(NFA_STATE_CLASS);
    state.character_class = *character_class;

    return nfa_program_append(program, state);
}

size_t nfa_program_add_split(NfaProgram *program)
{
    return nfa_program_append(
        program,
        nfa_make_state(NFA_STATE_SPLIT)
    );
}

size_t nfa_program_add_assert_start(NfaProgram *program)
{
    return nfa_program_append(
        program,
        nfa_make_state(NFA_STATE_ASSERT_START)
    );
}

size_t nfa_program_add_assert_end(NfaProgram *program)
{
    return nfa_program_append(
        program,
        nfa_make_state(NFA_STATE_ASSERT_END)
    );
}

size_t nfa_program_add_match(NfaProgram *program)
{
    return nfa_program_append(
        program,
        nfa_make_state(NFA_STATE_MATCH)
    );
}

static int nfa_valid_destination(
    const NfaProgram *program,
    size_t destination
)
{
    if (destination == NFA_INVALID_STATE) {
        return 1;
    }

    return destination < program->count;
}

int nfa_program_patch_out1(
    NfaProgram *program,
    size_t state_index,
    size_t destination
)
{
    if (
        program == NULL ||
        state_index >= program->count ||
        !nfa_valid_destination(program, destination)
    ) {
        return 0;
    }

    program->states[state_index].out1 = destination;

    return 1;
}

int nfa_program_patch_out2(
    NfaProgram *program,
    size_t state_index,
    size_t destination
)
{
    if (
        program == NULL ||
        state_index >= program->count ||
        !nfa_valid_destination(program, destination)
    ) {
        return 0;
    }

    program->states[state_index].out2 = destination;

    return 1;
}

int nfa_program_set_start(
    NfaProgram *program,
    size_t state_index
)
{
    if (
        program == NULL ||
        state_index >= program->count
    ) {
        return 0;
    }

    program->start = state_index;

    return 1;
}

static void nfa_print_destination(
    FILE *output,
    size_t destination
)
{
    if (destination == NFA_INVALID_STATE) {
        fputs("?", output);
    } else {
        fprintf(
            output,
            "%lu",
            (unsigned long) destination
        );
    }
}

void nfa_program_print(
    FILE *output,
    const NfaProgram *program
)
{
    size_t index;

    if (output == NULL || program == NULL) {
        return;
    }

    fprintf(
        output,
        "start: "
    );

    nfa_print_destination(output, program->start);
    fputc('\n', output);

    for (index = 0; index < program->count; index++) {
        const NfaState *state;

        state = &program->states[index];

        fprintf(
            output,
            "%lu: %-13s ",
            (unsigned long) index,
            nfa_state_type_name(state->type)
        );

        switch (state->type) {
            case NFA_STATE_CHAR:
                fprintf(
                    output,
                    "'%c' -> ",
                    state->character
                );
                nfa_print_destination(output, state->out1);
                break;

            case NFA_STATE_ANY:
            case NFA_STATE_CLASS:
            case NFA_STATE_ASSERT_START:
            case NFA_STATE_ASSERT_END:
                fputs("-> ", output);
                nfa_print_destination(output, state->out1);
                break;

            case NFA_STATE_SPLIT:
                fputs("-> ", output);
                nfa_print_destination(output, state->out1);
                fputs(", ", output);
                nfa_print_destination(output, state->out2);
                break;

            case NFA_STATE_MATCH:
                break;
        }

        fputc('\n', output);
    }
}