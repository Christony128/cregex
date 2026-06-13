#include "cregex/compiler.h"

#include <stdlib.h>

typedef struct PatchReference PatchReference;

struct PatchReference {
    size_t state_index;
    int output_slot;
    PatchReference *next;
};

typedef struct {
    size_t start;
    PatchReference *out;
} Fragment;

typedef struct {
    NfaProgram *program;
    int failed;
    CompilerError error;
} Compiler;

static void compiler_set_error(
    Compiler *compiler,
    size_t position,
    const char *message
)
{
    if (compiler->failed) {
        return;
    }

    compiler->failed = 1;
    compiler->error.position = position;
    compiler->error.message = message;
}

static PatchReference *patch_reference_create(
    Compiler *compiler,
    size_t state_index,
    int output_slot,
    size_t position
)
{
    PatchReference *reference;

    reference = (PatchReference *) malloc(
        sizeof(PatchReference)
    );

    if (reference == NULL) {
        compiler_set_error(
            compiler,
            position,
            "failed to allocate patch reference"
        );

        return NULL;
    }

    reference->state_index = state_index;
    reference->output_slot = output_slot;
    reference->next = NULL;

    return reference;
}

static void patch_list_free(PatchReference *list)
{
    while (list != NULL) {
        PatchReference *next;

        next = list->next;
        free(list);
        list = next;
    }
}

static PatchReference *patch_list_join(
    PatchReference *first,
    PatchReference *second
)
{
    PatchReference *current;

    if (first == NULL) {
        return second;
    }

    current = first;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = second;
    return first;
}

static int patch_list_apply(
    Compiler *compiler,
    PatchReference *list,
    size_t destination,
    size_t position
)
{
    PatchReference *current;

    current = list;

    while (current != NULL) {
        int success;

        if (current->output_slot == 1) {
            success = nfa_program_patch_out1(
                compiler->program,
                current->state_index,
                destination
            );
        } else {
            success = nfa_program_patch_out2(
                compiler->program,
                current->state_index,
                destination
            );
        }

        if (!success) {
            compiler_set_error(
                compiler,
                position,
                "failed to patch NFA transition"
            );

            return 0;
        }

        current = current->next;
    }

    return 1;
}

static int patch_list_apply_and_free(
    Compiler *compiler,
    PatchReference *list,
    size_t destination,
    size_t position
)
{
    int success;

    success = patch_list_apply(
        compiler,
        list,
        destination,
        position
    );

    patch_list_free(list);
    return success;
}

static int compile_node(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
);

static int compile_single_output_state(
    Compiler *compiler,
    size_t state_index,
    size_t position,
    Fragment *fragment
)
{
    PatchReference *reference;

    if (state_index == NFA_INVALID_STATE) {
        compiler_set_error(
            compiler,
            position,
            "failed to allocate NFA state"
        );

        return 0;
    }

    reference = patch_reference_create(
        compiler,
        state_index,
        1,
        position
    );

    if (reference == NULL) {
        return 0;
    }

    fragment->start = state_index;
    fragment->out = reference;

    return 1;
}

static int compile_empty_at(
    Compiler *compiler,
    size_t position,
    Fragment *fragment
)
{
    size_t jump;

    jump = nfa_program_add_jump(compiler->program);

    return compile_single_output_state(
        compiler,
        jump,
        position,
        fragment
    );
}

static int compile_empty(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    return compile_empty_at(
        compiler,
        node->position,
        fragment
    );
}

static int compile_literal(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    size_t state;

    state = nfa_program_add_char(
        compiler->program,
        node->value.character
    );

    return compile_single_output_state(
        compiler,
        state,
        node->position,
        fragment
    );
}

static int compile_dot(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    size_t state;

    state = nfa_program_add_any(compiler->program);

    return compile_single_output_state(
        compiler,
        state,
        node->position,
        fragment
    );
}

static int compile_character_class(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    size_t state;

    state = nfa_program_add_class(
        compiler->program,
        &node->value.character_class
    );

    return compile_single_output_state(
        compiler,
        state,
        node->position,
        fragment
    );
}

static int compile_assertion(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    size_t state;

    if (node->type == AST_ASSERT_START) {
        state = nfa_program_add_assert_start(
            compiler->program
        );
    } else {
        state = nfa_program_add_assert_end(
            compiler->program
        );
    }

    return compile_single_output_state(
        compiler,
        state,
        node->position,
        fragment
    );
}

static int fragment_append(
    Compiler *compiler,
    Fragment *left,
    Fragment *right,
    size_t position
)
{
    if (
        !patch_list_apply_and_free(
            compiler,
            left->out,
            right->start,
            position
        )
    ) {
        patch_list_free(right->out);
        left->out = NULL;
        return 0;
    }

    left->out = right->out;
    return 1;
}

static int compile_concatenation(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    Fragment left;
    Fragment right;

    if (
        !compile_node(
            compiler,
            node->value.binary.left,
            &left
        )
    ) {
        return 0;
    }

    if (
        !compile_node(
            compiler,
            node->value.binary.right,
            &right
        )
    ) {
        patch_list_free(left.out);
        return 0;
    }

    if (
        !fragment_append(
            compiler,
            &left,
            &right,
            node->position
        )
    ) {
        return 0;
    }

    *fragment = left;
    return 1;
}

static int compile_alternation(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    Fragment left;
    Fragment right;
    size_t split;

    if (
        !compile_node(
            compiler,
            node->value.binary.left,
            &left
        )
    ) {
        return 0;
    }

    if (
        !compile_node(
            compiler,
            node->value.binary.right,
            &right
        )
    ) {
        patch_list_free(left.out);
        return 0;
    }

    split = nfa_program_add_split(compiler->program);

    if (split == NFA_INVALID_STATE) {
        patch_list_free(left.out);
        patch_list_free(right.out);

        compiler_set_error(
            compiler,
            node->position,
            "failed to allocate alternation state"
        );

        return 0;
    }

    if (
        !nfa_program_patch_out1(
            compiler->program,
            split,
            left.start
        ) ||
        !nfa_program_patch_out2(
            compiler->program,
            split,
            right.start
        )
    ) {
        patch_list_free(left.out);
        patch_list_free(right.out);

        compiler_set_error(
            compiler,
            node->position,
            "failed to connect alternation branches"
        );

        return 0;
    }

    fragment->start = split;
    fragment->out = patch_list_join(
        left.out,
        right.out
    );

    return 1;
}

static int compile_star_child(
    Compiler *compiler,
    const AstNode *child_node,
    size_t position,
    Fragment *fragment
)
{
    Fragment child;
    PatchReference *exit_reference;
    size_t split;

    if (!compile_node(compiler, child_node, &child)) {
        return 0;
    }

    split = nfa_program_add_split(compiler->program);

    if (split == NFA_INVALID_STATE) {
        patch_list_free(child.out);

        compiler_set_error(
            compiler,
            position,
            "failed to allocate repetition state"
        );

        return 0;
    }

    if (
        !nfa_program_patch_out1(
            compiler->program,
            split,
            child.start
        )
    ) {
        patch_list_free(child.out);

        compiler_set_error(
            compiler,
            position,
            "failed to connect repetition body"
        );

        return 0;
    }

    if (
        !patch_list_apply_and_free(
            compiler,
            child.out,
            split,
            position
        )
    ) {
        return 0;
    }

    exit_reference = patch_reference_create(
        compiler,
        split,
        2,
        position
    );

    if (exit_reference == NULL) {
        return 0;
    }

    fragment->start = split;
    fragment->out = exit_reference;

    return 1;
}

static int compile_plus_child(
    Compiler *compiler,
    const AstNode *child_node,
    size_t position,
    Fragment *fragment
)
{
    Fragment child;
    PatchReference *exit_reference;
    size_t split;

    if (!compile_node(compiler, child_node, &child)) {
        return 0;
    }

    split = nfa_program_add_split(compiler->program);

    if (split == NFA_INVALID_STATE) {
        patch_list_free(child.out);

        compiler_set_error(
            compiler,
            position,
            "failed to allocate plus state"
        );

        return 0;
    }

    if (
        !nfa_program_patch_out1(
            compiler->program,
            split,
            child.start
        )
    ) {
        patch_list_free(child.out);

        compiler_set_error(
            compiler,
            position,
            "failed to connect plus loop"
        );

        return 0;
    }

    if (
        !patch_list_apply_and_free(
            compiler,
            child.out,
            split,
            position
        )
    ) {
        return 0;
    }

    exit_reference = patch_reference_create(
        compiler,
        split,
        2,
        position
    );

    if (exit_reference == NULL) {
        return 0;
    }

    fragment->start = child.start;
    fragment->out = exit_reference;

    return 1;
}

static int compile_optional_child(
    Compiler *compiler,
    const AstNode *child_node,
    size_t position,
    Fragment *fragment
)
{
    Fragment child;
    PatchReference *skip_reference;
    size_t split;

    if (!compile_node(compiler, child_node, &child)) {
        return 0;
    }

    split = nfa_program_add_split(compiler->program);

    if (split == NFA_INVALID_STATE) {
        patch_list_free(child.out);

        compiler_set_error(
            compiler,
            position,
            "failed to allocate optional state"
        );

        return 0;
    }

    if (
        !nfa_program_patch_out1(
            compiler->program,
            split,
            child.start
        )
    ) {
        patch_list_free(child.out);

        compiler_set_error(
            compiler,
            position,
            "failed to connect optional body"
        );

        return 0;
    }

    skip_reference = patch_reference_create(
        compiler,
        split,
        2,
        position
    );

    if (skip_reference == NULL) {
        patch_list_free(child.out);
        return 0;
    }

    fragment->start = split;
    fragment->out = patch_list_join(
        child.out,
        skip_reference
    );

    return 1;
}

static int compile_star(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    return compile_star_child(
        compiler,
        node->value.child,
        node->position,
        fragment
    );
}

static int compile_plus(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    return compile_plus_child(
        compiler,
        node->value.child,
        node->position,
        fragment
    );
}

static int compile_optional(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    return compile_optional_child(
        compiler,
        node->value.child,
        node->position,
        fragment
    );
}

static int repeat_append_part(
    Compiler *compiler,
    Fragment *result,
    int *has_result,
    Fragment *part,
    size_t position
)
{
    if (!*has_result) {
        *result = *part;
        *has_result = 1;
        return 1;
    }

    return fragment_append(
        compiler,
        result,
        part,
        position
    );
}

static int compile_repeat(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    const AstNode *child_node;
    size_t minimum;
    size_t maximum;
    size_t index;
    Fragment result;
    int has_result;

    child_node = node->value.repetition.child;
    minimum = node->value.repetition.minimum;
    maximum = node->value.repetition.maximum;

    if (
        child_node == NULL ||
        minimum > CREGEX_MAX_REPEAT_COUNT ||
        (
            maximum != CREGEX_REPEAT_UNBOUNDED &&
            (
                maximum < minimum ||
                maximum > CREGEX_MAX_REPEAT_COUNT
            )
        )
    ) {
        compiler_set_error(
            compiler,
            node->position,
            "invalid bounded repetition AST node"
        );

        return 0;
    }

    result.start = NFA_INVALID_STATE;
    result.out = NULL;
    has_result = 0;

    for (index = 0U; index < minimum; index++) {
        Fragment part;

        if (!compile_node(compiler, child_node, &part)) {
            patch_list_free(result.out);
            return 0;
        }

        if (
            !repeat_append_part(
                compiler,
                &result,
                &has_result,
                &part,
                node->position
            )
        ) {
            return 0;
        }
    }

    if (maximum == CREGEX_REPEAT_UNBOUNDED) {
        Fragment part;

        if (
            !compile_star_child(
                compiler,
                child_node,
                node->position,
                &part
            )
        ) {
            patch_list_free(result.out);
            return 0;
        }

        if (
            !repeat_append_part(
                compiler,
                &result,
                &has_result,
                &part,
                node->position
            )
        ) {
            return 0;
        }
    } else {
        for (index = minimum; index < maximum; index++) {
            Fragment part;

            if (
                !compile_optional_child(
                    compiler,
                    child_node,
                    node->position,
                    &part
                )
            ) {
                patch_list_free(result.out);
                return 0;
            }

            if (
                !repeat_append_part(
                    compiler,
                    &result,
                    &has_result,
                    &part,
                    node->position
                )
            ) {
                return 0;
            }
        }
    }

    if (!has_result) {
        return compile_empty_at(
            compiler,
            node->position,
            fragment
        );
    }

    *fragment = result;
    return 1;
}

static int compile_node(
    Compiler *compiler,
    const AstNode *node,
    Fragment *fragment
)
{
    if (node == NULL) {
        compiler_set_error(
            compiler,
            0U,
            "cannot compile a null AST node"
        );

        return 0;
    }

    switch (node->type) {
        case AST_EMPTY:
            return compile_empty(
                compiler,
                node,
                fragment
            );

        case AST_LITERAL:
            return compile_literal(
                compiler,
                node,
                fragment
            );

        case AST_DOT:
            return compile_dot(
                compiler,
                node,
                fragment
            );

        case AST_CHARACTER_CLASS:
            return compile_character_class(
                compiler,
                node,
                fragment
            );

        case AST_ASSERT_START:
        case AST_ASSERT_END:
            return compile_assertion(
                compiler,
                node,
                fragment
            );

        case AST_CONCATENATION:
            return compile_concatenation(
                compiler,
                node,
                fragment
            );

        case AST_ALTERNATION:
            return compile_alternation(
                compiler,
                node,
                fragment
            );

        case AST_STAR:
            return compile_star(
                compiler,
                node,
                fragment
            );

        case AST_PLUS:
            return compile_plus(
                compiler,
                node,
                fragment
            );

        case AST_OPTIONAL:
            return compile_optional(
                compiler,
                node,
                fragment
            );

        case AST_REPEAT:
            return compile_repeat(
                compiler,
                node,
                fragment
            );
    }

    compiler_set_error(
        compiler,
        node->position,
        "unknown AST node type"
    );

    return 0;
}

int compiler_compile(
    const AstNode *root,
    NfaProgram *program,
    CompilerError *error
)
{
    Compiler compiler;
    Fragment fragment;
    size_t match;

    compiler.program = program;
    compiler.failed = 0;
    compiler.error.position = 0U;
    compiler.error.message = NULL;

    fragment.start = NFA_INVALID_STATE;
    fragment.out = NULL;

    if (program == NULL) {
        compiler_set_error(
            &compiler,
            0U,
            "compiler received a null NFA program"
        );
    } else if (
        program->states != NULL ||
        program->count != 0U ||
        program->capacity != 0U
    ) {
        compiler_set_error(
            &compiler,
            0U,
            "NFA program must be empty before compilation"
        );
    }

    if (
        !compiler.failed &&
        !compile_node(
            &compiler,
            root,
            &fragment
        )
    ) {
        compiler.failed = 1;
    }

    if (!compiler.failed) {
        match = nfa_program_add_match(program);

        if (match == NFA_INVALID_STATE) {
            patch_list_free(fragment.out);

            compiler_set_error(
                &compiler,
                0U,
                "failed to allocate match state"
            );
        } else if (
            !patch_list_apply_and_free(
                &compiler,
                fragment.out,
                match,
                0U
            )
        ) {
            compiler.failed = 1;
        } else if (
            !nfa_program_set_start(
                program,
                fragment.start
            )
        ) {
            compiler_set_error(
                &compiler,
                0U,
                "failed to set NFA entry point"
            );
        }
    }

    if (compiler.failed && program != NULL) {
        nfa_program_free(program);
        nfa_program_init(program);
    }

    if (error != NULL) {
        *error = compiler.error;
    }

    return !compiler.failed;
}