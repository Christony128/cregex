#include "cregex/charset.h"

#include <string.h>

static unsigned int charset_byte_index(
    unsigned char character
)
{
    return (unsigned int) character / 8U;
}

static unsigned int charset_bit_index(
    unsigned char character
)
{
    return (unsigned int) character % 8U;
}

void charset_clear(CharSet *set)
{
    if (set == NULL) {
        return;
    }

    memset(
        set->bits,
        0,
        sizeof(set->bits)
    );

    set->negated = 0;
}

void charset_add_char(
    CharSet *set,
    unsigned char character
)
{
    unsigned int byte_index;
    unsigned int bit_index;
    unsigned char mask;

    if (set == NULL) {
        return;
    }

    byte_index =
        charset_byte_index(character);

    bit_index =
        charset_bit_index(character);

    mask =
        (unsigned char) (1U << bit_index);

    set->bits[byte_index] =
        (unsigned char) (
            set->bits[byte_index] | mask
        );
}

int charset_add_range(
    CharSet *set,
    unsigned char start,
    unsigned char end
)
{
    unsigned int value;

    if (set == NULL || start > end) {
        return 0;
    }

    for (
        value = (unsigned int) start;
        value <= (unsigned int) end;
        value++
    ) {
        charset_add_char(
            set,
            (unsigned char) value
        );
    }

    return 1;
}

void charset_add_set(
    CharSet *destination,
    const CharSet *source
)
{
    unsigned int value;

    if (
        destination == NULL ||
        source == NULL
    ) {
        return;
    }

    for (value = 0U; value <= 255U; value++) {
        unsigned char character;

        character = (unsigned char) value;

        if (
            charset_contains(
                source,
                character
            )
        ) {
            charset_add_char(
                destination,
                character
            );
        }
    }
}

void charset_set_negated(
    CharSet *set,
    int negated
)
{
    if (set == NULL) {
        return;
    }

    set->negated = negated != 0;
}

int charset_contains(
    const CharSet *set,
    unsigned char character
)
{
    unsigned int byte_index;
    unsigned int bit_index;
    unsigned char mask;
    int present;

    if (set == NULL) {
        return 0;
    }

    byte_index =
        charset_byte_index(character);

    bit_index =
        charset_bit_index(character);

    mask =
        (unsigned char) (1U << bit_index);

    present =
        (set->bits[byte_index] & mask) != 0;

    if (set->negated) {
        return !present;
    }

    return present;
}

CharSet charset_digit(void)
{
    CharSet set;

    charset_clear(&set);
    charset_add_range(&set, '0', '9');

    return set;
}

CharSet charset_word(void)
{
    CharSet set;

    charset_clear(&set);

    charset_add_range(&set, 'a', 'z');
    charset_add_range(&set, 'A', 'Z');
    charset_add_range(&set, '0', '9');
    charset_add_char(&set, '_');

    return set;
}

CharSet charset_space(void)
{
    CharSet set;

    charset_clear(&set);

    charset_add_char(&set, ' ');
    charset_add_char(&set, '\t');
    charset_add_char(&set, '\n');
    charset_add_char(&set, '\r');
    charset_add_char(&set, '\f');
    charset_add_char(&set, '\v');

    return set;
}