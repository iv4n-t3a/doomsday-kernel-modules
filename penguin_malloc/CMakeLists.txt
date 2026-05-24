cmake_minimum_required(VERSION 3.14)
project(penguin_malloc VERSION 0.1.0 LANGUAGES C CXX)

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------
set(PENGUIN_MALLOC_SOURCES
    src/buddy.c
    src/buddy_util.c
    src/freelist.c
    src/penguin_malloc.c
    src/pfalloc.c
    src/slab.c
    src/util.c
)

add_library(penguin_malloc STATIC ${PENGUIN_MALLOC_SOURCES})

target_include_directories(penguin_malloc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(penguin_malloc PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# Tests (optional)
# -----------------------------------------------------------------------------
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    include(FetchContent)
    
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_compile_options(-fsanitize=address)
    add_compile_options(-pthread)
    add_link_options(-fsanitize=address)


    set(TEST_SOURCES
        tests/buddy_test.cpp
        tests/buddy_util_test.cpp
        tests/freelist_test.cpp
        tests/penguin_malloc_tests.cpp
        tests/slab_test.cpp
        tests/util_test.cpp
        tests/mem_test_util.cpp
    )

    add_executable(penguin_malloc_tests ${TEST_SOURCES})
    
    target_link_libraries(penguin_malloc_tests penguin_malloc)

    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    FetchContent_MakeAvailable(googletest)

    target_include_directories(penguin_malloc_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    set_target_properties(penguin_malloc_tests PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )

    include(GoogleTest)
    target_link_libraries(penguin_malloc_tests GTest::gtest_main)
    gtest_discover_tests(penguin_malloc_tests)
endif()
