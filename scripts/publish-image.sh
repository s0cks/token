#!/usr/bin/env bash

VERSION="$1"
PROJECT="tkn-events"
COMPONENT="token-ledger"
REGISTRY_HOSTNAME="gcr.io"
echo "publishing $COMPONENT v${VERSION}...."
git tag "v${VERSION}"
git push --tags
docker push "$REGISTRY_HOSTNAME"/"$PROJECT"/"$COMPONENT":"$VERSION"