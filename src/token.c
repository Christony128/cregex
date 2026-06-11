#include "cregex/token.h"

const char *token_type_name(TokenType type)
{
    switch (type) {
        case TOKEN_LITERAL:
            return "LITERAL";

        case TOKEN_DOT:
            return "DOT";

        case TOKEN_PIPE:
            return "PIPE";

        case TOKEN_STAR:
            return "STAR";

        case TOKEN_PLUS:
            return "PLUS";

        case TOKEN_QUESTION:
            return "QUESTION";

        case TOKEN_LPAREN:
            return "LPAREN";

        case TOKEN_RPAREN:
            return "RPAREN";

        case TOKEN_LBRACKET:
            return "LBRACKET";

        case TOKEN_RBRACKET:
            return "RBRACKET";

        case TOKEN_DASH:
            return "DASH";

        case TOKEN_CARET:
            return "CARET";

        case TOKEN_DOLLAR:
            return "DOLLAR";

        case TOKEN_DIGIT_CLASS:
            return "DIGIT_CLASS";

        case TOKEN_WORD_CLASS:
            return "WORD_CLASS";

        case TOKEN_SPACE_CLASS:
            return "SPACE_CLASS";

        case TOKEN_END:
            return "END";

        case TOKEN_ERROR:
            return "ERROR";
    }

    return "UNKNOWN";
}