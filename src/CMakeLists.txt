cmake_minimum_required(VERSION 3.6)
project(mcbopomofo VERSION 2.5.2)

find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(fmt REQUIRED)
find_package(Gettext REQUIRED)

add_subdirectory(Engine)

set(MCBOPOMOFO_LIB_SOURCES
 KeyHandler.cpp
 LanguageModelLoader.cpp
 UTF8Helper.cpp
 Log.cpp)

# https://stackoverflow.com/questions/26549137/shared-library-on-linux-and-fpic-error
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Setup some compiler option that is generally useful and compatible with Fcitx 5 (C++17)
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_library(McBopomofoLib ${MCBOPOMOFO_LIB_SOURCES})
target_compile_options(McBopomofoLib PRIVATE -Wno-unknown-pragmas)
target_link_libraries(McBopomofoLib PRIVATE Fcitx5::Utils gramambular2_lib McBopomofoLMLib MandarinLib)
target_include_directories(McBopomofoLib PRIVATE Fcitx5::Utils)
target_compile_definitions(McBopomofoLib PRIVATE FCITX_GETTEXT_DOMAIN=\"fcitx5-mcbopomofo\")

include_directories(Engine)
include_directories(Engine/Gramambular)
include_directories(Engine/Mandarin)

add_library(mcbopomofo MODULE McBopomofo.cpp)
# Use this to build McBopomofo on, for example, Ubuntu 20.04 LTS
if (USE_LEGACY_FCITX5_API)
    MESSAGE(STATUS "Using legacy (pre-2021) Fcitx5 API")
    add_compile_definitions(mcbopomofo USE_LEGACY_FCITX5_API=1)
endif()
target_compile_options(mcbopomofo PRIVATE -Wno-unknown-pragmas)
target_link_libraries(mcbopomofo PRIVATE Fcitx5::Core McBopomofoLib fmt::fmt)
target_include_directories(mcbopomofo PRIVATE Fcitx5::Core fmt::fmt)
set_target_properties(mcbopomofo PROPERTIES PREFIX "")
target_compile_definitions(mcbopomofo PRIVATE FCITX_GETTEXT_DOMAIN=\"fcitx5-mcbopomofo\")
install(TARGETS mcbopomofo DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")

# Addon config file
# We need additional layer of conversion because we want PROJECT_VERSION in it.
configure_file(mcbopomofo-addon.conf.in.in mcbopomofo-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf.in" mcbopomofo-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf" RENAME mcbopomofo.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input Method registration file
configure_file(mcbopomofo.conf.in.in mcbopomofo.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo.conf.in" mcbopomofo.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
configure_file(mcbopomofo-plain.conf.in.in mcbopomofo-plain.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-plain.conf.in" mcbopomofo-plain.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-plain.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")

if (ENABLE_TEST)
        enable_testing()
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
        endif()

        # Let CMake fetch Google Test for us.
        # https://github.com/google/googletest/tree/main/googletest#incorporating-into-an-existing-cmake-project
        include(FetchContent)

        FetchContent_Declare(
                googletest
                # Specify the commit you depend on and update it regularly.
                URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        include(GoogleTest)

        # Test target declarations.
        add_executable(McBopomofoTest
                KeyHandlerTest.cpp UTF8HelperTest.cpp)
        target_compile_options(McBopomofoTest PRIVATE -Wno-unknown-pragmas)
        target_link_libraries(McBopomofoTest PRIVATE Fcitx5::Core gtest_main gmock_main McBopomofoLib)
        target_include_directories(McBopomofoTest PRIVATE Fcitx5::Core)

        configure_file(../data/data.txt mcbopomofo-test-data.txt)

        gtest_discover_tests(McBopomofoTest)

        add_custom_target(
                runTest
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/McBopomofoTest
        )
        add_dependencies(runTest McBopomofoTest)
endif ()
