#!/usr/bin/env bash
# `make`: build the game.
#
#   build/all.sh          (inside the build container; the Makefile runs it)
#
# For now this compiles and links what has source (build/compile.sh: AvP2's
# game DLLs, their resources and the dedicated server, with the release's
# prebuilt lithshared libraries); `make progress` scores it. Assembling a
# game directory from it and the retail data (out/avp2-legacy-release/) is
# docs/PLAN.md's package 3, still to do.
set -e
cd "$(dirname "${BASH_SOURCE[0]}")/.."
bash build/compile.sh all
