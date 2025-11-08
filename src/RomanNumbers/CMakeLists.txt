cmake_minimum_required(VERSION 3.10)
project(RomanNumbersLib)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(RomanNumbersLib
        RomanNumbers.h
        RomanNumbers.cpp)

if (ENABLE_CLANG_TIDY)
    set_target_properties(RomanNumbersLib PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif ()

if (ENABLE_TEST)
        enable_testing()
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
        endif()

        # Test target declarations.
        add_executable(RomanNumbersLibTest
                RomanNumbersTest.cpp
        )
        target_link_libraries(RomanNumbersLibTest GTest::gtest_main RomanNumbersLib)
        include(GoogleTest)
        gtest_discover_tests(RomanNumbersLibTest)

        add_custom_target(
                runRomanNumbersLibTest
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/RomanNumbersLibTest
        )
        add_dependencies(runRomanNumbersLibTest RomanNumbersLibTest)
endif ()
