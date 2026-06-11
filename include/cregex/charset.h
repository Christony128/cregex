#ifndef CREGEX_CHARSET_H
#define CREGEX_CHARSET_H

#define CREGEX_CHARSET_SIZE 32

typedef struct {
    unsigned char bits[CREGEX_CHARSET_SIZE];
    int negated;
} CharSet;

void charset_clear(CharSet *set);

void charset_add_char(
    CharSet *set,
    unsigned char character
);

int charset_add_range(
    CharSet *set,
    unsigned char start,
    unsigned char end
);
void charset_add_set(
    CharSet *destination,
    const CharSet *source
);

void charset_set_negated(
    CharSet *set,
    int negated
);

int charset_contains(
    const CharSet *set,
    unsigned char character
);

CharSet charset_digit(void);

CharSet charset_word(void);

CharSet charset_space(void);

#endif