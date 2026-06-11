#include "cregex/regex.h"

#include <assert.h>
#include <stdio.h>

static Regex *compile_successfully(
    const char *pattern
)
{
    RegexError error;
    Regex *regex;

    regex = regex_compile(
        pattern,
        &error
    );

    assert(regex != NULL);
    assert(error.message == NULL);
    assert(
        error.position ==
        REGEX_ERROR_NO_POSITION
    );

    return regex;
}

static void expect_full_match(
    const Regex *regex,
    const char *text,
    int expected
)
{
    RegexError error;
    int matched;
    int success;

    matched = -1;

    success = regex_full_match(
        regex,
        text,
        &matched,
        &error
    );

    assert(success);
    assert(matched == expected);
    assert(error.message == NULL);
    assert(
        error.position ==
        REGEX_ERROR_NO_POSITION
    );
}

static void test_compile_and_reuse(void)
{
    Regex *regex;

    regex = compile_successfully(
        "^[a-z]+\\d?$"
    );

    expect_full_match(regex, "hello", 1);
    expect_full_match(regex, "hello7", 1);
    expect_full_match(regex, "Hello7", 0);
    expect_full_match(regex, "hello77", 0);

    regex_free(regex);
}

static void test_required_character_classes(void)
{
    Regex *regex;

    regex = compile_successfully("[abc]");
    expect_full_match(regex, "a", 1);
    expect_full_match(regex, "d", 0);
    regex_free(regex);

    regex = compile_successfully("[a-z]");
    expect_full_match(regex, "m", 1);
    expect_full_match(regex, "M", 0);
    regex_free(regex);

    regex = compile_successfully("[^0-9]");
    expect_full_match(regex, "x", 1);
    expect_full_match(regex, "5", 0);
    regex_free(regex);

    regex = compile_successfully(
        "[a-zA-Z0-9_]+"
    );
    expect_full_match(regex, "Ab_09", 1);
    expect_full_match(regex, "Ab-09", 0);
    regex_free(regex);

    regex = compile_successfully("[\\]\\-]+");
    expect_full_match(regex, "]--]", 1);
    expect_full_match(regex, "a", 0);
    regex_free(regex);

    regex = compile_successfully("[\\dA-F]+");
    expect_full_match(regex, "19AF", 1);
    expect_full_match(regex, "19G", 0);
    regex_free(regex);
}

static void test_leftmost_longest_search(void)
{
    RegexError error;
    RegexMatch match;
    Regex *regex;
    int matched;
    int success;

    regex = compile_successfully("a|ab");

    success = regex_search(
        regex,
        "zzababa",
        &matched,
        &match,
        &error
    );

    assert(success);
    assert(matched);
    assert(match.start == 2U);
    assert(match.end == 4U);
    assert(error.message == NULL);

    regex_free(regex);
}

static void test_search_no_match(void)
{
    RegexError error;
    RegexMatch match;
    Regex *regex;
    int matched;

    regex = compile_successfully("[0-9]+");

    match.start = 99U;
    match.end = 99U;

    assert(
        regex_search(
            regex,
            "letters only",
            &matched,
            &match,
            &error
        )
    );

    assert(!matched);
    assert(match.start == 0U);
    assert(match.end == 0U);
    assert(error.message == NULL);

    regex_free(regex);
}

static void test_empty_pattern(void)
{
    RegexError error;
    RegexMatch match;
    Regex *regex;
    int matched;

    regex = compile_successfully("");

    expect_full_match(regex, "", 1);
    expect_full_match(regex, "x", 0);

    assert(
        regex_search(
            regex,
            "abc",
            &matched,
            &match,
            &error
        )
    );

    assert(matched);
    assert(match.start == 0U);
    assert(match.end == 0U);

    regex_free(regex);
}

static void test_compile_error(void)
{
    RegexError error;
    Regex *regex;

    regex = regex_compile(
        "[z-a]",
        &error
    );

    assert(regex == NULL);
    assert(error.message != NULL);
    assert(error.position == 1U);
}

static void test_invalid_arguments(void)
{
    RegexError error;
    RegexMatch match;
    Regex *regex;
    int matched;

    regex = regex_compile(NULL, &error);
    assert(regex == NULL);
    assert(error.message != NULL);
    assert(
        error.position ==
        REGEX_ERROR_NO_POSITION
    );

    regex = compile_successfully("abc");

    matched = 7;

    assert(
        !regex_full_match(
            NULL,
            "abc",
            &matched,
            &error
        )
    );

    assert(matched == 0);
    assert(error.message != NULL);

    matched = 7;

    assert(
        !regex_full_match(
            regex,
            NULL,
            &matched,
            &error
        )
    );

    assert(matched == 0);
    assert(error.message != NULL);

    assert(
        !regex_full_match(
            regex,
            "abc",
            NULL,
            &error
        )
    );

    assert(error.message != NULL);

    matched = 7;
    match.start = 9U;
    match.end = 9U;

    assert(
        !regex_search(
            regex,
            "abc",
            &matched,
            NULL,
            &error
        )
    );

    assert(matched == 0);
    assert(error.message != NULL);

    assert(
        !regex_search(
            regex,
            "abc",
            NULL,
            &match,
            &error
        )
    );

    assert(match.start == 0U);
    assert(match.end == 0U);
    assert(error.message != NULL);

    regex_free(regex);
    regex_free(NULL);
}

int main(void)
{
    test_compile_and_reuse();
    test_required_character_classes();
    test_leftmost_longest_search();
    test_search_no_match();
    test_empty_pattern();
    test_compile_error();
    test_invalid_arguments();

    puts("All public API tests passed.");

    return 0;
}