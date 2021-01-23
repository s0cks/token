#!/usr/bin/env bash
token-node \
  --path /usr/share/ledger \
  --service-port 8080 \
  --server-port 8081 \
  --healthcheck-port 8082 \
  --miner-interval 30000