#include "cregex/parse_tree.h"

#include <ctype.h>
#include <stdlib.h>

ParseTreeNode *parse_tree_node_create(
    GrammarSymbol symbol
)
{
    ParseTreeNode *node;

    node = (ParseTreeNode *) calloc(
        1U,
        sizeof(ParseTreeNode)
    );

    if (node == NULL) {
        return NULL;
    }

    node->production = PARSE_PRODUCTION_NONE;

    switch (symbol.type) {
        case GRAMMAR_SYMBOL_TERMINAL:
            node->type = PARSE_TREE_TERMINAL;
            node->symbol.terminal = symbol.value.terminal;
            break;

        case GRAMMAR_SYMBOL_NONTERMINAL:
            node->type = PARSE_TREE_NONTERMINAL;
            node->symbol.nonterminal = symbol.value.nonterminal;
            break;

        case GRAMMAR_SYMBOL_EPSILON:
            node->type = PARSE_TREE_EPSILON;
            break;
    }

    return node;
}

int parse_tree_node_allocate_children(
    ParseTreeNode *node,
    size_t count
)
{
    if (node == NULL || node->children != NULL) {
        return 0;
    }

    if (count == 0U) {
        node->child_count = 0U;
        return 1;
    }

    node->children = (ParseTreeNode **) calloc(
        count,
        sizeof(ParseTreeNode *)
    );

    if (node->children == NULL) {
        return 0;
    }

    node->child_count = count;
    return 1;
}

static void parse_tree_print_indent(
    FILE *output,
    unsigned int depth
)
{
    unsigned int index;

    for (index = 0U; index < depth; index++) {
        fputs("  ", output);
    }
}

static void parse_tree_print_terminal(
    FILE *output,
    const ParseTreeNode *node
)
{
    fprintf(
        output,
        "%s",
        parse_terminal_name(node->symbol.terminal)
    );

    if (node->symbol.terminal == PARSE_TERMINAL_LITERAL) {
        if (isprint((int) node->token.character)) {
            fprintf(output, " '%c'", node->token.character);
        } else {
            fprintf(
                output,
                " 0x%02X",
                (unsigned int) node->token.character
            );
        }
    }

    fprintf(
        output,
        " @%lu\n",
        (unsigned long) node->token.position
    );
}

static void parse_tree_print_recursive(
    FILE *output,
    const ParseTreeNode *node,
    unsigned int depth
)
{
    size_t index;

    parse_tree_print_indent(output, depth);

    if (node == NULL) {
        fputs("<null>\n", output);
        return;
    }

    switch (node->type) {
        case PARSE_TREE_TERMINAL:
            parse_tree_print_terminal(output, node);
            return;

        case PARSE_TREE_NONTERMINAL:
            fprintf(
                output,
                "%s [%s]\n",
                parse_nonterminal_name(node->symbol.nonterminal),
                parse_production_name(node->production)
            );
            break;

        case PARSE_TREE_EPSILON:
            fputs("epsilon\n", output);
            return;
    }

    for (index = 0U; index < node->child_count; index++) {
        parse_tree_print_recursive(
            output,
            node->children[index],
            depth + 1U
        );
    }
}

void parse_tree_print(
    FILE *output,
    const ParseTreeNode *root
)
{
    if (output == NULL) {
        return;
    }

    parse_tree_print_recursive(output, root, 0U);
}

void parse_tree_free(ParseTreeNode *root)
{
    size_t index;

    if (root == NULL) {
        return;
    }

    for (index = 0U; index < root->child_count; index++) {
        parse_tree_free(root->children[index]);
    }

    free(root->children);
    free(root);
}