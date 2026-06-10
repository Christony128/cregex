#include "cregex/lexer.h"
#include "cregex/token.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static void print_token(Token token)
{
    printf(
        "%-14s position=%lu",
        token_type_name(token.type),
        (unsigned long) token.position
    );

    if (token.type == TOKEN_LITERAL) {
        if (isprint((int) token.character)) {
            printf(" value='%c'", token.character);
        } else {
            printf(
                " value=0x%02X",
                (unsigned int) token.character
            );
        }
    }

    if (token.type == TOKEN_ERROR) {
        printf(
            " message=\"%s\"",
            token.error_message != NULL
                ? token.error_message
                : "unknown lexer error"
        );
    }

    putchar('\n');
}

int main(int argc, char **argv)
{
    Lexer lexer;

    if (argc != 2) {
        fprintf(
            stderr,
            "Usage: %s <regex-pattern>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    lexer_init(&lexer, argv[1]);

    for (;;) {
        Token token = lexer_next(&lexer);

        print_token(token);

        if (token.type == TOKEN_ERROR) {
            return EXIT_FAILURE;
        }

        if (token.type == TOKEN_END) {
            break;
        }
    }

    return EXIT_SUCCESS;
}