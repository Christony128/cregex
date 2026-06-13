#ifndef CREGEX_PARSE_TREE_H
#define CREGEX_PARSE_TREE_H

#include "cregex/charset.h"
#include "cregex/grammar.h"
#include "cregex/token.h"

#include <stddef.h>
#include <stdio.h>

typedef enum {
    PARSE_TREE_TERMINAL,
    PARSE_TREE_NONTERMINAL,
    PARSE_TREE_EPSILON
} ParseTreeNodeType;

typedef struct ParseTreeNode ParseTreeNode;

struct ParseTreeNode {
    ParseTreeNodeType type;
    ParseProduction production;

    union {
        ParseTerminal terminal;
        ParseNonterminal nonterminal;
    } symbol;

    Token token;
    CharSet character_class;
    int has_character_class;

    ParseTreeNode **children;
    size_t child_count;
};

ParseTreeNode *parse_tree_node_create(
    GrammarSymbol symbol
);

int parse_tree_node_allocate_children(
    ParseTreeNode *node,
    size_t count
);

void parse_tree_print(
    FILE *output,
    const ParseTreeNode *root
);

void parse_tree_free(ParseTreeNode *root);

#endif

