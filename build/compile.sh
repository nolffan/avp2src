#!/usr/bin/env bash
# Compile and link: build/build.mk, in parallel, in the tree that is built.
#
#   build/compile.sh [target...]     (inside the build container)
#
# Targets are build/build.mk's: all (the default), a project by its workspace
# name (ClientShellDLL, Object, ClientRes, ServerRes, ServerApp), or any
# output file. Started in a checkout it builds in out/release-tree/ (src/
# without its variant blocks: build/env.sh, avp2src_release_tree) and copies
# the outputs back into this checkout's src/; in a build/variant.sh tree,
# there. AVP2SRC_JOBS sets the number of parallel jobs (default: the CPU
# count).
set -e
source "$(dirname "$0")/env.sh"
avp2src_release_tree "$@"
avp2src_make "${@:-all}"
