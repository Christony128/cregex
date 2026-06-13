#include "cregex/parser.h"

#include "cregex/grammar.h"
#include "cregex/lexer.h"
#include "cregex/token.h"

#include <stdlib.h>

typedef struct {
    ParseTerminal terminal;
    Token token;
    CharSet character_class;
    int has_character_class;
} ParserToken;

typedef struct {
    ParseTreeNode **items;
    size_t count;
    size_t capacity;
} ParseStack;

typedef struct {
    Lexer lexer;
    ParserToken current;
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

static int parser_raw_token_to_character(
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

static void parser_add_shorthand_class(
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

    charset_add_set(destination, &source);
}

static int parser_scan_character_class(
    Parser *parser,
    Token opening,
    ParserToken *result
)
{
    CharSet set;
    Token current;
    int item_count;
    int negated;

    charset_clear(&set);
    item_count = 0;
    negated = 0;

    current = lexer_next(&parser->lexer);

    if (current.type == TOKEN_ERROR) {
        parser_set_error(
            parser,
            current.position,
            current.error_message
        );
        return 0;
    }

    if (current.type == TOKEN_CARET) {
        negated = 1;
        current = lexer_next(&parser->lexer);
    }

    if (current.type == TOKEN_RBRACKET) {
        parser_set_error(
            parser,
            opening.position,
            "character class cannot be empty"
        );
        return 0;
    }

    while (current.type != TOKEN_RBRACKET) {
        Token first_token;

        if (current.type == TOKEN_ERROR) {
            parser_set_error(
                parser,
                current.position,
                current.error_message
            );
            return 0;
        }

        if (current.type == TOKEN_END) {
            parser_set_error(
                parser,
                opening.position,
                "missing closing bracket"
            );
            return 0;
        }

        first_token = current;

        if (
            first_token.type == TOKEN_DIGIT_CLASS ||
            first_token.type == TOKEN_WORD_CLASS ||
            first_token.type == TOKEN_SPACE_CLASS
        ) {
            parser_add_shorthand_class(
                &set,
                first_token.type
            );
            item_count++;
            current = lexer_next(&parser->lexer);
            continue;
        }

        {
            unsigned char first_character;

            if (
                !parser_raw_token_to_character(
                    first_token,
                    &first_character
                )
            ) {
                parser_set_error(
                    parser,
                    first_token.position,
                    "invalid token inside character class"
                );
                return 0;
            }

            current = lexer_next(&parser->lexer);

            if (current.type == TOKEN_DASH) {
                Token after_dash;

                after_dash = lexer_peek(&parser->lexer);

                if (after_dash.type == TOKEN_RBRACKET) {
                    charset_add_char(&set, first_character);
                    charset_add_char(&set, '-');
                    item_count += 2;
                    current = lexer_next(&parser->lexer);
                    continue;
                }

                current = lexer_next(&parser->lexer);

                {
                    unsigned char last_character;

                    if (
                        !parser_raw_token_to_character(
                            current,
                            &last_character
                        )
                    ) {
                        parser_set_error(
                            parser,
                            current.position,
                            "range endpoint must be a literal character"
                        );
                        return 0;
                    }

                    if (first_character > last_character) {
                        parser_set_error(
                            parser,
                            first_token.position,
                            "character range is reversed"
                        );
                        return 0;
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
                        return 0;
                    }

                    item_count++;
                    current = lexer_next(&parser->lexer);
                }
            } else {
                charset_add_char(&set, first_character);
                item_count++;
            }
        }
    }

    if (item_count == 0) {
        parser_set_error(
            parser,
            opening.position,
            "character class cannot be empty"
        );
        return 0;
    }

    charset_set_negated(&set, negated);

    result->terminal = PARSE_TERMINAL_CLASS;
    result->token = opening;
    result->character_class = set;
    result->has_character_class = 1;

    return 1;
}

static int parser_scan_token(
    Parser *parser,
    ParserToken *result
)
{
    Token token;

    token = lexer_next(&parser->lexer);

    result->token = token;
    result->has_character_class = 0;
    charset_clear(&result->character_class);

    if (token.type == TOKEN_ERROR) {
        parser_set_error(
            parser,
            token.position,
            token.error_message
        );
        return 0;
    }

    switch (token.type) {
        case TOKEN_LITERAL:
            result->terminal = PARSE_TERMINAL_LITERAL;
            return 1;

        case TOKEN_DOT:
            result->terminal = PARSE_TERMINAL_DOT;
            return 1;

        case TOKEN_LBRACKET:
            return parser_scan_character_class(
                parser,
                token,
                result
            );

        case TOKEN_DIGIT_CLASS:
            result->terminal = PARSE_TERMINAL_CLASS;
            result->character_class = charset_digit();
            result->has_character_class = 1;
            return 1;

        case TOKEN_WORD_CLASS:
            result->terminal = PARSE_TERMINAL_CLASS;
            result->character_class = charset_word();
            result->has_character_class = 1;
            return 1;

        case TOKEN_SPACE_CLASS:
            result->terminal = PARSE_TERMINAL_CLASS;
            result->character_class = charset_space();
            result->has_character_class = 1;
            return 1;

        case TOKEN_CARET:
            result->terminal = PARSE_TERMINAL_CARET;
            return 1;

        case TOKEN_DOLLAR:
            result->terminal = PARSE_TERMINAL_DOLLAR;
            return 1;

        case TOKEN_LPAREN:
            result->terminal = PARSE_TERMINAL_LPAREN;
            return 1;

        case TOKEN_RPAREN:
            result->terminal = PARSE_TERMINAL_RPAREN;
            return 1;

        case TOKEN_PIPE:
            result->terminal = PARSE_TERMINAL_PIPE;
            return 1;

        case TOKEN_STAR:
            result->terminal = PARSE_TERMINAL_STAR;
            return 1;

        case TOKEN_PLUS:
            result->terminal = PARSE_TERMINAL_PLUS;
            return 1;

        case TOKEN_QUESTION:
            result->terminal = PARSE_TERMINAL_QUESTION;
            return 1;

        case TOKEN_END:
            result->terminal = PARSE_TERMINAL_END;
            return 1;

        case TOKEN_RBRACKET:
            parser_set_error(
                parser,
                token.position,
                "unexpected closing bracket"
            );
            return 0;

        case TOKEN_DASH:
            parser_set_error(
                parser,
                token.position,
                "unexpected range operator"
            );
            return 0;

        case TOKEN_ERROR:
            break;
    }

    parser_set_error(
        parser,
        token.position,
        "unexpected token"
    );
    return 0;
}

static void parser_advance(Parser *parser)
{
    if (parser->failed) {
        return;
    }

    parser_scan_token(parser, &parser->current);
}

static int parse_stack_push(
    ParseStack *stack,
    ParseTreeNode *node
)
{
    ParseTreeNode **resized;
    size_t new_capacity;

    if (stack->count < stack->capacity) {
        stack->items[stack->count] = node;
        stack->count++;
        return 1;
    }

    if (stack->capacity == 0U) {
        new_capacity = 16U;
    } else {
        if (
            stack->capacity >
            ((size_t) -1) / 2U
        ) {
            return 0;
        }

        new_capacity = stack->capacity * 2U;
    }

    if (
        new_capacity >
        ((size_t) -1) / sizeof(ParseTreeNode *)
    ) {
        return 0;
    }

    resized = (ParseTreeNode **) realloc(
        stack->items,
        new_capacity * sizeof(ParseTreeNode *)
    );

    if (resized == NULL) {
        return 0;
    }

    stack->items = resized;
    stack->capacity = new_capacity;
    stack->items[stack->count] = node;
    stack->count++;

    return 1;
}

static ParseTreeNode *parse_stack_pop(ParseStack *stack)
{
    if (stack->count == 0U) {
        return NULL;
    }

    stack->count--;
    return stack->items[stack->count];
}

static void parser_set_table_error(
    Parser *parser,
    ParseNonterminal nonterminal
)
{
    ParseTerminal terminal;

    terminal = parser->current.terminal;

    if (
        terminal == PARSE_TERMINAL_STAR ||
        terminal == PARSE_TERMINAL_PLUS ||
        terminal == PARSE_TERMINAL_QUESTION
    ) {
        if (
            nonterminal == PARSE_NONTERMINAL_CONCATENATION_TAIL ||
            nonterminal == PARSE_NONTERMINAL_ALTERNATION_TAIL
        ) {
            parser_set_error(
                parser,
                parser->current.token.position,
                "multiple consecutive quantifiers are not supported"
            );
        } else {
            parser_set_error(
                parser,
                parser->current.token.position,
                "quantifier has no preceding expression"
            );
        }
        return;
    }

    if (terminal == PARSE_TERMINAL_PIPE) {
        parser_set_error(
            parser,
            parser->current.token.position,
            "alternation operator is missing an operand"
        );
        return;
    }

    if (terminal == PARSE_TERMINAL_RPAREN) {
        if (
            nonterminal == PARSE_NONTERMINAL_ALTERNATION ||
            nonterminal == PARSE_NONTERMINAL_CONCATENATION
        ) {
            parser_set_error(
                parser,
                parser->current.token.position,
                "empty or incomplete group"
            );
        } else {
            parser_set_error(
                parser,
                parser->current.token.position,
                "unexpected closing parenthesis"
            );
        }
        return;
    }

    if (terminal == PARSE_TERMINAL_END) {
        parser_set_error(
            parser,
            parser->current.token.position,
            "expected an expression"
        );
        return;
    }

    parser_set_error(
        parser,
        parser->current.token.position,
        "unexpected token in regex expression"
    );
}

static void parser_set_terminal_error(
    Parser *parser,
    ParseTerminal expected
)
{
    if (
        expected == PARSE_TERMINAL_RPAREN &&
        parser->current.terminal == PARSE_TERMINAL_END
    ) {
        parser_set_error(
            parser,
            parser->current.token.position,
            "missing closing parenthesis"
        );
        return;
    }

    if (
        expected == PARSE_TERMINAL_END &&
        parser->current.terminal == PARSE_TERMINAL_RPAREN
    ) {
        parser_set_error(
            parser,
            parser->current.token.position,
            "unexpected closing parenthesis"
        );
        return;
    }

    parser_set_error(
        parser,
        parser->current.token.position,
        "unexpected token"
    );
}

static int parser_expand_node(
    Parser *parser,
    ParseStack *stack,
    ParseTreeNode *node,
    ParseProduction production
)
{
    const GrammarSymbol *symbols;
    size_t count;
    size_t index;

    symbols = grammar_production_symbols(
        production,
        &count
    );

    if (symbols == NULL || count == 0U) {
        parser_set_error(
            parser,
            parser->current.token.position,
            "invalid grammar production"
        );
        return 0;
    }

    if (!parse_tree_node_allocate_children(node, count)) {
        parser_set_error(
            parser,
            parser->current.token.position,
            "failed to allocate parse tree children"
        );
        return 0;
    }

    node->production = production;

    for (index = 0U; index < count; index++) {
        node->children[index] = parse_tree_node_create(
            symbols[index]
        );

        if (node->children[index] == NULL) {
            parser_set_error(
                parser,
                parser->current.token.position,
                "failed to allocate parse tree node"
            );
            return 0;
        }
    }

    for (index = count; index > 0U; index--) {
        ParseTreeNode *child;

        child = node->children[index - 1U];

        if (
            child->type != PARSE_TREE_EPSILON &&
            !parse_stack_push(stack, child)
        ) {
            parser_set_error(
                parser,
                parser->current.token.position,
                "failed to grow parser stack"
            );
            return 0;
        }
    }

    return 1;
}

ParseTreeNode *parser_parse_tree(
    const char *pattern,
    ParserError *error
)
{
    GrammarSymbol root_symbol;
    ParseTreeNode *root;
    ParseStack stack;
    Parser parser;

    lexer_init(&parser.lexer, pattern);
    parser.failed = 0;
    parser.error.position = 0U;
    parser.error.message = NULL;

    stack.items = NULL;
    stack.count = 0U;
    stack.capacity = 0U;

    root_symbol.type = GRAMMAR_SYMBOL_NONTERMINAL;
    root_symbol.value.nonterminal = PARSE_NONTERMINAL_REGEX;

    root = parse_tree_node_create(root_symbol);

    if (root == NULL) {
        parser_set_error(
            &parser,
            0U,
            "failed to allocate parse tree root"
        );
    }

    parser_advance(&parser);

    if (
        !parser.failed &&
        !parse_stack_push(&stack, root)
    ) {
        parser_set_error(
            &parser,
            0U,
            "failed to allocate parser stack"
        );
    }

    while (!parser.failed && stack.count > 0U) {
        ParseTreeNode *node;

        node = parse_stack_pop(&stack);

        if (node->type == PARSE_TREE_TERMINAL) {
            if (node->symbol.terminal != parser.current.terminal) {
                parser_set_terminal_error(
                    &parser,
                    node->symbol.terminal
                );
                break;
            }

            node->token = parser.current.token;
            node->has_character_class =
                parser.current.has_character_class;

            if (parser.current.has_character_class) {
                node->character_class =
                    parser.current.character_class;
            }

            if (node->symbol.terminal != PARSE_TERMINAL_END) {
                parser_advance(&parser);
            }
        } else if (node->type == PARSE_TREE_NONTERMINAL) {
            ParseProduction production;

            production = grammar_table_lookup(
                node->symbol.nonterminal,
                parser.current.terminal
            );

            if (production == PARSE_PRODUCTION_NONE) {
                parser_set_table_error(
                    &parser,
                    node->symbol.nonterminal
                );
                break;
            }

            parser_expand_node(
                &parser,
                &stack,
                node,
                production
            );
        }
    }

    free(stack.items);

    if (parser.failed) {
        parse_tree_free(root);
        root = NULL;
    }

    if (error != NULL) {
        *error = parser.error;
    }

    return root;
}

static void lower_set_error(
    ParserError *error,
    size_t position,
    const char *message
)
{
    if (error == NULL || error->message != NULL) {
        return;
    }

    error->position = position;
    error->message = message;
}

static AstNode *lower_make_binary(
    AstType type,
    AstNode *left,
    AstNode *right,
    size_t position,
    ParserError *error
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
        lower_set_error(
            error,
            position,
            "failed to allocate binary AST node"
        );
    }

    return node;
}

static AstNode *lower_alternation(
    const ParseTreeNode *node,
    ParserError *error
);

static AstNode *lower_atom(
    const ParseTreeNode *node,
    ParserError *error
)
{
    const ParseTreeNode *terminal;
    AstNode *result;

    if (
        node == NULL ||
        node->type != PARSE_TREE_NONTERMINAL ||
        node->symbol.nonterminal != PARSE_NONTERMINAL_ATOM
    ) {
        lower_set_error(error, 0U, "invalid atom parse tree");
        return NULL;
    }

    terminal = node->children[0];
    result = NULL;

    switch (node->production) {
        case PARSE_PRODUCTION_ATOM_LITERAL:
            result = ast_make_literal(
                terminal->token.character,
                terminal->token.position
            );
            break;

        case PARSE_PRODUCTION_ATOM_DOT:
            result = ast_make_dot(terminal->token.position);
            break;

        case PARSE_PRODUCTION_ATOM_CLASS:
            result = ast_make_character_class(
                &terminal->character_class,
                terminal->token.position
            );
            break;

        case PARSE_PRODUCTION_ATOM_CARET:
            result = ast_make_assertion(
                AST_ASSERT_START,
                terminal->token.position
            );
            break;

        case PARSE_PRODUCTION_ATOM_DOLLAR:
            result = ast_make_assertion(
                AST_ASSERT_END,
                terminal->token.position
            );
            break;

        case PARSE_PRODUCTION_ATOM_GROUP:
            return lower_alternation(
                node->children[1],
                error
            );

        default:
            lower_set_error(error, 0U, "invalid atom production");
            return NULL;
    }

    if (result == NULL) {
        lower_set_error(
            error,
            terminal->token.position,
            "failed to allocate atom AST node"
        );
    }

    return result;
}

static AstNode *lower_repetition(
    const ParseTreeNode *node,
    ParserError *error
)
{
    const ParseTreeNode *quantifier;
    const ParseTreeNode *operator_node;
    AstNode *child;
    AstNode *result;
    AstType type;

    if (
        node == NULL ||
        node->production != PARSE_PRODUCTION_REPETITION
    ) {
        lower_set_error(error, 0U, "invalid repetition parse tree");
        return NULL;
    }

    child = lower_atom(node->children[0], error);

    if (child == NULL) {
        return NULL;
    }

    quantifier = node->children[1];

    if (
        quantifier->production ==
        PARSE_PRODUCTION_QUANTIFIER_EMPTY
    ) {
        return child;
    }

    operator_node = quantifier->children[0];

    switch (quantifier->production) {
        case PARSE_PRODUCTION_QUANTIFIER_STAR:
            type = AST_STAR;
            break;

        case PARSE_PRODUCTION_QUANTIFIER_PLUS:
            type = AST_PLUS;
            break;

        case PARSE_PRODUCTION_QUANTIFIER_QUESTION:
            type = AST_OPTIONAL;
            break;

        default:
            ast_free(child);
            lower_set_error(
                error,
                operator_node->token.position,
                "invalid quantifier production"
            );
            return NULL;
    }

    result = ast_make_unary(
        type,
        child,
        operator_node->token.position
    );

    if (result == NULL) {
        ast_free(child);
        lower_set_error(
            error,
            operator_node->token.position,
            "failed to allocate quantifier AST node"
        );
    }

    return result;
}

static AstNode *lower_concatenation_tail(
    const ParseTreeNode *node,
    AstNode *left,
    ParserError *error
)
{
    AstNode *right;
    AstNode *combined;

    if (
        node->production ==
        PARSE_PRODUCTION_CONCATENATION_TAIL_EMPTY
    ) {
        return left;
    }

    if (
        node->production !=
        PARSE_PRODUCTION_CONCATENATION_TAIL_REPETITION
    ) {
        ast_free(left);
        lower_set_error(error, 0U, "invalid concatenation tail");
        return NULL;
    }

    right = lower_repetition(node->children[0], error);

    if (right == NULL) {
        ast_free(left);
        return NULL;
    }

    combined = lower_make_binary(
        AST_CONCATENATION,
        left,
        right,
        right->position,
        error
    );

    if (combined == NULL) {
        return NULL;
    }

    return lower_concatenation_tail(
        node->children[1],
        combined,
        error
    );
}

static AstNode *lower_concatenation(
    const ParseTreeNode *node,
    ParserError *error
)
{
    AstNode *left;

    if (
        node == NULL ||
        node->production != PARSE_PRODUCTION_CONCATENATION
    ) {
        lower_set_error(error, 0U, "invalid concatenation parse tree");
        return NULL;
    }

    left = lower_repetition(node->children[0], error);

    if (left == NULL) {
        return NULL;
    }

    return lower_concatenation_tail(
        node->children[1],
        left,
        error
    );
}

static AstNode *lower_alternation_tail(
    const ParseTreeNode *node,
    AstNode *left,
    ParserError *error
)
{
    AstNode *right;
    AstNode *combined;
    size_t position;

    if (
        node->production ==
        PARSE_PRODUCTION_ALTERNATION_TAIL_EMPTY
    ) {
        return left;
    }

    if (
        node->production !=
        PARSE_PRODUCTION_ALTERNATION_TAIL_PIPE
    ) {
        ast_free(left);
        lower_set_error(error, 0U, "invalid alternation tail");
        return NULL;
    }

    position = node->children[0]->token.position;
    right = lower_concatenation(node->children[1], error);

    if (right == NULL) {
        ast_free(left);
        return NULL;
    }

    combined = lower_make_binary(
        AST_ALTERNATION,
        left,
        right,
        position,
        error
    );

    if (combined == NULL) {
        return NULL;
    }

    return lower_alternation_tail(
        node->children[2],
        combined,
        error
    );
}

static AstNode *lower_alternation(
    const ParseTreeNode *node,
    ParserError *error
)
{
    AstNode *left;

    if (
        node == NULL ||
        node->production != PARSE_PRODUCTION_ALTERNATION
    ) {
        lower_set_error(error, 0U, "invalid alternation parse tree");
        return NULL;
    }

    left = lower_concatenation(node->children[0], error);

    if (left == NULL) {
        return NULL;
    }

    return lower_alternation_tail(
        node->children[1],
        left,
        error
    );
}

static AstNode *lower_parse_tree(
    const ParseTreeNode *root,
    ParserError *error
)
{
    if (
        root == NULL ||
        root->type != PARSE_TREE_NONTERMINAL ||
        root->symbol.nonterminal != PARSE_NONTERMINAL_REGEX
    ) {
        lower_set_error(error, 0U, "invalid parse tree root");
        return NULL;
    }

    if (root->production == PARSE_PRODUCTION_REGEX_END) {
        AstNode *empty;

        empty = ast_make_empty(0U);

        if (empty == NULL) {
            lower_set_error(
                error,
                0U,
                "failed to allocate empty AST node"
            );
        }

        return empty;
    }

    if (
        root->production !=
        PARSE_PRODUCTION_REGEX_ALTERNATION_END
    ) {
        lower_set_error(error, 0U, "invalid regex production");
        return NULL;
    }

    return lower_alternation(root->children[0], error);
}

AstNode *parser_parse(
    const char *pattern,
    ParserError *error
)
{
    ParserError local_error;
    ParseTreeNode *tree;
    AstNode *root;

    local_error.position = 0U;
    local_error.message = NULL;

    tree = parser_parse_tree(pattern, &local_error);

    if (tree == NULL) {
        if (error != NULL) {
            *error = local_error;
        }
        return NULL;
    }

    root = lower_parse_tree(tree, &local_error);
    parse_tree_free(tree);

    if (error != NULL) {
        *error = local_error;
    }

    return root;
}
