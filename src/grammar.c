#include "cregex/grammar.h"

#define TERMINAL(value) \
    { GRAMMAR_SYMBOL_TERMINAL, { .terminal = value } }

#define NONTERMINAL(value) \
    { GRAMMAR_SYMBOL_NONTERMINAL, { .nonterminal = value } }

#define EPSILON \
    { GRAMMAR_SYMBOL_EPSILON, { .terminal = PARSE_TERMINAL_END } }

static const GrammarSymbol production_regex_alternation_end[] = {
    NONTERMINAL(PARSE_NONTERMINAL_ALTERNATION),
    TERMINAL(PARSE_TERMINAL_END)
};

static const GrammarSymbol production_regex_end[] = {
    TERMINAL(PARSE_TERMINAL_END)
};

static const GrammarSymbol production_alternation[] = {
    NONTERMINAL(PARSE_NONTERMINAL_CONCATENATION),
    NONTERMINAL(PARSE_NONTERMINAL_ALTERNATION_TAIL)
};

static const GrammarSymbol production_alternation_tail_pipe[] = {
    TERMINAL(PARSE_TERMINAL_PIPE),
    NONTERMINAL(PARSE_NONTERMINAL_CONCATENATION),
    NONTERMINAL(PARSE_NONTERMINAL_ALTERNATION_TAIL)
};

static const GrammarSymbol production_alternation_tail_empty[] = {
    EPSILON
};

static const GrammarSymbol production_concatenation[] = {
    NONTERMINAL(PARSE_NONTERMINAL_REPETITION),
    NONTERMINAL(PARSE_NONTERMINAL_CONCATENATION_TAIL)
};

static const GrammarSymbol production_concatenation_tail_repetition[] = {
    NONTERMINAL(PARSE_NONTERMINAL_REPETITION),
    NONTERMINAL(PARSE_NONTERMINAL_CONCATENATION_TAIL)
};

static const GrammarSymbol production_concatenation_tail_empty[] = {
    EPSILON
};

static const GrammarSymbol production_repetition[] = {
    NONTERMINAL(PARSE_NONTERMINAL_ATOM),
    NONTERMINAL(PARSE_NONTERMINAL_QUANTIFIER)
};

static const GrammarSymbol production_quantifier_star[] = {
    TERMINAL(PARSE_TERMINAL_STAR)
};

static const GrammarSymbol production_quantifier_plus[] = {
    TERMINAL(PARSE_TERMINAL_PLUS)
};

static const GrammarSymbol production_quantifier_question[] = {
    TERMINAL(PARSE_TERMINAL_QUESTION)
};

static const GrammarSymbol production_quantifier_empty[] = {
    EPSILON
};

static const GrammarSymbol production_atom_literal[] = {
    TERMINAL(PARSE_TERMINAL_LITERAL)
};

static const GrammarSymbol production_atom_dot[] = {
    TERMINAL(PARSE_TERMINAL_DOT)
};

static const GrammarSymbol production_atom_class[] = {
    TERMINAL(PARSE_TERMINAL_CLASS)
};

static const GrammarSymbol production_atom_caret[] = {
    TERMINAL(PARSE_TERMINAL_CARET)
};

static const GrammarSymbol production_atom_dollar[] = {
    TERMINAL(PARSE_TERMINAL_DOLLAR)
};

static const GrammarSymbol production_atom_group[] = {
    TERMINAL(PARSE_TERMINAL_LPAREN),
    NONTERMINAL(PARSE_NONTERMINAL_ALTERNATION),
    TERMINAL(PARSE_TERMINAL_RPAREN)
};

static int terminal_starts_atom(ParseTerminal terminal)
{
    switch (terminal) {
        case PARSE_TERMINAL_LITERAL:
        case PARSE_TERMINAL_DOT:
        case PARSE_TERMINAL_CLASS:
        case PARSE_TERMINAL_CARET:
        case PARSE_TERMINAL_DOLLAR:
        case PARSE_TERMINAL_LPAREN:
            return 1;

        case PARSE_TERMINAL_RPAREN:
        case PARSE_TERMINAL_PIPE:
        case PARSE_TERMINAL_STAR:
        case PARSE_TERMINAL_PLUS:
        case PARSE_TERMINAL_QUESTION:
        case PARSE_TERMINAL_END:
        case PARSE_TERMINAL_COUNT:
            return 0;
    }

    return 0;
}

const char *parse_terminal_name(ParseTerminal terminal)
{
    switch (terminal) {
        case PARSE_TERMINAL_LITERAL:
            return "LITERAL";

        case PARSE_TERMINAL_DOT:
            return "DOT";

        case PARSE_TERMINAL_CLASS:
            return "CLASS";

        case PARSE_TERMINAL_CARET:
            return "CARET";

        case PARSE_TERMINAL_DOLLAR:
            return "DOLLAR";

        case PARSE_TERMINAL_LPAREN:
            return "LPAREN";

        case PARSE_TERMINAL_RPAREN:
            return "RPAREN";

        case PARSE_TERMINAL_PIPE:
            return "PIPE";

        case PARSE_TERMINAL_STAR:
            return "STAR";

        case PARSE_TERMINAL_PLUS:
            return "PLUS";

        case PARSE_TERMINAL_QUESTION:
            return "QUESTION";

        case PARSE_TERMINAL_END:
            return "END";

        case PARSE_TERMINAL_COUNT:
            return "INVALID_TERMINAL";
    }

    return "INVALID_TERMINAL";
}

const char *parse_nonterminal_name(ParseNonterminal nonterminal)
{
    switch (nonterminal) {
        case PARSE_NONTERMINAL_REGEX:
            return "Regex";

        case PARSE_NONTERMINAL_ALTERNATION:
            return "Alternation";

        case PARSE_NONTERMINAL_ALTERNATION_TAIL:
            return "AlternationTail";

        case PARSE_NONTERMINAL_CONCATENATION:
            return "Concatenation";

        case PARSE_NONTERMINAL_CONCATENATION_TAIL:
            return "ConcatenationTail";

        case PARSE_NONTERMINAL_REPETITION:
            return "Repetition";

        case PARSE_NONTERMINAL_QUANTIFIER:
            return "Quantifier";

        case PARSE_NONTERMINAL_ATOM:
            return "Atom";

        case PARSE_NONTERMINAL_COUNT:
            return "InvalidNonterminal";
    }

    return "InvalidNonterminal";
}

const char *parse_production_name(ParseProduction production)
{
    switch (production) {
        case PARSE_PRODUCTION_NONE:
            return "none";

        case PARSE_PRODUCTION_REGEX_ALTERNATION_END:
            return "Regex -> Alternation END";

        case PARSE_PRODUCTION_REGEX_END:
            return "Regex -> END";

        case PARSE_PRODUCTION_ALTERNATION:
            return "Alternation -> Concatenation AlternationTail";

        case PARSE_PRODUCTION_ALTERNATION_TAIL_PIPE:
            return "AlternationTail -> PIPE Concatenation AlternationTail";

        case PARSE_PRODUCTION_ALTERNATION_TAIL_EMPTY:
            return "AlternationTail -> epsilon";

        case PARSE_PRODUCTION_CONCATENATION:
            return "Concatenation -> Repetition ConcatenationTail";

        case PARSE_PRODUCTION_CONCATENATION_TAIL_REPETITION:
            return "ConcatenationTail -> Repetition ConcatenationTail";

        case PARSE_PRODUCTION_CONCATENATION_TAIL_EMPTY:
            return "ConcatenationTail -> epsilon";

        case PARSE_PRODUCTION_REPETITION:
            return "Repetition -> Atom Quantifier";

        case PARSE_PRODUCTION_QUANTIFIER_STAR:
            return "Quantifier -> STAR";

        case PARSE_PRODUCTION_QUANTIFIER_PLUS:
            return "Quantifier -> PLUS";

        case PARSE_PRODUCTION_QUANTIFIER_QUESTION:
            return "Quantifier -> QUESTION";

        case PARSE_PRODUCTION_QUANTIFIER_EMPTY:
            return "Quantifier -> epsilon";

        case PARSE_PRODUCTION_ATOM_LITERAL:
            return "Atom -> LITERAL";

        case PARSE_PRODUCTION_ATOM_DOT:
            return "Atom -> DOT";

        case PARSE_PRODUCTION_ATOM_CLASS:
            return "Atom -> CLASS";

        case PARSE_PRODUCTION_ATOM_CARET:
            return "Atom -> CARET";

        case PARSE_PRODUCTION_ATOM_DOLLAR:
            return "Atom -> DOLLAR";

        case PARSE_PRODUCTION_ATOM_GROUP:
            return "Atom -> LPAREN Alternation RPAREN";

        case PARSE_PRODUCTION_COUNT:
            return "invalid production";
    }

    return "invalid production";
}

ParseProduction grammar_table_lookup(
    ParseNonterminal nonterminal,
    ParseTerminal lookahead
)
{
    switch (nonterminal) {
        case PARSE_NONTERMINAL_REGEX:
            if (lookahead == PARSE_TERMINAL_END) {
                return PARSE_PRODUCTION_REGEX_END;
            }

            if (terminal_starts_atom(lookahead)) {
                return PARSE_PRODUCTION_REGEX_ALTERNATION_END;
            }

            break;

        case PARSE_NONTERMINAL_ALTERNATION:
            if (terminal_starts_atom(lookahead)) {
                return PARSE_PRODUCTION_ALTERNATION;
            }

            break;

        case PARSE_NONTERMINAL_ALTERNATION_TAIL:
            if (lookahead == PARSE_TERMINAL_PIPE) {
                return PARSE_PRODUCTION_ALTERNATION_TAIL_PIPE;
            }

            if (
                lookahead == PARSE_TERMINAL_RPAREN ||
                lookahead == PARSE_TERMINAL_END
            ) {
                return PARSE_PRODUCTION_ALTERNATION_TAIL_EMPTY;
            }

            break;

        case PARSE_NONTERMINAL_CONCATENATION:
            if (terminal_starts_atom(lookahead)) {
                return PARSE_PRODUCTION_CONCATENATION;
            }

            break;

        case PARSE_NONTERMINAL_CONCATENATION_TAIL:
            if (terminal_starts_atom(lookahead)) {
                return PARSE_PRODUCTION_CONCATENATION_TAIL_REPETITION;
            }

            if (
                lookahead == PARSE_TERMINAL_PIPE ||
                lookahead == PARSE_TERMINAL_RPAREN ||
                lookahead == PARSE_TERMINAL_END
            ) {
                return PARSE_PRODUCTION_CONCATENATION_TAIL_EMPTY;
            }

            break;

        case PARSE_NONTERMINAL_REPETITION:
            if (terminal_starts_atom(lookahead)) {
                return PARSE_PRODUCTION_REPETITION;
            }

            break;

        case PARSE_NONTERMINAL_QUANTIFIER:
            switch (lookahead) {
                case PARSE_TERMINAL_STAR:
                    return PARSE_PRODUCTION_QUANTIFIER_STAR;

                case PARSE_TERMINAL_PLUS:
                    return PARSE_PRODUCTION_QUANTIFIER_PLUS;

                case PARSE_TERMINAL_QUESTION:
                    return PARSE_PRODUCTION_QUANTIFIER_QUESTION;

                default:
                    if (
                        terminal_starts_atom(lookahead) ||
                        lookahead == PARSE_TERMINAL_PIPE ||
                        lookahead == PARSE_TERMINAL_RPAREN ||
                        lookahead == PARSE_TERMINAL_END
                    ) {
                        return PARSE_PRODUCTION_QUANTIFIER_EMPTY;
                    }

                    break;
            }

            break;

        case PARSE_NONTERMINAL_ATOM:
            switch (lookahead) {
                case PARSE_TERMINAL_LITERAL:
                    return PARSE_PRODUCTION_ATOM_LITERAL;

                case PARSE_TERMINAL_DOT:
                    return PARSE_PRODUCTION_ATOM_DOT;

                case PARSE_TERMINAL_CLASS:
                    return PARSE_PRODUCTION_ATOM_CLASS;

                case PARSE_TERMINAL_CARET:
                    return PARSE_PRODUCTION_ATOM_CARET;

                case PARSE_TERMINAL_DOLLAR:
                    return PARSE_PRODUCTION_ATOM_DOLLAR;

                case PARSE_TERMINAL_LPAREN:
                    return PARSE_PRODUCTION_ATOM_GROUP;

                default:
                    break;
            }

            break;

        case PARSE_NONTERMINAL_COUNT:
            break;
    }

    return PARSE_PRODUCTION_NONE;
}

const GrammarSymbol *grammar_production_symbols(
    ParseProduction production,
    size_t *count
)
{
    if (count == NULL) {
        return NULL;
    }

    switch (production) {
        case PARSE_PRODUCTION_REGEX_ALTERNATION_END:
            *count = sizeof(production_regex_alternation_end) /
                sizeof(production_regex_alternation_end[0]);
            return production_regex_alternation_end;

        case PARSE_PRODUCTION_REGEX_END:
            *count = sizeof(production_regex_end) /
                sizeof(production_regex_end[0]);
            return production_regex_end;

        case PARSE_PRODUCTION_ALTERNATION:
            *count = sizeof(production_alternation) /
                sizeof(production_alternation[0]);
            return production_alternation;

        case PARSE_PRODUCTION_ALTERNATION_TAIL_PIPE:
            *count = sizeof(production_alternation_tail_pipe) /
                sizeof(production_alternation_tail_pipe[0]);
            return production_alternation_tail_pipe;

        case PARSE_PRODUCTION_ALTERNATION_TAIL_EMPTY:
            *count = sizeof(production_alternation_tail_empty) /
                sizeof(production_alternation_tail_empty[0]);
            return production_alternation_tail_empty;

        case PARSE_PRODUCTION_CONCATENATION:
            *count = sizeof(production_concatenation) /
                sizeof(production_concatenation[0]);
            return production_concatenation;

        case PARSE_PRODUCTION_CONCATENATION_TAIL_REPETITION:
            *count = sizeof(production_concatenation_tail_repetition) /
                sizeof(production_concatenation_tail_repetition[0]);
            return production_concatenation_tail_repetition;

        case PARSE_PRODUCTION_CONCATENATION_TAIL_EMPTY:
            *count = sizeof(production_concatenation_tail_empty) /
                sizeof(production_concatenation_tail_empty[0]);
            return production_concatenation_tail_empty;

        case PARSE_PRODUCTION_REPETITION:
            *count = sizeof(production_repetition) /
                sizeof(production_repetition[0]);
            return production_repetition;

        case PARSE_PRODUCTION_QUANTIFIER_STAR:
            *count = sizeof(production_quantifier_star) /
                sizeof(production_quantifier_star[0]);
            return production_quantifier_star;

        case PARSE_PRODUCTION_QUANTIFIER_PLUS:
            *count = sizeof(production_quantifier_plus) /
                sizeof(production_quantifier_plus[0]);
            return production_quantifier_plus;

        case PARSE_PRODUCTION_QUANTIFIER_QUESTION:
            *count = sizeof(production_quantifier_question) /
                sizeof(production_quantifier_question[0]);
            return production_quantifier_question;

        case PARSE_PRODUCTION_QUANTIFIER_EMPTY:
            *count = sizeof(production_quantifier_empty) /
                sizeof(production_quantifier_empty[0]);
            return production_quantifier_empty;

        case PARSE_PRODUCTION_ATOM_LITERAL:
            *count = sizeof(production_atom_literal) /
                sizeof(production_atom_literal[0]);
            return production_atom_literal;

        case PARSE_PRODUCTION_ATOM_DOT:
            *count = sizeof(production_atom_dot) /
                sizeof(production_atom_dot[0]);
            return production_atom_dot;

        case PARSE_PRODUCTION_ATOM_CLASS:
            *count = sizeof(production_atom_class) /
                sizeof(production_atom_class[0]);
            return production_atom_class;

        case PARSE_PRODUCTION_ATOM_CARET:
            *count = sizeof(production_atom_caret) /
                sizeof(production_atom_caret[0]);
            return production_atom_caret;

        case PARSE_PRODUCTION_ATOM_DOLLAR:
            *count = sizeof(production_atom_dollar) /
                sizeof(production_atom_dollar[0]);
            return production_atom_dollar;

        case PARSE_PRODUCTION_ATOM_GROUP:
            *count = sizeof(production_atom_group) /
                sizeof(production_atom_group[0]);
            return production_atom_group;

        case PARSE_PRODUCTION_NONE:
        case PARSE_PRODUCTION_COUNT:
            *count = 0U;
            return NULL;
    }

    *count = 0U;
    return NULL;
}
