add_executable(bite_tests test_bite.cpp)
target_link_libraries(bite_tests PRIVATE bite::bite)

target_compile_options(
    bite_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion>
)

add_test(NAME bite_tests COMMAND bite_tests)
