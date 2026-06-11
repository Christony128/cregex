#include "cregex/compiler.h"
#include "cregex/parser.h"
#include "cregex/vm.h"

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
            "Compiler failed for \"%s\": %s\n",
            pattern,
            compiler_error.message != NULL
                ? compiler_error.message
                : "unknown compiler error"
        );
    }

    assert(success);

    ast_free(root);

    return program;
}

static void expect_match(
    const char *pattern,
    const char *text,
    int expected
)
{
    NfaProgram program;
    VmError vm_error;
    int matched;
    int success;

    program = compile_pattern(pattern);

    success = nfa_vm_full_match(
        &program,
        text,
        &matched,
        &vm_error
    );

    if (!success) {
        fprintf(
            stderr,
            "VM failed for pattern \"%s\" and text \"%s\": %s\n",
            pattern,
            text,
            vm_error.message != NULL
                ? vm_error.message
                : "unknown VM error"
        );
    }

    assert(success);
    assert(matched == expected);

    nfa_program_free(&program);
}

static void test_literals(void)
{
    expect_match("a", "a", 1);
    expect_match("a", "b", 0);
    expect_match("a", "", 0);
    expect_match("a", "aa", 0);

    expect_match("abc", "abc", 1);
    expect_match("abc", "ab", 0);
    expect_match("abc", "abcd", 0);
}

static void test_alternation(void)
{
    expect_match("a|b", "a", 1);
    expect_match("a|b", "b", 1);
    expect_match("a|b", "c", 0);

    expect_match("cat|dog", "cat", 1);
    expect_match("cat|dog", "dog", 1);
    expect_match("cat|dog", "catalog", 0);
}

static void test_star(void)
{
    expect_match("a*", "", 1);
    expect_match("a*", "a", 1);
    expect_match("a*", "aaaa", 1);
    expect_match("a*", "aaab", 0);
}

static void test_plus(void)
{
    expect_match("a+", "", 0);
    expect_match("a+", "a", 1);
    expect_match("a+", "aaaa", 1);
    expect_match("a+", "b", 0);
}

static void test_optional(void)
{
    expect_match("a?", "", 1);
    expect_match("a?", "a", 1);
    expect_match("a?", "aa", 0);

    expect_match("ab?c", "ac", 1);
    expect_match("ab?c", "abc", 1);
    expect_match("ab?c", "abbc", 0);
}

static void test_groups(void)
{
    expect_match("a(b|c)*", "a", 1);
    expect_match("a(b|c)*", "ab", 1);
    expect_match("a(b|c)*", "acbcbb", 1);
    expect_match("a(b|c)*", "abcd", 0);
}

static void test_dot(void)
{
    expect_match(".", "x", 1);
    expect_match(".", "7", 1);
    expect_match(".", "", 0);
    expect_match(".", "ab", 0);

    expect_match("a.c", "abc", 1);
    expect_match("a.c", "a7c", 1);
    expect_match("a.c", "ac", 0);
}

static void test_shorthand_classes(void)
{
    expect_match("\\d+", "12345", 1);
    expect_match("\\d+", "123a", 0);
    expect_match("\\d+", "", 0);

    expect_match("\\w+", "hello_123", 1);
    expect_match("\\w+", "hello-123", 0);

    expect_match("\\s+", " \t\n", 1);
    expect_match("\\s+", "abc", 0);
}

static void test_anchors(void)
{
    expect_match("^abc$", "abc", 1);
    expect_match("^abc$", "xabc", 0);
    expect_match("^abc$", "abcx", 0);

    expect_match("^$", "", 1);
    expect_match("^$", "a", 0);
}

static void test_empty_pattern(void)
{
    expect_match("", "", 1);
    expect_match("", "a", 0);
}

static void test_nested_repetition(void)
{
    expect_match("(ab|c)*", "", 1);
    expect_match("(ab|c)*", "ab", 1);
    expect_match("(ab|c)*", "cababc", 1);
    expect_match("(ab|c)*", "acb", 0);
}

int main(void)
{
    test_literals();
    test_alternation();
    test_star();
    test_plus();
    test_optional();
    test_groups();
    test_dot();
    test_shorthand_classes();
    test_anchors();
    test_empty_pattern();
    test_nested_repetition();

    puts("All VM tests passed.");

    return 0;
}