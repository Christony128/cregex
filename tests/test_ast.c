#include "cregex/ast.h"
#include "cregex/charset.h"

#include <assert.h>
#include <stdio.h>

static void test_character_sets(void)
{
    CharSet digits;
    CharSet word;
    CharSet spaces;
    CharSet negated;

    digits = charset_digit();

    assert(charset_contains(&digits, '0'));
    assert(charset_contains(&digits, '7'));
    assert(charset_contains(&digits, '9'));
    assert(!charset_contains(&digits, 'a'));

    word = charset_word();

    assert(charset_contains(&word, 'a'));
    assert(charset_contains(&word, 'Z'));
    assert(charset_contains(&word, '5'));
    assert(charset_contains(&word, '_'));
    assert(!charset_contains(&word, '-'));

    spaces = charset_space();

    assert(charset_contains(&spaces, ' '));
    assert(charset_contains(&spaces, '\t'));
    assert(charset_contains(&spaces, '\n'));
    assert(!charset_contains(&spaces, 'x'));

    charset_clear(&negated);
    charset_add_char(&negated, 'a');
    charset_set_negated(&negated, 1);

    assert(!charset_contains(&negated, 'a'));
    assert(charset_contains(&negated, 'b'));
}

static void test_ast_construction(void)
{
    AstNode *literal_a;
    AstNode *literal_b;
    AstNode *literal_c;
    AstNode *alternation;
    AstNode *star;
    AstNode *root;

    /*
     * Build the AST for:
     *
     *     a(b|c)*
     */

    literal_a = ast_make_literal('a', 0);
    literal_b = ast_make_literal('b', 2);
    literal_c = ast_make_literal('c', 4);

    assert(literal_a != NULL);
    assert(literal_b != NULL);
    assert(literal_c != NULL);

    alternation = ast_make_binary(
        AST_ALTERNATION,
        literal_b,
        literal_c,
        3
    );

    assert(alternation != NULL);

    star = ast_make_unary(
        AST_STAR,
        alternation,
        6
    );

    assert(star != NULL);

    root = ast_make_binary(
        AST_CONCATENATION,
        literal_a,
        star,
        1
    );

    assert(root != NULL);

    assert(root->type == AST_CONCATENATION);
    assert(root->value.binary.left->type == AST_LITERAL);
    assert(root->value.binary.left->value.character == 'a');

    assert(root->value.binary.right->type == AST_STAR);

    assert(
        root
            ->value.binary.right
            ->value.child
            ->type == AST_ALTERNATION
    );

    ast_free(root);
}

static void test_character_class_ast(void)
{
    CharSet digits;
    AstNode *node;

    digits = charset_digit();

    node = ast_make_character_class(&digits, 0);

    assert(node != NULL);
    assert(node->type == AST_CHARACTER_CLASS);

    assert(
        charset_contains(
            &node->value.character_class,
            '4'
        )
    );

    assert(
        !charset_contains(
            &node->value.character_class,
            'x'
        )
    );

    ast_free(node);
}

int main(void)
{
    test_character_sets();
    test_ast_construction();
    test_character_class_ast();

    puts("All AST tests passed.");

    return 0;
}