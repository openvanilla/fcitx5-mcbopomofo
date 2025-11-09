cmake_minimum_required(VERSION 3.10)
project(Big5UtilsLib)

find_package(ICU COMPONENTS uc REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(Big5UtilsLib
        Big5Utils.h
        Big5Utils.cpp)
target_link_libraries(Big5UtilsLib PRIVATE ICU::uc)

if (ENABLE_CLANG_TIDY)
    set_target_properties(Big5UtilsLib PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif ()

if (ENABLE_TEST)
        enable_testing()
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
        endif()

        # Test target declarations.
        add_executable(Big5UtilsLibTest
                Big5UtilsTest.cpp
        )
        target_link_libraries(Big5UtilsLibTest GTest::gtest_main Big5UtilsLib)
        include(GoogleTest)
        gtest_discover_tests(Big5UtilsLibTest)

        add_custom_target(
                runBig5UtilsLibTest
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Big5UtilsLibTest
        )
        add_dependencies(runBig5UtilsLibTest Big5UtilsLibTest)
endif ()
