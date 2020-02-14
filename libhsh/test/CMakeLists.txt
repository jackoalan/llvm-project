llvm_canonicalize_cmake_booleans(
        CLANG_BUILD_EXAMPLES
        CLANG_ENABLE_ARCMT
        CLANG_ENABLE_STATIC_ANALYZER
        CLANG_SPAWN_CC1
        ENABLE_BACKTRACES
        ENABLE_EXPERIMENTAL_NEW_PASS_MANAGER
        LLVM_ENABLE_ZLIB
        LLVM_ENABLE_PER_TARGET_RUNTIME_DIR
        LLVM_ENABLE_PLUGINS
        LLVM_ENABLE_THREADS)

configure_lit_site_cfg(
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
        MAIN_CONFIG
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

list(APPEND HSH_TEST_DEPS hshgen FileCheck count not)

add_custom_target(hsh-test-depends DEPENDS ${HSH_TEST_DEPS})
set_target_properties(hsh-test-depends PROPERTIES FOLDER "hsh tests")

add_lit_testsuite(check-hsh "Running the hsh regression tests"
        ${CMAKE_CURRENT_BINARY_DIR}
        #LIT ${LLVM_LIT}
        PARAMS ${HSH_TEST_PARAMS}
        DEPENDS ${HSH_TEST_DEPS}
        ARGS ${HSH_TEST_EXTRA_ARGS}
        )
set_target_properties(check-hsh PROPERTIES FOLDER "hsh tests")

add_lit_testsuites(HSH ${CMAKE_CURRENT_SOURCE_DIR}
        PARAMS ${HSH_TEST_PARAMS}
        DEPENDS ${HSH_TEST_DEPS}
        )
