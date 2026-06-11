#include "cregex/lexer.h"

#include <assert.h>
#include <stdio.h>

static void expect_token(
    Lexer *lexer,
    TokenType expected_type,
    unsigned char expected_character
)
{
    Token token;

    token = lexer_next(lexer);

    assert(token.type == expected_type);

    if (expected_type == TOKEN_LITERAL) {
        assert(
            token.character ==
            expected_character
        );
    }
}

static void test_basic_pattern(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "a(b|c)*"
    );

    expect_token(&lexer, TOKEN_LITERAL, 'a');
    expect_token(&lexer, TOKEN_LPAREN, '\0');
    expect_token(&lexer, TOKEN_LITERAL, 'b');
    expect_token(&lexer, TOKEN_PIPE, '\0');
    expect_token(&lexer, TOKEN_LITERAL, 'c');
    expect_token(&lexer, TOKEN_RPAREN, '\0');
    expect_token(&lexer, TOKEN_STAR, '\0');
    expect_token(&lexer, TOKEN_END, '\0');
}

static void test_escaped_operators(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "\\*\\|\\("
    );

    expect_token(&lexer, TOKEN_LITERAL, '*');
    expect_token(&lexer, TOKEN_LITERAL, '|');
    expect_token(&lexer, TOKEN_LITERAL, '(');
    expect_token(&lexer, TOKEN_END, '\0');
}

static void test_shorthand_classes(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "\\d\\w\\s"
    );

    expect_token(
        &lexer,
        TOKEN_DIGIT_CLASS,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_WORD_CLASS,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_SPACE_CLASS,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_END,
        '\0'
    );
}

static void test_character_class_range(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "[a-z]"
    );

    expect_token(
        &lexer,
        TOKEN_LBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_LITERAL,
        'a'
    );

    expect_token(
        &lexer,
        TOKEN_DASH,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_LITERAL,
        'z'
    );

    expect_token(
        &lexer,
        TOKEN_RBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_END,
        '\0'
    );
}

static void test_negated_character_class(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "[^0-9]"
    );

    expect_token(
        &lexer,
        TOKEN_LBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_CARET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_LITERAL,
        '0'
    );

    expect_token(
        &lexer,
        TOKEN_DASH,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_LITERAL,
        '9'
    );

    expect_token(
        &lexer,
        TOKEN_RBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_END,
        '\0'
    );
}

static void test_operators_are_literals_inside_class(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "[.*+?()]"
    );

    expect_token(
        &lexer,
        TOKEN_LBRACKET,
        '\0'
    );

    expect_token(&lexer, TOKEN_LITERAL, '.');
    expect_token(&lexer, TOKEN_LITERAL, '*');
    expect_token(&lexer, TOKEN_LITERAL, '+');
    expect_token(&lexer, TOKEN_LITERAL, '?');
    expect_token(&lexer, TOKEN_LITERAL, '(');
    expect_token(&lexer, TOKEN_LITERAL, ')');

    expect_token(
        &lexer,
        TOKEN_RBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_END,
        '\0'
    );
}

static void test_escaped_class_characters(void)
{
    Lexer lexer;

    lexer_init(
        &lexer,
        "[\\]\\-]"
    );

    expect_token(
        &lexer,
        TOKEN_LBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_LITERAL,
        ']'
    );

    expect_token(
        &lexer,
        TOKEN_LITERAL,
        '-'
    );

    expect_token(
        &lexer,
        TOKEN_RBRACKET,
        '\0'
    );

    expect_token(
        &lexer,
        TOKEN_END,
        '\0'
    );
}

static void test_incomplete_escape(void)
{
    Lexer lexer;
    Token token;

    lexer_init(
        &lexer,
        "abc\\"
    );

    expect_token(&lexer, TOKEN_LITERAL, 'a');
    expect_token(&lexer, TOKEN_LITERAL, 'b');
    expect_token(&lexer, TOKEN_LITERAL, 'c');

    token = lexer_next(&lexer);

    assert(token.type == TOKEN_ERROR);
    assert(token.error_message != NULL);
}

static void test_peek_does_not_advance(void)
{
    Lexer lexer;
    Token peeked;
    Token actual;

    lexer_init(
        &lexer,
        "ab"
    );

    peeked = lexer_peek(&lexer);
    actual = lexer_next(&lexer);

    assert(peeked.type == TOKEN_LITERAL);
    assert(peeked.character == 'a');

    assert(actual.type == TOKEN_LITERAL);
    assert(actual.character == 'a');
}

int main(void)
{
    test_basic_pattern();
    test_escaped_operators();
    test_shorthand_classes();
    test_character_class_range();
    test_negated_character_class();
    test_operators_are_literals_inside_class();
    test_escaped_class_characters();
    test_incomplete_escape();
    test_peek_does_not_advance();

    puts("All lexer tests passed.");

    return 0;
}