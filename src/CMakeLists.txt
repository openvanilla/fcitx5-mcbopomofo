add_library(mcbopomofo SHARED mcbopomofo.cpp)
target_link_libraries(mcbopomofo PRIVATE Fcitx5::Core)
set_target_properties(mcbopomofo PROPERTIES PREFIX "")
install(TARGETS mcbopomofo DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")

# Addon config file
# We need additional layer of conversion because we want PROJECT_VERSION in it.
configure_file(mcbopomofo-addon.conf.in mcbopomofo-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf" RENAME mcbopomofo.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input Method registration file
install(FILES "mcbopomofo.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
