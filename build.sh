#!/usr/bin/env bash

VERSION="$1"
PROJECT="tkn-events"
COMPONENT="token-ledger"
REGISTRY_HOSTNAME="gcr.io"
echo "building $VERSION...."
docker build . \
  -t "$COMPONENT":"$VERSION" \
  -t "$REGISTRY_HOSTNAME"/"$PROJECT"/"$COMPONENT":"$VERSION"