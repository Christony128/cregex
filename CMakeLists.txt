cmake_minimum_required(VERSION 3.16)

project(
    cregex
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
    )
endif()

add_library(cregex_core STATIC
    src/token.c
    src/lexer.c
    src/charset.c
    src/ast.c
    src/parser.c
    src/nfa.c
)

target_include_directories(cregex_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(cregex
    cli/main.c
)

target_link_libraries(cregex
    PRIVATE
        cregex_core
)

enable_testing()

add_executable(test_lexer
    tests/test_lexer.c
)

target_link_libraries(test_lexer
    PRIVATE
        cregex_core
)

add_test(
    NAME lexer_tests
    COMMAND test_lexer
)

add_executable(test_ast
    tests/test_ast.c
)

target_link_libraries(test_ast
    PRIVATE
        cregex_core
)

add_test(
    NAME ast_tests
    COMMAND test_ast
)

add_executable(test_parser
    tests/test_parser.c
)

target_link_libraries(test_parser
    PRIVATE
        cregex_core
)

add_test(
    NAME parser_tests
    COMMAND test_parser
)
add_executable(test_nfa
    tests/test_nfa.c
)

target_link_libraries(test_nfa
    PRIVATE
        cregex_core
)

add_test(
    NAME nfa_tests
    COMMAND test_nfa
)