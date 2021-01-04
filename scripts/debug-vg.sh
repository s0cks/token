#!/usr/bin/env bash
cd build/ || exit
cmake --build .
valgrind ./token-node \
  --path /home/tazz/CLionProjects/libtoken-ledger/ledger1 \
  --server-port 8080 \
  --service-port 8081