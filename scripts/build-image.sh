#!/usr/bin/env bash

VERSION="$1"
PROJECT="tkn-events"
COMPONENT="token-ledger"
REGISTRY_HOSTNAME="gcr.io"
GITHUB_TOKEN=$(cat ./token.txt)

echo "building $VERSION...."
if [[ "$VERSION" == "latest" ]]; then
  docker build . \
    --build-arg "GITHUB_TOKEN=$GITHUB_TOKEN" \
    --build-arg "TOKEN_VERSION=master" \
    -t "$COMPONENT:$VERSION" \
    -t "$REGISTRY_HOSTNAME/$PROJECT/$COMPONENT:latest"
else
  docker build . \
    --build-arg "GITHUB_TOKEN=$GITHUB_TOKEN" \
    --build-arg "TOKEN_VERSION=$VERSION" \
    -t "$COMPONENT:$VERSION" \
    -t "$REGISTRY_HOSTNAME/$PROJECT/$COMPONENT:$VERSION"
fi