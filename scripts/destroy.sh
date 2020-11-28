#!/usr/bin/env bash
ENVIRONMENT="$1"
kubectl delete -f "manifests/${ENVIRONMENT}/deployment.yml"
kubectl delete -f "manifests/${ENVIRONMENT}/service.yml"