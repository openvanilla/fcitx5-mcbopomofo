message(STATUS "Data subdirectory: CMAKE_INSTALL_DATADIR is ${CMAKE_INSTALL_DATADIR}")

foreach(size 16 22 24 32 64)
    message(STATUS "Queueing installation for icon size: ${size}x${size} to ${CMAKE_INSTALL_DATADIR}/icons/hicolor")
    install(DIRECTORY ${size}x${size} DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
        PATTERN .* EXCLUDE
        PATTERN *~ EXCLUDE)
endforeach(size 16 22 24 32 64)
