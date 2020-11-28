#!/usr/bin/env bash

VERSION="$1"
PROJECT="tkn-events"
COMPONENT="token-ledger"
REGISTRY_HOSTNAME="gcr.io"
GITHUB_TOKEN=$(cat ./token.txt)
echo "building $VERSION...."
docker build . \
  --build-arg "GITHUB_TOKEN=${GITHUB_TOKEN}" \
  -t "$COMPONENT":"$VERSION" \
  -t "$REGISTRY_HOSTNAME"/"$PROJECT"/"$COMPONENT":"$VERSION"