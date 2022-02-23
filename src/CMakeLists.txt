add_library(mcbopomofo SHARED McBopomofo.cpp KeyValueBlobReader.cpp McBopomofoLM.cpp ParselessLM.cpp ParselessPhraseDB.cpp PhraseReplacementMap.cpp UserOverrideModel.cpp UserPhrasesLM.cpp Mandarin.cpp)
target_compile_options(mcbopomofo PRIVATE -Wall -Wextra)
target_link_libraries(mcbopomofo PRIVATE Fcitx5::Core)
set_target_properties(mcbopomofo PROPERTIES PREFIX "")
install(TARGETS mcbopomofo DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")

# Addon config file
# We need additional layer of conversion because we want PROJECT_VERSION in it.
configure_file(mcbopomofo-addon.conf.in mcbopomofo-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf" RENAME mcbopomofo.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input Method registration file
install(FILES "mcbopomofo.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
