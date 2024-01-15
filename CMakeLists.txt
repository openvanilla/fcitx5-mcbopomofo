cmake_minimum_required(VERSION 3.6)
project(fcitx5-mcbopomofo VERSION 2.5.2)

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(fmt REQUIRED)
find_package(Gettext REQUIRED)

pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET "json-c")

include(FeatureSummary)
include(GNUInstallDirs)
include(ECMSetupVersion)
include(ECMUninstallTarget)

option(ENABLE_TEST "Build Test" On)

# clang-tidy
option(ENABLE_CLANG_TIDY "Enable clang-tidy" Off)
if (ENABLE_CLANG_TIDY)
    MESSAGE(STATUS "Enabled clang-tidy")
    find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
    MESSAGE(STATUS "Found clang-tidy: ${CLANG_TIDY}")
    string(CONCAT CLANG_TIDY_CHECKS "-checks=-*,"
        "clang-analyzer-*,bugprone-*,cert-*,readability-*,"
        "clang-diagnostic-*,"
        "cppcoreguidelines-explicit-virtual-functions,"
        "cppcoreguidelines-virtual-class-destructor,"
        "-bugprone-easily-swappable-parameters,"
        "-readability-const-return-type,"
        "-readability-convert-member-functions-to-static,"
        "-readability-function-cognitive-complexity,"
        "-readability-identifier-length,"
        "-readability-implicit-bool-conversion,"
        "-readability-isolate-declaration,"
        "-readability-suspicious-call-argument"
    )
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY}" "${CLANG_TIDY_CHECKS}")
    # one may wants to use the following instead to focus on a specific category
    # set(CLANG_TIDY_COMMAND "${CLANG_TIDY}" "-checks=-*,clang-analyzer-*")
endif ()

# Setup some compiler option that is generally useful and compatible with Fcitx 5 (C++17)
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

# So that `make install` does not also install Google Test
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)

message(STATUS "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}")

# McBopomofo data
configure_file(data/data.txt mcbopomofo-data.txt)
configure_file(data/data-plain-bpmf.txt mcbopomofo-data-plain-bpmf.txt)
configure_file(data/add-phrase-hook.sh mcbopomofo-add-phrase-hook.sh)
configure_file(data/associated-phrases.txt mcbopomofo-associated-phrases.txt)
configure_file(data/dictionary_service.json mcbopomofo-dictionary-service.json)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-data.txt" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-data-plain-bpmf.txt" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-add-phrase-hook.sh" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-associated-phrases.txt" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-dictionary-service.json" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.McBopomofo.metainfo.xml.in
                              org.fcitx.Fcitx5.Addon.McBopomofo.metainfo.xml XML
                              PO_DIRECTORY "${CMAKE_SOURCE_DIR}/po")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.McBopomofo.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

if (ENABLE_TEST)
    enable_testing()
endif ()
