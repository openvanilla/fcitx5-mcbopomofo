#!/bin/sh

cmake -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug # use Debug for easy debugging with gdb
cmake --build build                                                 # or ninja, depending on your system
sudo cmake --install build                                          # or sudo ninja install
