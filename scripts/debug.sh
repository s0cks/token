cd build/ || exit
cmake --build .
./token-node \
  --path /home/tazz/CLionProjects/libtoken-ledger/ledger1 \
  --server-port 8080 \
  --service-port 8081