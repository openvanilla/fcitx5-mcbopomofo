set(MCBOPOMOFO_SOURCES
 McBopomofo.cpp
 InputState.cpp
 KeyHandler.cpp
 Engine/KeyValueBlobReader.cpp 
 Engine/McBopomofoLM.cpp
 Engine/ParselessLM.cpp
 Engine/ParselessPhraseDB.cpp
 Engine/PhraseReplacementMap.cpp
 Engine/UserOverrideModel.cpp
 Engine/UserPhrasesLM.cpp
 Engine/Mandarin/Mandarin.cpp
)

add_library(mcbopomofo MODULE ${MCBOPOMOFO_SOURCES})

include_directories(Engine)
include_directories(Engine/Gramambular)
include_directories(Engine/Mandarin)

# target_compile_options(mcbopomofo PRIVATE -Wall -Wextra)
target_link_libraries(mcbopomofo PRIVATE Fcitx5::Core)
target_include_directories(mcbopomofo PRIVATE Fcitx5::Core)
set_target_properties(mcbopomofo PROPERTIES PREFIX "")
install(TARGETS mcbopomofo DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")

# Addon config file
# We need additional layer of conversion because we want PROJECT_VERSION in it.
configure_file(mcbopomofo-addon.conf.in mcbopomofo-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo-addon.conf" RENAME mcbopomofo.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input Method registration file
configure_file(mcbopomofo.conf.in mcbopomofo.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcbopomofo.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
