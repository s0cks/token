#!/bin/bash
# This is expected to run as root for setting the ulimits
set -e
ulimit -n 65536
ulimit -u 2048
ulimit -l unlimited
su token bin/es-docker "$@"