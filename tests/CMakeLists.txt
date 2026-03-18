# ============================================
# BuLang Test Suite
# Lightweight target: only depends on libbu (no SDL/OpenGL/plugins)
# Build:  cmake --build build-debug --target bulang_test
# Run:    cd build-debug && ctest --output-on-failure
# ============================================

cmake_minimum_required(VERSION 3.20)

# ── Test runner executable ──────────────────────────────────

add_executable(bulang_test bulang_runner.cpp)

target_link_libraries(bulang_test PRIVATE libbu)
target_include_directories(bulang_test PRIVATE
    ${CMAKE_SOURCE_DIR}/libbu/include
    ${CMAKE_SOURCE_DIR}/libbu/src
)

set_target_properties(bulang_test PROPERTIES
    OUTPUT_NAME "bulang_test"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# Match libbu build flags (sanitizers in debug)
if(CMAKE_BUILD_TYPE MATCHES Debug AND UNIX)
    target_compile_options(bulang_test PRIVATE
        -fsanitize=address -fsanitize=undefined -fsanitize=leak
        -g -O1 -fno-omit-frame-pointer
    )
    target_link_options(bulang_test PRIVATE
        -fsanitize=address -fsanitize=undefined -fsanitize=leak -g
    )
endif()

# ── CTest registration ─────────────────────────────────────

set(BULANG_TEST_RUNNER $<TARGET_FILE:bulang_test>)
set(BULANG_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts/tests)

# Pure language tests (headless — no SDL/OpenGL/plugins needed)
set(BULANG_LANG_TESTS
    test_variables
    test_operators
    test_control_flow
    test_functions
    test_strings
    test_arrays_maps
    test_classes
    test_structs_enums
    test_math_builtins
    test_error_handling
    test_loops
    test_parser_edge_cases
    test_throw_stress
    test_stress_runtime
    test_closures_stress
    test_class_stress
    test_int_edge_cases
)

foreach(test_name IN LISTS BULANG_LANG_TESTS)
    add_test(
        NAME "bulang/${test_name}"
        COMMAND ${BULANG_TEST_RUNNER} ${BULANG_SCRIPTS_DIR}/${test_name}.bu
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("bulang/${test_name}" PROPERTIES
        TIMEOUT 10
        LABELS "bulang;lang"
    )
endforeach()

# ── Convenience target: run all bulang tests ────────────────

add_custom_target(bulang_run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L bulang
    DEPENDS bulang_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running BuLang language tests..."
)

# ── Parser error recovery tests (bad syntax → must fail, not hang) ──

set(BULANG_ERROR_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts/tests/errors)

set(BULANG_ERROR_TESTS
    err_class_unclosed_brace
    err_class_no_name
    err_class_bad_parent
    err_class_method_unclosed
    err_struct_unclosed_brace
    err_struct_no_name
    err_struct_var_alone
    err_var_no_name
    err_func_no_body
    err_func_unclosed_brace
    err_nested_unclosed_braces
    err_while_no_condition
    err_for_empty
    err_keyword_in_expr
    err_stack_overflow_recursion
    err_mutual_recursion_overflow
    err_nil_index
    err_call_nil
    err_call_int
    err_nil_method
    err_nil_property
    err_nil_arithmetic
    err_array_map_arithmetic
    err_try_depth_overflow
    err_throw_no_try
    err_method_wrong_type
    err_undefined_variable
    err_undefined_method
    err_array_oob
    err_array_negative_oob
    err_division_by_zero
    err_modulo_by_zero
    err_unterminated_string
    err_consecutive_operators
    err_trailing_operator
    err_deeply_nested_parens
    err_duplicate_else
    err_break_outside_loop
    err_continue_outside_loop
    err_set_index_non_indexable
    err_set_property_non_object
    err_wrong_arg_count
    err_self_inheritance
)

foreach(test_name IN LISTS BULANG_ERROR_TESTS)
    add_test(
        NAME "bulang/error/${test_name}"
        COMMAND ${BULANG_TEST_RUNNER} ${BULANG_ERROR_SCRIPTS_DIR}/${test_name}.bu
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("bulang/error/${test_name}" PROPERTIES
        WILL_FAIL TRUE
        TIMEOUT 3
        LABELS "bulang;errors"
    )
endforeach()
