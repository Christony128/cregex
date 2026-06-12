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

static void expect_find_all(
    const char *pattern,
    const char *text,
    const RegexMatch *expected,
    size_t expected_count
)
{
    RegexError error;
    RegexMatchList result;
    Regex *regex;
    size_t index;

    regex = compile_successfully(pattern);

    result.matches = (RegexMatch *) 1;
    result.count = 99U;

    assert(
        regex_find_all(
            regex,
            text,
            &result,
            &error
        )
    );

    assert(error.message == NULL);
    assert(result.count == expected_count);

    for (index = 0U; index < expected_count; index++) {
        assert(
            result.matches[index].start ==
            expected[index].start
        );

        assert(
            result.matches[index].end ==
            expected[index].end
        );
    }

    regex_match_list_free(&result);
    assert(result.matches == NULL);
    assert(result.count == 0U);

    regex_free(regex);
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

static void test_find_all_basic(void)
{
    static const RegexMatch expected[] = {
        {1U, 3U},
        {4U, 7U}
    };

    expect_find_all(
        "[0-9]+",
        "a12b345c",
        expected,
        sizeof(expected) / sizeof(expected[0])
    );
}

static void test_find_all_leftmost_longest(void)
{
    static const RegexMatch expected[] = {
        {0U, 2U},
        {2U, 4U},
        {4U, 5U}
    };

    expect_find_all(
        "a|ab",
        "ababa",
        expected,
        sizeof(expected) / sizeof(expected[0])
    );
}

static void test_find_all_non_overlapping(void)
{
    static const RegexMatch expected[] = {
        {0U, 2U},
        {2U, 4U}
    };

    expect_find_all(
        "aa",
        "aaaaa",
        expected,
        sizeof(expected) / sizeof(expected[0])
    );
}

static void test_find_all_no_match(void)
{
    expect_find_all(
        "[0-9]+",
        "letters",
        NULL,
        0U
    );
}

static void test_find_all_with_anchors(void)
{
    static const RegexMatch start_expected[] = {
        {0U, 3U}
    };

    static const RegexMatch end_expected[] = {
        {3U, 6U}
    };

    expect_find_all(
        "^abc",
        "abcabc",
        start_expected,
        sizeof(start_expected) /
            sizeof(start_expected[0])
    );

    expect_find_all(
        "abc$",
        "abcabc",
        end_expected,
        sizeof(end_expected) /
            sizeof(end_expected[0])
    );
}

static void test_find_all_zero_length_matches(void)
{
    static const RegexMatch empty_expected[] = {
        {0U, 0U},
        {1U, 1U},
        {2U, 2U},
        {3U, 3U}
    };

    static const RegexMatch star_expected[] = {
        {0U, 0U},
        {1U, 3U},
        {3U, 3U}
    };

    static const RegexMatch end_expected[] = {
        {3U, 3U}
    };

    expect_find_all(
        "",
        "abc",
        empty_expected,
        sizeof(empty_expected) /
            sizeof(empty_expected[0])
    );

    expect_find_all(
        "a*",
        "baa",
        star_expected,
        sizeof(star_expected) /
            sizeof(star_expected[0])
    );

    expect_find_all(
        "$",
        "abc",
        end_expected,
        sizeof(end_expected) /
            sizeof(end_expected[0])
    );
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
    RegexMatchList list;
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

    list.matches = (RegexMatch *) 1;
    list.count = 99U;

    assert(
        !regex_find_all(
            NULL,
            "abc",
            &list,
            &error
        )
    );

    assert(list.matches == NULL);
    assert(list.count == 0U);
    assert(error.message != NULL);

    list.matches = (RegexMatch *) 1;
    list.count = 99U;

    assert(
        !regex_find_all(
            regex,
            NULL,
            &list,
            &error
        )
    );

    assert(list.matches == NULL);
    assert(list.count == 0U);
    assert(error.message != NULL);

    assert(
        !regex_find_all(
            regex,
            "abc",
            NULL,
            &error
        )
    );

    assert(error.message != NULL);

    regex_match_list_free(NULL);
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

    test_find_all_basic();
    test_find_all_leftmost_longest();
    test_find_all_non_overlapping();
    test_find_all_no_match();
    test_find_all_with_anchors();
    test_find_all_zero_length_matches();

    test_compile_error();
    test_invalid_arguments();

    puts("All public API tests passed.");

    return 0;
}