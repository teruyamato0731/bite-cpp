set(BITE_TEST_SOURCES
    test_integer.cpp
    test_scalar.cpp
    test_composite.cpp
    test_errors.cpp
    test_compile_time.cpp
)

add_executable(bite_tests ${BITE_TEST_SOURCES})

target_link_libraries(
    bite_tests
    PRIVATE
        bite::bite
        GTest::gtest_main
)

target_compile_options(
    bite_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion>
)

include(GoogleTest)
gtest_discover_tests(bite_tests)
