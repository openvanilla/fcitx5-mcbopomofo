cmake_minimum_required(VERSION 3.10)
project(ChineseNumbersLib)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(ChineseNumbersLib
        ChineseNumbers.h
        ChineseNumbers.cpp
        StringUtils.h
        StringUtils.cpp
        SuzhouNumbers.h
        SuzhouNumbers.cpp)

if (ENABLE_CLANG_TIDY)
    set_target_properties(ChineseNumbersLib PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif ()

if (ENABLE_TEST)
        enable_testing()
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
        endif()

        # Test target declarations.
        add_executable(ChineseNumbersLibTest
                ChineseNumbersTest.cpp
                SuzhouNumbersTest.cpp
        )
        target_link_libraries(ChineseNumbersLibTest GTest::gtest_main ChineseNumbersLib)
        include(GoogleTest)
        gtest_discover_tests(ChineseNumbersLibTest)

        add_custom_target(
                runChineseNumbersLibTest
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ChineseNumbersLibTest
        )
        add_dependencies(runChineseNumbersLibTest ChineseNumbersLibTest)
endif ()
