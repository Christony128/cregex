#include "cregex/compiler.h"
#include "cregex/parser.h"

#include <assert.h>
#include <stdio.h>

static NfaProgram compile_pattern(
    const char *pattern
)
{
    ParserError parser_error;
    CompilerError compiler_error;
    AstNode *root;
    NfaProgram program;
    int success;

    root = parser_parse(
        pattern,
        &parser_error
    );

    if (root == NULL) {
        fprintf(
            stderr,
            "Parser failed for \"%s\" at position %lu: %s\n",
            pattern,
            (unsigned long) parser_error.position,
            parser_error.message != NULL
                ? parser_error.message
                : "unknown parser error"
        );
    }

    assert(root != NULL);

    nfa_program_init(&program);

    success = compiler_compile(
        root,
        &program,
        &compiler_error
    );

    if (!success) {
        fprintf(
            stderr,
            "Compiler failed for \"%s\" at position %lu: %s\n",
            pattern,
            (unsigned long) compiler_error.position,
            compiler_error.message != NULL
                ? compiler_error.message
                : "unknown compiler error"
        );
    }

    assert(success);

    ast_free(root);

    return program;
}

static void test_literal(void)
{
    NfaProgram program;

    program = compile_pattern("a");

    assert(program.count == 2U);
    assert(program.start == 0U);

    assert(
        program.states[0].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[0].character ==
        'a'
    );

    assert(
        program.states[0].out1 ==
        1U
    );

    assert(
        program.states[1].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_concatenation(void)
{
    NfaProgram program;

    program = compile_pattern("ab");

    assert(program.count == 3U);
    assert(program.start == 0U);

    assert(
        program.states[0].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[0].out1 ==
        1U
    );

    assert(
        program.states[1].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[1].out1 ==
        2U
    );

    assert(
        program.states[2].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_alternation(void)
{
    NfaProgram program;
    size_t split;
    size_t left;
    size_t right;

    program = compile_pattern("a|b");

    assert(program.count == 4U);

    split = program.start;

    assert(
        program.states[split].type ==
        NFA_STATE_SPLIT
    );

    left = program.states[split].out1;
    right = program.states[split].out2;

    assert(
        program.states[left].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[right].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[left].out1 ==
        3U
    );

    assert(
        program.states[right].out1 ==
        3U
    );

    assert(
        program.states[3].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_star(void)
{
    NfaProgram program;
    size_t split;

    program = compile_pattern("a*");

    assert(program.count == 3U);

    split = program.start;

    assert(
        program.states[split].type ==
        NFA_STATE_SPLIT
    );

    assert(
        program.states[split].out1 ==
        0U
    );

    assert(
        program.states[split].out2 ==
        2U
    );

    assert(
        program.states[0].out1 ==
        split
    );

    assert(
        program.states[2].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_plus(void)
{
    NfaProgram program;

    program = compile_pattern("a+");

    assert(program.count == 3U);
    assert(program.start == 0U);

    assert(
        program.states[0].type ==
        NFA_STATE_CHAR
    );

    assert(
        program.states[0].out1 ==
        1U
    );

    assert(
        program.states[1].type ==
        NFA_STATE_SPLIT
    );

    assert(
        program.states[1].out1 ==
        0U
    );

    assert(
        program.states[1].out2 ==
        2U
    );

    assert(
        program.states[2].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_optional(void)
{
    NfaProgram program;
    size_t split;

    program = compile_pattern("a?");

    assert(program.count == 3U);

    split = program.start;

    assert(
        program.states[split].type ==
        NFA_STATE_SPLIT
    );

    assert(
        program.states[split].out1 ==
        0U
    );

    assert(
        program.states[split].out2 ==
        2U
    );

    assert(
        program.states[0].out1 ==
        2U
    );

    assert(
        program.states[2].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_empty_pattern(void)
{
    NfaProgram program;

    program = compile_pattern("");

    assert(program.count == 2U);

    assert(
        program.states[program.start].type ==
        NFA_STATE_JUMP
    );

    assert(
        program.states[program.start].out1 ==
        1U
    );

    assert(
        program.states[1].type ==
        NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

static void test_complex_pattern(void)
{
    NfaProgram program;

    program = compile_pattern(
        "^(ab|c)*\\d+$"
    );

    assert(program.count > 0U);

    assert(
        program.start !=
        NFA_INVALID_STATE
    );

    assert(
        program.states[
            program.count - 1U
        ].type == NFA_STATE_MATCH
    );

    nfa_program_free(&program);
}

int main(void)
{
    test_literal();
    test_concatenation();
    test_alternation();
    test_star();
    test_plus();
    test_optional();
    test_empty_pattern();
    test_complex_pattern();

    puts("All compiler tests passed.");

    return 0;
}