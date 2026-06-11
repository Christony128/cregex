#ifndef CREGEX_COMPILER_H
#define CREGEX_COMPILER_H

#include "cregex/ast.h"
#include "cregex/nfa.h"

#include <stddef.h>

typedef struct {
    size_t position;
    const char *message;
} CompilerError;
int compiler_compile(
    const AstNode *root,
    NfaProgram *program,
    CompilerError *error
);

#endif