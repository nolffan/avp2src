// server.dll — the debug-output half of the system layer (reconstruction).
// Build /O1.
//
// The file name is ours, after the dsi_ prefix the rest of the engine uses
// for this layer. Only the printer is reconstructed so far; the interface
// table points its DebugOut entry straight at it, with no server-side
// wrapper in between.
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <new.h>
#include "servermgr.h"
#include "de_error.h"

// Declared here rather than through windows.h, whose macros rename half the
// engine's own methods out from under it. The shipped call goes through the
// import table, which is what dllimport gives.
extern "C" {
__declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);
}

// 0x100583c8 and 0x100583e4 — the allocation-tracking bank and how many
// times it has been asked for. Both are zeroed .bss in the shipped image;
// their addresses are relocations and so masked in verification, which is
// why what matters here is only that the counter is one dword of its own.
// The bank is not this file's alone: ltobject.cpp's poly-grid records are
// allocated from it and freed back to it, declared `extern` there (names
// ours).
StructBank        g_MemTrackBank;
static uint32     g_nMemTrackRefs;

// 0x10051d50 — the ninety-three bindings the engine ships with, the first of
// which names "BindIP" (name ours). server.dll's is defined below the
// variables it points at; lithtech.exe's twin is not rebuilt yet.
extern ConsoleBinding g_EngineConsoleBindings[];

// The variables those bindings point at that the net layer reads. Their
// home is this TU: the initialised ones sit in the .data run directly ahead
// of the table (0x10051c50 .. 0x10051d4c), the zeroed ones in the .bss run
// the .CRT$XCU initialisers beside dsi_GetConsoleBindings also own, and the
// values are the shipped .data words. The console names are the table's;
// the C names are the ones netmgr.cpp and servermgr.h already use (ours).
float g_AckTimeout           = 1.5f;    // 0x10051c68  "AckTimeout"
int   g_NetConnTroubleCount2 = 70;      // 0x10051c78  "ConnTroubleCount2"
int   g_NetTimeouts          = 1;       // 0x10051c80  "AllowTimeout"
int   g_NetMaxQueued         = 32;      // 0x10051c8c  "NetMaxQueue"

int   g_NetShowStats;                   // 0x100584ac  "ShowConnStats"
float g_NetResendTime;                  // 0x100584b8  "LatencySim"
float g_NetDropRate;                    // 0x100584bc  "DropRate"
int   g_NetShowTimeouts;                // 0x100584dc  "ShowThruput"
int   g_NetDebugAll;                    // 0x100584e0  "LocalDebug"
int   g_NetDebugLevel;                  // 0x100584ec  "TransportDebug"
int   g_NetLogPackets;                  // 0x100584f0  "ParseNet_Incoming"

// The same table's variables the server manager reads (smgr.cpp), on the
// same rule: the address is the one the table entry points at, the value
// the shipped .data word, the C names the ones smgr.cpp and servermgr.h
// already use (ours).
int    g_bCachingEnabled   = 1;         // 0x10051c60  "CacheFiles"
uint32 g_bTraceMarker51c64 = 1;         // 0x10051c64  "DelimitPackets"
uint32 g_MaxQueueSize      = 50;        // 0x10051c74  "ConnTroubleCount"
// 0x10051d20 is the one dword of the run 0x10051d14..0x10051d4c no table
// entry points at; it sits between LODScale (0x10051d1c) and TimeScale, so
// it is this file's all the same (30.0f, 0x41f00000).
float  g_UpdatesPerSec     = 30.0f;     // 0x10051d20
float  g_TimeScale         = 1.0f;      // 0x10051d24  "TimeScale"

int    g_Unknown5845c;                  // 0x1005845c  "FlashClients"
uint32 g_bPacketTrace58474;             // 0x10058474  "STracePackets"
int    g_NetBandwidth;                  // 0x10058484  "SendBandwidth"
int    g_bAutoDeactivate;               // 0x1005852c  "AutoDeactivate"
int    g_ShowGameTime;                  // 0x10058534  "ShowGameTime"
int    g_ShowClassTicks;                // 0x10058538  "ShowClassTicks"
int    g_ShowTickCounts;                // 0x1005853c  "ShowSphereFindTicks"
#ifdef LT_SERVER_BUILD
// Not bound, but inside this file's .bss run (0x10058430 is the message
// module handle below, 0x10058440.. are written by 0x1000e4e8..).
int    g_TotalTicks;                    // 0x10058438
#endif
// Variables that table binds, defined here beside it (zeroed .bss in the
// shipped image). Each address is the one its table entry points at; the
// console name is the entry's (the C++ names are ours).
char*          g_pBindAddress;           // 0x1005847c  "BindIP" (string)
char*          g_pLocalAddress;          // 0x10058480  "IP" (string)
// A short: the one reader, 0x1003760b, loads it `mov 0x10058488,%ax`.
unsigned short g_JoinPort;               // 0x10058488  "IPClientPort"
int            g_ShowNetInfo;            // 0x10058494  "IPDebug"
int            g_LocalNoFlag;            // 0x100584c4  "ForceRemote"
// The value pointer of the binding at 0x10052034 (name 0x100527dc); its one
// reader is servermessage.cpp's str_FormatMessage.
int            g_CV_DebugStrings;        // 0x10058500  "DebugStrings"
// 0x10051d28 — the float behind the "DebugMaxDims" binding (entry 0x100523a4
// of the table above: name 0x1005253c, value pointer 0x10051d28), 3000.0f
// (0x453b8000) in the shipped .data. The bound variables sit in one run
// directly ahead of the table (0x10051d14..0x10051d4c: LODScale 0x10051d1c,
// the update rate 0x10051d20, TimeScale 0x10051d24, this), so they belong to
// the table's file. SPhysicsLT::SetObjectDims (commonphysics.cpp) is the
// only reader (name ours; commonphysics.h declares it). lithtech.exe's twin
// (0x004a9f90, bound at 0x004aa60c) holds the same 3000.0f.
float g_DebugMaxDims = 3000.0f;

// lithtech.exe compiles this TU too, and its copy lays the table and every
// variable around it out exactly as server.dll's does: the initialised run
// 0x004a9eb8.. mirrors 0x10051c50.., the table 0x004a9fb8.. mirrors
// 0x10051d50.. (the same 93 entries in the same order), and the zeroed run
// 0x004b3234.. mirrors 0x1005845c.., offset for offset, every shipped
// initial value the same.  So the definitions are shared; a variable's
// lithtech address is its server one moved by 0x004a9eb8 - 0x10051c50 in the
// initialised run and by 0x004b3238 - 0x10058460 in the zeroed one.
#ifndef LT_SERVER_BUILD
// 0x004a9efc -- the one dword of the initialised run no binding points at
// (server.dll's 0x10051c94 is likewise unbound and not defined here);
// shipped 1, the run-server flag clientshell.cpp reads.
int   g_nUnknown4a9efc         = 1;     // 0x004a9efc
#endif

// The rest of what the table binds. The initialised ones are the .data run
// ahead of the table, with the shipped words; the zeroed ones are .bss. Each
// address is the one its table entry points at, the console name is the
// entry's, and the C names are ours except where another file already
// declares the variable (those are extern below and defined where their
// owners put them).
int   g_CV_RenderEnable        = 1;     // 0x10051c50  "RenderEnable"
int   g_CV_SoundEndCap         = 1;     // 0x10051c54  "SoundEndCap"
int   g_CV_SoundsPerBuffer     = 32;    // 0x10051c58  "SoundsPerBuffer"
int   g_CV_CursorCenter        = 1;     // 0x10051c5c  "CursorCenter"
int   g_CV_UpdateRate          = 10;    // 0x10051c70  "UpdateRate"
int   g_CV_ConnTroubleCount3   = 50;    // 0x10051c7c  "ConnTroubleCount3"
int   g_CV_MasterPaletteMode   = 2;     // 0x10051c84  "MasterPaletteMode"
int   g_CV_ModelTransitionMS   = 200;   // 0x10051c88  "ModelTransitionMS"
float g_CV_MaxExtrapolateTime  = 0.5f;  // 0x10051c90  "MaxExtrapolateTime"
float g_CV_FarZ                = 10000.0f; // 0x10051c98  "FarZ"
int   g_CV_BitDepth            = 16;    // 0x10051c9c  "BitDepth"
int   g_CV_ScreenWidth         = 640;   // 0x10051ca0  "ScreenWidth"
int   g_CV_ScreenHeight        = 480;   // 0x10051ca4  "ScreenHeight"
int   g_CV_NumConsoleLines     = 6;     // 0x10051ca8  "NumConsoleLines"
int   g_CV_ConsoleEnable       = 1;     // 0x10051cac  "ConsoleEnable"
int   g_CV_MusicEnable         = 1;     // 0x10051d14  "MusicEnable"
int   g_CV_SoundEnable         = 1;     // 0x10051d18  "SoundEnable"
float g_CV_LODScale            = 1.0f;  // 0x10051d1c  "LODScale"
int   g_CV_ConsoleHistoryLen   = 20;    // 0x10051d2c  "ConsoleHistoryLen"
int   g_CV_ConsoleBufferLen    = 500;   // 0x10051d30  "ConsoleBufferLen"
float g_CV_ConsoleAlpha        = 1.0f;  // 0x10051d34  "ConsoleAlpha"
int   g_CV_ConsoleLeft         = -1;    // 0x10051d38  "ConsoleLeft"
int   g_CV_ConsoleTop          = -1;    // 0x10051d3c  "ConsoleTop"
int   g_CV_ConsoleRight        = -1;    // 0x10051d40  "ConsoleRight"
int   g_CV_ConsoleBottom       = -2;    // 0x10051d44  "ConsoleBottom"
int   g_CV_LTDMConsoleOutput   = 2;     // 0x10051d48  "LTDMConsoleOutput"

int   g_CV_PlayDemoReps;                // 0x10058460  "PlayDemoReps"
int   g_CV_PlayDemoProfile;             // 0x10058464  "PlayDemoProfile"
int   g_CV_ForceNoSound;                // 0x10058468  "ForceNoSound"
int   g_CV_VideoDebug;                  // 0x1005846c  "VideoDebug"
int   g_CV_DebugLoaders;                // 0x10058470  "DebugLoaders"
int   g_CV_MeasurePackets;              // 0x10058478  "MeasurePackets"
int   g_CV_ForceConsole;                // 0x1005848c  "ForceConsole"
int   g_CV_IPDevice;                    // 0x10058490  "IPDevice"
float g_CV_MaxFPS;                      // 0x10058498  "MaxFPS"
int   g_CV_InputDebug;                  // 0x1005849c  "InputDebug"
int   g_CV_JoystickDisable;             // 0x100584a0  "JoystickDisable"
int   g_CV_TraceConsole;                // 0x100584a4  "TraceConsole"
// 0x100584a8 — "ShowFileAccess": how much the stream layer says about itself
// (fsstream.cpp: 1 a line per open, 2 per close, 4 per read; name ours).
int   g_StreamDebugLevel;
int   g_CV_FullLightScale;              // 0x100584b0  "FullLightScale"
int   g_CV_ForceClear;                  // 0x100584b4  "ForceClear"
int   g_CV_InputRate;                   // 0x100584c0  "InputRate"
int   g_CV_ShowRunningTime;             // 0x100584c8  "ShowRunningTime"
int   g_CV_NullRender;                  // 0x100584cc  "NullRender"
int   g_CV_HighPriority;                // 0x100584d0  "HighPriority"
int   g_CV_ShowPing;                    // 0x100584d8  "ShowPing"
int   g_CV_Prediction;                  // 0x100584e4  "Prediction"
int   g_CV_PredictionLines;             // 0x100584e8  "PredictionLines"
int   g_CV_ClientSleepMS;               // 0x100584f4  "ClientSleepMS"
int   g_CV_ShowTickCounts;              // 0x100584f8  "ShowTickCounts"
int   g_CV_ShowMemStats;                // 0x100584fc  "ShowMemStats"
int   g_CV_DebugPackets;                // 0x10058504  "DebugPackets"
LTCommandVar* g_CV_Name;                // 0x10058508  "Name"
int   g_CV_DebugNumSounds;              // 0x1005850c  "DebugNumSounds"
int   g_CV_DebugSound;                  // 0x10058510  "DebugSound"
int   g_CV_ErrorLog;                    // 0x10058514  "ErrorLog"
int   g_CV_AlwaysFlushLog;              // 0x10058518  "AlwaysFlushLog"
int   g_CV_Software;                    // 0x1005851c  "Software"
int   g_CV_ShowFrameRate;               // 0x10058520  "ShowFrameRate"
int   g_CV_DoExtraObjectStuff;          // 0x10058524  "DoExtraObjectStuff"
int   g_CV_ForceSoundDisable;           // 0x10058528  "ForceSoundDisable"

// Bound here, owned elsewhere: declared by the files that read them.
extern int    g_CV_DebugStrings;        // 0x10058500  "DebugStrings" (servermessage.cpp)
extern float  g_CV_DebugMaxPos;         // 0x10058530  "DebugMaxPos" (spritecontrol.cpp)
// 0x10052ee0 — "DefaultDrawIndexedDist", 500.0f. It sits in the model
// unit's .data (right after commonmodel.h's g_pEmptyFilename, 0x10052edc),
// so commonmodel.cpp defines it.
extern float  g_CV_DefaultDrawIndexedDist;

// 0x10051d50 — the table itself: { name, float*, int*, LTCommandVar**,
// char** }, one pointer set per entry, in the shipped order. Two of the
// bound variables are narrower or unsigned in the files that read them
// (IPClientPort's short, the uint32 trace flags), so their int* is a cast.
ConsoleBinding g_EngineConsoleBindings[93] =
{
	{ "BindIP",                  LTNULL, LTNULL, LTNULL, &g_pBindAddress },
	{ "IP",                      LTNULL, LTNULL, LTNULL, &g_pLocalAddress },
	{ "FlashClients",            LTNULL, &g_Unknown5845c, LTNULL, LTNULL },
	{ "RenderEnable",            LTNULL, &g_CV_RenderEnable, LTNULL, LTNULL },
	{ "PlayDemoReps",            LTNULL, &g_CV_PlayDemoReps, LTNULL, LTNULL },
	{ "PlayDemoProfile",         LTNULL, &g_CV_PlayDemoProfile, LTNULL, LTNULL },
	{ "ForceNoSound",            LTNULL, &g_CV_ForceNoSound, LTNULL, LTNULL },
	{ "SoundEndCap",             LTNULL, &g_CV_SoundEndCap, LTNULL, LTNULL },
	{ "SoundsPerBuffer",         LTNULL, &g_CV_SoundsPerBuffer, LTNULL, LTNULL },
	{ "CursorCenter",            LTNULL, &g_CV_CursorCenter, LTNULL, LTNULL },
	{ "CacheFiles",              LTNULL, &g_bCachingEnabled, LTNULL, LTNULL },
	{ "VideoDebug",              LTNULL, &g_CV_VideoDebug, LTNULL, LTNULL },
	{ "DebugLoaders",            LTNULL, &g_CV_DebugLoaders, LTNULL, LTNULL },
	{ "MeasurePackets",          LTNULL, &g_CV_MeasurePackets, LTNULL, LTNULL },
	{ "STracePackets",           LTNULL, (int*)&g_bPacketTrace58474, LTNULL, LTNULL },
	{ "DelimitPackets",          LTNULL, (int*)&g_bTraceMarker51c64, LTNULL, LTNULL },
	{ "SendBandwidth",           LTNULL, &g_NetBandwidth, LTNULL, LTNULL },
	{ "ForceConsole",            LTNULL, &g_CV_ForceConsole, LTNULL, LTNULL },
	{ "IPDevice",                LTNULL, &g_CV_IPDevice, LTNULL, LTNULL },
	{ "IPDebug",                 LTNULL, &g_ShowNetInfo, LTNULL, LTNULL },
	{ "IPClientPort",            LTNULL, (int*)&g_JoinPort, LTNULL, LTNULL },
	{ "InputDebug",              LTNULL, &g_CV_InputDebug, LTNULL, LTNULL },
	{ "UpdateRate",              LTNULL, &g_CV_UpdateRate, LTNULL, LTNULL },
	{ "InputRate",               LTNULL, &g_CV_InputRate, LTNULL, LTNULL },
	{ "ForceRemote",             LTNULL, &g_LocalNoFlag, LTNULL, LTNULL },
	{ "DebugLevel",              LTNULL, &g_DebugLevel, LTNULL, LTNULL },
	{ "ShowRunningTime",         LTNULL, &g_CV_ShowRunningTime, LTNULL, LTNULL },
	{ "NullRender",              LTNULL, &g_CV_NullRender, LTNULL, LTNULL },
	{ "LocalDebug",              LTNULL, &g_NetDebugAll, LTNULL, LTNULL },
	{ "TransportDebug",          LTNULL, &g_NetDebugLevel, LTNULL, LTNULL },
	{ "ParseNet_Incoming",       LTNULL, &g_NetLogPackets, LTNULL, LTNULL },
	{ "NetMaxQueue",             LTNULL, &g_NetMaxQueued, LTNULL, LTNULL },
	{ "ClientSleepMS",           LTNULL, &g_CV_ClientSleepMS, LTNULL, LTNULL },
	{ "ShowTickCounts",          LTNULL, &g_CV_ShowTickCounts, LTNULL, LTNULL },
	{ "ShowMemStats",            LTNULL, &g_CV_ShowMemStats, LTNULL, LTNULL },
	{ "Prediction",              LTNULL, &g_CV_Prediction, LTNULL, LTNULL },
	{ "PredictionLines",         LTNULL, &g_CV_PredictionLines, LTNULL, LTNULL },
	{ "DebugStrings",            LTNULL, &g_CV_DebugStrings, LTNULL, LTNULL },
	{ "DebugPackets",            LTNULL, &g_CV_DebugPackets, LTNULL, LTNULL },
	{ "DoExtraObjectStuff",      LTNULL, &g_CV_DoExtraObjectStuff, LTNULL, LTNULL },
	{ "DebugNumSounds",          LTNULL, &g_CV_DebugNumSounds, LTNULL, LTNULL },
	{ "DebugSound",              LTNULL, &g_CV_DebugSound, LTNULL, LTNULL },
	{ "ErrorLog",                LTNULL, &g_CV_ErrorLog, LTNULL, LTNULL },
	{ "AlwaysFlushLog",          LTNULL, &g_CV_AlwaysFlushLog, LTNULL, LTNULL },
	{ "ShowFrameRate",           LTNULL, &g_CV_ShowFrameRate, LTNULL, LTNULL },
	{ "NumConsoleLines",         LTNULL, &g_CV_NumConsoleLines, LTNULL, LTNULL },
	{ "ConsoleEnable",           LTNULL, &g_CV_ConsoleEnable, LTNULL, LTNULL },
	{ "ScreenWidth",             LTNULL, &g_CV_ScreenWidth, LTNULL, LTNULL },
	{ "ScreenHeight",            LTNULL, &g_CV_ScreenHeight, LTNULL, LTNULL },
	{ "Software",                LTNULL, &g_CV_Software, LTNULL, LTNULL },
	{ "BitDepth",                LTNULL, &g_CV_BitDepth, LTNULL, LTNULL },
	{ "MusicEnable",             LTNULL, &g_CV_MusicEnable, LTNULL, LTNULL },
	{ "SoundEnable",             LTNULL, &g_CV_SoundEnable, LTNULL, LTNULL },
	{ "ForceSoundDisable",       LTNULL, &g_CV_ForceSoundDisable, LTNULL, LTNULL },
	{ "AllowTimeout",            LTNULL, &g_NetTimeouts, LTNULL, LTNULL },
	{ "ShowSphereFindTicks",     LTNULL, &g_ShowTickCounts, LTNULL, LTNULL },
	{ "ShowClassTicks",          LTNULL, &g_ShowClassTicks, LTNULL, LTNULL },
	{ "ShowGameTime",            LTNULL, &g_ShowGameTime, LTNULL, LTNULL },
	{ "AutoDeactivate",          LTNULL, &g_bAutoDeactivate, LTNULL, LTNULL },
	{ "JoystickDisable",         LTNULL, &g_CV_JoystickDisable, LTNULL, LTNULL },
	{ "ConnTroubleCount",        LTNULL, (int*)&g_MaxQueueSize, LTNULL, LTNULL },
	{ "ConnTroubleCount2",       LTNULL, &g_NetConnTroubleCount2, LTNULL, LTNULL },
	{ "ConnTroubleCount3",       LTNULL, &g_CV_ConnTroubleCount3, LTNULL, LTNULL },
	{ "TraceConsole",            LTNULL, &g_CV_TraceConsole, LTNULL, LTNULL },
	{ "ShowFileAccess",          LTNULL, &g_StreamDebugLevel, LTNULL, LTNULL },
	{ "FullLightScale",          LTNULL, &g_CV_FullLightScale, LTNULL, LTNULL },
	{ "ForceClear",              LTNULL, &g_CV_ForceClear, LTNULL, LTNULL },
	{ "ModelTransitionMS",       LTNULL, &g_CV_ModelTransitionMS, LTNULL, LTNULL },
	{ "HighPriority",            LTNULL, &g_CV_HighPriority, LTNULL, LTNULL },
	{ "ShowConnStats",           LTNULL, &g_NetShowStats, LTNULL, LTNULL },
	{ "ShowPing",                LTNULL, &g_CV_ShowPing, LTNULL, LTNULL },
	{ "ShowThruput",             LTNULL, &g_NetShowTimeouts, LTNULL, LTNULL },
	{ "MasterPaletteMode",       LTNULL, &g_CV_MasterPaletteMode, LTNULL, LTNULL },
	{ "LTDMConsoleOutput",       LTNULL, &g_CV_LTDMConsoleOutput, LTNULL, LTNULL },
	{ "Name",                    LTNULL, LTNULL, &g_CV_Name, LTNULL },
	{ "DefaultDrawIndexedDist",  &g_CV_DefaultDrawIndexedDist, LTNULL, LTNULL, LTNULL },
	{ "MaxFPS",                  &g_CV_MaxFPS, LTNULL, LTNULL, LTNULL },
	{ "LatencySim",              &g_NetResendTime, LTNULL, LTNULL, LTNULL },
	{ "DropRate",                &g_NetDropRate, LTNULL, LTNULL, LTNULL },
	{ "FarZ",                    &g_CV_FarZ, LTNULL, LTNULL, LTNULL },
	{ "LODScale",                &g_CV_LODScale, LTNULL, LTNULL, LTNULL },
	{ "DebugMaxDims",            &g_DebugMaxDims, LTNULL, LTNULL, LTNULL },
	{ "DebugMaxPos",             &g_CV_DebugMaxPos, LTNULL, LTNULL, LTNULL },
	{ "TimeScale",               &g_TimeScale, LTNULL, LTNULL, LTNULL },
	{ "AckTimeout",              &g_AckTimeout, LTNULL, LTNULL, LTNULL },
	{ "MaxExtrapolateTime",      &g_CV_MaxExtrapolateTime, LTNULL, LTNULL, LTNULL },
	{ "ConsoleHistoryLen",       LTNULL, &g_CV_ConsoleHistoryLen, LTNULL, LTNULL },
	{ "ConsoleBufferLen",        LTNULL, &g_CV_ConsoleBufferLen, LTNULL, LTNULL },
	{ "ConsoleAlpha",            &g_CV_ConsoleAlpha, LTNULL, LTNULL, LTNULL },
	{ "ConsoleLeft",             LTNULL, &g_CV_ConsoleLeft, LTNULL, LTNULL },
	{ "ConsoleTop",              LTNULL, &g_CV_ConsoleTop, LTNULL, LTNULL },
	{ "ConsoleRight",            LTNULL, &g_CV_ConsoleRight, LTNULL, LTNULL },
	{ "ConsoleBottom",           LTNULL, &g_CV_ConsoleBottom, LTNULL, LTNULL },
};



// 0x10058370 and the four dwords around it — the module's own bookkeeping.
// Only the list head and the saved handler are read by anything
// reconstructed here; the rest are written and never looked at in this file.
// Four of them are dalloc.cpp's allocator books -- its flush-hook chain and
// its outstanding / lifetime counts -- so they are defined here under that
// file's names and declared `extern` there; CFlushHook is that file's class
// (names ours, from the offsets).
class CFlushHook
{
public:
	void        (*m_pFn)(void* pUser);       // +0x00
	void*       m_pUser;                     // +0x04
	CFlushHook* m_pNext;                     // +0x08
};

static uint32      g_nMemRefs;               // 0x1005837c
CFlushHook*        g_pFlushHooks;            // 0x10058370
static int       (*g_pOldNewHandler)(size_t);  // 0x10058368
static uint32      g_nUnknown64;             // 0x10058364
uint32             g_nTotalFrees;            // 0x1005836c
uint32             g_nTotalAllocs;           // 0x10058374
uint32             g_nOutstanding;           // 0x10058378

// What an allocation failure is reported through. server.dll's handler
// calls 0x1000e443 (`call 0x1000e443` at 0x1000d2a9), which is dalloc.cpp's
// dsi_OutOfMemory -- the app's handler, the message box, exit. lithtech.exe's
// is clientdsi.cpp's dsi_OnOutOfMemory (0x0042e560).
#ifdef LT_SERVER_BUILD
void dsi_OutOfMemory();
#else
void dsi_OnOutOfMemory();
#endif


#ifndef LT_SERVER_BUILD
// lithtech.exe 0x00428ed0 — give the process heap's free blocks back. The
// client's compile of this TU opens with it, directly before mem_Init
// (0x00428ef0); its one caller is the "HeapCompact" console command
// (ccclient.cpp's cc_HeapCompact, a tail `jmp` here). server.dll imports
// neither API, so it is client-only (name ours).
extern "C" {
__declspec(dllimport) void* __stdcall GetProcessHeap();
__declspec(dllimport) unsigned int __stdcall HeapCompact(void* hHeap, unsigned long dwFlags);
}

void dsi_HeapCompact()
{
	void* hHeap = GetProcessHeap();

	if (hHeap)
		HeapCompact(hHeap, 0);
}
#endif


// 0x1000d2a9 — the handler the module installs with the CRT for the duration.
// It reports and gives up: answering zero tells operator new not to retry, so
// the allocation fails anyway and this only exists to say so first. The size
// it is handed is not looked at.
static int mem_NewHandler(size_t nSize)
{
#ifdef LT_SERVER_BUILD
	dsi_OutOfMemory();
#else
	dsi_OnOutOfMemory();
#endif

	return 0;
}


// 0x1000d268 and 0x1000d2b1 — reference-counted either side. Init installs
// the handler and keeps whatever was there; Term puts that back, which is why
// the pair uses _set_new_handler's return value rather than a second call.
//
// The asymmetry is the shipped code's, not a transcription slip: Init clears
// five of the module's dwords and Term clears two of them, leaving the list
// head and two more holding whatever they last held. Nothing reconstructed so
// far reads them across a Term, so what that costs is not visible from here.
void mem_Init()
{
	if (g_nMemRefs == 0)
	{
		g_pFlushHooks = LTNULL;

		g_pOldNewHandler = _set_new_handler(mem_NewHandler);

		g_nUnknown64 = 0;
		g_nOutstanding = 0;
		g_nTotalFrees = 0;
		g_nTotalAllocs = 0;
	}

	g_nMemRefs++;
}

void mem_Term()
{
	if (--g_nMemRefs == 0)
	{
		CFlushHook* pBlock = g_pFlushHooks;

		while (pBlock)
		{
			CFlushHook* pNext = pBlock->m_pNext;

			free(pBlock);

			pBlock = pNext;
		}

		_set_new_handler(g_pOldNewHandler);

		g_nUnknown64 = 0;
		g_nOutstanding = 0;
	}
}


// 0x10058430 — the module handle, and 0x1000dff4 / 0x1000e043 either side of
// it. The load answers a plain int rather than the handle, which is why the
// test-and-setne survives instead of the handle being returned directly.
// Not static: servermsg.cpp's dsi_SetupMessage (0x1000deeb) formats its
// messages out of this module.
HMODULE g_hMsgModule;

#ifndef LT_SERVER_BUILD
// lithtech.exe's CServerMgr::UpdateObjects tick sum (seven references in
// 0x00473bc0, all `ds:0x4b3210`), in this file's .bss run beside the module
// handle (0x004b3204) as server.dll's is (0x10058438, defined above).
int    g_TotalTicks;                    // 0x004b3210
#endif

int dsi_InitMessageModule()
{
	g_hMsgModule = LoadLibraryA("ltmsg.dll");

	return g_hMsgModule != LTNULL;
}

void dsi_FreeMessageModule()
{
	if (g_hMsgModule)
	{
		FreeLibrary(g_hMsgModule);

		g_hMsgModule = LTNULL;
	}
}


// 0x1000d402 and 0x1000d423 — reference-counted either side, and only the
// first caller in and the last caller out touch the bank. Thirty-two byte
// records with a thirty-two deep cache, which is the same shape the message
// bank uses.
void memtrack_Init()
{
	if (g_nMemTrackRefs == 0)
		sb_Init(&g_MemTrackBank, 0x20, 0x20);

	g_nMemTrackRefs++;
}

void memtrack_Term()
{
	if (--g_nMemTrackRefs == 0)
		sb_Term(&g_MemTrackBank);
}


// 0x1000e4fd and 0x1000e503 — the console asks the system layer for the
// engine's own bindings at startup. The count is a literal rather than
// anything derived from the table, so the two have to be kept in step by
// hand; VC6 writes the small constant as a push/pop pair, which is a byte
// shorter than moving it.
ConsoleBinding* dsi_GetConsoleBindings()
{
	return g_EngineConsoleBindings;
}

int dsi_GetNumConsoleBindings()
{
	return 93;
}


// 0x1000d926 — format and hand the line to the debugger. The buffer is ten
// thousand bytes on the stack, which is why the function opens with a stack
// probe.
void con_Printf(char* pMsg, ...)
{
	char    msg[10000];
	va_list marker;

	va_start(marker, pMsg);
	_vsnprintf(msg, sizeof(msg) - 1, pMsg, marker);

	OutputDebugStringA(msg);
}


// 0x1000e3f9 — the engine's own error and trace printer. With no host
// application attached there is nothing to print to, so the formatting is
// skipped too. server.dll only: lithtech.exe's dsi_ServerError is the
// client's (clientdsi.cpp, 0x0042e6f0), and this body is not in that image.
#ifdef LT_SERVER_BUILD
void dsi_ServerError(char* pFormat, ...)
{
	char    msg[1000];
	va_list marker;

	if (!g_pServerMgr)
		return;

	if (!g_pServerMgr->m_pAppHandler)
		return;

	va_start(marker, pFormat);
	vsprintf(msg, pFormat, marker);

	g_pServerMgr->m_pAppHandler->ConsoleOutputFn(msg);
}
#endif


// ---- the system interface's own startup ---------------------------------- //

// 0x1005842c — whether this module was the one that started COM
// (servermgr.h). First of this unit's .bss after dhash.cpp's tables; zeroed.
int g_bComInitialized;

// 0x1000dfb0 — bring the platform layer up. COM comes first because the
// pieces below it use it; if it will not start there is nothing to undo. The
// message module is last and is the only thing here that can fail, and a
// failure unwinds everything through the ordinary teardown.
//
// Nonzero means it did not come up.
int dsi_Init()
{
	if (CoInitialize(LTNULL) >= 0)
	{
		g_bComInitialized = 1;

		mem_Init();
		stream_Init();
		fs_InitModule();
		memtrack_Init();
		netbank_Init();

		if (dsi_LoadMessageModule())
			return 0;

		dsi_Term();
	}

	return 1;
}

// 0x1000e00e — and back down again, in the reverse order. COM is only
// uninitialized if this was what started it.
void dsi_Term()
{
	netbank_Term();
	memtrack_Term();
	fs_TermModule();
	stream_Term();
	mem_Term();
	dsi_FreeMessageModule();

	if (g_bComInitialized)
	{
		CoUninitialize();

		g_bComInitialized = 0;
	}
}


// ======================================================================= //
//  0x1000e480, 65 bytes -- case-folding string equality.
//
//  NOT a CRT routine: it answers 1/0 rather than a difference, folds a-z
//  inline with no locale table, and sits below the 0x1003d000 CRT boundary
//  (stricmp is 0x10047080). svlast.cpp:155 already names it from the other
//  side; its sole caller 0x10009338 is a case-insensitive find-by-name.
//
//  Two spellings are settled. `sub $0x20` on a BYTE rather
//  than `add $0xe0` means the fold is an if/else writing a DIFFERENT
//  variable, not an in-place adjust. And the first fold copies in both arms
//  only because the raw `*pStr1` must survive for the `test %dl,%dl` NUL
//  check, where the second fold's raw char is dead and so coalesces -- that
//  asymmetry is allocation, so both folds are written identically.
// ======================================================================= //

LTBOOL dsi_StringsEqualUpperQ6L(char* pStr1, char* pStr2)
{
	char c1, c2;
	char cUpper1, cUpper2;

	while (1)
	{
		c1 = *pStr1;                                 // 0x1000e48a mov (%esi),%dl
		if (c1 >= 'a' && c1 <= 'z')                  // 0x1000e48c/0x1000e491
			cUpper1 = (char)(c1 - 0x20);             // 0x1000e496 mov ; 0x1000e498 sub
		else
			cUpper1 = c1;                            // 0x1000e49d mov %dl,%cl

		c2 = *pStr2;                                 // 0x1000e49f mov (%edi),%al
		if (c2 >= 'a' && c2 <= 'z')                  // 0x1000e4a1/0x1000e4a5
			cUpper2 = (char)(c2 - 0x20);             // 0x1000e4a9 sub $0x20,%al
		else
			cUpper2 = c2;

		if (cUpper1 != cUpper2)                      // 0x1000e4ab cmp %al,%cl
			return LTFALSE;                          // 0x1000e4b7 xor %eax,%eax

		if (c1 == 0)                                 // 0x1000e4af test %dl,%dl
			return LTTRUE;                           // 0x1000e4bb push $1 ; pop %eax

		pStr1++;                                     // 0x1000e4b3 inc %esi
		pStr2++;                                     // 0x1000e4b4 inc %edi
	}
}


#ifdef LT_SERVER_BUILD
// 0x10036d21 — the error hook every LT_ERROR-style return goes through
// (de_error.h). Empty in release: its shipped body is the one bare `ret`
// that /OPT:ICF folds all the engine's empty functions onto. lithtech.exe
// has its own (0x0042d760), so this is the server build's alone.
extern "C" void dsi_OnReturnError(int code)
{
}
#endif
