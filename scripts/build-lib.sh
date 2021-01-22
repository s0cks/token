#!/usr/bin/env bash

cd build/ || exit
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .