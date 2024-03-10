cmake_minimum_required(VERSION 3.6)
project(mcbopomofo VERSION 2.5.2)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS Notifications)
find_package(fmt REQUIRED)
find_package(Gettext REQUIRED)
find_package(ICU COMPONENTS uc i18n REQUIRED)

pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET "json-c")
include_directories(${JSONC_INCLUDE_DIRS})
link_directories(${JSONC_LIBRARY_DIRS})

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.23.0")
    find_package(GTest)
endif()

add_subdirectory(Engine)
add_subdirectory(ChineseNumbers)

set(MCBOPOMOFO_LIB_SOURCES
 KeyHandler.cpp
 LanguageModelLoader.cpp
 InputState.cpp
 InputMacro.cpp
 Log.cpp
 DictionaryService.cpp
)

# https://stackoverflow.com/questions/26549137/shared-library-on-linux-and-fpic-error
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Setup some compiler option that is generally useful and compatible with Fcitx 5 (C++17)
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_library(McBopomofoLib ${MCBOPOMOFO_LIB_SOURCES})
target_compile_options(McBopomofoLib PRIVATE -Wno-unknown-pragmas)
target_link_libraries(McBopomofoLib PRIVATE Fcitx5::Utils ICU::uc ICU::i18n ${JSONC_LIBRARIES} gramambular2_lib McBopomofoLMLib MandarinLib ChineseNumbersLib)
target_include_directories(McBopomofoLib PRIVATE Fcitx5::Utils)
target_compile_definitions(McBopomofoLib PRIVATE FCITX_GETTEXT_DOMAIN=\"fcitx5-mcbopomofo\")

if (ENABLE_CLANG_TIDY)
    set_target_properties(McBopomofoLib PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif ()

include_directories(Engine)
include_directories(Engine/Gramambular)
include_directories(Engine/Mandarin)
include_directories(ChineseNumbers)

add_library(mcbopomofo MODULE McBopomofo.cpp)
# Use this to build McBopomofo on, for example, Ubuntu 20.04 LTS
if (USE_LEGACY_FCITX5_API)
    MESSAGE(STATUS "Using legacy (pre-2021) Fcitx5 API")
    add_compile_definitions(mcbopomofo USE_LEGACY_FCITX5_API=1)
endif()
target_compile_options(mcbopomofo PRIVATE -Wno-unknown-pragmas)
target_link_libraries(mcbopomofo PRIVATE Fcitx5::Core Fcitx5::Module::Notifications McBopomofoLib fmt::fmt ${JSONC_LIBRARIES})
target_include_directories(mcbopomofo PRIVATE Fcitx5::Core Fcitx5::Module::Notifications fmt::fmt)
set_target_properties(mcbopomofo PROPERTIES PREFIX "")
target_compile_definitions(mcbopomofo PRIVATE FCITX_GETTEXT_DOMAIN=\"fcitx5-mcbopomofo\")
install(TARGETS mcbopomofo DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")

if (ENABLE_CLANG_TIDY)
    set_target_properties(mcbopomofo PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif ()

# Addon config file
# We need additional layer of conversion because we want PROJECT_VERSION in it.
configure_file(mcbopomofo-addon.conf.in.in mcbopomofo-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf.in" mcbopomofo-addon.conf PO_DIRECTORY "${CMAKE_SOURCE_DIR}/po")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf" RENAME mcbopomofo.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input Method registration file
configure_file(mcbopomofo.conf.in.in mcbopomofo.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo.conf.in" mcbopomofo.conf PO_DIRECTORY "${CMAKE_SOURCE_DIR}/po")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
configure_file(mcbopomofo-plain.conf.in.in mcbopomofo-plain.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-plain.conf.in" mcbopomofo-plain.conf PO_DIRECTORY "${CMAKE_SOURCE_DIR}/po")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-plain.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")

if (ENABLE_TEST)
        enable_testing()
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
                cmake_policy(SET CMP0135 NEW)
        endif()

        if (NOT GTest_FOUND)
            # Specify release version from https://github.com/google/googletest/releases
            set(GTEST_VERSION "v1.14.0")
            MESSAGE(STATUS "Fetching GoogleTest ${GTEST_VERSION} from GitHub")
            # Let CMake fetch Google Test for us.
            # https://github.com/google/googletest/tree/main/googletest#incorporating-into-an-existing-cmake-project
            include(FetchContent)

            FetchContent_Declare(
                    googletest
                    URL "https://github.com/google/googletest/archive/refs/tags/${GTEST_VERSION}.zip"
            )
            # For Windows: Prevent overriding the parent project's compiler/linker settings
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(googletest)
        endif()

        # Test target declarations.
        add_executable(McBopomofoTest KeyHandlerTest.cpp)
        target_compile_options(McBopomofoTest PRIVATE -Wno-unknown-pragmas)
        target_link_libraries(McBopomofoTest PRIVATE Fcitx5::Core GTest::gtest_main GTest::gmock_main McBopomofoLib fmt::fmt ${JSONC_LIBRARIES})
        target_include_directories(McBopomofoTest PRIVATE Fcitx5::Core fmt::fmt)

        configure_file(../data/data.txt mcbopomofo-test-data.txt)

        include(GoogleTest)
        gtest_discover_tests(McBopomofoTest)

        add_custom_target(
                runTest
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/McBopomofoTest
        )
        add_dependencies(runTest McBopomofoTest)
endif ()

