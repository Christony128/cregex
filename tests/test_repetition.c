#include "cregex/ast.h"
#include "cregex/compiler.h"
#include "cregex/lexer.h"
#include "cregex/parser.h"
#include "cregex/regex.h"
#include "cregex/vm.h"

#include <assert.h>
#include <stdio.h>

static void expect_token(
    Lexer *lexer,
    TokenType type,
    unsigned char character
)
{
    Token token;

    token = lexer_next(lexer);
    assert(token.type == type);

    if (type == TOKEN_LITERAL) {
        assert(token.character == character);
    }
}

static AstNode *parse_successfully(const char *pattern)
{
    ParserError error;
    AstNode *root;

    root = parser_parse(pattern, &error);

    if (root == NULL) {
        fprintf(
            stderr,
            "Pattern \"%s\" failed at position %lu: %s\n",
            pattern,
            (unsigned long) error.position,
            error.message != NULL
                ? error.message
                : "unknown parser error"
        );
    }

    assert(root != NULL);
    return root;
}

static void expect_parse_failure(const char *pattern)
{
    ParserError error;
    AstNode *root;

    root = parser_parse(pattern, &error);

    assert(root == NULL);
    assert(error.message != NULL);
}

static NfaProgram compile_pattern(const char *pattern)
{
    ParserError parser_error;
    CompilerError compiler_error;
    AstNode *root;
    NfaProgram program;

    root = parser_parse(pattern, &parser_error);
    assert(root != NULL);

    nfa_program_init(&program);

    assert(
        compiler_compile(
            root,
            &program,
            &compiler_error
        )
    );

    ast_free(root);
    return program;
}

static void expect_full_match(
    const char *pattern,
    const char *text,
    int expected
)
{
    NfaProgram program;
    VmError error;
    int matched;

    program = compile_pattern(pattern);

    assert(
        nfa_vm_full_match(
            &program,
            text,
            &matched,
            &error
        )
    );

    assert(matched == expected);
    nfa_program_free(&program);
}

static void expect_search(
    const char *pattern,
    const char *text,
    size_t expected_start,
    size_t expected_end
)
{
    NfaProgram program;
    VmError error;
    NfaMatch match;
    int matched;

    program = compile_pattern(pattern);

    assert(
        nfa_vm_search(
            &program,
            text,
            &matched,
            &match,
            &error
        )
    );

    assert(matched);
    assert(match.start == expected_start);
    assert(match.end == expected_end);

    nfa_program_free(&program);
}

static void test_lexer_tokens(void)
{
    Lexer lexer;

    lexer_init(&lexer, "a{2,5}");

    expect_token(&lexer, TOKEN_LITERAL, 'a');
    expect_token(&lexer, TOKEN_LBRACE, '\0');
    expect_token(&lexer, TOKEN_LITERAL, '2');
    expect_token(&lexer, TOKEN_LITERAL, ',');
    expect_token(&lexer, TOKEN_LITERAL, '5');
    expect_token(&lexer, TOKEN_RBRACE, '\0');
    expect_token(&lexer, TOKEN_END, '\0');

    lexer_init(&lexer, "\\{[{}]\\}");

    expect_token(&lexer, TOKEN_LITERAL, '{');
    expect_token(&lexer, TOKEN_LBRACKET, '\0');
    expect_token(&lexer, TOKEN_LITERAL, '{');
    expect_token(&lexer, TOKEN_LITERAL, '}');
    expect_token(&lexer, TOKEN_RBRACKET, '\0');
    expect_token(&lexer, TOKEN_LITERAL, '}');
    expect_token(&lexer, TOKEN_END, '\0');
}

static void test_ast_repeat_node(void)
{
    AstNode *literal;
    AstNode *repeat;

    literal = ast_make_literal('a', 0U);
    assert(literal != NULL);

    repeat = ast_make_repeat(
        literal,
        2U,
        5U,
        1U
    );

    assert(repeat != NULL);
    assert(repeat->type == AST_REPEAT);
    assert(repeat->value.repetition.child == literal);
    assert(repeat->value.repetition.minimum == 2U);
    assert(repeat->value.repetition.maximum == 5U);

    ast_free(repeat);

    literal = ast_make_literal('a', 0U);
    assert(literal != NULL);

    assert(
        ast_make_repeat(
            literal,
            5U,
            2U,
            1U
        ) == NULL
    );

    ast_free(literal);
}

static void test_parser_counts(void)
{
    AstNode *root;

    root = parse_successfully("a{3}");
    assert(root->type == AST_REPEAT);
    assert(root->value.repetition.minimum == 3U);
    assert(root->value.repetition.maximum == 3U);
    ast_free(root);

    root = parse_successfully("(ab){2,4}");
    assert(root->type == AST_REPEAT);
    assert(root->value.repetition.minimum == 2U);
    assert(root->value.repetition.maximum == 4U);
    assert(
        root->value.repetition.child->type ==
        AST_CONCATENATION
    );
    ast_free(root);

    root = parse_successfully("[0-9]{2,}");
    assert(root->type == AST_REPEAT);
    assert(root->value.repetition.minimum == 2U);
    assert(
        root->value.repetition.maximum ==
        CREGEX_REPEAT_UNBOUNDED
    );
    ast_free(root);

    root = parse_successfully("a{0}");
    assert(root->type == AST_REPEAT);
    assert(root->value.repetition.minimum == 0U);
    assert(root->value.repetition.maximum == 0U);
    ast_free(root);
}

static void test_parser_errors(void)
{
    expect_parse_failure("{2}a");
    expect_parse_failure("a{}");
    expect_parse_failure("a{,2}");
    expect_parse_failure("a{2,1}");
    expect_parse_failure("a{2");
    expect_parse_failure("a{2,");
    expect_parse_failure("a{2,x}");
    expect_parse_failure("a{2,3");
    expect_parse_failure("a{2,,3}");
    expect_parse_failure("a{2}*");
    expect_parse_failure("a*{2}");
    expect_parse_failure("a{2}{3}");
    expect_parse_failure("a{1001}");
    expect_parse_failure("a{999999999999999999999999}");
    expect_parse_failure("a}");
}

static void test_compiler_shapes(void)
{
    NfaProgram program;

    program = compile_pattern("a{3}");
    assert(program.count == 4U);
    assert(program.states[0].type == NFA_STATE_CHAR);
    assert(program.states[1].type == NFA_STATE_CHAR);
    assert(program.states[2].type == NFA_STATE_CHAR);
    assert(program.states[3].type == NFA_STATE_MATCH);
    nfa_program_free(&program);

    program = compile_pattern("a{2,4}");
    assert(program.count == 7U);
    assert(program.states[3].type == NFA_STATE_SPLIT);
    assert(program.states[5].type == NFA_STATE_SPLIT);
    assert(program.states[6].type == NFA_STATE_MATCH);
    nfa_program_free(&program);

    program = compile_pattern("a{2,}");
    assert(program.count == 5U);
    assert(program.states[3].type == NFA_STATE_SPLIT);
    assert(program.states[4].type == NFA_STATE_MATCH);
    nfa_program_free(&program);

    program = compile_pattern("a{0}");
    assert(program.count == 2U);
    assert(program.states[program.start].type == NFA_STATE_JUMP);
    assert(program.states[1].type == NFA_STATE_MATCH);
    nfa_program_free(&program);
}

static void test_vm_exact_and_ranges(void)
{
    expect_full_match("a{3}", "aaa", 1);
    expect_full_match("a{3}", "aa", 0);
    expect_full_match("a{3}", "aaaa", 0);

    expect_full_match("a{2,4}", "aa", 1);
    expect_full_match("a{2,4}", "aaa", 1);
    expect_full_match("a{2,4}", "aaaa", 1);
    expect_full_match("a{2,4}", "a", 0);
    expect_full_match("a{2,4}", "aaaaa", 0);

    expect_full_match("(ab|c){2,3}", "abc", 1);
    expect_full_match("(ab|c){2,3}", "abcc", 1);
    expect_full_match("(ab|c){2,3}", "ab", 0);
}

static void test_vm_open_and_zero_counts(void)
{
    expect_full_match("a{2,}", "aa", 1);
    expect_full_match("a{2,}", "aaaaaa", 1);
    expect_full_match("a{2,}", "a", 0);

    expect_full_match("a{0}", "", 1);
    expect_full_match("a{0}", "a", 0);

    expect_full_match("a{0,2}", "", 1);
    expect_full_match("a{0,2}", "a", 1);
    expect_full_match("a{0,2}", "aa", 1);
    expect_full_match("a{0,2}", "aaa", 0);

    expect_full_match("a{0,}", "", 1);
    expect_full_match("a{0,}", "aaaa", 1);
}

static void test_vm_groups_classes_and_search(void)
{
    expect_full_match("^[0-9]{4}$", "2026", 1);
    expect_full_match("^[0-9]{4}$", "026", 0);
    expect_full_match("^[0-9]{4}$", "20260", 0);

    expect_full_match("[\\dA-F]{2,4}", "9AF", 1);
    expect_full_match("[\\dA-F]{2,4}", "9G", 0);
    expect_full_match("\\{2\\}", "{2}", 1);
    expect_full_match("[{}]{2}", "{}", 1);

    expect_search(
        "[0-9]{2,4}",
        "a12345b",
        1U,
        5U
    );

    expect_search(
        "(ab){2,3}",
        "xabababy",
        1U,
        7U
    );
}

static void test_public_api_find_all(void)
{
    RegexError error;
    RegexMatchList matches;
    Regex *regex;

    regex = regex_compile("\\d{2,4}", &error);
    assert(regex != NULL);

    assert(
        regex_find_all(
            regex,
            "a1b12c34567",
            &matches,
            &error
        )
    );

    assert(matches.count == 2U);
    assert(matches.matches[0].start == 3U);
    assert(matches.matches[0].end == 5U);
    assert(matches.matches[1].start == 6U);
    assert(matches.matches[1].end == 10U);

    regex_match_list_free(&matches);
    regex_free(regex);

    regex = regex_compile("a{4,2}", &error);
    assert(regex == NULL);
    assert(error.message != NULL);
}

int main(void)
{
    test_lexer_tokens();
    test_ast_repeat_node();
    test_parser_counts();
    test_parser_errors();
    test_compiler_shapes();
    test_vm_exact_and_ranges();
    test_vm_open_and_zero_counts();
    test_vm_groups_classes_and_search();
    test_public_api_find_all();

    puts("All bounded repetition tests passed.");

    return 0;
}