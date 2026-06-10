#include "cregex/lexer.h"

#include <string.h>

static Token make_token(
    TokenType type,
    unsigned char character,
    size_t position
)
{
    Token token;

    token.type = type;
    token.character = character;
    token.position = position;
    token.error_message = NULL;

    return token;
}

static Token make_error_token(
    size_t position,
    const char *message
)
{
    Token token;

    token.type = TOKEN_ERROR;
    token.character = '\0';
    token.position = position;
    token.error_message = message;

    return token;
}

void lexer_init(Lexer *lexer, const char *pattern)
{
    if (lexer == NULL) {
        return;
    }

    if (pattern == NULL) {
        pattern = "";
    }

    lexer->pattern = pattern;
    lexer->length = strlen(pattern);
    lexer->position = 0;
}

static Token lexer_scan(Lexer *lexer)
{
    size_t token_position;
    unsigned char current;

    if (lexer->position >= lexer->length) {
        return make_token(
            TOKEN_END,
            '\0',
            lexer->position
        );
    }

    token_position = lexer->position;

    current = (unsigned char) lexer->pattern[lexer->position];
    lexer->position++;

    switch (current) {
        case '.':
            return make_token(TOKEN_DOT, current, token_position);

        case '|':
            return make_token(TOKEN_PIPE, current, token_position);

        case '*':
            return make_token(TOKEN_STAR, current, token_position);

        case '+':
            return make_token(TOKEN_PLUS, current, token_position);

        case '?':
            return make_token(TOKEN_QUESTION, current, token_position);

        case '(':
            return make_token(TOKEN_LPAREN, current, token_position);

        case ')':
            return make_token(TOKEN_RPAREN, current, token_position);

        case '^':
            return make_token(TOKEN_CARET, current, token_position);

        case '$':
            return make_token(TOKEN_DOLLAR, current, token_position);

        case '\\': {
            unsigned char escaped;

            if (lexer->position >= lexer->length) {
                return make_error_token(
                    token_position,
                    "pattern ends with an incomplete escape"
                );
            }

            escaped =
                (unsigned char) lexer->pattern[lexer->position];

            lexer->position++;

            switch (escaped) {
                case 'd':
                    return make_token(
                        TOKEN_DIGIT_CLASS,
                        escaped,
                        token_position
                    );

                case 'w':
                    return make_token(
                        TOKEN_WORD_CLASS,
                        escaped,
                        token_position
                    );

                case 's':
                    return make_token(
                        TOKEN_SPACE_CLASS,
                        escaped,
                        token_position
                    );

                default:
                    return make_token(
                        TOKEN_LITERAL,
                        escaped,
                        token_position
                    );
            }
        }

        default:
            return make_token(
                TOKEN_LITERAL,
                current,
                token_position
            );
    }
}

Token lexer_next(Lexer *lexer)
{
    if (lexer == NULL) {
        return make_error_token(
            0,
            "lexer_next received a null lexer"
        );
    }

    return lexer_scan(lexer);
}

Token lexer_peek(const Lexer *lexer)
{
    Lexer copy;

    if (lexer == NULL) {
        return make_error_token(
            0,
            "lexer_peek received a null lexer"
        );
    }

    copy = *lexer;

    return lexer_scan(&copy);
}