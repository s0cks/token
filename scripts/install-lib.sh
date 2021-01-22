#!/usr/bin/env bash
cd build/ || exit
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/
cmake --build . --target install