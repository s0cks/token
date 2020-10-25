#!/bin/bash
if [[ -z "${TOKEN_LEDGER}" ]]; then
  token-node \
    --path "/usr/share/ledger" \
    --port 8080
else
  token-node \
    --path "/usr/share/ledger" \
    --port 8080 \
    --peer "${TOKEN_LEDGER}"
fi