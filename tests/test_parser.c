#include "cregex/parser.h"

#include <assert.h>
#include <stdio.h>

static AstNode *parse_successfully(
    const char *pattern
)
{
    ParserError error;
    AstNode *root;

    root = parser_parse(
        pattern,
        &error
    );

    if (root == NULL) {
        fprintf(
            stderr,
            "Pattern \"%s\" failed at position %lu: %s\n",
            pattern,
            (unsigned long) error.position,
            error.message != NULL
                ? error.message
                : "unknown parser error"
        );
    }

    assert(root != NULL);

    return root;
}

static void expect_parse_failure(
    const char *pattern
)
{
    ParserError error;
    AstNode *root;

    root = parser_parse(
        pattern,
        &error
    );

    assert(root == NULL);
    assert(error.message != NULL);
}

static void test_empty_pattern(void)
{
    AstNode *root;

    root = parse_successfully("");

    assert(root->type == AST_EMPTY);

    ast_free(root);
}

static void test_literal_concatenation(void)
{
    AstNode *root;

    root = parse_successfully("ab");

    assert(
        root->type ==
        AST_CONCATENATION
    );

    assert(
        root->value.binary.left->type ==
        AST_LITERAL
    );

    assert(
        root->value.binary.left
            ->value.character == 'a'
    );

    assert(
        root->value.binary.right->type ==
        AST_LITERAL
    );

    assert(
        root->value.binary.right
            ->value.character == 'b'
    );

    ast_free(root);
}

static void test_operator_precedence(void)
{
    AstNode *root;

    /*
     * Must parse as:
     *
     *     (ab) | (c*)
     */
    root = parse_successfully(
        "ab|c*"
    );

    assert(
        root->type ==
        AST_ALTERNATION
    );

    assert(
        root->value.binary.left->type ==
        AST_CONCATENATION
    );

    assert(
        root->value.binary.right->type ==
        AST_STAR
    );

    ast_free(root);
}

static void test_grouping(void)
{
    AstNode *root;
    AstNode *star;
    AstNode *alternation;

    root = parse_successfully(
        "a(b|c)*"
    );

    assert(
        root->type ==
        AST_CONCATENATION
    );

    star =
        root->value.binary.right;

    assert(star->type == AST_STAR);

    alternation =
        star->value.child;

    assert(
        alternation->type ==
        AST_ALTERNATION
    );

    ast_free(root);
}

static void test_classes_and_assertions(void)
{
    AstNode *root;

    root = parse_successfully(
        "^\\d+\\w?$"
    );

    assert(
        root->type ==
        AST_CONCATENATION
    );

    ast_free(root);
}

static void test_simple_character_class(void)
{
    AstNode *root;

    root = parse_successfully(
        "[abc]"
    );

    assert(
        root->type ==
        AST_CHARACTER_CLASS
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'a'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'b'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'c'
        )
    );

    assert(
        !charset_contains(
            &root->value.character_class,
            'd'
        )
    );

    ast_free(root);
}

static void test_character_class_range(void)
{
    AstNode *root;

    root = parse_successfully(
        "[a-z]"
    );

    assert(
        root->type ==
        AST_CHARACTER_CLASS
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'a'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'm'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'z'
        )
    );

    assert(
        !charset_contains(
            &root->value.character_class,
            'A'
        )
    );

    ast_free(root);
}

static void test_negated_character_class(void)
{
    AstNode *root;

    root = parse_successfully(
        "[^0-9]"
    );

    assert(
        root->type ==
        AST_CHARACTER_CLASS
    );

    assert(
        root->value.character_class.negated
    );

    assert(
        !charset_contains(
            &root->value.character_class,
            '5'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'a'
        )
    );

    ast_free(root);
}

static void test_combined_character_class(void)
{
    AstNode *root;

    root = parse_successfully(
        "[a-zA-Z0-9_]"
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'a'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            'Z'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            '7'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            '_'
        )
    );

    assert(
        !charset_contains(
            &root->value.character_class,
            '-'
        )
    );

    ast_free(root);
}

static void test_escaped_character_class_values(void)
{
    AstNode *root;

    root = parse_successfully(
        "[\\]\\-]"
    );

    assert(
        charset_contains(
            &root->value.character_class,
            ']'
        )
    );

    assert(
        charset_contains(
            &root->value.character_class,
            '-'
        )
    );

    ast_free(root);
}

static void test_invalid_patterns(void)
{
    expect_parse_failure("(");
    expect_parse_failure("()");
    expect_parse_failure("a|");
    expect_parse_failure("|a");
    expect_parse_failure("*a");
    expect_parse_failure("a**");
    expect_parse_failure("a)");
    expect_parse_failure("abc\\");

    expect_parse_failure("[");
    expect_parse_failure("[]");
    expect_parse_failure("[^]");
    expect_parse_failure("[z-a]");
    expect_parse_failure("[a-");
}

int main(void)
{
    test_empty_pattern();
    test_literal_concatenation();
    test_operator_precedence();
    test_grouping();
    test_classes_and_assertions();

    test_simple_character_class();
    test_character_class_range();
    test_negated_character_class();
    test_combined_character_class();
    test_escaped_character_class_values();

    test_invalid_patterns();

    puts("All parser tests passed.");

    return 0;
}