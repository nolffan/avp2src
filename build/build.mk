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
#   src/AVP2/AVP2PH.dsw Primal Hunt's game DLLs, their resources and the
#                       dedicated server: the same sources, through project
#                       files of their own (<name>_PH.dsp, first made by
#                       build/phproject.py) whose Final (the server's and
#                       the resources': Release) defines PRIMAL_HUNT (the
#                       resources' also to rc) and builds into PrimalHunt/
WORKSPACES := \
    src/AVP2/AVP2.dsw:ClientShellDLL,Object,ClientRes,ServerRes,ServerApp \
    src/AVP2/AVP2PH.dsw:ClientShellDLL_PH,Object_PH,ClientRes_PH,ServerRes_PH,ServerApp_PH

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
CL_ServerApp_PH      := $(V8966)/BIN/cl.exe
# AvP2's CShell.dll is not 8168's throughout: its Rich header has C++ 8168
# x142 and 8966 x114, and the 8966 objects are not only the libraries'.
# Monolith moved to SP5 during 1.0.9.6, and the files rebuilt after that
# are 8966's. Each source was compiled with both revisions and compared
# with retail function by function: these have functions identical only
# under 8966 and none (bar a few under 64 bytes, and static names) only
# under 8168. The SFX files among them show it in their factory
# registration initializers (_$E..., 411 bytes under 8966, 290 under
# 8168); CWeaponFX::CreateBulletTrail is identical with 8966 (one inline
# Init more expanded), in no form with 8168. SoundButeMgr is undecided (its
# destructor is 8966's code, GetSoundSetFromName 8168's) and stays 8168's.
CSHELL_8966 := AutoTargetMgr BaseFolder BaseScaleFX BodyPropFX CameraMgrFX \
    CharacterButeMgr CharacterFX CharacterMovement CheatMgr ClientUtilities \
    ClientWeaponUtils DeathFX DebrisFX DebrisMgr ExplosionFX FireFX FlashLight \
    FolderCampaignLevel FolderCustomControls FolderCustomLevel FolderDisplay \
    FolderHost FolderHostConfig FolderHostMaps FolderJoin FolderJoinInfo \
    FolderLoad FolderMain FolderMulti FolderPerformance FolderPlayer \
    FolderPlayerJoin FolderProfile FolderSave FolderSingle FXButeMgr \
    GameClientShell GameSettings GibFX GraphFX HeadBobMgr HudMgr InterfaceMgr \
    InterfaceResMgr InterfaceTimer LensFlareFX LightningFX LTDialogueWnd \
    MarkSFX MessageMgr MotionDetector MultiDebrisFX MultiplayerClientMgr \
    MuzzleFlashFX NodeController NodeLinesFX OverlayMGR ParticleSystemFX \
    ParticleTrailFX ParticleTrailSegmentFX PerformanceMgr PlayerMovement \
    PolyDebrisFX PolyGridFX PolyLineFX PredTargetFx ProfileMgr ProjectileFX \
    SearchLightFX ServerOptions SFXMgr ShellCasingFX StarLightViewerFX SteamFX \
    Subtitle TracerFX ViewModeMGR WeaponFX WeaponMgr WeaponModel WeaponSoundFX \
    WeatherFX
$(foreach s,$(CSHELL_8966),$(eval CL_ClientShellDLL__$(s) := $(V8966)/BIN/cl.exe))
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
# Links under Microsoft's msvcrt.dll
#
# link.exe sorts each DLL's imports with the C runtime's qsort, among keys
# that are all equal: Microsoft's qsort first swaps the middle element to
# the front, Wine's builtin msvcrt.dll's keeps the order. So a retail
# image's KERNEL32 thunks start with EnterCriticalSection, where a link
# under Wine's msvcrt starts with GetCommandLineA. The links that have to
# match there run a copy of the VC6 tree's linker (with mspdb60.dll and
# cvtres.exe, which it loads and starts) beside a native msvcrt.dll, the
# one icl-5.0.1 ships, loaded in the place of Wine's (msvcrt=n).
# $(call nlink_dir,<VC6 tree>): where that copy is
nlink_dir = out/native-link/$(notdir $(1))
# $(call nlink,<VC6 tree>): its link.exe, as a prerequisite and to run
nlink = $(call nlink_dir,$(1))/link.exe
# $(call nlink_env,<VC6 tree>): the environment to run it in, one at a
# time: under Microsoft's msvcrt two links at once name the same temporary
# file (LNK1104, C:\windows\temp\lnk2)
nlink_env = $(call vc_env,$(1)) WINEPATH='$(call ws,$(ROOT)/$(call nlink_dir,$(1)))' WINEDLLOVERRIDES='mscoree,mshtml=;msvcrt=n' \
    flock $(ROOT)/out/native-link/.lock
$(foreach v,$(V8168) $(V8966),$(eval $(call tool,$(call nlink,$(v)),$(v)/BIN/link.exe $(TC)/icl-5.0.1/BIN/Msvcrt.dll,\
    mkdir -p $(call nlink_dir,$(v)) && cp $(v)/BIN/link.exe $(v)/BIN/mspdb60.dll $(v)/BIN/cvtres.exe $(call nlink_dir,$(v))/ && \
    cp $(TC)/icl-5.0.1/BIN/Msvcrt.dll $(call nlink_dir,$(v))/msvcrt.dll)))
# AvP2's client and server resources link so (the one source of each
# compiles as the rest do)
ENV_ClientRes       := $(call nlink_env,$(V8966))
ENV_ClientRes__main := $(ENV_GAME)
LINK_ClientRes      := $(ROOT)/$(call nlink,$(V8966))
src/AVP2/ClientRes/Release/CRes.dll: $(call nlink,$(V8966))
ENV_ServerRes       := $(call nlink_env,$(V8966))
ENV_ServerRes__Main := $(ENV_GAME)
LINK_ServerRes      := $(ROOT)/$(call nlink,$(V8966))
src/AVP2/ServerRes/Release/SRes.dll: $(call nlink,$(V8966))
# and Primal Hunt's (PHCRes.dll, PHSRes.dll: the same projects, through
# ClientRes_PH.dsp and ServerRes_PH.dsp, their resource scripts compiled
# with PRIMAL_HUNT, into PrimalHunt/), whose one object is cl 8966's (the
# Rich headers' C++ 8966 x1: progress/COMPILERS.md). Here, not with the
# other Primal Hunt sections below: the workspaces' rules read these as
# they are included.
CL_ClientRes_PH        := $(V8966)/BIN/cl.exe
ENV_ClientRes_PH       := $(call nlink_env,$(V8966))
ENV_ClientRes_PH__main := $(ENV_GAME)
LINK_ClientRes_PH      := $(ROOT)/$(call nlink,$(V8966))
src/AVP2/ClientRes/PrimalHunt/CRes.dll: $(call nlink,$(V8966))
CL_ServerRes_PH        := $(V8966)/BIN/cl.exe
ENV_ServerRes_PH       := $(call nlink_env,$(V8966))
ENV_ServerRes_PH__Main := $(ENV_GAME)
LINK_ServerRes_PH      := $(ROOT)/$(call nlink,$(V8966))
src/AVP2/ServerRes/PrimalHunt/SRes.dll: $(call nlink,$(V8966))

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

SL_SRCS := abstractio.cpp dynarray.cpp fileio.cpp goodlinklist.cpp helpers.cpp l_allocator.cpp \
    linklist.cpp lithexception.cpp memory.cpp memoryio.cpp stringholder.cpp struct_bank.cpp \
    sys\win\compressedio.cpp
$(eval $(call lslib,stdlith,StdLith,/I $(LT2LS)\stdlith /I $(LT2LS)\lith /I . /I sys\win /I ..\zlib,$(SL_SRCS)))
$(eval $(call lslib,CryptMgr,CryptMgr,/I $(LT2LS2)\\cryptmgr /I $(LT2LS2)\\incs /I .,cryptmgr.cpp blowfish.cpp))
# (lith.lib: the release names it in lower case, and so does Windows when
# ours replaces a copy of it)
$(eval $(call lslib,lith,lith,/I .,basehash.cpp baselist.cpp virtlist.cpp))
# (RezMgr.lib: Monolith's RezMgr, the REZMGRLITE build its header defaults
# to; the shipped builds hold these three objects)
RM_SRCS := rezmgr.cpp rezhash.cpp rezfile.cpp
$(eval $(call lslib,rezmgr,RezMgr,/D REZMGRLITE /I $(LT2LS2)\\incs /I .,$(RM_SRCS)))
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
# ButeMgr: MFC's build of the same sources (no _NOMFC, no MFCStub: it names
# nafxcw.lib and uses CString), the release's prebuilt ButeMgr.lib. Of the
# images, only Primal Hunt's dedicated server links it (ServerApp_PH, by
# its path here); AvP2's links its own 8168 build (svlib, below). Its
# objects go to Release/mfc/, apart from ButeMgrNoMFC's.
BM_MFC_SRCS := arange.cpp avector.cpp butemgr.cpp stdafx.cpp
$(foreach s,$(BM_MFC_SRCS),$(eval $(call compile,src/lithshared/ButeMgr/Release/mfc/$(s:.cpp=.obj),src/lithshared/ButeMgr,$(s),$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    $(LIB_CFLAGS) /I $(LT2LS)\butemgr /I $(LT2LS)\stl /I $(LT2LS)\stdlith /I $(LT2LS)\cryptmgr /I $(LT2LS)\lith /I $(LT2LS)\incs /FoRelease/mfc/,wine-drives)))
$(eval $(call tool,$(LOUT)/ButeMgr.lib,$(BM_MFC_SRCS:%.cpp=src/lithshared/ButeMgr/Release/mfc/%.obj) | wine-drives,\
    cd src/lithshared/ButeMgr && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:$(call ws,$(ROOT)/$(LOUT))\ButeMgr.lib' Release/mfc/arange.obj Release/mfc/avector.obj Release/mfc/butemgr.obj Release/mfc/stdafx.obj))
LS_OURS += ButeMgr

# WONAPI.lib: Monolith's WON client, which the release ships as source
# (src/LT2/lithshared/wonapi/, "wonapi052201") as well as prebuilt, so built
# from that source itself: WONAPI.dsp's "Win32 Release" flags (less /YX
# /FD), its __WON_MASTER_CPP_BUILD__ objects -- one master .cpp per
# directory, the twelve members of the prebuilt build -- in two builds:
#  - the release's prebuilt one (progress/targets.py, LIBS, scores it
#    against it), with STLport 4.0 (lithshared/stl) ahead of VC6's headers,
#    as its _STL names say; into $(LOUT)/wonapi/, beside the prebuilt
#    WONAPI.lib the game DLLs still link;
#  - Primal Hunt's engine's (src/engine/lithtech/Release/wonapi/), with
#    VC6's own STL: lithtech.exe's WON code (0x004aab40..0x004ba2a0) has
#    VC6's 16-byte basic_string where STLport's is 12 bytes (MD5Digest's
#    RawBuffer `out` comes first: MD5Digest::transform at 0x004b7df0 reads
#    its fields 4 bytes further on than the STLport build does).
#    Primal Hunt's game DLLs' WON code is this build too (PHCShell.dll).
# $(call wonlib,<object dir>,<it, as /Fo from WON_DIR>,<includes>)
WON_DIR    := src/LT2/lithshared/wonapi
WON_CFLAGS := /nologo /MT /W3 /GR /GX /O2 /D NDEBUG /D WIN32 /D _MBCS /D _LIB /D __WON_MASTER_CPP_BUILD__ /c
WON_SRCS   := WONAuth\Auth.cpp WONCommon\Common.cpp WONCrypt\Crypt.cpp WONDB\DB.cpp WONDir\Dir.cpp \
    WONMisc\Misc.cpp WONServer\Server.cpp WONSocket\Socket.cpp WONAPI.cpp WONStatus.cpp \
    WONMisc\gqueryreporting.c WONMisc\nonport.c
define wonlib
$(foreach s,$(WON_SRCS),$(eval $(call compile,$(call c_obj,$(s),$(1)),$(WON_DIR),$(s),$(ENV_GAME),$(V8966)/BIN/cl.exe,/Fo$(2) $(WON_CFLAGS) $(3),wine-drives)))
$(1)/.dir:
	@mkdir -p $$(@D) && touch $$@
$(foreach s,$(WON_SRCS),$(call c_obj,$(s),$(1))): | $(1)/.dir
$(call tool,$(1)/WONAPI.lib,$(foreach s,$(WON_SRCS),$(call c_obj,$(s),$(1))) | wine-drives,cd $(1) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo /out:WONAPI.lib $(foreach s,$(WON_SRCS),$(notdir $(call c_obj,$(s),$(1)))))
endef
$(eval $(call wonlib,$(LOUT)/wonapi,..\libs\Release\wonapi\,/I ..\stl /I .))
$(eval $(call wonlib,src/engine/lithtech/Release/wonapi,..\..\..\engine\lithtech\Release\wonapi\,/I .))
.PHONY: wonapi
wonapi: $(LOUT)/wonapi/WONAPI.lib src/engine/lithtech/Release/wonapi/WONAPI.lib

.PHONY: libs
libs: $(LS_OURS:%=$(LOUT)/%.lib) $(LOUT)/wonapi/WONAPI.lib

# The debug builds of StdLith and RezMgr. Primal Hunt's server.dll links
# them, not the release builds (its block from 0x1004a6e0, between the
# world tree and the net objects: frame pointers, asserts naming
# rezmgr.cpp and stringholder.cpp), and the release ships them prebuilt
# (libs/debug/). Their objects' own command lines (cl 8966): /MTd /W3 /GX
# /ZI /Od /D _DEBUG /D WIN32 /D _WINDOWS /D NO_PRAGMA_LIBS, RezMgr's with
# /D REZMGRLITE too; /ZI (edit and continue), not the /Zi /GZ the project
# files' Debug configurations say. Each object gets a program database of
# its own (/Fd), so that parallel compiles never share one. Into
# $(LDOUT), where the project files' Debug configurations look and the
# Final builds never do; progress/targets.py scores them against the
# shipped debug builds, and as server.dll's.
LIB_DCFLAGS := /nologo /MTd /W3 /GX /ZI /Od /D _DEBUG /D WIN32 /D _WINDOWS /D NO_PRAGMA_LIBS /c
LDOUT := src/LT2/lithshared/libs/Debug
LS_DEBUG :=
# $(call lsdlib,<dir>,<library>,<incs>,<sources>)
define lsdlib
$(foreach s,$(4),$(eval $(call compile,$(call c_obj,$(s),src/lithshared/$(1)/Debug),src/lithshared/$(1),$(s),$(ENV_GAME),$(V8966)/BIN/cl.exe,$(LIB_DCFLAGS) $(3) /FoDebug/ /FdDebug/$(basename $(notdir $(subst \,/,$(s)))).pdb,wine-drives)))
LSDOBJ_$(1) := $(foreach s,$(4),$(call c_obj,$(s),src/lithshared/$(1)/Debug))
$(call tool,$(LDOUT)/$(2).lib,$$(LSDOBJ_$(1)) | wine-drives,cd src/lithshared/$(1) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:$(call ws,$(ROOT)/$(LDOUT))\$(2).lib' $$(call q,$$(subst /,\,$$(patsubst src/lithshared/$(1)/%,%,$$(LSDOBJ_$(1))))))
LS_DEBUG += $(2)
endef
$(eval $(call lsdlib,stdlith,StdLith,/I $(LT2LS)\stdlith /I $(LT2LS)\lith /I . /I sys\win /I ..\zlib,$(SL_SRCS)))
$(eval $(call lsdlib,rezmgr,RezMgr,/D REZMGRLITE /I $(LT2LS2)\\incs /I .,$(RM_SRCS)))
.PHONY: debuglibs
debuglibs: $(LS_DEBUG:%=$(LDOUT)/%.lib)

# AvP2 1.0.9.6's engine's builds of lith, RezMgr and StdLith: the same
# sources and flags as the release builds above, compiled with the engine's
# own cl 12.00.9044 (the Processor Pack's) into <dir>/AvP2Release/ and
# $(LAOUT), which avp2-engine-link links. Its lithtech.exe and server.dll
# hold neither the release's prebuilt builds (8966) nor the debug ones:
# CRezItm::InitRezItm (lithtech.exe 0x004afd00, server.dll 0x10045a30)
# copies its name with VC6's /O2 strcpy under 9044 -- byte-identical --
# where 8966 /O2 inlines it as rep movs and /O1 under either is shorter.
# Their Rich headers agree: of 165 and 104 objects of C++ 12.00 beside
# the runtime's, 157 and 96 are 9044's, which leaves 8 of 8966 in each for
# WON's prebuilt members (below, avp2-engine-link).
V9044L := $(TC)/msvc-6.00-9044
LAOUT  := src/LT2/lithshared/libs/AvP2Release
LS_AVP :=
# $(call lsalib,<dir>,<library>,<incs>,<sources>)
define lsalib
$(foreach s,$(4),$(eval $(call compile,$(call c_obj,$(s),src/lithshared/$(1)/AvP2Release),src/lithshared/$(1),$(s),$(call vc_env,$(V9044L)),$(V9044L)/BIN/cl.exe,$(LIB_CFLAGS) $(3) /FoAvP2Release/,wine-drives)))
LSAOBJ_$(1) := $(foreach s,$(4),$(call c_obj,$(s),src/lithshared/$(1)/AvP2Release))
$(call tool,$(LAOUT)/$(2).lib,$$(LSAOBJ_$(1)) | wine-drives,cd src/lithshared/$(1) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:$(call ws,$(ROOT)/$(LAOUT))\$(2).lib' $$(call q,$$(subst /,\,$$(patsubst src/lithshared/$(1)/%,%,$$(LSAOBJ_$(1))))))
LS_AVP += $(2)
endef
$(eval $(call lsalib,stdlith,StdLith,/I $(LT2LS)\stdlith /I $(LT2LS)\lith /I . /I sys\win /I ..\zlib,$(SL_SRCS)))
$(eval $(call lsalib,lith,lith,/I .,basehash.cpp baselist.cpp virtlist.cpp))
$(eval $(call lsalib,rezmgr,RezMgr,/D REZMGRLITE /I $(LT2LS2)\\incs /I .,$(RM_SRCS)))
.PHONY: avp2libs
avp2libs: $(LS_AVP:%=$(LAOUT)/%.lib)

# ---------------------------------------------------------------------------
# The game DLLs' libraries, ours: LTGUIMgr, LithFontMgr and WONAPI
#
# ltguimgr_LT2.lib and LithFontMgr_LT2.lib: AvP2's builds of NOLF's GUI and
# font libraries, which the release ships prebuilt only. Ours are nolfsrc's
# sources (src/lithshared/ltguimgr, lithfontmgr) brought to the release's
# prebuilt builds (progress/targets.py, LIBS, scores them against those):
# a menu font of three faces, normal, selected and disabled (CLTGUIFont's
# second Init, DrawMenu, DrawMenuFormat), which every control draws its
# text with when its font is one; word-wrapped columns; an edit control's
# input modes; the previous string a CString (LITHFONTSAVEDATA), a system
# font's default colour. nolfsrc's flags, as the libraries above.
LTGUI_SRCS := ltguibitmapctrl.cpp ltguicolumntextctrl.cpp ltguicommandhandler.cpp ltguictrl.cpp ltguicyclectrl.cpp \
    ltguieditctrl.cpp ltguifadecolorctrl.cpp ltguifadeitemctrl.cpp ltguifont.cpp ltguilistctrl.cpp ltguimessagebox.cpp \
    ltguimgr.cpp ltguionoffctrl.cpp ltguiscrollbarctrl.cpp ltguisliderctrl.cpp ltguitextitemctrl.cpp
$(eval $(call lslib,ltguimgr,ltguimgr_LT2,/D _NOMFC /D _LITHTECH2 /I $(LT2LS)\ltguimgr /I $(LT2LS)\lithfontmgr /I $(LT2LS)\incs \
    /I $(LT2LS)\mfcstub /I $(LT2LS)\lith /I ..\..\LT2\sdk\inc /I .,$(LTGUI_SRCS)))
$(eval $(call lslib,lithfontmgr,LithFontMgr_LT2,/D _LITHTECH2 /I $(LT2LS)\lithfontmgr /I $(LT2LS)\incs /I $(LT2LS)\mfcstub \
    /I $(LT2LS)\lith /I ..\..\LT2\sdk\inc /I .,lithbitmapfont.cpp lithcursor.cpp lithfont.cpp lithfontmgr.cpp lithsystemfont.cpp))
# LithTrackMgrClient.lib: LithTrackMgr, the in-game camera-track editor,
# client build (_CLIENTBUILD), with cl 8966. The release has its headers
# only (LT2/lithshared/lithtrackmgr); the sources are written from the
# release's prebuilt build. That build also holds a copy of LithFontMgr's
# five objects.
LTM_SRCS := LithTrack.cpp LithTrackList.cpp LithTrackKey.cpp LithTrackDrawObj.cpp LithTrackBarBase.cpp \
    LithTrackBarCamera.cpp LithTrackBarEdit.cpp LithTrackBarHelp.cpp LithTrackBarKey.cpp LithTrackBarTrack.cpp \
    LithTrackMgr.cpp
$(eval $(call lsobjs,lithtrackmgr,/D _CLIENTBUILD /D _LITHTECH2 /I $(LT2LS)\lithtrackmgr /I $(LT2LS)\lithfontmgr \
    /I $(LT2LS)\incs /I $(LT2LS)\mfcstub /I $(LT2LS)\lith /I ..\..\LT2\sdk\inc /I .,$(LTM_SRCS)))
.PHONY: lithtrackobjs
lithtrackobjs: $(LSOBJ_lithtrackmgr)
#
# WONAPI.lib, as the game DLLs link it ($(LT2_DIR)\lithshared\libs\Release\
# WONAPI.lib in the project files): the release's STLport build (above,
# "WONAPI.lib"), so the links take a copy of ours of it. AvP2 1.0.9.6's
# game DLLs hold that build. Primal Hunt's hold Third Law's build of the
# same source with VC6's own STL, as Primal Hunt's engine's
# (tools/libscan.py: PHObject.lto holds 657 of the functions of our VC6-STL
# build and 317 of the STLport one's, PHCShell.dll 722 against 205, the
# latter above its runtime), but the _PH projects cannot link that build
# yet: the release's prebuilt GameSpyMgr.lib and GameSpyClientMgr.lib call
# WONAPI with STLport strings and lists (unresolved against VC6's), so
# Third Law rebuilt those too. Until they are ours, both games link this.
$(eval $(call tool,$(LOUT)/WONAPI.lib,$(LOUT)/wonapi/WONAPI.lib,cp $(LOUT)/wonapi/WONAPI.lib $(LOUT)/WONAPI.lib))
LS_OURS += WONAPI
.PHONY: gamelibs
gamelibs: $(LOUT)/ltguimgr_LT2.lib $(LOUT)/LithFontMgr_LT2.lib $(LOUT)/WONAPI.lib

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
all game: $(WANTED) ltmsg wonapi debuglibs

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
# Both keep nolfsrc's file lists and per-file flags. Primal Hunt's
# lithtech.exe and server.dll (below, engine-link) and d3d.ren (d3d-link)
# are linked for the scoreboard, not to run; the rest is objects only,
# which `make progress` scores as they are (progress/targets.py,
# partial_objs).
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

# server.dll: nolfsrc's /O1 (Primal Hunt adds /GX, below); localdriver.cpp /Os
SERVER := $(E)/server
SERVER_CL := /MT /W3 /O1 /D WIN32 /D NDEBUG /D _WINDOWS /D LT_SERVER_TIMERS /D LT_SERVER_BUILD /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I . /c
SERVER_SRC := server.cpp servermgr.cpp servermsg.cpp netmgr.cpp servermath.cpp commonphysics.cpp commonmodel.cpp \
    sharedcommon.cpp servermodel.cpp serverlightanim.cpp servercommon.cpp servermessage.cpp packet.cpp \
    lthread.cpp smgr.cpp counter.cpp dalloc.cpp modelanim.cpp streamsim.cpp fsstream.cpp basestream.cpp \
    objbank.cpp objtypes.cpp collection.cpp worldbsp.cpp worldmodels.cpp saveio.cpp worldsim.cpp \
    serverengine.cpp ltobject.cpp lmessage.cpp spritecontrol.cpp tcpdriver.cpp stringbank.cpp dsi.cpp \
    genericprop.cpp dhash.cpp filemgr.cpp console.cpp structbank.cpp csbase.cpp sdkforce.cpp netobj.cpp \
    savrest.cpp shade.cpp smgrnew.cpp modelnew.cpp objmover.cpp smgrmisc.cpp svnew.cpp svlast.cpp \
    svtiny.cpp worldtree.cpp worldisect.cpp sweptsphere.cpp interlink.cpp
SERVER_OS_SRC := localdriver.cpp
# the segment-query module, unoptimised in Primal Hunt and AvP2 1.0.9.6
# (server.dll 0x10012de7..0x10013ebf)
SERVER_OD_SRC := worldisect.cpp
server_flags = $(if $(filter $(1),$(SERVER_OS_SRC)),$(subst /O1,/Os /Ob1 /Gf,$(SERVER_CL)),\
    $(if $(filter $(1),$(SERVER_OD_SRC)),$(subst /O1,/Od /Os /Ob1,$(SERVER_CL)),$(SERVER_CL)))

# lithtech.exe: nolfsrc's /O2 (Primal Hunt adds /GX, below), with the server's timers
# (AvP2's client times what server.dll does); its copy of server's localdriver.cpp /Od with
# intrinsics, inlining and string pooling; controlfile.cpp with /GX
LITHTECH := $(E)/lithtech
LITHTECH_CL := /MT /W3 /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D LT_SERVER_TIMERS /D NO_PRAGMA_LIBS /I ..\..\LT2\sdk\inc /I ..\server /I . /c
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
    ..\server\worldbsp.cpp ..\server\worldmodels.cpp ..\server\worldsim.cpp worldtree.cpp clientmusic.cpp \
    ..\server\sweptsphere.cpp ..\server\interlink.cpp
LITHTECH_OD_SRC := ..\server\localdriver.cpp
# the segment-query module, unoptimised as in server.dll (lithtech.exe 0x0043b750..)
LITHTECH_ODS_SRC := ..\server\worldisect.cpp
LITHTECH_GX_SRC := controlfile.cpp
lithtech_flags = $(subst /O2,$(strip \
    $(if $(filter $(1),$(LITHTECH_OD_SRC)),/Od /Oi /Ob1 /Gf,\
    $(if $(filter $(1),$(LITHTECH_ODS_SRC)),/Od /Os /Ob1,\
    $(if $(filter $(1),$(LITHTECH_GX_SRC)),/GX /O2,/O2)))),$(LITHTECH_CL))
LITHTECH_ALL := $(LITHTECH_SRC) $(LITHTECH_OD_SRC) $(LITHTECH_ODS_SRC) $(LITHTECH_GX_SRC)
# ... and never with /GX, even in Primal Hunt (none so far: stdlithalloc.cpp
# was listed on a mispairing -- 0x004b4e30 is MFCStub's CString constructor,
# and ~LAllocSimpleBlock at 0x004bf320 has an exception frame)
LITHTECH_NOGX_SRC :=

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
    /nologo $(if $(filter $(s),$(LITHTECH_NOGX_SRC)),$(filter-out /GX,$(4)),$(4)) $(call lithtech_flags,$(s)) /Fo$(1)/,wine-drives)))
$(foreach s,$(D3D_CL_SRC),$(eval $(call compile,$(D3D)/$(1)/$(s:.cpp=.obj),$(D3D),$(s),$(3),$(2),\
    $(D3D_CL) $(filter-out /GX,$(4)) /Fo$(1)/,wine-drives)))
$(foreach s,$(filter-out $(D3D_CL_SRC),$(D3D_SRC)),$(eval $(call compile,$(D3D)/$(1)/$(s:.cpp=.obj),$(D3D),$(s),\
    $(if $(filter icl,$(5)),$(ICL_ENV),$(3)),$(if $(filter icl,$(5)),$(ICL),$(2)),\
    $(D3D_BASE) $(filter-out /GX,$(4)) $(if $(filter icl,$(5)),$(call d3d_opt,$(s)),/O2) /Fo$(1)/,$(if $(filter icl,$(5)),wine-plain,wine-drives))))
endef
# Primal Hunt's engine and server (not its renderer, whose flags are its own)
# are built with /GX: their functions with
# destructible locals carry exception frames that AvP2 1.0.9.6's lack (e.g.
# lithtech.exe 0x00452310, ~CModelSub1D4; server.dll 0x1000c130, the
# CWorldSurface constructor, against AvP2's frameless 0x1000be60).
$(eval $(call engine_variant,Release,$(ENG_PH_CC),$(ENG_PH_ENV),/D PRIMAL_HUNT /GX,cl))
$(eval $(call engine_variant,AvP2Release,$(ENG_AVP_CC),$(ENG_AVP_ENV),,icl))

eng_objs = $(sort $(patsubst %.cpp,$(SERVER)/$(1)/%.obj,$(SERVER_SRC) $(SERVER_OS_SRC))) \
    $(sort $(foreach s,$(LITHTECH_ALL),$(call c_obj,$(s),$(LITHTECH)/$(1)))) \
    $(sort $(D3D_SRC:%.cpp=$(D3D)/$(1)/%.obj))
# ltmsg.dll: the engine's message strings (ltmsg.rc, read out of the retail
# DLLs by tools/rsrc2rc.py: Primal Hunt's and 1.0.9.6's are the same
# resources) around the C runtime's DLL startup (ltmsg.cpp, nolfsrc's).
# Primal Hunt's is SP5's tools' (its Rich header: C++ 8966), 1.0.9.6's RTM's
# (C++ 8168, its runtime 8168's). Linked under Microsoft's msvcrt.dll
# (nlink, above), for the order of the imports.
LTMSG := $(E)/ltmsg
LTMSG_CL := /nologo /MT /W3 /O2 /D WIN32 /D NDEBUG /D _WINDOWS /c
# $(call ltmsg_variant,<object dir>,<VC6 tree>)
define ltmsg_variant
$(eval $(call compile,$(LTMSG)/$(1)/ltmsg.obj,$(LTMSG),ltmsg.cpp,$(call vc_env,$(2)),$(2)/BIN/cl.exe,$(LTMSG_CL) /Fo$(1)/,wine-drives))
$(eval $(call tool,$(LTMSG)/$(1)/ltmsg.res,$(LTMSG)/ltmsg.rc | wine-drives,\
    cd $(LTMSG) && $(call vc_env,$(2)) $(WINE) $(RC) $(AVP2SRC_RCFLAGS) /l 0x409 /d NDEBUG /fo$(1)/ltmsg.res ltmsg.rc))
$(eval $(call tool,$(LTMSG)/$(1)/ltmsg.dll,$(LTMSG)/$(1)/ltmsg.obj $(LTMSG)/$(1)/ltmsg.res $(call nlink,$(2)) | wine-drives,\
    cd $(LTMSG) && $(call nlink_env,$(2)) $(WINE) $(ROOT)/$(call nlink,$(2)) $(AVP2SRC_LDFLAGS) /nologo /subsystem:windows /dll /machine:I386 \
    /out:$(1)/ltmsg.dll /map:$(1)/ltmsg.map $(1)/ltmsg.obj $(1)/ltmsg.res kernel32.lib))
endef
$(eval $(call ltmsg_variant,Release,$(V8966)))
$(eval $(call ltmsg_variant,AvP2Release,$(V8168)))
.PHONY: ltmsg
ltmsg: $(LTMSG)/Release/ltmsg.dll $(LTMSG)/AvP2Release/ltmsg.dll

# Launch.dll
#
# NOLF's launcher DLL, which AvP2 and Primal Hunt ship unchanged and the
# engine loads for GetLithTechCommandLine (src/launch/). A 1999 Debug
# build of msvc-6.00-8168's (its Rich header: C++ 8168 x10, the debug
# runtime's C 8168 x85, link 6.00.8168): the frames are /Od with /ZI's 64
# bytes of edit-and-continue room and no /GZ fill, calls go through an
# incremental link's jump table, functions are padded with int3.
# cdcheck.cpp and RezFind.cpp are /GX (FindCdRom's CRegMgr has an
# exception frame), launch.cpp is not (ReadSettings' CRegMgr has none).
# /YX, as the project's Debug configuration had it: with it the compiler
# emits the inline CRegMgr constructor and destructor after the last
# function of cdcheck.cpp (0x100017c0, 0x100017f0), without it after the
# first that uses them. Each object gets a program database and a
# precompiled header of its own (/Fd, /Fp). CRegMgr's own members are
# the release build of lithshared's RegMgr (0x100026f0..0x10002bc6, /O2,
# placed after the objects and their import thunks, before the runtime: a
# library's member), compiled by VC5's compiler (msvc-5.00-7303): its
# object is the Rich header's one without a @comp.id, and 8168's orders
# Init's `return TRUE` after the pops where the image has it before them.
# Linked as Developer Studio's Debug DLL links, through a response file
# (which the scoreboard reads).
#
# Not byte-identical as a whole, and cannot be: the retail image links a
# libcmtd.lib none of the toolchains holds (its heap code has VC6's
# __MSVCRT_HEAP_SELECT, which neither 8168's nor 8447's debug runtime
# has; C 8168 x85 against our 83), names its program database by the
# original machine's path, and orders the jump table by the original
# symbol names. Every byte of the DLL's own code from 0x100010a0 to
# 0x10002bd0 is the retail one but for relocated addresses and the calls'
# displacements into that table; the data lie as retail's, 0x2688 bytes
# lower (the runtime's are smaller).
LAUNCHDLL := src/launch
LAUNCHDLL_SRC := cdcheck.cpp launch.cpp RezFind.cpp
LAUNCHDLL_NOGX := launch.cpp
LAUNCHDLL_CL := /nologo /MTd /W3 /ZI /Od /D WIN32 /D _DEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I ..\LT2\lithshared\incs /YX /FD /c
LAUNCHDLL_ENV := $(call vc_env,$(V8168))
$(foreach s,$(LAUNCHDLL_SRC),$(eval $(call compile,$(LAUNCHDLL)/Debug/$(s:.cpp=.obj),$(LAUNCHDLL),$(s),$(LAUNCHDLL_ENV),$(V8168)/BIN/cl.exe,\
    $(LAUNCHDLL_CL) $(if $(filter $(s),$(LAUNCHDLL_NOGX)),,/GX) /FoDebug/ /FdDebug/$(s:.cpp=)-vc60.pdb /FpDebug/$(s:.cpp=.pch),wine-drives)))
LAUNCHDLL_V5 := $(TC)/msvc-5.00-7303
$(eval $(call compile,$(LAUNCHDLL)/Debug/regmgr/regmgr.obj,src/lithshared/RegMgr,regmgr.cpp,$(call vc_env,$(LAUNCHDLL_V5)),$(LAUNCHDLL_V5)/BIN/cl.exe,\
    /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS /I $(LT2LS2)\\incs /I . /Fo..\..\launch\Debug\regmgr\ /c,wine-drives))
$(eval $(call tool,$(LAUNCHDLL)/Debug/RegMgr.lib,$(LAUNCHDLL)/Debug/regmgr/regmgr.obj | wine-drives,\
    cd $(LAUNCHDLL) && $(LAUNCHDLL_ENV) $(WINE) $(V8168)/BIN/lib.exe $(call q,/nologo /out:Debug\RegMgr.lib Debug\regmgr\regmgr.obj)))
# The link is incremental, as Developer Studio's Debug links are: the jump
# table at 0x10001005, the int3 padding after every function. link.exe maps
# its incremental database (Debug/Launch.ilk) at 0x3fff0000, where Wine has
# already put the first thread's TEB block (0x3fe00000..0x3fffffff), and
# then links nonincrementally ("LNK4073: cannot create map for .ILK file").
# So the link runs a copy of link.exe 6.00.8168 whose one 0x3fff0000 (the
# base it answers for offset 0, file offset 0x34f79) reads 0x5fff0000: where
# the database sits in the linker's memory changes nothing it writes. An
# incremental link writes no map (LNK4073 again, with /map), so there is
# none: the scoreboard places our functions by their objects.
LAUNCHDLL_LINK := out/launch-link/link.exe
$(eval $(call tool,$(LAUNCHDLL_LINK),$(V8168)/BIN/link.exe,\
    od -An -tx1 -j216953 -N4 $(V8168)/BIN/link.exe | grep -q '00 00 ff 3f' && \
    cp $(V8168)/BIN/link.exe $(LAUNCHDLL_LINK) && printf '\137' | dd of=$(LAUNCHDLL_LINK) bs=1 seek=216956 conv=notrunc 2>/dev/null))
LAUNCHDLL_OBJS := $(LAUNCHDLL_SRC:%.cpp=Debug/%.obj)
LAUNCHDLL_LIBS := kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib \
    oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Debug/RegMgr.lib
$(eval $(call tool,$(LAUNCHDLL)/Debug/Launch.dll,$(LAUNCHDLL_OBJS:%=$(LAUNCHDLL)/%) $(LAUNCHDLL)/Debug/RegMgr.lib $(LAUNCHDLL_LINK) | wine-drives,\
    cd $(LAUNCHDLL) && { printf '%s\n' $(call q,$(AVP2SRC_LDFLAGS) /nologo /subsystem:windows /dll /incremental:yes \
    /pdb:Debug\Launch.pdb /debug /machine:I386 /out:Debug\Launch.dll /implib:Debug\Launch.lib /pdbtype:sept $(subst /,\,$(LAUNCHDLL_OBJS) $(LAUNCHDLL_LIBS))) > $(ROOT)/$(LAUNCHDLL)/Debug/Launch.dll.rsp; } && \
    rm -f Debug/Launch.ilk && $(LAUNCHDLL_ENV) $(WINE) $(ROOT)/$(LAUNCHDLL_LINK) '@Debug\Launch.dll.rsp'))
.PHONY: launch-dll
launch-dll: $(LAUNCHDLL)/Debug/Launch.dll

# AVP2Up.dll and AVP2XUp.dll: the Sierra updater's resources for AvP2 and
# Primal Hunt (src/AVP2/AVP2Up/), one project built twice: AvP2's into
# Release/ from AVP2Up.rc, Primal Hunt's into PrimalHunt/ from AVP2XUp.rc.
# A Visual C++ "simple DLL" project: StdAfx.cpp makes the precompiled
# header (/Yc), AVP2Up.cpp's empty DllMain uses it (/Yu) -- the Rich
# headers' two objects of C++ 8966, in both images (AvP2's of 2001-08,
# Primal Hunt's of 2002-05), with SP5's libcmt.lib (C 8047) and cvtres
# 5.00.1735. Linked as Developer Studio links, through a response file
# and with a map.
#
# Linked under Microsoft's msvcrt.dll (nlink, above), for the order of
# the imports.
AVP2UP := src/AVP2/AVP2Up
AVP2UP_CL := /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D _MBCS /D _USRDLL /D AVP2UP_EXPORTS /c
AVP2UP_LIBS := kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib \
    oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
# $(call avp2up_variant,<object dir>,<dll and script stem>)
define avp2up_variant
$(eval $(call compile,$(AVP2UP)/$(1)/StdAfx.obj,$(AVP2UP),StdAfx.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    $(AVP2UP_CL) /Ycstdafx.h /Fp$(1)/AVP2Up.pch /Fo$(1)/,wine-drives))
$(eval $(call compile,$(AVP2UP)/$(1)/AVP2Up.obj,$(AVP2UP),AVP2Up.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    $(AVP2UP_CL) /Yustdafx.h /Fp$(1)/AVP2Up.pch /Fo$(1)/,wine-drives $(AVP2UP)/$(1)/StdAfx.obj))
$(eval $(call rcfile,$(AVP2UP)/$(1)/$(2).res,$(AVP2UP),$(2).rc,$(AVP2UP)/$(2).rc,/l 0x409 /d NDEBUG /fo$(1)/$(2).res))
$(eval $(call binary,$(AVP2UP)/$(1)/$(2).dll,$(AVP2UP),/nologo /subsystem:windows /dll /machine:I386 /out:$(1)\$(2).dll /implib:$(1)\$(2).lib,\
    $(AVP2UP)/$(1)/AVP2Up.obj $(AVP2UP)/$(1)/StdAfx.obj $(AVP2UP)/$(1)/$(2).res $(call nlink,$(V8966)),\
    $(AVP2UP_LIBS) $(1)/AVP2Up.obj $(1)/StdAfx.obj $(1)/$(2).res,,$(call nlink_env,$(V8966)),$(ROOT)/$(call nlink,$(V8966))))
endef
$(eval $(call avp2up_variant,Release,AVP2Up))
$(eval $(call avp2up_variant,PrimalHunt,AVP2XUp))
.PHONY: avp2up
avp2up: $(AVP2UP)/Release/AVP2Up.dll $(AVP2UP)/PrimalHunt/AVP2XUp.dll

# The launchers, AVP2.exe and PrimalHunt.exe: an MFC application (src/launcher/,
# the ancestor of the launcher in Monolith's NOLF2 source release, which it
# is reconstructed from, with NOLF's display-mode code, nolfsrc's), built
# twice from one source: AvP2's into Release/ with cl 12.00.9044 (its Rich
# header: 9044 x18), Primal Hunt's into PrimalHunt/ with cl 12.00.8966 and
# PRIMAL_HUNT; both against SP5's MFC, linked statically (nafxcw.lib, C++
# 8569 in both headers). The 18 are the 18 sources below; the header's one
# object of 8966 is RegMgr32's (below). The protection wrapper's sections
# (.cms_t, .cms_d) are not ours, nor the five pages of .text it encrypts.
LAUNCHER := src/launcher
LAUNCHER_SRC := AnimDlg.cpp BitmapCheckButton.cpp ButtonEx.cpp DetailSettingsDlg.cpp DisplayDlg.cpp DisplayMgr.cpp \
    DlgEx.cpp EditEx.cpp Launcher.cpp LauncherDlg.cpp MessageBoxDlg.cpp MoveDlg.cpp MultiplayerDlg.cpp OptionsDlg.cpp \
    RezFind.cpp StaticEx.cpp StdAfx.cpp Utils.cpp
LAUNCHER_CL := /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D _MBCS /I ..\lithshared\RegMgr32 /c
# With the project's precompiled header, as the .dsp builds it (StdAfx.cpp
# makes it, /Yc; the rest use it, /Yu): it decides how the destructors that
# MFC's headers define inline are expanded into ours -- their unwind states
# nest only with it (CBitmap's in every dialog's, AVP2.exe 0x401020). All
# but LauncherDlg.cpp: its own (COptionsDlg's in OnOptions, 0x4066a0) are
# expanded flat, as a compile without the header makes them.
LAUNCHER_NOPCH := LauncherDlg.cpp
# $(call launcher_variant,<object dir>,<cl>,<defines>)
define launcher_variant
$(eval $(call compile,$(LAUNCHER)/$(1)/StdAfx.obj,$(LAUNCHER),StdAfx.cpp,$(ENV_GAME),$(2),\
    $(LAUNCHER_CL) $(3) /Ycstdafx.h /Fp$(1)/Launcher.pch /Fo$(1)/,wine-drives))
$(foreach s,$(filter-out StdAfx.cpp,$(LAUNCHER_SRC)),$(eval $(call compile,$(LAUNCHER)/$(1)/$(s:.cpp=.obj),$(LAUNCHER),$(s),$(ENV_GAME),$(2),\
    $(LAUNCHER_CL) $(3) $(if $(filter $(s),$(LAUNCHER_NOPCH)),,/Yustdafx.h /Fp$(1)/Launcher.pch) /Fo$(1)/,wine-drives $(LAUNCHER)/$(1)/StdAfx.obj)))
endef
$(eval $(call launcher_variant,Release,$(V9044)/BIN/cl.exe,))
$(eval $(call launcher_variant,PrimalHunt,$(V8966)/BIN/cl.exe,/D PRIMAL_HUNT))
.PHONY: launcher-objs
launcher-objs: $(LAUNCHER_SRC:%.cpp=$(LAUNCHER)/Release/%.obj) $(LAUNCHER_SRC:%.cpp=$(LAUNCHER)/PrimalHunt/%.obj)

# The launchers' links: AVP2.exe and PrimalHunt.exe, each through a response
# file (<image>.rsp) with a map beside it, so that the scoreboard reads what
# our link put where (progress/targets.py's `built`) and the small functions
# verify by relocation. link 6.00.8447 (SP5's, $(LINK)) with the retail
# link's defaults, /OPT:REF among them: nothing makes the multiplayer
# dialog or its base class, so their constructors and vtables go, and only
# the handlers their message maps reach stay, as in the retail images.
#   - RegMgr32.lib: the registry class, from the NOLF2 release's libs
#     (src/lithshared/RegMgr32/), compiled with 8966 (the Rich header's one
#     object of it) into a library of its own; its code lies between the C
#     runtime and MFC in the retail images, outside their own code
#   - launcher.res: launcher.rc, with the art read out of the retail image
#     the variant is built against (tools/rsrc2rc.py, into <dir>/res/: not in
#     the tree), compiled in the object directory, where res\ is
#   - the import libraries the retail import tables name, and MFC's and the
#     C runtime's (SP5's, from the environment's LIB)
REGMGR32 := src/lithshared/RegMgr32
LAUNCHER_REF := $(firstword $(wildcard $(ROOT)/out/reference $(ROOT)/../reference))
LAUNCHER_LIBS := nafxcw.lib libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib advapi32.lib shell32.lib \
    comctl32.lib comdlg32.lib winmm.lib
# $(call launcher_link,<object dir>,<image>,<retail image>,<rc defines>)
define launcher_link
$(eval $(call compile,$(LAUNCHER)/$(1)/regmgr32/regmgr32.obj,$(REGMGR32),regmgr32.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D _MBCS /c /Fo..\..\launcher\$(1)\regmgr32\regmgr32.obj,wine-drives))
$(eval $(call library,$(LAUNCHER)/$(1)/regmgr32/RegMgr32.lib,$(LAUNCHER)/$(1)/regmgr32,/nologo /out:RegMgr32.lib,\
    $(LAUNCHER)/$(1)/regmgr32/regmgr32.obj,regmgr32.obj,$(ENV_GAME),$(LIBEXE)))
$(eval $(call tool,$(LAUNCHER)/$(1)/res/retail.rc,$(LAUNCHER_REF)/$(3),\
    python3 tools/rsrc2rc.py $(LAUNCHER_REF)/$(3) $(LAUNCHER)/$(1)/res/retail.rc --res-dir=. --name=launcher >/dev/null))
$(eval $(call tool,$(LAUNCHER)/$(1)/launcher.res,$(LAUNCHER)/launcher.rc $(LAUNCHER)/resource.h $(LAUNCHER)/$(1)/res/retail.rc | wine-drives,\
    cd $(LAUNCHER)/$(1) && $(ENV_GAME) $(WINE) $(RC) $(AVP2SRC_RCFLAGS) /l 0x409 /d NDEBUG $(4) /folauncher.res '..\launcher.rc'))
$(eval $(call tool,$(LAUNCHER)/$(1)/$(2),$(LAUNCHER_SRC:%.cpp=$(LAUNCHER)/$(1)/%.obj) $(LAUNCHER)/$(1)/launcher.res \
    $(LAUNCHER)/$(1)/regmgr32/RegMgr32.lib | wine-drives,\
    cd $(LAUNCHER) && { printf '%s\n' $(call q,/nologo $(AVP2SRC_LDFLAGS) /subsystem:windows /machine:I386 /out:$(1)\$(2) \
    /map:$(1)\$(basename $(2)).map $(addprefix $(1)\,$(LAUNCHER_SRC:.cpp=.obj)) $(1)\launcher.res $(1)\regmgr32\RegMgr32.lib $(LAUNCHER_LIBS)) \
    > $(ROOT)/$(LAUNCHER)/$(1)/$(2).rsp; } && \
    { $(ENV_GAME) $(WINE) $(LINK) '@$(call ws,$(ROOT)/$(LAUNCHER)/$(1)/$(2)).rsp' > $(ROOT)/$(LAUNCHER)/$(1)/$(basename $(2))-link.txt 2>&1 || \
      { grep -i 'error\|fatal' $(ROOT)/$(LAUNCHER)/$(1)/$(basename $(2))-link.txt | head -20 >&2; false; }; }))
endef
$(eval $(call launcher_link,Release,AVP2.exe,AVP2.exe,))
$(eval $(call launcher_link,PrimalHunt,PrimalHunt.exe,PrimalHunt.exe,/d PRIMAL_HUNT))
.PHONY: launchers
launchers: $(LAUNCHER)/Release/AVP2.exe $(LAUNCHER)/PrimalHunt/PrimalHunt.exe

.PHONY: engine-objs ph-engine-objs avp2-engine-objs
ph-engine-objs: $(call eng_objs,Release) src/engine/lithtech/Release/wonapi/WONAPI.lib
avp2-engine-objs: $(call eng_objs,AvP2Release)
engine-objs: ph-engine-objs avp2-engine-objs

# Primal Hunt's lithtech.exe and server.dll, linked from our objects: not to
# run (the engine is far from whole) but so that the scoreboard can read
# what our link put where -- progress/targets.py's `built` images, their
# response files (<image>.rsp: what was linked) and maps. In a linked image
# the relocations are resolved, which is what verifies the "small"
# functions (tools/progress.py, VERIFYING THE SMALL ONES).
#
# $(LINK), link.exe 6.00.8447 (SP5's, as the rest of Primal Hunt's build),
# with /FORCE: an image is written whatever is unresolved or defined twice.
# An unresolved symbol then resolves to the image's base, RVA 0 (our
# lithtech.exe's calls to unresolved functions all reach 0x00400000),
# which tools/progress.py never takes as naming anything. Neither link takes nolfsrc's link-time stand-ins
# (link/aliases.rsp, link/vtables.txt): they name NOLF's symbols and
# addresses, and an alias or a stand-in vtable that is wrong for Primal
# Hunt would make our image name a thing it does not have. No resources:
# they are no code. /OPT:NOREF keeps every function of ours in the image,
# where the retail link's /OPT:REF (the default) dropped what nothing
# reaches: with so much unresolved, nothing reaches most of the engine, and
# REF kept 2,222 of lithtech.exe's 7,252 code symbols -- a function not in
# our image cannot be verified through it. /OPT:ICF still folds identical
# COMDATs as the retail link did (REF implies it; NOREF does not): 7,252
# symbols at 5,916 addresses. Each link's output goes to its report,
# Release/<image>-link.txt; the rule fails only when no image comes out.
#
# The import libraries are what the retail images' import tables name:
#   lithtech.exe  WINMM DINPUT KERNEL32 USER32 GDI32 ole32 mss32 WSOCK32
#   server.dll    WINMM KERNEL32 USER32 ole32 WSOCK32
# Miles' mss32.dll exports its functions under their decorated names, and
# lithtech.exe imports them so (_AIL_startup@0): its import libraries are
# made from src/launcher/libs/mss32.def (lib.exe for the descriptor,
# tools/mkimplib.py --keep-decoration for the names), into Release/implib/
# on the link's LIB path, where the scoreboard does not take them for an
# input of ours (link_inputs reads only the directory the link runs in
# and its /libpath:s).
ENG_IMPLIB := $(LITHTECH)/Release/implib
MSS32_DEF := src/launcher/libs/mss32.def
MSS32_DECORATED = $(shell sed -n "s/^ *\(AIL_[A-Za-z0-9_]*@[0-9]*\) *$$/_\1/p" $(MSS32_DEF))
$(eval $(call tool,$(ENG_IMPLIB)/mss32.lib,$(MSS32_DEF) | wine-drives,\
    mkdir -p $(ENG_IMPLIB) && cd src/launcher && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo /machine:i386 '/def:libs\mss32.def' \
    '/out:$(call ws,$(ROOT)/$(ENG_IMPLIB))\mss32.lib' >/dev/null))
$(eval $(call tool,$(ENG_IMPLIB)/mss32dec.lib,$(MSS32_DEF),\
    python3 tools/mkimplib.py --keep-decoration $(ENG_IMPLIB)/mss32dec.lib mss32.dll $$(MSS32_DECORATED) >/dev/null))
ENG_LINK_ENV := $(subst LIB=',LIB='$(call ws,$(ROOT)/$(ENG_IMPLIB));,$(ENG_PH_ENV))

# $(call eng_link,<image>,<link dir>,<flags>,<inputs as the link dir sees them>,<prerequisites>[,<env>])
define eng_link
$(call tool,$(1),$(5) | wine-drives,\
    cd $(2) && { printf '%s\n' $(call q,/nologo $(3) /FORCE /OPT:NOREF /OPT:ICF /machine:I386 /out:$(subst /,\,$(patsubst $(2)/%,%,$(1))) \
    /map:$(subst /,\,$(patsubst $(2)/%,%,$(basename $(1)))).map $(subst /,\,$(4))) > $(1:%=$(ROOT)/%).rsp; } && \
    { $(or $(6),$(ENG_LINK_ENV)) $(WINE) $(LINK) '@$(call ws,$(ROOT)/$(1)).rsp' > $(basename $(1:%=$(ROOT)/%))-link.txt 2>&1 || true; } && \
    grep -c 'error LNK2001\|error LNK2019' $(basename $(1:%=$(ROOT)/%))-link.txt | sed 's/^/   LINK $(notdir $(1)): unresolved references: /' ; \
    grep -c 'error LNK2005' $(basename $(1:%=$(ROOT)/%))-link.txt | sed 's/^/   LINK $(notdir $(1)): defined twice: /' ; \
    test -f $(1:%=$(ROOT)/%) || { grep -i 'error\|fatal' $(basename $(1:%=$(ROOT)/%))-link.txt | head -20 >&2; false; })
endef
LS_R := ..\..\LT2\lithshared\libs\Release
LS_D := ..\..\LT2\lithshared\libs\Debug
LT_OBJS := $(sort $(foreach s,$(LITHTECH_ALL),$(call c_obj,$(s),$(LITHTECH)/Release)))
SV_OBJS := $(sort $(patsubst %.cpp,$(SERVER)/Release/%.obj,$(SERVER_SRC) $(SERVER_OS_SRC)))
# lithtech.exe: our objects, then WON (the engine's VC6-STL build), RezMgr,
# lith and StdLith, in the order their blocks lie in the retail image
# (progress/targets.py, lib_range: 0x004aab40, 0x004ba2a0..)
$(eval $(call eng_link,$(LITHTECH)/Release/lithtech.exe,$(LITHTECH),/subsystem:windows,\
    $(LT_OBJS:$(LITHTECH)/%=%) Release/wonapi/WONAPI.lib $(LS_R)\RezMgr.lib $(LS_R)\lith.lib $(LS_R)\StdLith.lib \
    winmm.lib dinput.lib dxguid.lib kernel32.lib user32.lib gdi32.lib ole32.lib uuid.lib mss32dec.lib mss32.lib wsock32.lib,\
    $(LT_OBJS) $(LITHTECH)/Release/wonapi/WONAPI.lib $(LOUT)/RezMgr.lib $(LOUT)/lith.lib $(LOUT)/StdLith.lib \
    $(ENG_IMPLIB)/mss32.lib $(ENG_IMPLIB)/mss32dec.lib))
# server.dll: our objects, then the debug RezMgr and StdLith (one block from
# 0x1004a6e0) and WON (0x10051e10); link/server.def names the DLL
# "server.DLL" as the retail export table does. The debug libraries name
# LIBCMTD as their default library, but the retail image has the release
# runtime's only (its "Assertion failed: %s, file %s, line %d" is
# libcmt.lib's assert.c; no "Debug Assertion Failed!", nothing of the
# debug heap or _CrtDbgReport), so not LIBCMTD.
$(eval $(call eng_link,$(SERVER)/Release/server.dll,$(SERVER),/dll /def:link\server.def /nodefaultlib:libcmtd.lib,\
    $(SV_OBJS:$(SERVER)/%=%) $(LS_D)\RezMgr.lib $(LS_D)\StdLith.lib ..\lithtech\Release\wonapi\WONAPI.lib \
    winmm.lib kernel32.lib user32.lib ole32.lib wsock32.lib,\
    $(SV_OBJS) $(LDOUT)/RezMgr.lib $(LDOUT)/StdLith.lib $(LITHTECH)/Release/wonapi/WONAPI.lib $(SERVER)/link/server.def))
.PHONY: engine-link
engine-link: $(LITHTECH)/Release/lithtech.exe $(SERVER)/Release/server.dll

# AvP2 1.0.9.6's lithtech.exe and server.dll, linked as Primal Hunt's are
# (above): /FORCE /OPT:NOREF /OPT:ICF, a response file and a map, for the
# scoreboard. From AvP2Release/'s objects (cl 9044) in the order the
# scoreboard's placed functions put them in the retail images (each object
# by the median retail address of the functions placed from it; the
# objects with none follow, sorted), approximately retail's. Their Rich
# headers name link 6.00.8447 (SP5's, as msvc-6.00-9044's link.exe is,
# byte for byte), the same DirectX 8.1 import libraries as Primal Hunt's
# (implib 7.00.9210 x4: dinput.lib's import descriptors; cl 13.00.9178,
# one C++ and one C object: dxguid.lib's and, by the data formats it puts
# in .text, dinput.lib's) and SP5's libcmt.lib (C 8047), and their
# import tables are Primal Hunt's: the same import libraries, Miles' from
# Release/implib/. The libraries are release configurations (tools/libscan.py
# finds none of the debug builds' functions in either image, where Primal
# Hunt's server.dll links them), built with cl 9044 as the engine is
# ($(LAOUT), above: "AvP2 1.0.9.6's engine's builds"); and WON is the
# release's STLport build ($(LOUT)/wonapi/, 8966's), not Primal Hunt's
# VC6-STL one: it places 840 of its functions in lithtech.exe and 777 in
# server.dll, against 689 and 594 for the same source built STLport with
# cl 9044. In the retail images, as the libraries are laid out:
#   lithtech.exe  the objects to 0x004a04f3; WON 0x004a0500..0x004af948;
#                 the DINPUT thunk and dinput.lib's object data formats
#                 (0x004af950, 0x004af960: in .text), an initialiser
#                 that registers AIL_shutdown with atexit (0x004afc20:
#                 no object of ours; Miles' mss32.lib's?); lith's
#                 baselist 0x004afc50; RezMgr 0x004afce0; StdLith from
#                 0x004b26b0; then controlfile.cpp's code (0x004b3490..
#                 0x004b42d9), after the libraries as a library's member
#                 would be (here the last object); the C runtime from
#                 0x004b42e0 (fpinit.obj), lith's basehash and virtlist
#                 inside it (0x004c31a0..0x004c33d4: loaded on the library
#                 search's second pass, for RezMgr)
#   server.dll    the objects to 0x10045a06; RezMgr 0x10045a10; StdLith
#                 0x10048410..0x100490e3; WON 0x10049210..0x10057b48; the
#                 C runtime from 0x10057b50 (fpinit.obj), all of lith
#                 inside it (0x10066a50..0x10066cf4: second pass, for
#                 RezMgr)
# So lith comes before RezMgr on both command lines, and in lithtech.exe
# after the import libraries.
LT_AVP_ORDER := \
    modelanim binkvideo counter clientwin clientphysics cltapi lvideomgr cltnew cltrest clientinterface \
    clientworldinfo cissurf clientblit clientmgr2 cmgrinit clientmgr3 startload csvis worldsim ltnew2 \
    sweptsphere console conline clientconsole ccline lt2new4 ccclient servermsg csbase clienttexmgr fsstream \
    worldmodels worldbsp dalloc ltobject dsi cmgsurf collelem dhash objbank clientshell clientworld demodtx \
    clientdsi packet ccobj worldisect wgrid savobj saveio genericprop basestream servermath sharedcommon \
    filemgr warpblit clientinput diblit lmessage localdriver cltcursor clientmusic dmextra ltnew1 dmgrow \
    lt2new2 dmloader lt2new1 lthread sdkforce modelnew commonmodel collection modchunk commonphysics wmrotate \
    msgterm netmgr nettrace netobj objtypes lt2new3 partfm fmconvert fmcvt32 lttexmod predobj clientrender \
    serverengine serverlightanim smgrmisc servercommon servermodel servermessage smgr smgrnew cmodelload \
    cchandler objmover sndinst2 clientsoundinst soundinst2 clientsoundmgr sndmgr2 clientsound spritecontrol \
    streamsim clientstreams stringbank tcpdriver tcpqarray transform fntdraw smackvideo worldtree2 worldtree \
    stdlithalloc
SV_AVP_ORDER := \
    modelanim counter structbank worldsim console csbase sweptsphere fsstream worldmodels servermsg worldbsp \
    dalloc ltobject dhash dsi svlast worldisect saveio genericprop basestream servermath svnew sharedcommon \
    lmessage localdriver lthread modelnew collection commonmodel sdkforce commonphysics netmgr packet \
    serverengine netobj objtypes objbank serverlightanim smgrmisc filemgr servermgr server servercommon \
    servermodel servermessage smgr smgrnew objmover spritecontrol streamsim stringbank tcpdriver worldtree
LT_AVP_OBJS := $(sort $(foreach s,$(LITHTECH_ALL),$(call c_obj,$(s),AvP2Release)))
LT_AVP_LINK := $(addprefix AvP2Release/,$(addsuffix .obj,$(LT_AVP_ORDER))) \
    $(filter-out $(addprefix AvP2Release/,$(addsuffix .obj,$(LT_AVP_ORDER) controlfile)),$(LT_AVP_OBJS)) AvP2Release/controlfile.obj
SV_AVP_OBJS := $(sort $(patsubst %.cpp,AvP2Release/%.obj,$(SERVER_SRC) $(SERVER_OS_SRC)))
SV_AVP_LINK := $(addprefix AvP2Release/,$(addsuffix .obj,$(SV_AVP_ORDER))) \
    $(filter-out $(addprefix AvP2Release/,$(addsuffix .obj,$(SV_AVP_ORDER))),$(SV_AVP_OBJS))
LS_A := ..\..\LT2\lithshared\libs\AvP2Release
ENG_AVP_LINK_ENV := $(subst LIB=',LIB='$(call ws,$(ROOT)/$(ENG_IMPLIB));,$(ENG_AVP_ENV))
$(eval $(call eng_link,$(LITHTECH)/AvP2Release/lithtech.exe,$(LITHTECH),/subsystem:windows,\
    $(LT_AVP_LINK) ..\..\LT2\lithshared\libs\Release\wonapi\WONAPI.lib \
    winmm.lib dinput.lib dxguid.lib kernel32.lib user32.lib gdi32.lib ole32.lib uuid.lib mss32dec.lib mss32.lib \
    $(LS_A)\lith.lib $(LS_A)\RezMgr.lib $(LS_A)\StdLith.lib wsock32.lib,\
    $(addprefix $(LITHTECH)/,$(LT_AVP_LINK)) $(LOUT)/wonapi/WONAPI.lib $(LAOUT)/RezMgr.lib $(LAOUT)/lith.lib $(LAOUT)/StdLith.lib \
    $(ENG_IMPLIB)/mss32.lib $(ENG_IMPLIB)/mss32dec.lib,$(ENG_AVP_LINK_ENV)))
$(eval $(call eng_link,$(SERVER)/AvP2Release/server.dll,$(SERVER),/dll /def:link\server.def,\
    $(SV_AVP_LINK) $(LS_A)\lith.lib $(LS_A)\RezMgr.lib $(LS_A)\StdLith.lib ..\..\LT2\lithshared\libs\Release\wonapi\WONAPI.lib \
    winmm.lib kernel32.lib user32.lib ole32.lib wsock32.lib,\
    $(addprefix $(SERVER)/,$(SV_AVP_LINK)) $(LAOUT)/RezMgr.lib $(LAOUT)/lith.lib $(LAOUT)/StdLith.lib $(LOUT)/wonapi/WONAPI.lib \
    $(SERVER)/link/server.def,$(ENG_AVP_LINK_ENV)))
.PHONY: avp2-engine-link
avp2-engine-link: $(LITHTECH)/AvP2Release/lithtech.exe $(SERVER)/AvP2Release/server.dll
# ... and engine-link links both engines
engine-link: avp2-engine-link

# Primal Hunt's d3d.ren, linked from our objects as lithtech.exe and
# server.dll are (engine-link, above): for the scoreboard, not to run, with
# /FORCE and /OPT:NOREF /OPT:ICF for the same reasons. Unresolved symbols
# resolve to the image base; Release/d3d-link.txt lists them. nolfsrc's
# link-time stand-ins (link/aliases.rsp, link/d3dviews.txt) are not taken:
# they name NOLF's renderer's addresses, not Primal Hunt's.
#
# As the retail image has it (its headers, export and import tables, Rich
# header): a DLL at 0x10000000 linked by link 6.00.8447, exporting
# FreeModeList, GetSupportedModes and RenderDLLSetup (ordinals 1..3, by
# name, from /export:, which numbers them alphabetically as the retail
# export table does), its one resource (d3d.rc; cvtres in the Rich
# header). Imports DDRAW (the DirectX 8.1 SDK's ddraw.lib, whose import
# descriptors are implib 7.00.9210's: the header's three), WINMM, KERNEL32,
# USER32, in that order: the import libraries in that order on the line,
# and the runtime's (libcmt.lib, by default) after. Linked under
# Microsoft's msvcrt (nlink, above), for the order of the imports within
# each DLL.
#
# After the objects and the three DDRAW import thunks (0x1003c136) the
# retail image has the DEBUG StdLith.lib, as server.dll does:
# struct_bank.obj, l_allocator.obj (LAlloc, g_DefAlloc) and dynarray.obj
# at 0x1003c150..0x1003c58b, unoptimised and int3-padded (tools/libscan.py
# finds the release's prebuilt debug build there, function for function),
# and then libcmt.lib. So $(LS_D)\StdLith.lib follows the import
# libraries, without its default LIBCMTD, and nolfsrc's w12_7fae0.cpp
# (NOLF's renderer's /O2 build of l_allocator.cpp) is left out of the
# link. Our objects still call DefaultCache's members out of line, which
# draws in the library's CompressedIO.obj too (and through it zlib's
# deflate, unresolved); retail has no such member. dxguid.lib (DirectX
# 8.1, @comp.id cl 13.00.9178: the header's one C++ object of that
# compiler) is offered last; d3dguid.obj still defines the GUIDs the
# renderer names, so nothing is drawn from it yet.
#
# Objects in the order the scoreboard's identical and near pairs place
# them in the retail image (MS link lays objects out in command-line
# order): each object by the median retail address of its identical
# functions, else its nears, else its small ones; an object with no
# evidence beside the one whose NOLF address (its file name) is nearest
# its own, an _os/_o2 sibling beside its sibling; the rest, which have
# neither, last. nolfsrc's files are split by Intel flag groups, not by
# Primal Hunt's translation units, so this approximates retail's order
# only. Objects not listed (new files) follow, sorted.
D3D_LINK_ORDER := \
    w6_10034f10_osoi w14_10008a40_qipni w2_10008da0 big8fd0 w57_10009860 w2_10009ae0 w64_10009f10_qip \
    w2_1000a110 w2_1000a770 w3_1000a7d0_os w2_1000a8a0_os w54_1000ae20_osoiqip w2_1000d940_os \
    w15_1000d9f0_os w2_10003a10 w14_10003bb0 big3c40 w9_1006f540_os w53_10070d10_os big729c0 big73140 \
    w10_10073400_osoiqip w59_10073640_osoi w56_10073700_osoiqipsf w29_10073c00_osoiqip big051a0 \
    w57_10053e20_os w57_100553d0_os w14_10006840 big7cc90 w22_1007dbd0_os w35_1007dc54_os w1_10001000 \
    w44_10001670 w1_10002310 w44_10002d40 w57_100037f0 d3dr5c_o2 big24090 w54_1000daa0_qipsf \
    w30_1000f020_osqipni w2_1000f2c0_os w14_10011980_osoiqip w56_1002ec60_osoiqipsf w28_1002ef30_osqipni \
    w26_100371e0_osoi w12_374a0 w18_10037570 w25_10037bf0_qipni d3dbatchb_os w3_10014580_os \
    w43_10017c10_osoiqipni d3dr2c d3dbatchb w24_10068c80_osqipni w17_10069a10_osoi w18_1001dc70_os \
    w32_1001e210 w31_1001e360 w17_10023930 w13_1006c6e0_osoiqip w9_1006d210_osoi w48_1006d3e0_os \
    w57_1006d500_os big6d910 w4_1001eb90_os big1ef30 d3dr2b d3dbatcha d3dbatcha_o2 d3dr5c \
    w17_10020e80_osoi w26_10020fd0_osoiqip w12_21c50_osoi w4_10021cb0_os w34_10021dc0_ossf \
    w15_10021e50_os w57_1003b1c0 w12_3b3d0_os big21f90 w15_10022e90_osoiqip big2b1b0 w57_1002db00 \
    w5_1002e4c0 w4_10023c40_os d3dr3a w4_10023fb0 big24020 w4_100234d0 w15_100235d0 w8_10069ca0_osoiqip \
    w8_10069ed0_osoi w8_1006a5f0_osoiqip w8_1006a860_os w4_1001e4a0_os d3dbatchd d3dbatchd_o2 \
    w28_100256d0_qipni w26_100262f0 w4_100265a0_qip w29_10027f70 w38_10028030_qip d3dr4a d3dneutral \
    w5_10033ba0 w53_10033f30_osoiqip d3dr2a d3dr2a_os d3ddraw d3ddraw_os w5_10030f30 w15_100316e0 \
    w26_10031e10 w38_100322c0_qip w31_10032a00_qip w81_10034ed0_osg5 w4_1001cd40_osoi w18_1001d180_os \
    w12_1d370 w45_1001d810_osoi d3dqip3 w18_10035a70_osoiqip w6_10035ab0_os w18_10035e20_osoi \
    w12_36320_os d3dmisc w6_1003acf0 w57_1007e830_osoiqip d3dr4b d3dr4b_o2 d3dqip2 \
    w6_10039730_qip w19_1003b6f0_osoiqip w6_1003b9c4_os w18_1003bb10_os w34_1003e620_osoiqipsf \
    w7_10040cc0 big40e10 d3dr3b d3dr5a_o2 d3dr5a d3dqip d3dr4a_o2 w27_1005dae0 w16_10042e80 \
    w35_10043e10_qip w31_10044ff0_qipni big47230 w7_10047d80_os w54_10048180_osoiqipsf w7_1004b5c0_os \
    big4b720 d3dr2b_o2 d3dr2d d3dr2d_os w35_1004e360_os big4e3e4 w16_1004fea4_qip w7_100500a0_qip \
    w19_10050450 d3dr5b w11_1007e9f0_osoi d3dr4c w17_1001c760_os w12_1c7e0_os w4_1001c8d0_os \
    w27_10050900_os w19_10050e10_os w23_10050e60_os w23_100525b0_os w27_10056ab0_osqipni w24_10059380_os \
    w24_10059980_osoi w27_10059c80_osoi w7_1005a550 w40_1005a960_qipni w8_1005d650 d3dr3c d3dr3c_os \
    w19_1005db50 big5dca0 w17_1005f580 w8_1005f9a0 w8_1005fb70 w63_10060010_qip d3dr5b_o2 w16_10064720 \
    w8_10064800 w31_10066d10_qipni d3dr3a_os d3dr3b_o2 w17_1006ad20_qip d3dr2c_os d3dbatchc_o2 \
    w57_1007bb20_qipni w26_1007be90 w10_1007c250 w22_1007c580 w10_1007cab4_os d3dqip4 w49_1007a700 \
    d3dblit d3dbatchc d3dhigh w12_15950_os w32_10015d60_osoi w27_10015e80_osoiqip w3_10016234_os \
    w55_10016324_os w29_10079fd0_osoiqip w10_1007a280_os w22_1007a3f0_os w59_1007dd24_osoi \
    w22_1007dde0_os d3d d3ddata d3dguid d3dinit d3dlow d3dmid d3dmid_os d3dopt2d d3dr3d d3dr5d d3dr5d_os \
    wC1_r5c
D3D_LINK_NOT := w12_7fae0
#
# Primal Hunt's own translation units, src/engine/d3d/ph/: one file for
# each object of the retail image (docs/D3D_OBJECTS.md, its TU numbers),
# compiled with that object's own flags (retail's renderer is /O1 and
# /O2 by file, without /GX). Each has a fragment ph/<name>.mk:
#   D3D_PH += <name>
#   D3D_PH_POS_<name> := <its TU number, two digits: its place in the link>
#   D3D_PH_CL_<name>  := <its flags, e.g. /O1>
#   D3D_PH_INC_<name> := <include paths it alone needs, ahead of the SDK's;
#                         optional>
# They link first, in TU order, ahead of nolfsrc's files. A function one of
# them takes over stays in nolfsrc's file for AvP2 1.0.9.6's Intel build,
# behind #ifndef PRIMAL_HUNT.
D3D_PH :=
include $(sort $(wildcard $(D3D)/ph/*.mk))
D3D_PH_ORDER := $(foreach e,$(sort $(foreach n,$(D3D_PH),$(D3D_PH_POS_$(n)):$(n))),$(lastword $(subst :, ,$(e))))
$(foreach n,$(D3D_PH),$(eval $(call compile,$(D3D)/Release/ph/$(n).obj,$(D3D),ph/$(n).cpp,$(ENG_PH_ENV),$(ENG_PH_CC),\
    $(D3D_PH_INC_$(n)) $(D3D_BASE) /D PRIMAL_HUNT $(D3D_PH_CL_$(n)) /FoRelease/ph/,wine-drives)))
#
# AvP2 1.0.9.6's renderer is the same source one compiler earlier: cl
# 12.00.8168 (VC6 RTM; its Rich header's 73 C++ objects), not Intel C/C++
# as NOLF's was -- none of Intel's padding, and the same objects in the same
# link order as Primal Hunt's, without the fog volumes (fogvolume.cpp is
# Primal Hunt's). So the ph/ files are compiled a second time, with 8168,
# their own flags and without PRIMAL_HUNT, into AvP2Release/ph/
# (docs/AVP2_D3D_OBJECTS.md has the map). Objects only, for
# now: the scoreboard scores them as they are (targets.py, partial_objs).
D3D_AVP_PH := $(filter-out fogvolume,$(D3D_PH))
$(foreach n,$(D3D_AVP_PH),$(eval $(call compile,$(D3D)/AvP2Release/ph/$(n).obj,$(D3D),ph/$(n).cpp,$(call vc_env,$(V8168)),$(V8168)/BIN/cl.exe,\
    $(D3D_PH_INC_$(n)) $(D3D_BASE) $(D3D_PH_CL_$(n)) /FoAvP2Release/ph/,wine-drives)))
.PHONY: avp2-d3d
avp2-d3d: $(D3D_AVP_PH:%=$(D3D)/AvP2Release/ph/%.obj)
D3D_LINK_OBJS := $(D3D_PH_ORDER:%=ph/%.obj) $(addsuffix .obj,$(D3D_LINK_ORDER)) \
    $(sort $(filter-out $(addsuffix .obj,$(D3D_LINK_ORDER) $(D3D_LINK_NOT)),$(D3D_SRC:.cpp=.obj)))
D3D_LINK_LIBS := ddraw.lib winmm.lib kernel32.lib user32.lib $(LS_D)\StdLith.lib dxguid.lib
D3D_EXPORTS := FreeModeList GetSupportedModes RenderDLLSetup
$(eval $(call tool,$(D3D)/Release/d3d.res,$(D3D)/d3d.rc | wine-drives,\
    cd $(D3D) && $(ENV_GAME) $(WINE) $(RC) $(AVP2SRC_RCFLAGS) /l 0x409 /d NDEBUG /foRelease/d3d.res d3d.rc))
$(eval $(call tool,$(D3D)/Release/d3d.ren,$(addprefix $(D3D)/Release/,$(D3D_LINK_OBJS) d3d.res) $(LDOUT)/StdLith.lib $(call nlink,$(V8966)) | wine-drives,\
    cd $(D3D) && { printf '%s\n' $(call q,/nologo /dll /base:0x10000000 /nodefaultlib:libcmtd.lib /FORCE /OPT:NOREF /OPT:ICF /machine:I386 \
    /out:Release\d3d.ren /map:Release\d3d.map $(addprefix /export:,$(D3D_EXPORTS)) \
    $(subst /,\,$(addprefix Release/,$(D3D_LINK_OBJS) d3d.res)) $(D3D_LINK_LIBS)) > $(ROOT)/$(D3D)/Release/d3d.ren.rsp; } && \
    { $(call nlink_env,$(V8966)) $(WINE) $(ROOT)/$(call nlink,$(V8966)) '@$(call ws,$(ROOT)/$(D3D)/Release/d3d.ren).rsp' \
      > $(ROOT)/$(D3D)/Release/d3d-link.txt 2>&1 || true; } && \
    grep -c 'error LNK2001\|error LNK2019' $(ROOT)/$(D3D)/Release/d3d-link.txt | sed 's/^/   LINK d3d.ren: unresolved references: /' ; \
    grep -c 'error LNK2005' $(ROOT)/$(D3D)/Release/d3d-link.txt | sed 's/^/   LINK d3d.ren: defined twice: /' ; \
    test -f $(ROOT)/$(D3D)/Release/d3d.ren || { grep -i 'error\|fatal' $(ROOT)/$(D3D)/Release/d3d-link.txt | head -20 >&2; false; }))
.PHONY: d3d-link
d3d-link: $(D3D)/Release/d3d.ren

# ---------------------------------------------------------------------------
# cdaudio.dll
#
# The engine's CD-audio music driver (2002-05-07): MusicDLLSetup's function
# table over lithshared's CRedMgr, MFC linked statically. Its Rich header:
# four C++ objects of cl 12.00.8966 -- cdaudio.cpp's own and the three
# library members it links (Redmgr.lib's redmgr.obj, StdLith's
# l_allocator.obj and dynarray.obj) -- MFC's nafxcw.lib (C++ 8569), SP5's
# C runtime, link 6.00.8447. Its code, in link order: cdaudio.obj
# (0x10001000), redmgr.obj (0x10002390), StdLith (0x10002960), nafxcw.lib.
#
# Redmgr.lib: the release ships it prebuilt only (libs/release/), so it is
# reconstructed here (src/lithshared/redmgr/redmgr.cpp, against the
# release's redmgr.h), with the lithshared libraries' flags and MFC's
# headers (its object names nafxcw.lib), into Release/ beside its source --
# not over the prebuilt copy the game links take.
CDAUDIO := $(E)/cdaudio
REDMGR  := src/lithshared/redmgr
$(eval $(call compile,$(REDMGR)/Release/redmgr.obj,$(REDMGR),redmgr.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    $(LIB_CFLAGS) /I $(LT2LS)\redmgr /I $(LT2LS)\incs /FoRelease/,wine-drives))
$(eval $(call tool,$(REDMGR)/Release/Redmgr.lib,$(REDMGR)/Release/redmgr.obj | wine-drives,\
    cd $(REDMGR) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:Release\Redmgr.lib' 'Release\redmgr.obj'))
# cdaudio.dll: cl 8966 /O2 /GX, static MFC and runtime, no _USRDLL (the
# image has the runtime's default DllMain, not MFC's), and an automatic
# precompiled header (/YX): through one, cl defers every inline function
# and template instance of the headers to the end of the object, as the
# retail one has them (src/engine/cdaudio/cdaudio.h). Linked through a
# response file with a map (progress/targets.py reads both), against our
# Redmgr.lib and StdLith.lib, then MFC, the runtime and the three import
# libraries the code calls: in that order the import directory comes out
# as the retail one's (USER32, COMCTL32, WINMM, then the runtime's
# KERNEL32, GDI32, WINSPOOL), and so do the names /OPT:REF leaves behind.
CDAUDIO_CL := /nologo /MT /W3 /GX /O2 /YX /FpRelease/cdaudio.pch /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS \
    /I ..\..\LT2\lithshared\redmgr /I ..\..\LT2\lithshared\incs /I ..\..\LT2\lithshared\stdlith /I ..\..\LT2\sdk\inc /c
$(eval $(call compile,$(CDAUDIO)/Release/cdaudio.obj,$(CDAUDIO),cdaudio.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    $(CDAUDIO_CL) /FoRelease/,wine-drives))
# StdLith's two members it links, built as the release's prebuilt
# StdLith.lib was (Lib_StdLith.dsp: /YX): with the automatic precompiled
# header cl puts l_allocator.obj's four initializers ahead of its inline
# functions, as the image has them (0x10002960..0x10002990, then
# DefStdlithAlloc), where our StdLith.lib, built without it, puts
# LAlloc::Alloc/Free and the scalar deleting destructor between them.
# Into a StdLith.lib of cdaudio's own, so that the map still names them
# StdLith's; the game DLLs' links keep taking $(LOUT)/StdLith.lib.
$(eval $(call compile,$(CDAUDIO)/Release/l_allocator.obj,src/lithshared/stdlith,l_allocator.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    $(LIB_CFLAGS) /YX /Fp..\..\engine\cdaudio\Release\l_allocator.pch /I $(LT2LS)\stdlith /I $(LT2LS)\lith /I . /I sys\win /I ..\zlib \
    /Fo..\..\engine\cdaudio\Release\l_allocator.obj,wine-drives))
$(eval $(call tool,$(CDAUDIO)/Release/StdLith.lib,$(CDAUDIO)/Release/l_allocator.obj src/lithshared/stdlith/Release/dynarray.obj | wine-drives,\
    cd $(CDAUDIO) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:Release\StdLith.lib' 'Release\l_allocator.obj' '..\..\lithshared\stdlith\Release\dynarray.obj'))
$(eval $(call tool,$(CDAUDIO)/Release/cdaudio.dll,$(CDAUDIO)/Release/cdaudio.obj $(REDMGR)/Release/Redmgr.lib $(CDAUDIO)/Release/StdLith.lib | wine-drives,\
    cd $(CDAUDIO) && { printf '%s\n' /nologo /dll /subsystem:windows /machine:I386 /out:Release\\cdaudio.dll \
    /implib:Release\\cdaudio.lib /map:Release\\cdaudio.map Release\\cdaudio.obj \
    ..\\..\\lithshared\\redmgr\\Release\\Redmgr.lib Release\\StdLith.lib \
    nafxcw.lib libcmt.lib user32.lib comctl32.lib winmm.lib > $(ROOT)/$(CDAUDIO)/Release/cdaudio.dll.rsp; } && \
    $(ENV_GAME) $(WINE) $(LINK) $(AVP2SRC_LDFLAGS) '@$(call ws,$(ROOT)/$(CDAUDIO)/Release/cdaudio.dll).rsp'))
.PHONY: cdaudio
cdaudio: $(CDAUDIO)/Release/cdaudio.dll
all game: cdaudio avp2up launch-dll launchers

# ---------------------------------------------------------------------------
# The dedicated servers: AVP2Serv.exe's libraries, and AVP2XServ.exe's
#
# Both are the workspaces' ServerApp projects (ServerApp, ServerApp_PH);
# what the project files cannot say is here.
#
# AVP2Serv.exe (AvP2 1.0.9.6, 2002-02-07) links butemgr.lib, cryptmgr.lib
# and StdLith.lib, but not the release's builds of them: its Rich header
# has no object of cl 8966 at all, only 8168 (C++ x136: its nine objects,
# MFC's and the runtime's, and the four library members it takes --
# butemgr.obj, cryptmgr.obj, struct_bank.obj, l_allocator.obj), where the
# release's prebuilt libraries are all 8966. So they are built again here
# with the server's own compiler: ButeMgr as MFC's build of it (the
# release's prebuilt ButeMgr.lib is MFC's: it names nafxcw.lib and uses
# CString), from the same sources and flags as ButeMgrNoMFC less _NOMFC,
# and CryptMgr and StdLith from theirs, into the project's own directory,
# where link.exe looks before the /libpath the project names (and where
# server.lib is already). tools/progress.py looks there too.
SVLIB_OUT := src/AVP2/AVP2Serv
SV_LIBS :=
# $(call svlib,<dir under src/lithshared>,<library>,<flags>,<sources>)
define svlib
$(foreach s,$(4),$(eval $(call compile,$(call c_obj,$(s),src/lithshared/$(1)/Release/ServerApp),src/lithshared/$(1),$(s),$(ENV_ServerApp),$(CL_ServerApp),$(LIB_CFLAGS) $(3) /FoRelease/ServerApp/,wine-drives)))
SVOBJ_$(1) := $(foreach s,$(4),$(call c_obj,$(s),src/lithshared/$(1)/Release/ServerApp))
$(call tool,$(SVLIB_OUT)/$(2).lib,$$(SVOBJ_$(1)) | wine-drives,cd src/lithshared/$(1) && $(ENV_ServerApp) $(WINE) $(V8168)/BIN/lib.exe /nologo '/out:$(call ws,$(ROOT)/$(SVLIB_OUT))\$(2).lib' $$(call q,$$(subst /,\,$$(patsubst src/lithshared/$(1)/%,%,$$(SVOBJ_$(1))))))
SV_LIBS += $(SVLIB_OUT)/$(2).lib
endef
$(eval $(call svlib,ButeMgr,ButeMgr,/I $(LT2LS)\butemgr /I $(LT2LS)\stl /I $(LT2LS)\stdlith /I $(LT2LS)\cryptmgr /I $(LT2LS)\lith /I $(LT2LS)\incs,\
    arange.cpp avector.cpp butemgr.cpp stdafx.cpp))
$(eval $(call svlib,CryptMgr,CryptMgr,/I $(LT2LS2)\\cryptmgr /I $(LT2LS2)\\incs /I .,cryptmgr.cpp blowfish.cpp))
$(eval $(call svlib,stdlith,StdLith,/I $(LT2LS)\stdlith /I $(LT2LS)\lith /I . /I sys\win /I ..\zlib,$(SL_SRCS)))
src/AVP2/AVP2Serv/Release/AVP2Serv.exe: $(SV_LIBS)
#
# AVP2XServ.exe (Primal Hunt, 2002-06-21): ServerApp_PH, the server's
# Release configuration with PRIMAL_HUNT, built as Primal Hunt's game DLLs
# are -- cl 12.00.8966 (CL_ServerApp_PH, above: its Rich header's 8966 x17
# are its own objects and the library members, of which ours has 15 so
# far; its 8168 x25 are objects of SP5's own libraries, which our link
# has as well), SP5's MFC (C++ 8569) and runtime, link 6.00.8447 --
# against the release's lithshared libraries, and one more: RegMgr32.lib,
# the registry class Primal Hunt's AddResources reads AvP2's install
# directory with (its code, 0x410580..0x4108b0, follows ButeMgr's), built
# as the launchers' is, into the directory the project names the libraries
# in. (Its link order is not yet the retail one: there StdLith's and
# CryptMgr's members lie above the runtime, at 0x429bf0..0x429dd0.)
# AVP2Serv.exe's own builds of the libraries in the project directory
# would be found first by this link too, so ServerApp_PH.dsp names the
# release's by path, as source files of the project.
$(eval $(call compile,src/lithshared/RegMgr32/Release/regmgr32.obj,$(REGMGR32),regmgr32.cpp,$(ENV_GAME),$(V8966)/BIN/cl.exe,\
    /nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D _MBCS /c /FoRelease/,wine-drives))
$(eval $(call tool,$(LOUT)/RegMgr32.lib,src/lithshared/RegMgr32/Release/regmgr32.obj | wine-drives,\
    cd $(REGMGR32) && $(ENV_GAME) $(WINE) $(LIBEXE) /nologo '/out:$(call ws,$(ROOT)/$(LOUT))\RegMgr32.lib' 'Release\regmgr32.obj'))
# Its splash screen is Primal Hunt's (ServerApp.rc, PRIMAL_HUNT): read out
# of the retail image (tools/rsrc2rc.py) into PrimalHunt/res/, not the tree.
SVPH_REF := $(firstword $(wildcard $(ROOT)/out/reference $(ROOT)/../reference))
$(eval $(call tool,src/AVP2/AVP2Serv/PrimalHunt/res/splash.bmp,$(SVPH_REF)/AVP2XServ.exe,\
    python3 tools/rsrc2rc.py $(SVPH_REF)/AVP2XServ.exe src/AVP2/AVP2Serv/PrimalHunt/res/retail.rc --res-dir=. --name=retail >/dev/null && \
    cp src/AVP2/AVP2Serv/PrimalHunt/res/retail_142.bmp src/AVP2/AVP2Serv/PrimalHunt/res/splash.bmp))
src/AVP2/AVP2Serv/PrimalHunt/ServerApp.res: src/AVP2/AVP2Serv/PrimalHunt/res/splash.bmp
.PHONY: servers
servers: ServerApp ServerApp_PH

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
