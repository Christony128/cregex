#include "cregex/parser.h"

#include "cregex/lexer.h"
#include "cregex/token.h"

#include <stddef.h>

typedef struct {
    Lexer lexer;
    Token current;

    int failed;
    ParserError error;
} Parser;

static void parser_set_error(
    Parser *parser,
    size_t position,
    const char *message
)
{
    if (parser->failed) {
        return;
    }

    parser->failed = 1;
    parser->error.position = position;
    parser->error.message = message;
}

static void parser_advance(Parser *parser)
{
    if (parser->failed) {
        return;
    }

    parser->current =
        lexer_next(&parser->lexer);

    if (parser->current.type == TOKEN_ERROR) {
        parser_set_error(
            parser,
            parser->current.position,
            parser->current.error_message
        );
    }
}

static int token_starts_atom(TokenType type)
{
    switch (type) {
        case TOKEN_LITERAL:
        case TOKEN_DOT:
        case TOKEN_LPAREN:
        case TOKEN_LBRACKET:
        case TOKEN_CARET:
        case TOKEN_DOLLAR:
        case TOKEN_DIGIT_CLASS:
        case TOKEN_WORD_CLASS:
        case TOKEN_SPACE_CLASS:
            return 1;

        case TOKEN_PIPE:
        case TOKEN_STAR:
        case TOKEN_PLUS:
        case TOKEN_QUESTION:
        case TOKEN_RPAREN:
        case TOKEN_RBRACKET:
        case TOKEN_DASH:
        case TOKEN_END:
        case TOKEN_ERROR:
            return 0;
    }

    return 0;
}

static AstNode *parse_alternation(Parser *parser);

static AstNode *make_unary_node(
    Parser *parser,
    AstType type,
    AstNode *child,
    size_t position
)
{
    AstNode *node;

    node = ast_make_unary(
        type,
        child,
        position
    );

    if (node == NULL) {
        ast_free(child);

        parser_set_error(
            parser,
            position,
            "failed to allocate unary AST node"
        );

        return NULL;
    }

    return node;
}

static AstNode *make_binary_node(
    Parser *parser,
    AstType type,
    AstNode *left,
    AstNode *right,
    size_t position
)
{
    AstNode *node;

    node = ast_make_binary(
        type,
        left,
        right,
        position
    );

    if (node == NULL) {
        ast_free(left);
        ast_free(right);

        parser_set_error(
            parser,
            position,
            "failed to allocate binary AST node"
        );

        return NULL;
    }

    return node;
}

static int class_token_to_character(
    Token token,
    unsigned char *character
)
{
    if (character == NULL) {
        return 0;
    }

    switch (token.type) {
        case TOKEN_LITERAL:
            *character = token.character;
            return 1;

        case TOKEN_CARET:
            *character = '^';
            return 1;

        case TOKEN_DASH:
            *character = '-';
            return 1;

        default:
            return 0;
    }
}

static void add_shorthand_class(
    CharSet *destination,
    TokenType type
)
{
    CharSet source;

    switch (type) {
        case TOKEN_DIGIT_CLASS:
            source = charset_digit();
            break;

        case TOKEN_WORD_CLASS:
            source = charset_word();
            break;

        case TOKEN_SPACE_CLASS:
            source = charset_space();
            break;

        default:
            return;
    }

    charset_add_set(
        destination,
        &source
    );
}

static AstNode *parse_character_class(
    Parser *parser
)
{
    Token opening;
    CharSet set;
    int negated;
    int item_count;

    opening = parser->current;

    charset_clear(&set);

    negated = 0;
    item_count = 0;
    parser_advance(parser);
    if (parser->current.type == TOKEN_CARET) {
        negated = 1;
        parser_advance(parser);
    }

    if (parser->current.type == TOKEN_RBRACKET) {
        parser_set_error(
            parser,
            opening.position,
            "character class cannot be empty"
        );

        return NULL;
    }

    while (
        !parser->failed &&
        parser->current.type != TOKEN_RBRACKET
    ) {
        Token first_token;

        first_token = parser->current;

        if (first_token.type == TOKEN_END) {
            parser_set_error(
                parser,
                opening.position,
                "missing closing bracket"
            );

            return NULL;
        }

        if (
            first_token.type == TOKEN_DIGIT_CLASS ||
            first_token.type == TOKEN_WORD_CLASS ||
            first_token.type == TOKEN_SPACE_CLASS
        ) {
            add_shorthand_class(
                &set,
                first_token.type
            );

            item_count++;
            parser_advance(parser);

            continue;
        }

        {
            unsigned char first_character;

            if (
                !class_token_to_character(
                    first_token,
                    &first_character
                )
            ) {
                parser_set_error(
                    parser,
                    first_token.position,
                    "invalid token inside character class"
                );

                return NULL;
            }

            parser_advance(parser);

            if (parser->current.type == TOKEN_DASH) {
                Token token_after_dash;

                token_after_dash =
                    lexer_peek(&parser->lexer);
                if (
                    token_after_dash.type ==
                    TOKEN_RBRACKET
                ) {
                    charset_add_char(
                        &set,
                        first_character
                    );

                    charset_add_char(
                        &set,
                        '-'
                    );

                    item_count += 2;
                    parser_advance(parser);

                    continue;
                }
                parser_advance(parser);

                {
                    unsigned char last_character;

                    if (
                        !class_token_to_character(
                            parser->current,
                            &last_character
                        )
                    ) {
                        parser_set_error(
                            parser,
                            parser->current.position,
                            "range endpoint must be a literal character"
                        );

                        return NULL;
                    }

                    if (first_character > last_character) {
                        parser_set_error(
                            parser,
                            first_token.position,
                            "character range is reversed"
                        );

                        return NULL;
                    }

                    if (
                        !charset_add_range(
                            &set,
                            first_character,
                            last_character
                        )
                    ) {
                        parser_set_error(
                            parser,
                            first_token.position,
                            "failed to add character range"
                        );

                        return NULL;
                    }

                    item_count++;
                    parser_advance(parser);
                }
            } else {
                charset_add_char(
                    &set,
                    first_character
                );

                item_count++;
            }
        }
    }

    if (parser->failed) {
        return NULL;
    }

    if (parser->current.type != TOKEN_RBRACKET) {
        parser_set_error(
            parser,
            opening.position,
            "missing closing bracket"
        );

        return NULL;
    }

    if (item_count == 0) {
        parser_set_error(
            parser,
            opening.position,
            "character class cannot be empty"
        );

        return NULL;
    }
    parser_advance(parser);

    charset_set_negated(
        &set,
        negated
    );

    {
        AstNode *node;

        node = ast_make_character_class(
            &set,
            opening.position
        );

        if (node == NULL) {
            parser_set_error(
                parser,
                opening.position,
                "failed to allocate character class"
            );
        }

        return node;
    }
}

static AstNode *parse_atom(Parser *parser)
{
    AstNode *node;
    Token token;
    CharSet set;

    if (parser->failed) {
        return NULL;
    }

    token = parser->current;

    switch (token.type) {
        case TOKEN_LITERAL:
            parser_advance(parser);

            node = ast_make_literal(
                token.character,
                token.position
            );

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate literal AST node"
                );
            }

            return node;

        case TOKEN_DOT:
            parser_advance(parser);

            node = ast_make_dot(token.position);

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate wildcard AST node"
                );
            }

            return node;

        case TOKEN_LBRACKET:
            return parse_character_class(parser);

        case TOKEN_CARET:
            parser_advance(parser);

            node = ast_make_assertion(
                AST_ASSERT_START,
                token.position
            );

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate start assertion"
                );
            }

            return node;

        case TOKEN_DOLLAR:
            parser_advance(parser);

            node = ast_make_assertion(
                AST_ASSERT_END,
                token.position
            );

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate end assertion"
                );
            }

            return node;

        case TOKEN_DIGIT_CLASS:
            parser_advance(parser);

            set = charset_digit();

            node = ast_make_character_class(
                &set,
                token.position
            );

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate digit class"
                );
            }

            return node;

        case TOKEN_WORD_CLASS:
            parser_advance(parser);

            set = charset_word();

            node = ast_make_character_class(
                &set,
                token.position
            );

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate word class"
                );
            }

            return node;

        case TOKEN_SPACE_CLASS:
            parser_advance(parser);

            set = charset_space();

            node = ast_make_character_class(
                &set,
                token.position
            );

            if (node == NULL) {
                parser_set_error(
                    parser,
                    token.position,
                    "failed to allocate whitespace class"
                );
            }

            return node;

        case TOKEN_LPAREN: {
            size_t opening_position;

            opening_position = token.position;

            parser_advance(parser);

            if (
                parser->current.type == TOKEN_RPAREN ||
                parser->current.type == TOKEN_END
            ) {
                parser_set_error(
                    parser,
                    opening_position,
                    "empty or incomplete group"
                );

                return NULL;
            }

            node = parse_alternation(parser);

            if (node == NULL) {
                return NULL;
            }

            if (parser->current.type != TOKEN_RPAREN) {
                ast_free(node);

                parser_set_error(
                    parser,
                    opening_position,
                    "missing closing parenthesis"
                );

                return NULL;
            }

            parser_advance(parser);

            return node;
        }

        case TOKEN_STAR:
        case TOKEN_PLUS:
        case TOKEN_QUESTION:
            parser_set_error(
                parser,
                token.position,
                "quantifier has no preceding expression"
            );

            return NULL;

        case TOKEN_RPAREN:
            parser_set_error(
                parser,
                token.position,
                "unexpected closing parenthesis"
            );

            return NULL;

        case TOKEN_RBRACKET:
            parser_set_error(
                parser,
                token.position,
                "unexpected closing bracket"
            );

            return NULL;

        case TOKEN_DASH:
            parser_set_error(
                parser,
                token.position,
                "unexpected range operator"
            );

            return NULL;

        case TOKEN_PIPE:
            parser_set_error(
                parser,
                token.position,
                "alternation operator has no left operand"
            );

            return NULL;

        case TOKEN_END:
            parser_set_error(
                parser,
                token.position,
                "expected an expression"
            );

            return NULL;

        case TOKEN_ERROR:
            parser_set_error(
                parser,
                token.position,
                token.error_message
            );

            return NULL;
    }

    parser_set_error(
        parser,
        token.position,
        "unexpected token"
    );

    return NULL;
}

static AstNode *parse_repetition(Parser *parser)
{
    AstNode *node;
    AstType type;
    size_t operator_position;

    node = parse_atom(parser);

    if (node == NULL) {
        return NULL;
    }

    switch (parser->current.type) {
        case TOKEN_STAR:
            type = AST_STAR;
            break;

        case TOKEN_PLUS:
            type = AST_PLUS;
            break;

        case TOKEN_QUESTION:
            type = AST_OPTIONAL;
            break;

        default:
            return node;
    }

    operator_position =
        parser->current.position;

    parser_advance(parser);

    node = make_unary_node(
        parser,
        type,
        node,
        operator_position
    );

    if (node == NULL) {
        return NULL;
    }
    if (
        parser->current.type == TOKEN_STAR ||
        parser->current.type == TOKEN_PLUS ||
        parser->current.type == TOKEN_QUESTION
    ) {
        ast_free(node);

        parser_set_error(
            parser,
            parser->current.position,
            "multiple consecutive quantifiers are not supported"
        );

        return NULL;
    }

    return node;
}

static AstNode *parse_concatenation(Parser *parser)
{
    AstNode *left;

    if (!token_starts_atom(parser->current.type)) {
        parser_set_error(
            parser,
            parser->current.position,
            "expected an expression"
        );

        return NULL;
    }

    left = parse_repetition(parser);

    if (left == NULL) {
        return NULL;
    }

    while (
        token_starts_atom(
            parser->current.type
        )
    ) {
        AstNode *right;
        size_t position;

        position =
            parser->current.position;

        right = parse_repetition(parser);

        if (right == NULL) {
            ast_free(left);
            return NULL;
        }

        left = make_binary_node(
            parser,
            AST_CONCATENATION,
            left,
            right,
            position
        );

        if (left == NULL) {
            return NULL;
        }
    }

    return left;
}

static AstNode *parse_alternation(Parser *parser)
{
    AstNode *left;

    left = parse_concatenation(parser);

    if (left == NULL) {
        return NULL;
    }

    while (parser->current.type == TOKEN_PIPE) {
        AstNode *right;
        size_t pipe_position;

        pipe_position =
            parser->current.position;

        parser_advance(parser);

        if (!token_starts_atom(parser->current.type)) {
            ast_free(left);

            parser_set_error(
                parser,
                pipe_position,
                "alternation operator has no right operand"
            );

            return NULL;
        }

        right = parse_concatenation(parser);

        if (right == NULL) {
            ast_free(left);
            return NULL;
        }

        left = make_binary_node(
            parser,
            AST_ALTERNATION,
            left,
            right,
            pipe_position
        );

        if (left == NULL) {
            return NULL;
        }
    }

    return left;
}

AstNode *parser_parse(
    const char *pattern,
    ParserError *error
)
{
    Parser parser;
    AstNode *root;

    lexer_init(
        &parser.lexer,
        pattern
    );

    parser.failed = 0;
    parser.error.position = 0U;
    parser.error.message = NULL;

    parser_advance(&parser);

    /*
     * The empty pattern matches the empty string.
     */
    if (
        !parser.failed &&
        parser.current.type == TOKEN_END
    ) {
        root = ast_make_empty(0U);

        if (root == NULL) {
            parser_set_error(
                &parser,
                0U,
                "failed to allocate empty AST node"
            );
        }
    } else {
        root = parse_alternation(&parser);
    }

    if (
        !parser.failed &&
        parser.current.type != TOKEN_END
    ) {
        ast_free(root);
        root = NULL;

        if (parser.current.type == TOKEN_RPAREN) {
            parser_set_error(
                &parser,
                parser.current.position,
                "unexpected closing parenthesis"
            );
        } else if (
            parser.current.type ==
            TOKEN_RBRACKET
        ) {
            parser_set_error(
                &parser,
                parser.current.position,
                "unexpected closing bracket"
            );
        } else {
            parser_set_error(
                &parser,
                parser.current.position,
                "unexpected token after expression"
            );
        }
    }

    if (parser.failed) {
        ast_free(root);
        root = NULL;
    }

    if (error != NULL) {
        *error = parser.error;
    }

    return root;
}