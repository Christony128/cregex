#include "cregex/vm.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const NfaProgram *program;

    size_t *current;
    size_t current_count;

    size_t *next;
    size_t next_count;

    size_t *work;
    size_t work_count;

    unsigned int *seen;
    unsigned int generation;
} NfaVm;

static void vm_set_error(
    VmError *error,
    const char *message
)
{
    if (error != NULL) {
        error->message = message;
    }
}

static void vm_destroy(NfaVm *vm)
{
    if (vm == NULL) {
        return;
    }

    free(vm->current);
    free(vm->next);
    free(vm->work);
    free(vm->seen);

    vm->current = NULL;
    vm->next = NULL;
    vm->work = NULL;
    vm->seen = NULL;

    vm->current_count = 0U;
    vm->next_count = 0U;
    vm->work_count = 0U;
    vm->generation = 0U;
}

static int vm_initialize(
    NfaVm *vm,
    const NfaProgram *program,
    VmError *error
)
{
    size_t state_count;

    if (vm == NULL || program == NULL) {
        vm_set_error(
            error,
            "VM received a null program"
        );

        return 0;
    }

    vm->program = program;

    vm->current = NULL;
    vm->next = NULL;
    vm->work = NULL;
    vm->seen = NULL;

    vm->current_count = 0U;
    vm->next_count = 0U;
    vm->work_count = 0U;
    vm->generation = 0U;

    state_count = program->count;

    if (
        state_count == 0U ||
        program->start == NFA_INVALID_STATE ||
        program->start >= state_count
    ) {
        vm_set_error(
            error,
            "VM received an invalid NFA program"
        );

        return 0;
    }

    vm->current = (size_t *) malloc(
        state_count * sizeof(size_t)
    );

    vm->next = (size_t *) malloc(
        state_count * sizeof(size_t)
    );

    vm->work = (size_t *) malloc(
        state_count * sizeof(size_t)
    );

    vm->seen = (unsigned int *) calloc(
        state_count,
        sizeof(unsigned int)
    );

    if (
        vm->current == NULL ||
        vm->next == NULL ||
        vm->work == NULL ||
        vm->seen == NULL
    ) {
        vm_set_error(
            error,
            "failed to allocate VM state lists"
        );

        vm_destroy(vm);

        return 0;
    }

    return 1;
}

static void vm_begin_state_list(
    NfaVm *vm,
    size_t *count
)
{
    *count = 0U;

    if (vm->generation == UINT_MAX) {
        memset(
            vm->seen,
            0,
            vm->program->count *
                sizeof(unsigned int)
        );

        vm->generation = 1U;
    } else {
        vm->generation++;
    }
}

static int vm_push_unseen_state(
    NfaVm *vm,
    size_t state_index,
    VmError *error
)
{
    if (
        state_index == NFA_INVALID_STATE ||
        state_index >= vm->program->count
    ) {
        vm_set_error(
            error,
            "NFA contains an invalid transition"
        );

        return 0;
    }

    if (
        vm->seen[state_index] ==
        vm->generation
    ) {
        return 1;
    }

    vm->seen[state_index] = vm->generation;

    vm->work[vm->work_count] = state_index;
    vm->work_count++;

    return 1;
}

static int vm_add_closure(
    NfaVm *vm,
    size_t *list,
    size_t *list_count,
    size_t start_state,
    size_t input_position,
    size_t input_length,
    VmError *error
)
{
    vm->work_count = 0U;

    if (
        !vm_push_unseen_state(
            vm,
            start_state,
            error
        )
    ) {
        return 0;
    }

    while (vm->work_count > 0U) {
        size_t state_index;
        const NfaState *state;

        vm->work_count--;

        state_index =
            vm->work[vm->work_count];

        state =
            &vm->program->states[state_index];

        switch (state->type) {
            case NFA_STATE_SPLIT:
                if (
                    !vm_push_unseen_state(
                        vm,
                        state->out1,
                        error
                    ) ||
                    !vm_push_unseen_state(
                        vm,
                        state->out2,
                        error
                    )
                ) {
                    return 0;
                }

                break;

            case NFA_STATE_JUMP:
                if (
                    !vm_push_unseen_state(
                        vm,
                        state->out1,
                        error
                    )
                ) {
                    return 0;
                }

                break;

            case NFA_STATE_ASSERT_START:
                if (input_position == 0U) {
                    if (
                        !vm_push_unseen_state(
                            vm,
                            state->out1,
                            error
                        )
                    ) {
                        return 0;
                    }
                }

                break;

            case NFA_STATE_ASSERT_END:
                if (input_position == input_length) {
                    if (
                        !vm_push_unseen_state(
                            vm,
                            state->out1,
                            error
                        )
                    ) {
                        return 0;
                    }
                }

                break;

            case NFA_STATE_CHAR:
            case NFA_STATE_ANY:
            case NFA_STATE_CLASS:
            case NFA_STATE_MATCH:
                list[*list_count] = state_index;
                (*list_count)++;

                break;
        }
    }

    return 1;
}

static int state_matches_character(
    const NfaState *state,
    unsigned char character
)
{
    switch (state->type) {
        case NFA_STATE_CHAR:
            return state->character == character;

        case NFA_STATE_ANY:
            return 1;

        case NFA_STATE_CLASS:
            return charset_contains(
                &state->character_class,
                character
            );

        case NFA_STATE_SPLIT:
        case NFA_STATE_JUMP:
        case NFA_STATE_ASSERT_START:
        case NFA_STATE_ASSERT_END:
        case NFA_STATE_MATCH:
            return 0;
    }

    return 0;
}

static int state_list_contains_match(
    const NfaVm *vm,
    const size_t *list,
    size_t list_count
)
{
    size_t index;

    for (index = 0U; index < list_count; index++) {
        size_t state_index;

        state_index = list[index];

        if (
            vm->program->states[state_index].type ==
            NFA_STATE_MATCH
        ) {
            return 1;
        }
    }

    return 0;
}

static void vm_swap_state_lists(NfaVm *vm)
{
    size_t *temporary_states;
    size_t temporary_count;

    temporary_states = vm->current;
    vm->current = vm->next;
    vm->next = temporary_states;

    temporary_count = vm->current_count;
    vm->current_count = vm->next_count;
    vm->next_count = temporary_count;
}

static int vm_run_from(
    NfaVm *vm,
    const char *text,
    size_t text_length,
    size_t start_position,
    int require_text_end,
    int *matched,
    size_t *match_end,
    VmError *error
)
{
    size_t position;
    size_t longest_end;

    *matched = 0;
    *match_end = start_position;

    if (start_position > text_length) {
        vm_set_error(
            error,
            "search start position exceeds input length"
        );

        return 0;
    }

    longest_end = NFA_INVALID_STATE;

    vm_begin_state_list(
        vm,
        &vm->current_count
    );

    if (
        !vm_add_closure(
            vm,
            vm->current,
            &vm->current_count,
            vm->program->start,
            start_position,
            text_length,
            error
        )
    ) {
        return 0;
    }

    if (
        state_list_contains_match(
            vm,
            vm->current,
            vm->current_count
        )
    ) {
        if (
            !require_text_end ||
            start_position == text_length
        ) {
            longest_end = start_position;
        }
    }

    for (
        position = start_position;
        position < text_length;
        position++
    ) {
        size_t list_index;
        unsigned char character;

        character =
            (unsigned char) text[position];

        vm_begin_state_list(
            vm,
            &vm->next_count
        );

        for (
            list_index = 0U;
            list_index < vm->current_count;
            list_index++
        ) {
            size_t state_index;
            const NfaState *state;

            state_index =
                vm->current[list_index];

            state =
                &vm->program->states[state_index];

            if (
                state_matches_character(
                    state,
                    character
                )
            ) {
                if (
                    !vm_add_closure(
                        vm,
                        vm->next,
                        &vm->next_count,
                        state->out1,
                        position + 1U,
                        text_length,
                        error
                    )
                ) {
                    return 0;
                }
            }
        }

        vm_swap_state_lists(vm);

        if (
            state_list_contains_match(
                vm,
                vm->current,
                vm->current_count
            )
        ) {
            size_t ending_position;

            ending_position = position + 1U;

            if (
                !require_text_end ||
                ending_position == text_length
            ) {
                longest_end = ending_position;
            }
        }

        if (vm->current_count == 0U) {
            break;
        }
    }

    if (longest_end != NFA_INVALID_STATE) {
        *matched = 1;
        *match_end = longest_end;
    }

    return 1;
}

int nfa_vm_full_match(
    const NfaProgram *program,
    const char *text,
    int *matched,
    VmError *error
)
{
    NfaVm vm;
    size_t text_length;
    size_t match_end;
    int success;

    if (error != NULL) {
        error->message = NULL;
    }

    if (matched == NULL) {
        vm_set_error(
            error,
            "VM received a null match-result pointer"
        );

        return 0;
    }

    *matched = 0;

    if (text == NULL) {
        vm_set_error(
            error,
            "VM received null input text"
        );

        return 0;
    }

    if (!vm_initialize(&vm, program, error)) {
        return 0;
    }

    text_length = strlen(text);

    success = vm_run_from(
        &vm,
        text,
        text_length,
        0U,
        1,
        matched,
        &match_end,
        error
    );

    vm_destroy(&vm);

    return success;
}

int nfa_vm_search_from(
    const NfaProgram *program,
    const char *text,
    size_t search_start,
    int *matched,
    NfaMatch *match,
    VmError *error
)
{
    NfaVm vm;
    size_t text_length;
    size_t start_position;

    if (error != NULL) {
        error->message = NULL;
    }

    if (matched == NULL || match == NULL) {
        vm_set_error(
            error,
            "VM received a null search-result pointer"
        );

        return 0;
    }

    *matched = 0;
    match->start = 0U;
    match->end = 0U;

    if (text == NULL) {
        vm_set_error(
            error,
            "VM received null input text"
        );

        return 0;
    }

    text_length = strlen(text);

    if (search_start > text_length) {
        vm_set_error(
            error,
            "search start position exceeds input length"
        );

        return 0;
    }

    if (!vm_initialize(&vm, program, error)) {
        return 0;
    }

    for (
        start_position = search_start;
        start_position <= text_length;
        start_position++
    ) {
        int found_at_start;
        size_t match_end;

        if (
            !vm_run_from(
                &vm,
                text,
                text_length,
                start_position,
                0,
                &found_at_start,
                &match_end,
                error
            )
        ) {
            vm_destroy(&vm);
            return 0;
        }

        if (found_at_start) {
            *matched = 1;
            match->start = start_position;
            match->end = match_end;

            vm_destroy(&vm);

            return 1;
        }
    }

    vm_destroy(&vm);

    return 1;
}

int nfa_vm_search(
    const NfaProgram *program,
    const char *text,
    int *matched,
    NfaMatch *match,
    VmError *error
)
{
    return nfa_vm_search_from(
        program,
        text,
        0U,
        matched,
        match,
        error
    );
}