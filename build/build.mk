# build/build.mk -- every compile and link of the game, as one Makefile.
#
# Monolith's Visual C++ 6 workspace (src/AVP2/AVP2.dsw, the source release's
# own) as Developer Studio would build it: tools/dsp2mk.py reads the
# workspace and its project files and writes the rules (.build/*.mk) --
# every compile with the flags its project's configuration gives it
# ("Final", what the retail game DLLs are: none of the strings the release
# keeps for `#ifndef _FINAL` is in them; a project's "Release" where it has
# no "Final"), one source per compile, every library and link. `make -j`
# then runs the compiles of every binary at once, and a rebuild redoes only
# what changed: an object is rebuilt when its source, a header it includes
# (tools/mkdeps.py finds them), or its command line changes (the line is
# kept in <object>.cmd), a binary when any of its inputs does.
#
# It is run from the root of the tree being built, never from a checkout:
# build/compile.sh runs it in out/release-tree/ (src/ without its variant
# blocks, build/env.sh) and build/variant.sh in out/build-<variant>/, as
#
#   make -f build/build.mk -j<N> [target...]
#
# Targets: all (the default: the workspace's wanted projects, below); a
# project by its workspace name; any single output.
#
# The compilers see the tree's src/ as drive S: and toolchains/ as T:, so
# they see the same paths wherever the tree is. The project files name the
# SDK and the libraries through $(LT2_DIR), which Monolith's machines set to
# C:\PROJ\LT2 (src/AVP2/SetBuildVars.bat): here it is S:\LT2 (dsp2mk.py
# --macro).
#
# From the environment (build/variant.sh sets them; empty for the legacy
# build, whose command lines are then exactly the project files' ones):
#   AVP2SRC_CFLAGS   appended to every compile (/D AVP2_MODERN, /Z7 ...)
#   AVP2SRC_LDFLAGS  put first on every link
#   AVP2SRC_RCFLAGS  put first on every resource compile

ROOT := $(CURDIR)
SHELL := /bin/bash
.SUFFIXES:
.DELETE_ON_ERROR:
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.DEFAULT_GOAL := all

ifeq ($(wildcard $(ROOT)/src/AVP2/AVP2.dsw),)
$(error run build/build.mk from the root of a tree, as build/compile.sh does)
endif

empty :=
space := $(empty) $(empty)
comma := ,
tab := $(empty)	$(empty)
define nl


endef
# one shell word per make word, quoted: Windows paths keep their backslashes
q = $(foreach w,$(1),'$(subst ','\'',$(w))')

AVP2SRC_CFLAGS  ?=
AVP2SRC_LDFLAGS ?=
AVP2SRC_RCFLAGS ?=

# The workspaces, each with the projects of it the game needs. A workspace
# that is not in the tree yet is skipped: the game is built up one binary at
# a time (docs/PLAN.md), and each binary's target in progress/targets.py is
# optional until its workspace is here.
#   src/AVP2/AVP2.dsw   AvP2's game DLLs, their resources and the dedicated
#                       server (the source release's workspace)
#   src/AVP2/AVP2PH.dsw Primal Hunt's game DLLs: the same sources, through
#                       project files of their own (<name>_PH.dsp, first
#                       made by build/phproject.py) whose Final defines
#                       PRIMAL_HUNT and builds into PrimalHunt/
WORKSPACES := \
    src/AVP2/AVP2.dsw:ClientShellDLL,Object,ClientRes,ServerRes,ServerApp \
    src/AVP2/AVP2PH.dsw:ClientShellDLL_PH,Object_PH

# ---------------------------------------------------------------------------
# Wine and the toolchains
export WINEDEBUG := -all
export WINEDLLOVERRIDES := mscoree,mshtml=
WINE := wine
PFX := $(ROOT)/.wine/drives

# The Rich headers (progress/COMPILERS.md): AvP2's game DLLs are cl
# 12.00.8168 (VC6 RTM)'s code, linked by link 6.00.8447 with VC6 SP5's C
# runtime (its C objects are build 8047's, which msvc-6.00-8966's
# libcmt.lib holds), against the release's lithshared libraries (cl 8966).
# So: 8168's compiler, SP5's linker, librarian, headers and libraries. The
# dedicated server is 8168's throughout, its linker and MFC included
# (CL_ServerApp, LINK_ServerApp, ENV_ServerApp). The resource compiler is
# SP6's (msvc-6.00-9782), as nolfsrc's. The DirectX 8.1 SDK comes first on
# the paths, as the release's readme sets it up.
TC      := $(ROOT)/toolchains
V8168   := $(TC)/msvc-6.00-8168
V8966   := $(TC)/msvc-6.00-8966
DXSDK   := $(TC)/dxsdk-8.1
CL      := $(V8168)/BIN/cl.exe
LINK    := $(V8966)/BIN/link.exe
LIBEXE  := $(V8966)/BIN/lib.exe
RC      := $(TC)/msvc-6.00-9782/Common/MSDev98/Bin/RC.EXE

# Windows paths: src/ is S:, toolchains/ T:, the rest Z:
wz = z:$(subst /,\,$(1))
ws = $(if $(filter $(ROOT)/src $(ROOT)/src/%,$(1)),s:$(subst /,\,$(patsubst $(ROOT)/src%,%,$(1))),$(if $(filter $(TC) $(TC)/%,$(1)),t:$(subst /,\,$(patsubst $(TC)%,%,$(1))),$(call wz,$(1))))

# The environment of a tool run, by VC6 tree. src/avp2src/ (the project's
# own headers) is on every compiler's include path, last.
# $(call vc_env,<VC6 tree>)
vc_env = WINEPREFIX=$(PFX) WINEPATH='$(call ws,$(1))\BIN' \
    INCLUDE='$(call ws,$(DXSDK))\include;$(call ws,$(1))\INCLUDE;$(call ws,$(1))\MFC\INCLUDE;$(call ws,$(ROOT)/src/avp2src)' \
    LIB='$(call ws,$(DXSDK))\lib;$(call ws,$(1))\LIB;$(call ws,$(1))\MFC\LIB'
ENV_GAME := $(call vc_env,$(V8966))
# Primal Hunt's game DLLs are SP5's (cl 12.00.8966) for all but a few
# objects (progress/COMPILERS.md).
CL_ClientShellDLL_PH := $(V8966)/BIN/cl.exe
CL_Object_PH         := $(V8966)/BIN/cl.exe
CL_ServerApp   := $(V8168)/BIN/cl.exe
LINK_ServerApp := $(V8168)/BIN/link.exe
ENV_ServerApp  := $(call vc_env,$(V8168))

# The prefix: made the first time, then only checked; every tool run waits
# for it (an order-only prerequisite, so it causes no rebuild).
$(PFX)/.ready:
	@echo "   WINE prefix $(@D)"
	@mkdir -p $(@D) && export WINEPREFIX=$(@D) && { $(WINE) wineboot -i >/dev/null 2>&1; \
	  for v in TEMP TMP; do $(WINE) reg add 'HKCU\Environment' /v $$v /t REG_SZ /d 'C:\windows\temp' /f >/dev/null 2>&1; done; \
	  wineserver -w; } && test -f $(@D)/system.reg && touch $@
.PHONY: wine-drives FORCE
wine-drives: $(PFX)/.ready
	@ln -sfn $(ROOT)/src $(PFX)/dosdevices/s:
	@ln -sfn $(TC) $(PFX)/dosdevices/t:

# ---------------------------------------------------------------------------
# The machinery
#
# Every output's command is a variable, cmd_<output>, that the rule runs and
# keeps in <output>.cmd. An output whose command differs from the kept one
# is rebuilt (FORCE): a changed flag, a file added to a link.
cmd_changed = $(call str_ne,$(strip $(cmd_$(1))),$(strip $(file <$(1).cmd)),FORCE)
str_ne = $(if $(subst x$(1),,x$(2))$(subst x$(2),,x$(1)),$(3))

OBJECTS :=
DEPSPEC :=
TOOLS :=

# $(call compile,<object>,<working dir>,<source as spelled>,<env>,<exe>,<flags>,<prefix>)
# One compile: `<exe> <flags> <source> $(AVP2SRC_CFLAGS)` in <working dir>
# (the variant flags last: /Oy- must follow the /O2 that implies /Oy).
define compile
OBJECTS += $(1)
cmd_$(1) := cd $(2) && $(4) $(WINE) $(5) $(call q,$(6) $(3) $(AVP2SRC_CFLAGS))
DEPSPEC += $(subst $(space),|,$(1)$(tab)$(2)$(tab)$(3)$(tab)$(strip $(6)))
$(1): $$(call cmd_changed,$(1)) | $(7)
endef

# $(call tool,<output>,<prerequisites>,<command>): any other build step
define tool
cmd_$(1) := $(3)
$(1): $$(call cmd_changed,$(1)) $(2)
TOOLS += $(1)
endef

# $(call rcfile,<res>,<project dir>,<rc as spelled>,<rc in the tree>,<flags>)
# What the script can include tools/mkdeps.py finds, as it finds a
# compile's headers.
define rcfile
DEPSPEC += $(subst $(space),|,$(1)$(tab)$(2)$(tab)$(3)$(tab)$(strip $(AVP2SRC_RCFLAGS) $(5)))
$(call tool,$(1),$(4) | wine-drives,\
    cd $(2) && $(ENV_GAME) $(WINE) $(RC) $(AVP2SRC_RCFLAGS) $(call q,$(5) $(3)))
endef

# $(call library,<lib>,<project dir>,<flags>,<objects>,<objects as the project dir sees them>,<env>,<lib.exe>)
define library
$(call tool,$(1),$(4) | wine-drives,\
    cd $(2) && { printf '%s\n' $(call q,$(3) $(subst /,\,$(5))) > $(1:%=$(ROOT)/%).rsp; } && \
    $(6) $(WINE) $(7) '@$(call ws,$(ROOT)/$(1)).rsp')
endef

# $(call binary,<dll or exe>,<project dir>,<flags>,<prerequisites>,<inputs as the project dir sees them>,
#   <the libraries' precompiled-header objects, as the project dir sees them>,<env>,<link.exe>)
# Through a response file, as Developer Studio links; with a map beside it.
# Compiled with /Z7 (the debug builds), the libraries' precompiled-header
# objects are linked too: the others refer to the type information in them
# (LNK1211 without). Every link waits for the prebuilt libraries (below),
# which a project may name only through /libpath.
Z7 := $(findstring /Z7,$(AVP2SRC_CFLAGS))
define binary
$(call tool,$(1),$(4) | wine-drives prebuilt,\
    cd $(2) && { printf '%s\n' $(call q,$(AVP2SRC_LDFLAGS) $(3) /map:$(call ws,$(ROOT)/$(basename $(1))).map $(subst /,\,$(5) $(if $(Z7),$(6)))) > $(1:%=$(ROOT)/%).rsp; } && \
    $(7) $(WINE) $(8) '@$(call ws,$(ROOT)/$(1)).rsp')
endef

# $(call custom,<outputs>,<input>,<project dir>,<commands>): a source's
# custom build step, its commands run by cmd.exe
define custom
$(1) &: $(2) | wine-drives
	@echo "   GEN  $(1)"
	@cd $(3) && $(ENV_GAME) $(WINE) cmd /c $(call q,$(4)) >/dev/null
endef

# ---------------------------------------------------------------------------
# The prebuilt libraries
#
# The release ships most of the lithshared libraries as binaries only
# (progress/reference/LT2/lithshared/libs/, as it came), and the dedicated
# server's import library for server.dll (AVP2Serv/server.lib). Until each
# is built from source here (docs/PLAN.md, packages 4 and 7), the links
# take them from there, into the places the project files name them:
# $(LT2_DIR)\lithshared\libs\Release\ and the server's own directory. The
# scoreboard leaves them out of our side (progress/targets.py,
# third_party): their code in our images is Monolith's, not ours.
PREBUILT_DIR := progress/reference/LT2/lithshared/libs/release
PREBUILT := $(patsubst $(PREBUILT_DIR)/%,src/LT2/lithshared/libs/Release/%,$(wildcard $(PREBUILT_DIR)/*.lib)) \
    src/AVP2/AVP2Serv/server.lib
src/LT2/lithshared/libs/Release/%.lib: $(PREBUILT_DIR)/%.lib
	@mkdir -p $(@D) && cp $< $@
src/AVP2/AVP2Serv/server.lib: progress/reference/AVP2/AVP2Serv/server.lib
	@cp $< $@
.PHONY: prebuilt
prebuilt: $(PREBUILT)

# ---------------------------------------------------------------------------
# The projects (tools/dsp2mk.py)

DEPDIR := $(ROOT)/.build
$(shell mkdir -p $(DEPDIR))
PROJECTS :=
WANTED :=
DSP2MK := python3 tools/dsp2mk.py --macro 'LT2_DIR=s:\LT2' --drive s=src
# $(call workspace,<dsw>,<projects, comma-separated>): its rules, from
# tools/dsp2mk.py, which gets the projects earlier workspaces already build
# (--have), so that a library two workspaces share is built once.
define workspace
ifneq ($(wildcard $(ROOT)/$(1)),)
DSP_OK := $$(shell $(DSP2MK) --have "$$(PROJECTS)" $(1) Final $(DEPDIR)/$(basename $(notdir $(1))).mk $(subst $(comma), ,$(2)) 2>$(DEPDIR)/dsp2mk-$(basename $(notdir $(1))).log && echo ok)
ifneq ($$(DSP_OK),ok)
$$(error tools/dsp2mk.py failed on $(1): $$(file <$(DEPDIR)/dsp2mk-$(basename $(notdir $(1))).log))
endif
include $(DEPDIR)/$(basename $(notdir $(1))).mk
WANTED += $$(filter $$(PROJECTS),$(subst $(comma), ,$(2)))
endif
endef
$(foreach w,$(WORKSPACES),$(eval $(call workspace,$(firstword $(subst :, ,$(w))),$(lastword $(subst :, ,$(w))))))

.PHONY: all game
all game: $(WANTED)

# ---------------------------------------------------------------------------
# The engine: lithtech.exe, server.dll, d3d.ren
#
# nolfsrc's reconstruction of NOLF's engine (src/engine/, imported from it:
# docs/PLAN.md, package 7), to be brought to Primal Hunt's. Its file lists
# and per-file flags are nolfsrc's (build/build.mk there), its compiler
# Primal Hunt's: cl 12.00.8966 throughout, the renderer included (Primal
# Hunt's d3d.ren is Microsoft's code, where NOLF's was Intel C/C++ 4.0's;
# its files build /O2 until their flags are known). Objects only, so far:
# the links need nolfsrc's lithshared libraries (StdLith, RezMgr, Lith) and
# its link-time tables (link/vtables.txt, link/aliases.rsp), which name
# NOLF's addresses. `make progress` scores the objects (progress/targets.py,
# partial_objs).
E := src/engine
ENG_ENV := $(ENV_GAME)
ENG_CC  := $(V8966)/BIN/cl.exe
c_obj = $(2)/$(basename $(notdir $(subst \,/,$(1)))).obj

# server.dll: nolfsrc's /O1, no /GX; localdriver.cpp /Os
SERVER := $(E)/server
SERVER_CL := /MT /W3 /O1 /D PRIMAL_HUNT /D WIN32 /D NDEBUG /D _WINDOWS /D LT_SERVER_TIMERS /D LT_SERVER_BUILD /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I . /c /FoRelease/
SERVER_SRC := server.cpp servermgr.cpp servermsg.cpp netmgr.cpp servermath.cpp commonphysics.cpp commonmodel.cpp \
    sharedcommon.cpp servermodel.cpp serverlightanim.cpp servercommon.cpp servermessage.cpp packet.cpp \
    lthread.cpp smgr.cpp counter.cpp dalloc.cpp modelanim.cpp streamsim.cpp fsstream.cpp basestream.cpp \
    objbank.cpp objtypes.cpp collection.cpp worldbsp.cpp worldmodels.cpp saveio.cpp worldsim.cpp \
    serverengine.cpp ltobject.cpp lmessage.cpp spritecontrol.cpp tcpdriver.cpp stringbank.cpp dsi.cpp \
    genericprop.cpp dhash.cpp filemgr.cpp console.cpp structbank.cpp csbase.cpp sdkforce.cpp netobj.cpp \
    savrest.cpp shade.cpp smgrnew.cpp modelnew.cpp objmover.cpp smgrmisc.cpp svnew.cpp svlast.cpp \
    svtiny.cpp worldtree.cpp
SERVER_OS_SRC := localdriver.cpp
server_flags = $(if $(filter $(1),$(SERVER_OS_SRC)),$(subst /O1,/Os /Ob1 /Gf,$(SERVER_CL)),$(SERVER_CL))
$(foreach s,$(SERVER_SRC) $(SERVER_OS_SRC),$(eval $(call compile,$(SERVER)/Release/$(s:.cpp=.obj),$(SERVER),$(s),$(ENG_ENV),$(ENG_CC),\
    /nologo $(call server_flags,$(s)),wine-drives)))
SERVER_OBJ := $(sort $(patsubst %.cpp,$(SERVER)/Release/%.obj,$(SERVER_SRC) $(SERVER_OS_SRC)))
.PHONY: server-objs
server-objs: $(SERVER_OBJ)

# lithtech.exe: nolfsrc's /O2; its copy of server's localdriver.cpp /Od with
# intrinsics, inlining and string pooling; controlfile.cpp with /GX
LITHTECH := $(E)/lithtech
LITHTECH_CL := /MT /W3 /O2 /D PRIMAL_HUNT /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I ..\server /I . /c /FoRelease/
LITHTECH_SRC := lithtech.cpp ..\server\msgterm.cpp ..\server\sdkforce.cpp ..\server\netobj.cpp \
    ..\server\smgrnew.cpp ..\server\modelnew.cpp ..\server\objmover.cpp ..\server\smgrmisc.cpp \
    ..\server\shade.cpp wmrotate.cpp collelem.cpp cchandler.cpp cmodelload.cpp dmextra.cpp csvis.cpp \
    diblit.cpp savobj.cpp conline.cpp nettrace.cpp modchunk.cpp partfm.cpp ccobj.cpp demodtx.cpp \
    cissurf.cpp cltapi.cpp sndinst2.cpp sndmgr2.cpp predtick.cpp cmgrinit.cpp wgrid.cpp dmgrow.cpp \
    cmgsurf.cpp warpblit.cpp startload.cpp fntdraw.cpp predobj.cpp fmcvt32.cpp fmconvert.cpp \
    ccclient.cpp clientshell.cpp ccline.cpp binkvideo.cpp smackvideo.cpp lvideomgr.cpp soundinst2.cpp \
    tcpqarray.cpp clientmgr3.cpp clientwin.cpp worldtree2.cpp transform.cpp clientinterface.cpp \
    clientphysics.cpp clientsound.cpp dmloader.cpp clientmgr2.cpp stdlithalloc.cpp clientsoundinst.cpp \
    clientworld.cpp cltcursor.cpp clientconsole.cpp clientdsi.cpp lttexmod.cpp clienttexmgr.cpp \
    clientstreams.cpp clientinput.cpp clientsoundmgr.cpp clientworldinfo.cpp clientblit.cpp \
    clientrender.cpp ltnew1.cpp cltnew.cpp cltrest.cpp lt2new1.cpp lt2new2.cpp lt2new3.cpp lt2new4.cpp \
    ltnew2.cpp ..\server\csbase.cpp ..\server\servermgr.cpp ..\server\servermsg.cpp \
    ..\server\netmgr.cpp ..\server\servermath.cpp ..\server\commonphysics.cpp ..\server\commonmodel.cpp \
    ..\server\sharedcommon.cpp ..\server\servermodel.cpp ..\server\serverlightanim.cpp \
    ..\server\servercommon.cpp ..\server\servermessage.cpp ..\server\packet.cpp ..\server\lthread.cpp \
    ..\server\smgr.cpp ..\server\ltobject.cpp ..\server\lmessage.cpp ..\server\spritecontrol.cpp \
    ..\server\server.cpp ..\server\stringbank.cpp ..\server\dsi.cpp ..\server\genericprop.cpp \
    ..\server\dhash.cpp ..\server\filemgr.cpp ..\server\console.cpp ..\server\structbank.cpp \
    ..\server\counter.cpp ..\server\dalloc.cpp ..\server\modelanim.cpp ..\server\basestream.cpp \
    ..\server\collection.cpp ..\server\fsstream.cpp ..\server\objbank.cpp ..\server\objtypes.cpp \
    ..\server\saveio.cpp ..\server\serverengine.cpp ..\server\streamsim.cpp ..\server\tcpdriver.cpp \
    ..\server\worldbsp.cpp ..\server\worldmodels.cpp ..\server\worldsim.cpp worldtree.cpp clientmusic.cpp
LITHTECH_OD_SRC := ..\server\localdriver.cpp
LITHTECH_GX_SRC := controlfile.cpp
lithtech_flags = $(subst /O2,$(strip \
    $(if $(filter $(1),$(LITHTECH_OD_SRC)),/Od /Oi /Ob1 /Gf,\
    $(if $(filter $(1),$(LITHTECH_GX_SRC)),/GX /O2,/O2))),$(LITHTECH_CL))
LITHTECH_ALL := $(LITHTECH_SRC) $(LITHTECH_OD_SRC) $(LITHTECH_GX_SRC)
$(foreach s,$(LITHTECH_ALL),$(eval $(call compile,$(call c_obj,$(s),$(LITHTECH)/Release),$(LITHTECH),$(s),$(ENG_ENV),$(ENG_CC),\
    /nologo $(call lithtech_flags,$(s)),wine-drives)))
LITHTECH_OBJ := $(sort $(foreach s,$(LITHTECH_ALL),$(call c_obj,$(s),$(LITHTECH)/Release)))
.PHONY: lithtech-objs
lithtech-objs: $(LITHTECH_OBJ)

# d3d.ren: every file /O2 for now (nolfsrc's flag groups are Intel C/C++'s)
D3D := $(E)/d3d
D3D_SRC := $(notdir $(wildcard $(D3D)/*.cpp))
# Not the renderer's own: StdLith's l_allocator.cpp (LAlloc, g_DefAlloc),
# library code linked into d3d.ren, built as StdLith is (nolfsrc's
# w12_7fae0.cpp)
D3D_CL_SRC := w12_7fae0.cpp
$(foreach s,$(D3D_CL_SRC),$(eval $(call compile,$(D3D)/Release/$(s:.cpp=.obj),$(D3D),$(s),$(ENG_ENV),$(ENG_CC),\
    /nologo /MT /W3 /GX /O2 /D PRIMAL_HUNT /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /c /I ..\..\LT2\lithshared\stdlith /FoRelease/,wine-drives)))
$(foreach s,$(filter-out $(D3D_CL_SRC),$(D3D_SRC)),$(eval $(call compile,$(D3D)/Release/$(s:.cpp=.obj),$(D3D),$(s),$(ENG_ENV),$(ENG_CC),\
    /nologo /MT /W3 /O2 /D PRIMAL_HUNT /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I . /c /FoRelease/,wine-drives)))
D3D_OBJ := $(sort $(D3D_SRC:%.cpp=$(D3D)/Release/%.obj))
.PHONY: d3d-objs engine-objs
d3d-objs: $(D3D_OBJ)
engine-objs: server-objs lithtech-objs d3d-objs

# ---------------------------------------------------------------------------
# The rules all compiles and tools share

dups := $(strip $(foreach o,$(sort $(OBJECTS)),$(if $(word 2,$(filter $(o),$(OBJECTS))),$(o))))
ifneq ($(dups),)
$(error two sources compile to one object: $(dups))
endif

# Under Wine, a compile that uses a precompiled header now and then dies
# with C1001 (an internal compiler error) or C1060 ("out of heap space")
# while other compiles run; the same compile run again succeeds, and gives
# the same object. Those two are retried, up to three tries in all (said so
# in the log); any other error fails the build at once.
$(OBJECTS):
	@echo "   CC   $@"
	@mkdir -p $(@D) && rm -f $@
	@for try in 1 2 3; do \
	  out=$$( { $(cmd_$@); } 2>&1 ); rc=$$?; \
	  if [ $$rc = 0 ] || [ $$try = 3 ] || ! grep -qE 'error C10(01|60)' <<< "$$out"; then printf '%s\n' "$$out"; break; fi; \
	  echo "   CC   $@: $$(grep -oE 'error C10(01|60)' <<< "$$out" | head -1), compiling it again"; rm -f $@; \
	done; exit $$rc
	@test -f $@ || { echo "error: $@ was not built" >&2; exit 1; }
	@printf '%s\n' '$(subst ','\'',$(cmd_$@))' > $@.cmd

$(TOOLS):
	@echo "   GEN  $@"
	@mkdir -p $(@D) && rm -f $@
	@$(cmd_$@)
	@test -f $@ || { echo "error: $@ was not built" >&2; exit 1; }
	@printf '%s\n' '$(subst ','\'',$(cmd_$@))' > $@.cmd

FORCE:

# Header dependencies (tools/mkdeps.py), brought up to date on every run
$(file >$(DEPDIR)/deps.spec,$(subst |, ,$(subst $(space),$(nl),$(DEPSPEC))))
DEPS_OK := $(shell python3 tools/mkdeps.py $(DEPDIR)/deps.spec $(DEPDIR)/deps.mk && echo ok)
ifneq ($(DEPS_OK),ok)
$(error tools/mkdeps.py failed)
endif
include $(DEPDIR)/deps.mk
