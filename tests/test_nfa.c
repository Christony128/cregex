#include "cregex/nfa.h"

#include <assert.h>
#include <stdio.h>

static void test_simple_character_program(void)
{
    NfaProgram program;
    size_t character;
    size_t match;

    nfa_program_init(&program);

    character = nfa_program_add_char(&program, 'a');
    match = nfa_program_add_match(&program);

    assert(character != NFA_INVALID_STATE);
    assert(match != NFA_INVALID_STATE);

    assert(
        nfa_program_patch_out1(
            &program,
            character,
            match
        )
    );

    assert(
        nfa_program_set_start(
            &program,
            character
        )
    );

    assert(program.count == 2);
    assert(program.start == character);

    assert(
        program.states[character].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[character].character ==
        'a'
    );

    assert(
        program.states[character].out1 ==
        match
    );

    assert(
        program.states[match].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_split_state(void)
{
    NfaProgram program;
    size_t split;
    size_t left;
    size_t right;

    nfa_program_init(&program);

    split = nfa_program_add_split(&program);
    left = nfa_program_add_char(&program, 'a');
    right = nfa_program_add_char(&program, 'b');

    assert(split != NFA_INVALID_STATE);
    assert(left != NFA_INVALID_STATE);
    assert(right != NFA_INVALID_STATE);

    assert(nfa_program_patch_out1(&program, split, left));
    assert(nfa_program_patch_out2(&program, split, right));

    assert(program.states[split].out1 == left);
    assert(program.states[split].out2 == right);

    nfa_program_free(&program);
}

static void test_character_class_state(void)
{
    NfaProgram program;
    CharSet digits;
    size_t state;

    nfa_program_init(&program);

    digits = charset_digit();

    state = nfa_program_add_class(
        &program,
        &digits
    );

    assert(state != NFA_INVALID_STATE);

    assert(
        program.states[state].type ==
        NFA_STATE_CLASS
    );

    assert(
        charset_contains(
            &program.states[state].character_class,
            '5'
        )
    );

    assert(
        !charset_contains(
            &program.states[state].character_class,
            'x'
        )
    );

    nfa_program_free(&program);
}

static void test_invalid_operations(void)
{
    NfaProgram program;
    size_t match;

    nfa_program_init(&program);

    match = nfa_program_add_match(&program);

    assert(match != NFA_INVALID_STATE);

    assert(
        !nfa_program_patch_out1(
            &program,
            99,
            match
        )
    );

    assert(
        !nfa_program_patch_out1(
            &program,
            match,
            99
        )
    );

    assert(
        !nfa_program_set_start(
            &program,
            99
        )
    );

    nfa_program_free(&program);
}

int main(void)
{
    test_simple_character_program();
    test_split_state();
    test_character_class_state();
    test_invalid_operations();

    puts("All NFA tests passed.");

    return 0;
}