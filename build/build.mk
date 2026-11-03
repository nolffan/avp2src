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
# The lithshared libraries, ours
#
# AvP2's release ships them as headers (src/LT2/lithshared/) and prebuilt
# builds only (cl 8966: progress/COMPILERS.md), as NOLF's did; these are
# nolfsrc's reconstructions (src/lithshared/, from nolfsrc's), compiled with
# 8966 and nolfsrc's flags, into the very paths the project files link them
# from ($(LT2_DIR)\lithshared\libs\Release\), so the game DLLs link ours.
# progress/targets.py scores each against the release's prebuilt one
# (LIBS), object by object. Each library is lib.exe over its objects, in
# the order its sources are listed here.
LIB_CFLAGS := /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /c
LT2LS  := ..\..\LT2\lithshared
LT2LS2 := ..\\..\\LT2\\lithshared
LOUT   := src/LT2/lithshared/libs/Release
LS_OURS :=
# $(call lsobjs,<dir>,<incs>,<sources>) -> compile each into <dir>/Release
define lsobjs
$(foreach s,$(3),$(eval $(call compile,$(call c_obj,$(s),src/lithshared/$(1)/Release),src/lithshared/$(1),$(s),$(ENV_GAME),$(V8966)/BIN/cl.exe,$(LIB_CFLAGS) $(2) /FoRelease/,wine-drives)))
LSOBJ_$(1) := $(foreach s,$(3),$(call c_obj,$(s),src/lithshared/$(1)/Release))
endef
# $(call lslib,<dir>,<library>,<incs>,<sources>)
define lslib
$(call lsobjs,$(1),$(3),$(4))
$(call tool,$(LOUT)/$(2).lib,$$(LSOBJ_$(1)) | wine-drives,cd src/lithshared/$(1) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:$(call ws,$(ROOT)/$(LOUT))\$(2).lib' $$(call q,$$(subst /,\,$$(patsubst src/lithshared/$(1)/%,%,$$(LSOBJ_$(1))))))
LS_OURS += $(2)
endef
c_obj = $(2)/$(basename $(notdir $(subst \,/,$(1)))).obj

$(eval $(call lslib,stdlith,StdLith,/I $(LT2LS)\stdlith /I $(LT2LS)\lith /I . /I sys\win /I ..\zlib,\
    abstractio.cpp dynarray.cpp fileio.cpp goodlinklist.cpp helpers.cpp l_allocator.cpp \
    linklist.cpp lithexception.cpp memory.cpp memoryio.cpp stringholder.cpp struct_bank.cpp \
    sys\win\compressedio.cpp))
$(eval $(call lslib,CryptMgr,CryptMgr,/I $(LT2LS2)\\cryptmgr /I $(LT2LS2)\\incs /I .,cryptmgr.cpp blowfish.cpp))
# (lith.lib: the release names it in lower case, and so does Windows when
# ours replaces a copy of it)
$(eval $(call lslib,lith,lith,/I .,basehash.cpp baselist.cpp virtlist.cpp))
$(eval $(call lslib,rezmgr,RezMgr,/I $(LT2LS2)\\incs /I .,rezmgr.cpp rezmgr2.cpp rezfile.cpp rezemu.cpp rezdir.cpp rezhash.cpp))
$(eval $(call lslib,RegMgr,RegMgr,/I $(LT2LS2)\\incs /I .,regmgr.cpp))
$(eval $(call lslib,MFCStub,MFCStub,/I $(LT2LS2)\\mfcstub /I $(LT2LS2)\\incs /I $(LT2LS2)\\lith /I .,mfcs_string.cpp mfcs_rect.cpp mfcs_point.cpp))
# Not yet ours: AvP2's LTGUIMgr is another version of the library than
# NOLF's (menu fonts with normal, selected and disabled faces, DrawMenu,
# GetTextExtents, three fonts where NOLF's had sixteen), and its
# LithFontMgr keeps the previous string as a CString and draws with a
# default colour (lithfontdefs.h); both are to be reconstructed from the
# release's prebuilt builds, which the links take until then.

# ButeMgrNoMFC: NOLF2's released ButeMgr (src/lithshared/ButeMgr/, its .cpp
# files; nolf2src's src/libs/ButeMgr), against AvP2's headers
# (src/LT2/lithshared/butemgr/): AvP2's parser is NOLF2's line -- STLport
# symbol tables -- at an earlier stage (CString accessors, a second tag
# table). The release's prebuilt builds hold the same four objects (ARange,
# AVector, ButeMgr, Stdafx). NOLF2's flags but the static runtime (/MT), as
# AvP2's game DLLs link it: _NOMFC, STLport, StdLith, MFCStub, CryptMgr and
# Lith on the path.
$(eval $(call lslib,ButeMgr,ButeMgrNoMFC,/D _NOMFC /I $(LT2LS)\butemgr /I $(LT2LS)\stl /I $(LT2LS)\stdlith /I $(LT2LS)\mfcstub /I $(LT2LS)\cryptmgr /I $(LT2LS)\lith /I $(LT2LS)\incs,\
    arange.cpp avector.cpp butemgr.cpp stdafx.cpp))

.PHONY: libs
libs: $(LS_OURS:%=$(LOUT)/%.lib)

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
# (every one we do not build ourselves, above)
PREBUILT := $(filter-out $(LS_OURS:%=$(LOUT)/%.lib),\
    $(patsubst $(PREBUILT_DIR)/%,src/LT2/lithshared/libs/Release/%,$(wildcard $(PREBUILT_DIR)/*.lib))) \
    src/AVP2/AVP2Serv/server.lib
$(filter-out $(LS_OURS:%=$(LOUT)/%.lib),$(patsubst $(PREBUILT_DIR)/%,src/LT2/lithshared/libs/Release/%,$(wildcard $(PREBUILT_DIR)/*.lib))): src/LT2/lithshared/libs/Release/%.lib: $(PREBUILT_DIR)/%.lib
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
all game: $(WANTED) ltmsg

# ---------------------------------------------------------------------------
# The engine: lithtech.exe, server.dll, d3d.ren
#
# nolfsrc's reconstruction of NOLF's engine (src/engine/, imported from it:
# docs/PLAN.md, package 7), brought to AvP2's, built twice from one source:
#
#   Primal Hunt's   objects in <binary>/Release/; /D PRIMAL_HUNT; cl
#                   12.00.8966 throughout, the renderer included (Primal
#                   Hunt's d3d.ren is Microsoft's code; its files build /O2
#                   until their flags are known)
#   AvP2 1.0.9.6's  objects in <binary>/AvP2Release/; the engine and the server cl
#                   12.00.9044 (the Processor Pack's, as their Rich headers
#                   say), the renderer Intel C/C++ 4.0 with nolfsrc's
#                   per-file flag groups (1.0.9.6's d3d.ren is NOLF's
#                   renderer's shape)
#
# Both keep nolfsrc's file lists and per-file flags. Objects only, so far:
# the links want the lithshared libraries and nolfsrc's link-time tables,
# which name NOLF's addresses; `make progress` scores the objects
# (progress/targets.py, partial_objs).
E := src/engine
V9044 := $(TC)/msvc-6.00-9044
ENG_PH_CC   := $(V8966)/BIN/cl.exe
ENG_PH_ENV  := $(ENV_GAME)
ENG_AVP_CC  := $(V9044)/BIN/cl.exe
ENG_AVP_ENV := $(call vc_env,$(V9044))

# Intel C/C++ 4.0 runs in a prefix of its own WITHOUT the S: and T: drives:
# Wine names a working directory by the deepest drive that holds it, and
# nolfsrc found icl builds a different d3d.ren from S:\engine\d3d than from
# Z:\...\src\engine\d3d. It uses msvc-6.00-8168's headers beside its own.
PFX_PLAIN := $(ROOT)/.wine/plain
$(PFX_PLAIN)/.ready:
	@echo "   WINE prefix $(@D)"
	@mkdir -p $(@D) && export WINEPREFIX=$(@D) && { $(WINE) wineboot -i >/dev/null 2>&1; \
	  for v in TEMP TMP; do $(WINE) reg add 'HKCU\Environment' /v $$v /t REG_SZ /d 'C:\windows\temp' /f >/dev/null 2>&1; done; \
	  wineserver -w; } && test -f $(@D)/system.reg && touch $@
.PHONY: wine-plain
wine-plain: $(PFX_PLAIN)/.ready
	@rm -f $(PFX_PLAIN)/dosdevices/s: $(PFX_PLAIN)/dosdevices/t:
ICL     := $(TC)/icl-4.0/BIN/icl.exe
ICL_ENV := WINEPREFIX=$(PFX_PLAIN) WINEPATH='$(call wz,$(TC)/icl-4.0)\BIN;$(call wz,$(V8168))\BIN' \
    INCLUDE='$(call wz,$(TC)/icl-4.0)\INCLUDE;$(call wz,$(V8168))\INCLUDE;$(call wz,$(ROOT)/src/avp2src)' \
    LIB='$(call wz,$(TC)/icl-4.0)\LIB;$(call wz,$(V8168))\LIB'

# server.dll: nolfsrc's /O1, no /GX; localdriver.cpp /Os
SERVER := $(E)/server
SERVER_CL := /MT /W3 /O1 /D WIN32 /D NDEBUG /D _WINDOWS /D LT_SERVER_TIMERS /D LT_SERVER_BUILD /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I . /c
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

# lithtech.exe: nolfsrc's /O2; its copy of server's localdriver.cpp /Od with
# intrinsics, inlining and string pooling; controlfile.cpp with /GX
LITHTECH := $(E)/lithtech
LITHTECH_CL := /MT /W3 /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I ..\server /I . /c
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

# d3d.ren. Not the renderer's own: StdLith's l_allocator.cpp (LAlloc,
# g_DefAlloc), library code linked into d3d.ren, built as StdLith is
# (nolfsrc's w12_7fae0.cpp).
D3D := $(E)/d3d
D3D_SRC := $(notdir $(wildcard $(D3D)/*.cpp))
D3D_CL_SRC := w12_7fae0.cpp
D3D_CL := /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /c /I ..\..\LT2\lithshared\stdlith
D3D_BASE := /nologo /MT /W3 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I . /c
# nolfsrc's Intel C/C++ flag groups (AvP2's renderer): files not in any
# group build /O2; /Qip interprocedural register calling between one
# file's functions, /Qip_no_inlining the same without inlining, /Os
# favour size, /Oi- CRT intrinsics called, /Qrcd fast float-to-int,
# /Qsfalign16 realign the stack only for 16-byte locals, /G5 Pentium
# scheduling. Files named w<N>_*_<flags>.cpp join a group by their suffix;
# the first group that names a file gives its flags.
d3dw = $(filter w%_$(1).cpp,$(D3D_SRC))
D3D_GROUPS := OS_G5 OS_SF QIP_OS_OI_SF QIP_SF QIP_NI_SF QIP_OS_OI_NI QIP_OS_OI QIP_OS_NI RCD OI OS_OI QIP_NI OS QIP QIP_OS
D3D_OS_G5        := /Os /G5                              $(call d3dw,osg5)
D3D_OS_SF        := /Os /Qsfalign16                      $(call d3dw,ossf)
D3D_QIP_OS_OI_SF := /Os /Oi- /Qip /Qsfalign16            $(call d3dw,osoiqipsf)
D3D_QIP_SF       := /O2 /Qip /Qsfalign16                 $(call d3dw,qipsf)
D3D_QIP_NI_SF    := /O2 /Qip /Qip_no_inlining /Qsfalign16 big24090.cpp $(call d3dw,qipnisf)
D3D_QIP_OS_OI_NI := /Os /Oi- /Qip /Qip_no_inlining       $(call d3dw,osoiqipni)
D3D_QIP_OS_OI    := /Os /Oi- /Qip                        d3dblit.cpp big1ef30.cpp $(call d3dw,osoiqip)
D3D_QIP_OS_NI    := /Os /Qip /Qip_no_inlining            $(call d3dw,osqipni) big6d910.cpp
D3D_RCD          := /O2 /Qrcd                            big8fd0.cpp big2b1b0.cpp big051a0.cpp big5dca0.cpp big40e10.cpp \
                                                         w4_100234d0.cpp w16_10042e80.cpp
D3D_OI           := /O2 /Oi-                             big4b720.cpp
D3D_OS_OI        := /Os /Oi-                             d3dr5c.cpp d3dr2d_os.cpp d3dbatchd.cpp big4e3e4.cpp $(call d3dw,osoi)
D3D_QIP_NI       := /O2 /Qip /Qip_no_inlining            d3dqip4.cpp $(call d3dw,qipni)
D3D_OS           := /Os                                  big729c0.cpp d3d.cpp d3dr5a.cpp d3dr5d_os.cpp d3dbatchb_os.cpp \
    d3dmid.cpp d3dr2a_os.cpp d3dr2c_os.cpp d3dr3a_os.cpp d3ddraw_os.cpp d3dmid_os.cpp d3dneutral.cpp d3dopt2d.cpp d3dlow.cpp \
    d3dhigh.cpp d3dbatchc.cpp d3dbatcha.cpp d3dr2b.cpp d3dr3b.cpp d3dr3d.cpp d3dr4a.cpp d3dr4c.cpp d3dr4b.cpp d3dr3c_os.cpp \
    $(call d3dw,os)
D3D_QIP          := /O2 /Qip                             d3dqip.cpp d3dqip2.cpp $(call d3dw,qip)
D3D_QIP_OS       := /Os /Qip                             d3dqip3.cpp big7cc90.cpp d3dr5b.cpp
D3D_GROUPS := OS_G5 OS_SF QIP_OS_OI_SF QIP_SF QIP_NI_SF QIP_OS_OI_NI QIP_OS_OI QIP_OS_NI RCD OI OS_OI QIP_NI OS QIP QIP_OS
d3d_group = $(firstword $(foreach g,$(D3D_GROUPS),$(if $(filter $(1),$(D3D_$(g))),$(g))))
d3d_opt = $(if $(call d3d_group,$(1)),$(filter /%,$(D3D_$(call d3d_group,$(1)))),/O2)

# $(call engine_variant,<object dir>,<cl>,<env>,<defines>,<renderer: icl|cl>)
define engine_variant
$(foreach s,$(SERVER_SRC) $(SERVER_OS_SRC),$(eval $(call compile,$(SERVER)/$(1)/$(s:.cpp=.obj),$(SERVER),$(s),$(3),$(2),\
    /nologo $(4) $(call server_flags,$(s)) /Fo$(1)/,wine-drives)))
$(foreach s,$(LITHTECH_ALL),$(eval $(call compile,$(call c_obj,$(s),$(LITHTECH)/$(1)),$(LITHTECH),$(s),$(3),$(2),\
    /nologo $(4) $(call lithtech_flags,$(s)) /Fo$(1)/,wine-drives)))
$(foreach s,$(D3D_CL_SRC),$(eval $(call compile,$(D3D)/$(1)/$(s:.cpp=.obj),$(D3D),$(s),$(3),$(2),\
    $(D3D_CL) $(4) /Fo$(1)/,wine-drives)))
$(foreach s,$(filter-out $(D3D_CL_SRC),$(D3D_SRC)),$(eval $(call compile,$(D3D)/$(1)/$(s:.cpp=.obj),$(D3D),$(s),\
    $(if $(filter icl,$(5)),$(ICL_ENV),$(3)),$(if $(filter icl,$(5)),$(ICL),$(2)),\
    $(D3D_BASE) $(4) $(if $(filter icl,$(5)),$(call d3d_opt,$(s)),/O2) /Fo$(1)/,$(if $(filter icl,$(5)),wine-plain,wine-drives))))
endef
$(eval $(call engine_variant,Release,$(ENG_PH_CC),$(ENG_PH_ENV),/D PRIMAL_HUNT,cl))
$(eval $(call engine_variant,AvP2Release,$(ENG_AVP_CC),$(ENG_AVP_ENV),,icl))

eng_objs = $(sort $(patsubst %.cpp,$(SERVER)/$(1)/%.obj,$(SERVER_SRC) $(SERVER_OS_SRC))) \
    $(sort $(foreach s,$(LITHTECH_ALL),$(call c_obj,$(s),$(LITHTECH)/$(1)))) \
    $(sort $(D3D_SRC:%.cpp=$(D3D)/$(1)/%.obj))
# ltmsg.dll: the engine's message strings (ltmsg.rc, read out of the retail
# DLLs by tools/rsrc2rc.py: Primal Hunt's and 1.0.9.6's are the same
# resources) around the C runtime's DLL startup (ltmsg.cpp, nolfsrc's).
# Primal Hunt's is SP5's tools' (its Rich header: C++ 8966), 1.0.9.6's RTM's
# (C++ 8168, its runtime 8168's).
LTMSG := $(E)/ltmsg
LTMSG_CL := /nologo /MT /W3 /O2 /D WIN32 /D NDEBUG /D _WINDOWS /c
# $(call ltmsg_variant,<object dir>,<VC6 tree>)
define ltmsg_variant
$(eval $(call compile,$(LTMSG)/$(1)/ltmsg.obj,$(LTMSG),ltmsg.cpp,$(call vc_env,$(2)),$(2)/BIN/cl.exe,$(LTMSG_CL) /Fo$(1)/,wine-drives))
$(eval $(call tool,$(LTMSG)/$(1)/ltmsg.res,$(LTMSG)/ltmsg.rc | wine-drives,\
    cd $(LTMSG) && $(call vc_env,$(2)) $(WINE) $(RC) $(AVP2SRC_RCFLAGS) /l 0x409 /d NDEBUG /fo$(1)/ltmsg.res ltmsg.rc))
$(eval $(call tool,$(LTMSG)/$(1)/ltmsg.dll,$(LTMSG)/$(1)/ltmsg.obj $(LTMSG)/$(1)/ltmsg.res | wine-drives,\
    cd $(LTMSG) && $(call vc_env,$(2)) $(WINE) $(2)/BIN/link.exe $(AVP2SRC_LDFLAGS) /nologo /subsystem:windows /dll /machine:I386 \
    /out:$(1)/ltmsg.dll /map:$(1)/ltmsg.map $(1)/ltmsg.obj $(1)/ltmsg.res kernel32.lib))
endef
$(eval $(call ltmsg_variant,Release,$(V8966)))
$(eval $(call ltmsg_variant,AvP2Release,$(V8168)))
.PHONY: ltmsg
ltmsg: $(LTMSG)/Release/ltmsg.dll $(LTMSG)/AvP2Release/ltmsg.dll

# The launchers, AVP2.exe and PrimalHunt.exe: nolfsrc's reconstruction of
# NOLF's launcher (src/launcher/, from nolfsrc's src/engine/launcher), to be
# brought to AvP2's, built twice from one source: AvP2's into Release/ with
# cl 12.00.9044 (its Rich header: 9044 x18), Primal Hunt's into PrimalHunt/
# with cl 12.00.8966 and PRIMAL_HUNT; both against SP5's MFC (nafxcw.lib,
# C++ 8569 in both headers). nolfsrc's flags. Objects only, so far
# (progress/targets.py, partial_objs): the links want the launchers'
# resources and art, and their protection wrapper's sections are not ours.
LAUNCHER := src/launcher
LAUNCHER_SRC := stdafx.cpp launcher.cpp gamedlg.cpp gamehelp.cpp dlgconfig.cpp dlgcustom.cpp dlghelp.cpp dlgsound.cpp \
    dlgdisplay.cpp preview.cpp cdcheck.cpp dib.cpp dibmgr.cpp dibpal.cpp RezFind.cpp Utils.cpp globals.cpp
LAUNCHER_CL := /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I ..\LT2\lithshared\incs /c
# $(call launcher_variant,<object dir>,<cl>,<defines>)
define launcher_variant
$(foreach s,$(LAUNCHER_SRC),$(eval $(call compile,$(LAUNCHER)/$(1)/$(s:.cpp=.obj),$(LAUNCHER),$(s),$(ENV_GAME),$(2),\
    $(LAUNCHER_CL) $(3) /Fo$(1)/,wine-drives)))
endef
$(eval $(call launcher_variant,Release,$(V9044)/BIN/cl.exe,))
$(eval $(call launcher_variant,PrimalHunt,$(V8966)/BIN/cl.exe,/D PRIMAL_HUNT))
.PHONY: launcher-objs
launcher-objs: $(LAUNCHER_SRC:%.cpp=$(LAUNCHER)/Release/%.obj) $(LAUNCHER_SRC:%.cpp=$(LAUNCHER)/PrimalHunt/%.obj)

.PHONY: engine-objs ph-engine-objs avp2-engine-objs
ph-engine-objs: $(call eng_objs,Release)
avp2-engine-objs: $(call eng_objs,AvP2Release)
engine-objs: ph-engine-objs avp2-engine-objs

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
