#!/usr/bin/env bash

VERSION="$1"
PROJECT="tkn-events"
COMPONENT="token-ledger"
REGISTRY_HOSTNAME="gcr.io"
echo "publishing v${VERSION}...."
docker push "$REGISTRY_HOSTNAME"/"$PROJECT"/"$COMPONENT":"$VERSION"