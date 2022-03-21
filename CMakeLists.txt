cmake_minimum_required(VERSION 3.6)
project(fcitx5-mcbopomofo)

find_package(Fcitx5Core REQUIRED)
# Setup some compiler option that is generally useful and compatible with Fcitx 5 (C++17)
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_subdirectory(src)
add_subdirectory(data)

# McBopomofo data
configure_file(data/data.txt mcbopomofo-data.txt)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-data.txt" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")

set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest." FORCE)
