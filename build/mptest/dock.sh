#!/usr/bin/env bash
# dock.sh <container name> <command...>: the build image, this checkout at
# /repo, on the docker network mp-net (made if missing), as the Makefile's
# DOCKER_RUN runs it.
set -u
N="$1"; shift
docker network inspect mp-net >/dev/null 2>&1 || docker network create mp-net >/dev/null
exec docker run --rm --init --name "$N" --hostname "$N" --network mp-net \
    -u "$(id -u):$(id -g)" -e HOME=/tmp/home ${DOCK_ENV:-} \
    -v "$(cd "$(dirname "$0")/../.." && pwd)":/repo -w /repo avp2src-build "$@"
