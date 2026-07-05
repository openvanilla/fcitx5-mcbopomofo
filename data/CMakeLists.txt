foreach(size 16 22 24 32 64)
    install(DIRECTORY ${size}x${size} DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
        PATTERN .* EXCLUDE
        PATTERN *~ EXCLUDE
        PATTERN fcitx_mcbopomofo.png EXCLUDE)
    install(CODE "
        set(_dest \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps\")
        file(REMOVE \"\${_dest}/fcitx_mcbopomofo.png\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink
            org.fcitx.Fcitx5.fcitx_mcbopomofo.png
            \"\${_dest}/fcitx_mcbopomofo.png\")
    ")
endforeach(size 16 22 24 32 64)

