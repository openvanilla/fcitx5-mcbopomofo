#!/bin/sh

# rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug # use Debug for easy debugging with gdb
make                                                          # or ninja, depending on your system
sudo make install                                             # or sudo ninja install
