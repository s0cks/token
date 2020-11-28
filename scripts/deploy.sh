#!/usr/bin/env bash
ENVIRONMENT="$1"
kubectl apply -f "manifests/${ENVIRONMENT}/deployment.yml"
kubectl apply -f "manifests/${ENVIRONMENT}/service.yml"