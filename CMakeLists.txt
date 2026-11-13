cmake_minimum_required(VERSION 3.20)

project(bite VERSION 0.1.0 LANGUAGES CXX)

add_library(bite INTERFACE)
add_library(bite::bite ALIAS bite)

target_compile_features(bite INTERFACE cxx_std_23)
target_include_directories(
    bite
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(BITE_IS_TOP_LEVEL ON)
else()
    set(BITE_IS_TOP_LEVEL OFF)
endif()

option(BITE_BUILD_TESTS "Build bite unit tests" ${BITE_IS_TOP_LEVEL})
option(BITE_BUILD_EXAMPLES "Build bite examples" ${BITE_IS_TOP_LEVEL})

if(BITE_BUILD_TESTS)
    enable_testing()

    find_package(GTest 1.17.0 QUIET)

    if(GTest_FOUND)
        message(STATUS "Using system GoogleTest")
    else()
        message(STATUS "GoogleTest >= 1.17.0 not found. Fetching v1.17.0.")

        include(FetchContent)

        set(BUILD_GMOCK OFF)
        set(INSTALL_GTEST OFF)

        if(POLICY CMP0135)
            cmake_policy(SET CMP0135 NEW)
        endif()

        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
            URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
        )

        FetchContent_MakeAvailable(googletest)
    endif()

    add_subdirectory(tests)
endif()

if(BITE_BUILD_EXAMPLES)
    add_executable(bite_basic_example examples/basic.cpp)
    target_link_libraries(bite_basic_example PRIVATE bite::bite)
endif()
