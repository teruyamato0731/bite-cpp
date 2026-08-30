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
    add_subdirectory(tests)
endif()

if(BITE_BUILD_EXAMPLES)
    add_executable(bite_basic_example examples/basic.cpp)
    target_link_libraries(bite_basic_example PRIVATE bite::bite)
endif()
