# Aliens versus Predator 2 and Primal Hunt, built from source.
#
#   make          build both games into out/avp2-legacy-release/ (and its -private / -public .tar.gz)
#   make modern   the games with AVP2SRC_MODERN, in out/avp2-modern-release/
#   make progress how much of the build is byte-identical to the retail game;
#                 fails if a function recorded as identical stops matching
#   make help     everything else
#
# Needs only docker on the host and the submodules (`git submodule
# update --init`): the compilers, ext/toolchains/ (ext/toolchains/README.md); ext/rez/,
# which fetches the redump images of the European discs from archive.org
# and installs Primal Hunt, and AvP2 with its English 1.0.9.6 patch, from
# them (ext/rez/README.md;
# `make retail`, which every build runs first, does it once); and ext/tools/, the
# reconstruction tools; and, for the modern build only, ext/openmiles/
# (OpenMiles) and ext/ffmpeg/ (FFmpeg), its open middleware (build/ext.sh).
# Every step runs in a throwaway container of the image
# docker/Dockerfile describes, with this checkout mounted at /repo, as you.

IMAGE     ?= avp2src-build
CONTAINER ?= avp2src-build
# Parallel compile jobs inside the build (build/build.mk); empty: one per CPU
JOBS ?=
# The game directory `make test` plays: the release build's, or a variant's
# (make test TEST_GAME=out/avp2-modern-release; build/variant.sh), which of
# its games (avp2, ph, or both in turn), the world (- : the main menu) and
# the input script (build/playtest-*.txt; empty: none, idle TEST_SECS)
TEST_GAME ?= out/avp2-legacy-release
TEST_WHICH ?= avp2 ph
TEST_WORLD ?= -
TEST_SCRIPT ?=
TEST_SECS ?= $(if $(TEST_SCRIPT),5,60)
TEST_NAME = $(if $(filter out/avp2-legacy-release,$(TEST_GAME:%/=%)),test,test-$(notdir $(TEST_GAME:%/=%)))

DOCKER_RUN = docker run --rm --init --name $(CONTAINER) \
	-u $(shell id -u):$(shell id -g) -e HOME=/tmp/home $(if $(JOBS),-e AVP2SRC_JOBS=$(JOBS)) \
	$(if $(AVP2_ARCHIVES),-e AVP2_ARCHIVES="$(AVP2_ARCHIVES)") \
	-v $(CURDIR):/repo -w /repo $(IMAGE)

# The VC6 revisions the Rich headers name (progress/COMPILERS.md): SP5's
# 8966 for Primal Hunt's engine and game DLLs (and the lithshared
# libraries), 8168 for AvP2 1.0.9.6's game DLLs, the Processor Pack's back
# end (9044) for AvP2's launcher.
TOOLCHAIN_FILES = ext/toolchains/msvc-6.00-9044/BIN/cl.exe ext/toolchains/msvc-6.00-8966/BIN/cl.exe \
	ext/toolchains/msvc-6.00-8168/BIN/cl.exe ext/toolchains/msvc-6.00-8168/BIN/link.exe

.PHONY: all game debug modern modern-debug image retail check-rez check-tools check-toolchain check-ext \
	test reference progress progress-update shell clean distclean help

all: game

help:
	@echo "make            build both games into out/avp2-legacy-release/ (AVP2/, PrimalHunt/)"
	@echo "                and its -private / -public .tar.gz archives"
	@echo "make debug      the games with symbols and AVP2SRC_DEBUG logging, in out/avp2-legacy-debug/"
	@echo "make modern     the games with AVP2SRC_MODERN, in out/avp2-modern-release/"
	@echo "make modern-debug  both, in out/avp2-modern-debug/"
	@echo "                (none of these is ever scored)"
	@echo "make test       run AvP2 and then Primal Hunt from out/avp2-legacy-release/ under Xvfb"
	@echo "                and Wine, to the main menu (build/playtest.sh; TEST_WHICH=avp2|ph,"
	@echo "                TEST_WORLD=<world>, TEST_SCRIPT=<script>, TEST_SECS=<n>,"
	@echo "                TEST_GAME=out/avp2-modern-release; results in out/play/test*/)"
	@echo "make progress   score the build against the retail binaries; fails if a"
	@echo "                function recorded in progress/identical.txt stops matching"
	@echo "make progress-update  record the score in progress/ and README.md"
	@echo "make reference  extract the retail binaries and their function tables into out/reference/"
	@echo "make retail     fetch the disc images and install Primal Hunt, and AvP2 with its"
	@echo "                1.0.9.6 patch, from them, and fetch the USA releases' discs too, for"
	@echo "                the public archive (ext/rez/, once; every build runs it); check them"
	@echo "make shell      a shell in the build container (build/*.sh, ext/tools/ work there)"
	@echo "make image      (re)build the docker image $(IMAGE)"
	@echo "make clean      remove out/ and every build output in the source tree"
	@echo "make distclean  clean, and remove the docker image too"
	@echo
	@echo "Variables: JOBS=<n> (parallel compiles; default one per CPU)"
	@echo "           AVP2_ARCHIVES=\"private public\" (the archives build/package.sh writes)"
	@echo "           IMAGE_BUILD=0 (use the docker image as it is, as CI does)"
	@echo "           COMPLETE=1, COVERED=1, ACCEPT_LOSSES=1 (progress/README.md)"

game: check-toolchain retail
	$(DOCKER_RUN) bash build/all.sh

debug: check-toolchain retail
	$(DOCKER_RUN) bash build/variant.sh $@

# the modern build also builds OpenMiles and its Bink player from ext/
# (build/ext.sh)
modern modern-debug: check-toolchain check-ext retail
	$(DOCKER_RUN) bash build/variant.sh $@

# Each game in turn (build/playtest.sh), every one run even when one fails.
test: image
	@[ -f $(TEST_GAME)/AVP2/lithtech.exe ] && [ -f $(TEST_GAME)/PrimalHunt/lithtech.exe ] || \
	  { echo "error: no $(TEST_GAME)/ -- build it first (make, or make debug/modern/modern-debug)" >&2; exit 1; }
	@rc=0; for g in $(TEST_WHICH); do \
	  echo "== $$g"; \
	  $(DOCKER_RUN) env PLAYTEST_NAME=$(TEST_NAME) $(if $(TEST_SCRIPT),PLAYTEST_SCRIPT=$(TEST_SCRIPT)) \
	    bash build/playtest.sh $(TEST_GAME) $$g '$(TEST_WORLD)' $(TEST_SECS) || rc=1; \
	done; exit $$rc

# IMAGE_BUILD=0: use $(IMAGE) as it is (CI builds it with a layer cache
# first, .github/workflows/public-archives.yml).
IMAGE_BUILD ?= 1
image:
ifeq ($(IMAGE_BUILD),0)
	@docker image inspect $(IMAGE) >/dev/null 2>&1 || \
	  { echo "error: IMAGE_BUILD=0, but there is no docker image $(IMAGE)" >&2; exit 1; }
else
	@docker build -q -t $(IMAGE) docker/ >/dev/null || \
	  { echo "error: could not build the docker image (docker/Dockerfile)" >&2; exit 1; }
endif
	@echo "== docker image $(IMAGE) ready"

# The retail games the build takes its data from (build/retail.sh): installed
# by ext/rez/ the first time, checked against ext/rez/games.sha256 every time; and
# the USA releases' disc images, which the public archive's install scripts
# read too.
retail: check-rez check-tools image
	@$(DOCKER_RUN) bash -c 'source build/retail.sh && retail_install && retail_discs && retail_verify'

reference: retail
	$(DOCKER_RUN) bash build/reference.sh

check-rez:
	@[ -f ext/rez/install-games.sh ] || { echo "error: ext/rez/ is empty -- the disc images and their installer are the ext/rez/ submodule: run git submodule update --init (see ext/rez/README.md)" >&2; exit 1; }

check-tools:
	@[ -f ext/tools/progress.py ] || { echo "error: ext/tools/ is empty -- the reconstruction tools are the ext/tools/ submodule: run git submodule update --init" >&2; exit 1; }

check-ext:
	@for d in ext/openmiles ext/ffmpeg; do \
	  [ -f $$d/LICENSE ] || [ -f $$d/LICENSE.md ] || { echo "error: $$d is empty -- the modern build's middleware is the ext/ submodules: run git submodule update --init" >&2; exit 1; }; \
	done

check-toolchain:
	@for f in $(TOOLCHAIN_FILES); do \
	  [ -f "$$f" ] || { echo "error: $$f is missing -- the compilers are the ext/toolchains/ submodule: run git submodule update --init (see ext/toolchains/README.md)" >&2; exit 1; }; \
	done

# what ext/tools/evidence.py cannot ask git for in the container (the build
# image has no git)
EVIDENCE_ENV = EVIDENCE_REVISION=$(shell git rev-parse HEAD 2>/dev/null) \
	EVIDENCE_MODIFIED=$(shell git status --porcelain --untracked-files=no 2>/dev/null | wc -l) \
	EVIDENCE_SUBMODULES="$(shell git submodule status 2>/dev/null | sed 's/^ *//' | tr '\n' ';')"

progress: retail
	$(DOCKER_RUN) env COMPLETE=$(COMPLETE) COVERED=$(COVERED) $(EVIDENCE_ENV) bash build/progress.sh check

progress-update: retail
	$(DOCKER_RUN) env ACCEPT_LOSSES=$(ACCEPT_LOSSES) $(EVIDENCE_ENV) bash build/progress.sh update

shell: image
	docker run --rm -it --init --name $(CONTAINER)-shell \
	  -u $(shell id -u):$(shell id -g) -e HOME=/tmp/home \
	  -v $(CURDIR):/repo -w /repo $(IMAGE) bash

clean:
	rm -rf out .wine .build
	[ ! -d src ] || find src -depth -type d \( -name built -o -name Built \) -exec rm -rf {} +

distclean: clean
	-docker rmi $(IMAGE)
