// *********************************************************************** //
//
//  MODULE:   CLIENTCONSOLE2 (.CPP) -- lithtech.exe 0x0041da40..0x0041e915
//
//  PURPOSE:  The CLIENT console's command handlers, second half, plus the
//            built-in command table and the console-state initialiser that
//            registers it.  (reconstruction; build cl 8447 /O2,
//            /I ..\..\LT2\sdk\inc /I ..\server /I .)
//
//  WHAT THIS IS AND HOW IT WAS NAMED.  Nothing here had to be guessed at:
//  0x0041e890 plants a 33-entry {name, fn, user} table at 0x004a8830 into
//  the client's ConsoleState at 0x004b2f00, and reading that table names
//  every handler in the range outright --
//
//     0x0041da40 TimeDemo    0x0041dba0 Bind         0x0041dc20 RangeBind
//     0x0041dce0 Scale       0x0041dd50 RangeScale   0x0041deb0 RestartRender
//     0x0041df20 ResizeScreen 0x0041dfd0 serv        0x0041e0b0 EnvMap
//     0x0041e160 ShowTicks   0x0041e3d0 WriteHistory 0x0041e4b0 ReadHistory
//     0x0041e590 Exec        0x0041e5f0 MoveConsole
//
//  -- exactly the fourteen command handlers in this range, plus the
//  initialiser itself as the fifteenth.
//
//  WHAT WAS REUSED.  This is the direct continuation of
//  src/engine/lithtech/clientconsole.cpp, which already carries the FIRST
//  half of the same translation unit (cc_World, cc_EnableDevice,
//  cc_Screenshot, cc_ListHistory, cc_ClearHistory, cc_ListCommands,
//  cc_ShowGameVars, cc_SaveModelVars and the console/history classes).  The
//  string-literal order in .data proves the two halves are one source file
//  and that its function order is its address order: "renderDLL %s"
//  (0x004a8c54, RestartRender) < "Setting screen to %dx%d" (0x004a8c64,
//  ResizeScreen) < "\"%s\" " (0x004a8c7c, serv) < "Couldn't find texture %s."
//  (0x004a8c84, EnvMap) ...  Those already-written functions are NOT
//  redefined here -- they are declared extern so the command table can name
//  them.  src/engine/lithtech/clientmgr.h supplied CClientMgr's +0x1508
//  input table and +0x150c connection, src/engine/server/servermgr.h the
//  ConsoleState / ConsoleCommandDef layout and con_RunFile / con_Init, and
//  src/LT2/sdk/inc/ltbasedefs.h StartGameRequest -- whose NOLF2-era layout is
//  byte-for-byte what 0x0041da40 builds (see cc_TimeDemo).
//
// *********************************************************************** //

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "servermgr.h"          // ConsoleState, ConsoleCommandDef, con_RunFile
                                // -> server_interface.h -> ltbasedefs.h, which
                                //    is where StartGameRequest comes from


// ======================================================================= //
//  THE ONE DECISION THAT RUNS THROUGH THE WHOLE INPUT FAMILY
// ======================================================================= //
//
//  Five handlers here (Bind, RangeBind, Scale, RangeScale, and AddAction
//  next door at 0x0041dde0) call one entry of the input table and hand that
//  same table back as the call's first argument.  The shipped code loads
//  `g_pClientMgr->m_pInput` TWICE wherever an atof() sits between the two
//  uses (Scale at 0x0041dcf9 and 0x0041dd18; RangeBind at 0x0041dc72 and
//  0x0041dc9c; RangeScale at 0x0041dd67 and 0x0041dda6) and ONCE where
//  nothing intervenes (Bind at 0x0041dbb9 and 0x0041dbe0).
//
//  That is decisive.  A local `pInput` would survive the atof call in a
//  register and give ONE load everywhere; two textually identical loads with
//  a call between them is what an expression written out twice gives, because
//  VC6 will not carry a memory CSE across a call.  So every call site below
//  repeats `g_pClientMgr->m_pInput` rather than caching it -- and the one
//  place clientconsole.cpp already uses a local (cc_EnableDevice, which has
//  no intervening call) is consistent with either, so it is left alone.
//
//  If the family comes out near, THIS is the single edit to flip: introduce
//  `CConClientInput* pInput = g_pClientMgr->m_pInput;` in all five and use it.


// ----------------------------------------------------------------------- //
//  What this run calls but does not define
// ----------------------------------------------------------------------- //

// The input object at CClientMgr+0x1508 is a TABLE OF PLAIN FUNCTION
// POINTERS, not a class with a vtable: every call is `call *0xNN(%eax)` with
// no vtable load and the caller cleans the stack, so the entries are cdecl.
// Each one is handed the table itself as its own first argument.
// clientmgr.h already lays out +0x24 (EnableDevice) and +0x34 upwards; the
// four below are pinned by their argument counts, which are exactly the
// LithTech 2.x ILTClient input signatures:
//    +0x20  3 args  (name, code)                          -> AddAction
//    +0x28  3 args  (device, trigger)                     -> ClearBindings
//    +0x2c  6 args  (device, trigger, action, low, high)  -> AddBinding
//    +0x30  7 args  (device, trigger, scale, min, max, preCenter)
//                                                         -> ScaleTrigger
// (the class name is ours, to stay out of clientmgr.h's way)
class CConClientInput
{
public:
	uint8  m_pad_00[0x0c];
	// 0x0041e090's one call: handed the table, answer ignored.
	void   (*ListDevices)(CConClientInput* pInput);                     // 0x0c
	uint8  m_pad_10[0x20 - 0x10];
	LTBOOL (*AddAction)(CConClientInput* pInput, char* pActionName,
	                    int nActionCode);                                // 0x20
	LTBOOL (*EnableDevice)(CConClientInput* pInput, char* strDeviceName); // 0x24
	LTBOOL (*ClearBindings)(CConClientInput* pInput, char* pDeviceName,
	                        char* pTriggerName);                         // 0x28
	LTBOOL (*AddBinding)(CConClientInput* pInput, char* pDeviceName,
	                     char* pTriggerName, char* pActionName,
	                     float fRangeLow, float fRangeHigh);             // 0x2c
	LTBOOL (*ScaleTrigger)(CConClientInput* pInput, char* pDeviceName,
	                       char* pTriggerName, float fScale,
	                       float fRangeScaleMin, float fRangeScaleMax,
	                       float fRangeScalePreCenterOffset);            // 0x30
};

// The object at CClientMgr+0x150c -- clientmgr.h's CClientConnection, the
// link to the server.  The one method this run needs is 0x00413c60: a
// thiscall taking one char*, `ret $0x4`, which packets the string up and
// sends it (it ends in a 0x0a terminator and a message send).  Both callers
// hand it a console command line built with sprintf, so that is its name.
class CConConnection
{
public:
	void RunConsoleString(char* pCommand);                      // 0x00413c60
};

// Only the two fields of CClientMgr this run touches, at clientmgr.h's
// offsets, plus the one method.  StartGame is clientmgr.h's already.
class CConClientMgr
{
public:
	LTRESULT StartGame(StartGameRequest* pRequest);             // 0x0040fd90
	void     Sub4105c0();                                       // 0x004105c0

	uint8            m_pad_0000[0x1508];
	CConClientInput* m_pInput;               // 0x1508
	CConConnection*  m_pConnection;          // 0x150c
};

// The client's console state and the CClientMgr the console works through;
// clientconsole.cpp and clientmgr2.cpp already name both.  NOTE the manager
// pointer here is 0x004b2f44, which is NOT clientmgr.h's g_pClientMgr at
// 0x004b17e0 -- the console keeps its own.
extern ConsoleState   g_ClientConsoleState;                     // 0x004b2f00
extern CConClientMgr* g_pClientMgr;                             // 0x004b2f44

// The two console printers, both in the shared console TU.
// clientconsole.cpp names them: con_ConsolePrint is variadic cdecl with the
// format first; con_PrintColored takes a colour and a level before it, which
// is pinned by 0x0041d320 reading its second argument as the level.
void con_ConsolePrint(char* pFormat, ...);                      // 0x0042e6f0
void con_PrintColored(uint32 nColor, int nLevel, char* pFormat, ...); // 0x0041b8a0
void con_Print(char* pFormat, ...);                             // 0x0041b8c0

// 0x0041e930 -- run one already-formatted string as a console command:
// two instructions, `con_RunString(&g_ClientConsoleState, pStr)` at
// 0x0041abe0.  clientrender.cpp records that this body is byte-for-byte the
// renderer's +0x10 callback at 0x004622b0 and that /OPT:ICF did NOT fold
// them.  (name ours)
void cc_RunConsoleString(char* pCommand);                       // 0x0041e930

// 0x004290b0 / 0x004290c0 -- src/engine/lithtech/stdlithalloc.cpp names both.
void* dalloc(unsigned long nSize);
void  dfree(void* pMem);

// 0x0042f660 / 0x0042f670 -- the engine's console-binding table and its
// count, both one instruction (`mov $0x4a9fb8,%eax` / `mov $0x5d,%eax`).
// The records are 0x14 bytes with the name at +0x00, i.e. ConsoleBinding, so
// these are the client's dsi_GetConsoleBindings pair.  (clientconsole.cpp
// reaches the same two through the completion enumerator and calls them
// cn_GetExtraNames / cn_GetNumExtraNames; same functions.)
ConsoleBinding* dsi_GetConsoleBindings();                       // 0x0042f660
int             dsi_GetNumConsoleBindings();                    // 0x0042f670

// The renderer bracket the two restart commands drive.
// 0x004627d0 is clientrender.cpp's r_TermRender (clientmgr.h declares the
// address as sub_4627d0); 0x004116d0 wraps r_InitRender at 0x004623d0 with
// the current mode fetched by 0x0040f810, and 0x0041fc90 is the engine's
// error/shutdown funnel -- it dispatches on bits 0x02000000 and 0x04000000
// of its second argument, which is why both callers OR 0x04000000 into the
// LTRESULT before handing it over.  (names ours where marked)
void     sub_4627d0(CConClientMgr* pClientMgr, LTBOOL bReportFailure); // 0x004627d0
LTRESULT sub_4116d0(CConClientMgr* pClientMgr);                 // 0x004116d0
void     sub_41fc90(CConClientMgr* pClientMgr, uint32 nFlags);  // 0x0041fc90

// clientmgr.h's CClientFileRef verbatim: 0xc bytes, zeroed by an inline
// constructor -- which is exactly the dword/dword/word triple 0x0041e0bb
// plants before anything else happens.  (redeclared rather than included so
// this TU does not pull clientmgr.h in)
class CConClientFileRef
{
public:
	CConClientFileRef() { m_nFileType = 0; m_pFilename = LTNULL; m_nLocalID = 0; }

	uint32 m_nFileType;                      // 0x00
	char*  m_pFilename;                      // 0x04
	uint16 m_nLocalID;                       // 0x08
};

// 0x0041ff90 -- lttexmod.cpp: resolve one CClientFileRef to a SharedTexture,
// or null.  The SharedTexture type is not needed here, only the pointer.
void* sub_41ff90(CConClientMgr* pClientMgr, CConClientFileRef* pRef);

// The console's name-list hierarchy, all four classes already reconstructed
// in clientconsole.cpp -- redeclared here under our own names, with the
// SLOT ORDER preserved exactly, because this TU does not include that file.
// Slots: 0x00 First, 0x04 Last, 0x08 Next, 0x0c Prev, 0x10 ~dtor,
//        0x14 FindFirst, 0x18 FindLast, 0x1c FindNext, 0x20 FindPrev,
//        0x24 GetName.
class CConNameList
{
public:
	virtual LTBOOL First();                            // +0x00
	virtual LTBOOL Last();                             // +0x04
	virtual LTBOOL Next();                             // +0x08
	virtual LTBOOL Prev();                             // +0x0c
	virtual ~CConNameList();                           // +0x10
	virtual LTBOOL FindFirst(char* pPrefix);           // +0x14
	virtual LTBOOL FindLast(char* pPrefix);            // +0x18
	virtual LTBOOL FindNext(char* pPrefix);            // +0x1c
	virtual LTBOOL FindPrev(char* pPrefix);            // +0x20
	virtual char*  GetName();                          // +0x24
};

class CConHistory : public CConNameList
{
public:
	void Add(char* pText);                             // 0x0041c110
};

// A layout view of the console object at 0x004b2af8; only the one method
// this run calls is declared.  0x0041cba0 stores its argument at CConsole
// +0x3e8, falling back to the do-nothing list at 0x004b2af0 when handed
// null.  clientconsole.cpp calls it SetHistory; what 0x0041e890 actually
// hands it is the COMPLETION enumerator at 0x004b2ee8 (its dynamic
// initialiser 0x0041d610 is `mov $0x4b2ee8,%ecx ; jmp 0x0041e950`, and
// 0x0041e950 is the completion class's constructor), so the slot is the
// console's tab-completion name source and the name below says so.
//
// The history 0x004b2ec4 is NOT its own object: it is clientconsole.cpp's
// g_Console.m_History at CConsole+0x3cc (g_Console's constructor builds it),
// so it is reached as that field.
class CConWindow
{
public:
	void SetNameList(CConNameList* pList);             // 0x0041cba0

	uint8       m_pad_000[0x3cc];
	CConHistory m_History;                             // 0x3cc  0x004b2ec4
};

extern CConWindow   g_Console;                                  // 0x004b2af8
extern CConNameList g_ConsoleCompletion;                        // 0x004b2ee8


// ----------------------------------------------------------------------- //
//  File-scope state these commands read and write
// ----------------------------------------------------------------------- //

// The requested screen size.  0x004a9f08 / 0x004a9f0c ship holding 640 and
// 480, which is what says they are width and height and in that order; the
// "Setting screen to %dx%d" line prints them the same way round.
extern int g_CV_ScreenWidth;                                    // 0x004a9f08  "ScreenWidth" (dsi.cpp)
extern int g_CV_ScreenHeight;                                   // 0x004a9f0c  "ScreenHeight" (dsi.cpp)

// The ShowTicks section mask.  Bit 0 summary, 1 renderer, 2 game, 3 engine,
// 4 graph -- read straight off the XOR constants below.
extern int g_CV_ShowTickCounts;          // 0x004b32d0, server/dsi.cpp "ShowTickCounts"

// The console rectangle, as MoveConsole prints and rewrites it.  The four
// ship as -1,-1,-1,-2, which is this file's own "Top" preset -- that is what
// fixes the order left, top, right, bottom.
extern int g_CV_ConsoleLeft;                                       // 0x004a9fa0
extern int g_CV_ConsoleTop;                                        // 0x004a9fa4
extern int g_CV_ConsoleRight;                                      // 0x004a9fa8
extern int g_CV_ConsoleBottom;                                     // 0x004a9fac

// The renderer's environment map is not a global of its own: clientrender.cpp
// places RenderStruct at 0x004b4358 and its environment-map SharedTexture* at
// +0x104, i.e. exactly 0x004b445c -- CConRenderStruct below carries it.


// ======================================================================= //
//  The command handlers
// ======================================================================= //
//
//  Every one is cdecl (int argc, char** argv) and argv does NOT carry the
//  command name -- `Bind mouse ##x turn` reaches cc_Bind with argc 3 and
//  argv[0] "mouse", the same convention clientconsole.cpp already recorded
//  for cc_World and src/engine/server/console.cpp's handlers.


// 0x0041da40 -- "TimeDemo <record filename (w/o extension)>": start a server
// if there is not one, then send the server a `timedemo <name>` command.
//
// THE FRAME IS THE WHOLE STORY.  `mov $0x12f8,%eax ; call __chkstk` gives
// 4,856 bytes of locals and the inline memset at 0x0041dac4 covers 0x47e
// dwords == 0x11f8 == 4,600 of them starting 0x100 in.  4,600 is
// sizeof(StartGameRequest) in src/LT2/sdk/inc/ltbasedefs.h EXACTLY:
// 4 + 4*100 + 4 (m_pNetSession) + 4172 (NetHost, 12 + 4096 + 64) + 5*4.
// And the eleven stores before the argc test are that class's inline
// constructor, in ITS order and at ITS offsets --
//
//   +0x000 = 4      m_Type = STARTGAME_NORMAL   (0x0041da57)
//   +0x004 = 0 byte m_WorldName[0]              (0x0041da62)
//   +0x068 = 0 byte m_TCPAddress[0]             (0x0041da69)
//   +0x0cc = 0 byte m_RecordFilename[0]         (0x0041da70)
//   +0x130 = 0 byte m_PlaybackFilename[0]       (0x0041da77)
//   +0x11e4 = 0     m_flags                     (0x0041da7e)
//   +0x11e8 = 0     m_pGameInfo                 (0x0041da85)
//   +0x11ec = 0     m_GameInfoLen               (0x0041da8c)
//   +0x194  = 0     m_pNetSession               (0x0041da93)  <- out of
//   +0x11f0 = 0     m_pClientData               (0x0041da9a)     ADDRESS
//   +0x11f4 = 0     m_ClientDataLen             (0x0041daa1)     order, in
//                                                                SOURCE order
//
// so the SDK header is the shipped header here, unmodified.  The remaining
// 256 bytes of frame are the command buffer, and the request takes the HIGH
// end, which is why it is declared first.
//
// The usage line is the ELSE arm, not a guard: the shipped code falls
// through into the body and jumps BACK past everything to the message at
// 0x0041db44, and the three `pop %ebx ; add $0x12f8,%esp ; ret` epilogues
// say the three exits are three separate statements.
void cc_TimeDemo(int argc, char** argv)
{
	StartGameRequest req;
	char             cmd[256];

	if (argc >= 1)
	{
		// The connection is what says a server is already running; starting
		// one and still not having it is the failure.
		if (!g_pClientMgr->m_pConnection)
		{
			memset(&req, 0, sizeof(req));
			req.m_Type = STARTGAME_NORMAL;

			g_pClientMgr->StartGame(&req);

			if (!g_pClientMgr->m_pConnection)
			{
				con_ConsolePrint("Unable to start a server to run demo.");
				return;
			}
		}

		sprintf(cmd, "timedemo %s", argv[0]);
		g_pClientMgr->m_pConnection->RunConsoleString(cmd);
	}
	else
	{
		con_ConsolePrint("TimeDemo <record filename (w/o extension)>");
	}
}


// 0x0041dba0 -- "Bind <device> <trigger> <action> [action ...]": clear the
// trigger's bindings, then bind every remaining argument to it at range
// 0..0, i.e. as a plain on/off action.
//
// The two float zeros are `push $0x0` twice (0.0f is the all-zero word), and
// the loop's entry test is a SECOND `cmp $0x2` -- `jl` for the guard at
// 0x0041dba9 and `jle` for the loop at 0x0041dbcb -- which is `i = 2` tested
// against argc, not a re-check of the guard.
void cc_Bind(int argc, char** argv)
{
	int i;

	if (argc < 2)
		return;

	g_pClientMgr->m_pInput->ClearBindings(g_pClientMgr->m_pInput,
	                                      argv[0], argv[1]);

	for (i = 2; i < argc; i++)
	{
		if (!g_pClientMgr->m_pInput->AddBinding(g_pClientMgr->m_pInput,
		                                        argv[0], argv[1], argv[i],
		                                        0.0f, 0.0f))
		{
			con_PrintColored(0xffffff, 1,
			                 "Error binding device: %s, trigger: %s",
			                 argv[0], argv[1]);
		}
	}
}


// 0x0041dc20 -- "RangeBind <device> <trigger> <low> <high> <action> ...":
// the same, in triples, with a real range per action.
//
// `lea -0x2(%edi),%ecx ; mov $0x55555556,%eax ; imul ; shr $0x1f ; add` is
// signed division by three: the count is (argc - 2) / 3, computed once and
// then tested `jle`, which is a `for (i = 0; i < n; i++)`.  The source
// INDEXES argv by i*3; VC6 runs i down in %ebx and strength-reduces the
// argv walk into %edi, stepping 0xc a turn.  That is why the `lea
// 0x8(%esi),%edi` sits AFTER the `jle` (0x0041dc63, in the loop preheader):
// a hand-written `pCur = &argv[2]` walker is set before the test.
//
// The triple is (low, high, action): 0x0041dc6e reads argv[i*3+3] for the
// LAST float argument and 0x0041dc7d reads argv[i*3+2] for the one before
// it, and 0x0041dc88 reads argv[i*3+4] as the action name.  IDENTICAL.
void cc_RangeBind(int argc, char** argv)
{
	int    nRanges;
	int    i;

	if (argc < 4)
		return;

	g_pClientMgr->m_pInput->ClearBindings(g_pClientMgr->m_pInput,
	                                      argv[0], argv[1]);

	nRanges = (argc - 2) / 3;

	for (i = 0; i < nRanges; i++)
	{
		if (!g_pClientMgr->m_pInput->AddBinding(g_pClientMgr->m_pInput,
		                                        argv[0], argv[1], argv[i*3+4],
		                                        (float)atof(argv[i*3+2]),
		                                        (float)atof(argv[i*3+3])))
		{
			con_PrintColored(0xffffff, 1,
			                 "Error binding device: %s, trigger: %s",
			                 argv[0], argv[1]);
		}
	}
}


// 0x0041dce0 -- "Scale <device> <trigger> <scale>": the trigger's scale,
// with the three range-scale terms left at zero.
//
// Seven arguments (`add $0x1c,%esp`) is what identifies slot +0x30 as
// ScaleTrigger; the three trailing zeros go down as `push $0x0` before the
// atof result, because arguments are pushed right to left.
void cc_Scale(int argc, char** argv)
{
	if (argc < 3)
		return;

	if (!g_pClientMgr->m_pInput->ScaleTrigger(g_pClientMgr->m_pInput,
	                                          argv[0], argv[1],
	                                          (float)atof(argv[2]),
	                                          0.0f, 0.0f, 0.0f))
	{
		con_PrintColored(0xffffff, 1,
		                 "Error finding device: %s, trigger: %s",
		                 argv[0], argv[1]);
	}
}


// 0x0041dd50 -- "RangeScale <device> <trigger> <scale> <min> <max>
// <preCenter>": the same entry with all four float terms supplied.
//
// A BUG IN THE SHIPPED SOURCE, KEPT.  The guard is `cmp $0x5 ; jl`, so five
// arguments are demanded -- but the body reads argv[5] (`mov 0x14(%esi),%ecx`
// at 0x0041dd64), which needs six.  The test is written against 5 here
// because that is what the image does.
//
// The four atof calls run in REVERSE argument order (argv[5] first at
// 0x0041dd64, argv[2] last at 0x0041dd8b): that is VC6 evaluating a call's
// arguments right to left, not source order, and it is not a lever.
void cc_RangeScale(int argc, char** argv)
{
	if (argc < 5)
		return;

	if (!g_pClientMgr->m_pInput->ScaleTrigger(g_pClientMgr->m_pInput,
	                                          argv[0], argv[1],
	                                          (float)atof(argv[2]),
	                                          (float)atof(argv[3]),
	                                          (float)atof(argv[4]),
	                                          (float)atof(argv[5])))
	{
		con_PrintColored(0xffffff, 1,
		                 "Error finding device: %s, trigger: %s",
		                 argv[0], argv[1]);
	}
}


// 0x0041dde0 -- "AddAction <name> <code>": register an input action.  The
// table pointer is loaded first (0x0041ddf2), then atoi(argv[1]), then the
// table re-read for the self argument -- the file's usual two-load shape.
// The result is ignored.
void cc_AddAction(int argc, char** argv)
{
	if (argc < 2)
		return;

	g_pClientMgr->m_pInput->AddAction(g_pClientMgr->m_pInput,
	                                  argv[0], atoi(argv[1]));
}


// The renderer's RenderStruct at 0x004b4358 (clientrender.cpp's
// g_RenderStruct), only down to the slots this file reaches:
// +0xb4 is 0x004b440c, +0x104 is 0x004b445c.  (view name ours)
struct CConRenderStruct
{
	uint8 m_pad_000[0xb4];
	void  (*RenderCommand)(int argc, char** argv);              // 0xb4
	uint8 m_pad_0b8[0x104 - 0xb8];
	void* m_pEnvMapTexture;                                     // 0x104
};

class RenderStruct;
extern RenderStruct g_RenderStruct;                             // 0x004b4358
#define g_ConRenderStruct (*(CConRenderStruct*)&g_RenderStruct)

// 0x0041de80 -- "RenderCommand" / "RCom": hand the arguments to the
// renderer.  The guard is the struct's ADDRESS (`mov $0x4b4358,%eax; test
// %eax,%eax; je`), not the slot's contents: the source takes a pointer to
// the global and null-tests the pointer, which VC6 keeps though it cannot
// fail (clientwin.cpp records the same test on the same address).
void cc_RenderCommand(int argc, char** argv)
{
	CConRenderStruct* pStruct = &g_ConRenderStruct;

	if (pStruct)
		pStruct->RenderCommand(argc, argv);
}


// 0x0041deb0 -- "RestartRender [dll]": optionally point the renderDLL
// console variable somewhere new, then take the renderer down and bring it
// back.
//
// The guard is `test %eax,%eax ; jle`, NOT `cmp $0x1 ; jl` -- this file uses
// both forms and they are source-visible, so this one is written `argc > 0`
// where TimeDemo and the rest are written against 1.
//
// The frame is 0xf8 with the sprintf target at esp+0 and nothing else on the
// stack, so the buffer is 248 bytes, not 256.
//
// The failure funnel is the same three lines as ResizeScreen below: an
// LTRESULT from 0x004116d0 with 0x04000000 ORed in, which is the bit
// 0x0041fc90 dispatches on at 0x0041fce4.
void cc_RestartRender(int argc, char** argv)
{
	char     str[248];
	LTRESULT dResult;

	if (argc > 0)
	{
		sprintf(str, "renderDLL %s", argv[0]);
		cc_RunConsoleString(str);
	}

	sub_4627d0(g_pClientMgr, 1);

	dResult = sub_4116d0(g_pClientMgr);

	if (dResult)
		sub_41fc90(g_pClientMgr, dResult | 0x4000000);
}


// 0x0041df20 -- "ResizeScreen <width> <height>": set the requested size,
// restart the renderer, and put the old size back if it will not come up.
//
// The old pair is read into %edi/%ebx BEFORE either atoi (0x0041df2f and
// 0x0041df35), which is what says they are locals saved across the attempt
// and not re-read afterwards.  The print takes g_CV_ScreenWidth back OUT of
// memory (0x0041df56) while using atoi's result in %eax for the height --
// the store to g_CV_ScreenHeight at 0x0041df6a comes after the pushes -- which
// is the globals being named in the format, not two locals.
void cc_ResizeScreen(int argc, char** argv)
{
	int      nOldWidth;
	int      nOldHeight;
	LTRESULT dResult;

	if (argc < 2)
		return;

	nOldWidth = g_CV_ScreenWidth;
	nOldHeight = g_CV_ScreenHeight;

	g_CV_ScreenWidth = atoi(argv[0]);
	g_CV_ScreenHeight = atoi(argv[1]);

	con_PrintColored(0xff00, 1, "Setting screen to %dx%d",
	                 g_CV_ScreenWidth, g_CV_ScreenHeight);

	sub_4627d0(g_pClientMgr, 1);

	if (sub_4116d0(g_pClientMgr))
	{
		// It refused the new size: go back to the old one, and only if THAT
		// fails is the renderer really gone.
		g_CV_ScreenWidth = nOldWidth;
		g_CV_ScreenHeight = nOldHeight;

		dResult = sub_4116d0(g_pClientMgr);

		if (dResult)
			sub_41fc90(g_pClientMgr, dResult | 0x4000000);
	}
}


// 0x0041dfd0 -- "serv <args...>": quote every argument and send the lot to
// the server as one console command line.
//
// FRAME.  0x320 of locals, in two pieces: the accumulator at esp+0x12c
// (0x1f4 == 500 bytes, so it is declared first and takes the high end) and
// the per-argument scratch at esp+0x000 (0x12c == 300).
//
// The strcat at 0x0041e030..0x0041e053 is VC6's inline expansion -- scan the
// source for its length, scan the destination, `dec %edi` back onto its
// terminator, then `rep movsl` / `rep movsb` -- so the source really is
// strcat and not a running pointer.
void cc_Serv(int argc, char** argv)
{
	char str[500];
	char temp[300];
	int  i;

	if (argc < 1)
		return;

	if (!g_pClientMgr->m_pConnection)
		return;

	str[0] = 0;

	for (i = 0; i < argc; i++)
	{
		sprintf(temp, "\"%s\" ", argv[i]);
		strcat(str, temp);
	}

	g_pClientMgr->m_pConnection->RunConsoleString(str);
}


// 0x0041e090 -- "ListInputDevices": the input table's +0x0c, handed the
// table itself; one load of the table serves both uses.
void cc_ListInputDevices(int argc, char** argv)
{
	g_pClientMgr->m_pInput->ListDevices(g_pClientMgr->m_pInput);
}


// 0x0041e0b0 -- "EnvMap <texture>": point the renderer's environment map at
// a texture.
//
// The dword/dword/word zeroing at 0x0041e0bb before anything else is
// CClientFileRef's inline constructor, which is why the reference is
// declared at the top rather than beside its use.  `mov $0x1,%ecx` serves
// both the argc test and the file-type store, which is VC6 sharing one
// materialised constant.
void cc_EnvMap(int argc, char** argv)
{
	CConClientFileRef ref;

	if (!g_pClientMgr)
		return;

	if (argc < 1)
		return;

	ref.m_nFileType = 1;
	ref.m_pFilename = argv[0];

	g_ConRenderStruct.m_pEnvMapTexture = sub_41ff90(g_pClientMgr, &ref);

	if (g_ConRenderStruct.m_pEnvMapTexture)
		con_ConsolePrint("Environment map set to %s.", argv[0]);
	else
		con_ConsolePrint("Couldn't find texture %s.", argv[0]);
}


// 0x0041e160 -- "ShowTicks <section> [...]": toggle the per-section timing
// display, one word per argument.
//
// The usage block is the FALL-THROUGH (`cmp $0x1 ; jge` past it), so it is a
// guard with a return and not an else arm -- the opposite of cc_TimeDemo,
// and the reason is that this one is nine statements long.
//
// THE `||` IS VISIBLE.  Every other arm toggles inline and jumps to the
// shared store at 0x0041e327; the SUMMARY arm is the only one whose body
// sits AFTER the whole chain, at 0x0041e31f, reached by two `je`s from two
// different compares.  That is "a shared block after the body
// means one `if (A || B)`" exactly.  The second compare really does read
// argv[0] and not argv[i] -- %ebx holds argv for the whole loop while %esi
// walks it, and 0x0041e279 is `mov (%ebx),%eax`.  Kept as shipped.
//
// The error line is pushed with THREE arguments and no vararg even though
// its format has a %s (0x0041e309: colour, level, format, and nothing else).
// Also shipped; also kept.
void cc_ShowTicks(int argc, char** argv)
{
	int i;

	if (argc < 1)
	{
		con_PrintColored(0xc0c0ff, 0, "Please include at least one command:");
		con_PrintColored(0xc0c0ff, 0, "  ALL - Show Everything");
		con_PrintColored(0xc0c0ff, 0, "  NONE - Show Nothing");
		con_PrintColored(0xc0c0ff, 0, "  SUMMARY - Toggle Summary information");
		con_PrintColored(0xc0c0ff, 0, "  RENDER - Toggle Renderer");
		con_PrintColored(0xc0c0ff, 0, "  GAME - Toggle Game processing");
		con_PrintColored(0xc0c0ff, 0, "  ENGINE - Toggle Engine processing");
		con_PrintColored(0xc0c0ff, 0, "  GRAPH - Toggle Summary graph");
		con_PrintColored(0xc0c0ff, 0, "          (Red = Renderer, Green = Game, Blue = Engine, White = System)");
		return;
	}

	for (i = 0; i < argc; i++)
	{
		if (stricmp(argv[i], "ALL") == 0)
			g_CV_ShowTickCounts = 0xffffffff;
		else if (stricmp(argv[i], "NONE") == 0)
			g_CV_ShowTickCounts = 0;
		else if (stricmp(argv[i], "SUMMARY") == 0 || stricmp(argv[0], "1") == 0)
			g_CV_ShowTickCounts ^= 1;
		else if (stricmp(argv[i], "RENDER") == 0)
			g_CV_ShowTickCounts ^= 2;
		else if (stricmp(argv[i], "GAME") == 0)
			g_CV_ShowTickCounts ^= 4;
		else if (stricmp(argv[i], "ENGINE") == 0)
			g_CV_ShowTickCounts ^= 8;
		else if (stricmp(argv[i], "GRAPH") == 0)
			g_CV_ShowTickCounts ^= 0x10;
		else
			con_PrintColored(0xffc0c0, 0,
			                 "Error: \"%s\" is not a valid ShowTicks section");
	}
}


// 0x0041e3d0 -- "WriteHistory <file>": every command in the history, one per
// line, into a text file.
//
// This is cc_ListHistory (0x0041e340, clientconsole.cpp) with a file instead
// of the console, and it is written the same way for the same reason: the
// history is reached through a POINTER so VC6 keeps the virtual dispatch and
// folds `this` to an immediate -- `mov 0x4b2ec4,%eax ; mov $0x4b2ec4,%ecx ;
// call *0x14(%eax)`.  Naming the object directly would bind statically and
// emit a direct call.
//
// Three guards, three separate `pop/pop/ret` epilogues: each is its own
// `if (...) { print; return; }`, and each print is the fall-through of a
// `jge`/`jne`/`jne`.
void cc_WriteHistory(int argc, char** argv)
{
	CConHistory* pHist = &g_Console.m_History;
	FILE*        pFile;
	char*        pName;
	int          nCount;

	if (argc < 1)
	{
		con_ConsolePrint("Specify a filename.");
		return;
	}

	if (!pHist->FindFirst(LTNULL))
	{
		con_ConsolePrint("No commands in history.");
		return;
	}

	pFile = fopen(argv[0], "wt");

	if (!pFile)
	{
		con_ConsolePrint("Error opening file %s.", argv[0]);
		return;
	}

	nCount = 0;

	while ((pName = pHist->GetName()) != LTNULL)
	{
		fputs(pName, pFile);
		fputc('\n', pFile);

		nCount++;

		if (!pHist->FindNext(LTNULL))
			break;
	}

	fclose(pFile);

	con_ConsolePrint("Successfully wrote %d commands to %s.", nCount, argv[0]);
}


// 0x0041e4b0 -- "ReadHistory <file>": fill the history from a text file.
//
// `testb $0x10,0xc(%esi)` at 0x0041e561 and 0x0041e513 is stdio.h's feof
// MACRO -- _IOEOF is 0x10 and FILE::_flag is at +0xc -- not a call, which is
// what the loop is written against.  VC6 rotated it: the test is emitted
// once before the body and once at the bottom.
//
// The 0x100-byte frame is the line buffer; %ebp is the count and survives
// the whole loop.  The reload of argv into %edi at 0x0041e567 is on the
// loop-exit path only, because the early-EOF path never clobbered it -- a
// cross-jump detail, not a second variable.
void cc_ReadHistory(int argc, char** argv)
{
	FILE* pFile;
	char  str[256];
	int   nCount;
	int   nLen;

	if (argc < 1)
	{
		con_ConsolePrint("Specify a filename.");
		return;
	}

	pFile = fopen(argv[0], "rt");

	if (!pFile)
	{
		con_ConsolePrint("Error opening file %s.", argv[0]);
		return;
	}

	nCount = 0;

	while (!feof(pFile))
	{
		// RELATIONAL, not a truth test: shipped closes with `jbe`, not
		// `je`, which is an UNSIGNED pointer comparison against zero.
		if (fgets(str, 256, pFile) > (char*)0)
		{
			nLen = strlen(str);

			// `jle` then `jne` into the same block: one `&&`, with the
			// length test first.
			if (nLen > 0 && str[nLen - 1] == '\n')
			{
				nLen--;
				str[nLen] = 0;
			}

			// `je`, not `jle`: this one is written against zero.
			if (nLen)
			{
				g_Console.m_History.Add(str);
				nCount++;
			}
		}
	}

	fclose(pFile);

	con_ConsolePrint("Successfully read %d commands from %s.", nCount, argv[0]);
}


// 0x0041e590 -- "Exec <file>": run a file of console commands.
//
// 0x0041b420 is the shared console TU's con_RunFile (servermgr.h already
// declares it at the server's 0x10005c25): it fopen()s "rt", fgets 0x1f4 at
// a time into con_RunStringEx and returns 1.  The two trailing arguments are
// a literal 0 and a literal 1 at every call site.
void cc_Exec(int argc, char** argv)
{
	if (argc < 1)
	{
		con_ConsolePrint("Specify a filename.");
		return;
	}

	if (con_RunFile(&g_ClientConsoleState, argv[0], 0, 1))
		con_ConsolePrint("Successfully executed %s.", argv[0]);
	else
		con_ConsolePrint("Error executing %s.", argv[0]);
}


// 0x0041e5f0 -- "MoveConsole <preset|left top right bottom>": report or set
// the console rectangle.  It does not write the four variables itself -- it
// runs four console commands, which is how the change goes through the same
// path a config file would take.
//
// The registers name themselves off the four sprintf calls: %esi left, %edi
// top, %ebx right, %ebp bottom.  The presets are then readable directly, and
// the shipped default of the four globals (-1,-1,-1,-2) IS the "Top" preset,
// which independently confirms the order.
//
// %esi carries argc until the last preset has been ruled out and becomes
// `left` after that; that is one register serving two dead-disjoint locals,
// not one variable.
//
// ONE ODDITY, KEPT.  The "Bottom" arm stores in the order left, right, top,
// bottom (0x0041e67b or, 0x0041e67e or, 0x0041e681 mov, 0x0041e686 or) while
// the other three arms store left, top, right, bottom.  These are register
// initialisations and could in principle have been scheduled, but the other
// three came out in source order, so this one is written as the image has it.
void cc_MoveConsole(int argc, char** argv)
{
	char str[200];
	int  nLeft;
	int  nTop;
	int  nRight;
	int  nBottom;

	if (argc == 0)
	{
		con_ConsolePrint("Console position : (%d,%d, %d,%d)",
		                 g_CV_ConsoleLeft, g_CV_ConsoleTop,
		                 g_CV_ConsoleRight, g_CV_ConsoleBottom);
		return;
	}

	if (stricmp(argv[0], "Top") == 0)
	{
		nLeft = -1;
		nTop = -1;
		nRight = -1;
		nBottom = -2;
	}
	else if (stricmp(argv[0], "Bottom") == 0)
	{
		nLeft = -1;
		nRight = -1;
		nTop = -2;
		nBottom = -1;
	}
	else if (stricmp(argv[0], "Middle") == 0)
	{
		nLeft = -4;
		nTop = -4;
		nRight = -4;
		nBottom = -4;
	}
	else if (stricmp(argv[0], "Full") == 0)
	{
		nLeft = -1;
		nTop = -1;
		nRight = -1;
		nBottom = -1;
	}
	else
	{
		if (argc < 4)
		{
			con_ConsolePrint("Specify Top, Bottom, Full, Middle, or 4 screen coordinates");
			return;
		}

		nLeft = atoi(argv[0]);
		nTop = atoi(argv[1]);
		nRight = atoi(argv[2]);
		nBottom = atoi(argv[3]);
	}

	sprintf(str, "ConsoleLeft %d", nLeft);
	cc_RunConsoleString(str);

	sprintf(str, "ConsoleTop %d", nTop);
	cc_RunConsoleString(str);

	sprintf(str, "ConsoleRight %d", nRight);
	cc_RunConsoleString(str);

	sprintf(str, "ConsoleBottom %d", nBottom);
	cc_RunConsoleString(str);
}


// ======================================================================= //
//  The built-in command table, and the console state that registers it
// ======================================================================= //
//
//  0x004a8828 (two config-file writers) and 0x004a8830 (thirty-three
//  commands) are adjacent in .data and are both planted by 0x0041e890, which
//  is what puts them in this translation unit.  Reading 0x004a8830 is what
//  named every handler above, so it is reproduced in full -- the handlers
//  outside this file are declared, not redefined.

// Already in src/engine/lithtech/clientconsole.cpp (the first half of this
// same source file); the names are that file's, so the table links.
void cc_World(int argc, char** argv);                       // 0x0041d840
void cc_EnableDevice(int argc, char** argv);                // 0x0041db60
void cc_Screenshot(int argc, char** argv);                  // 0x0041de20  "SSFile"
void cc_Sub41de60(int argc, char** argv);                   // 0x0041de60  "UpdateServer"
void cc_Sub41e130(int argc, char** argv);                   // 0x0041e130  "RebindTextures"
void cc_ListHistory(int argc, char** argv);                 // 0x0041e340  "ConsoleHistory"
void cc_ClearHistory(int argc, char** argv);                // 0x0041e3b0
void cc_ListCommands(int argc, char** argv);                // 0x0041e810
void cc_ShowGameVars(int argc, char** argv);                // 0x0041e840  "Set"
void cc_SaveModelVars(FILE* pFile);                         // 0x0041e7a0

// UNDECODED: the six remaining handlers in and around this range are named
// by the table but not written anywhere yet.  They are declared so the table
// is complete and NOT invented here.
//   0x00404470  cc_BlastServer        "BlastServer"
//   0x0041e090  cc_ListInputDevices   "ListInputDevices"  (now WRITTEN,
//               before cc_EnvMap above)
//   0x0041e080  cc_Quit               "quit"      (0x0042e6a0 with a null)
//   0x0041d640  cc_ModelAdd           "ModelAdd"
//   0x0041d740  cc_ModelDirAdd        "ModelDirAdd"
//   0x0041dde0  cc_AddAction          "AddAction"  (now WRITTEN, after
//               cc_RangeScale above)
//   0x0041de80  cc_RenderCommand      "RenderCommand" / "RCom"  (now
//               WRITTEN, after cc_AddAction above)
//   0x0041dea0  cc_RestartConsole     "RestartConsole"  (tail `jmp` to
//               0x004105c0 with %ecx = the console's CClientMgr)
//   0x0041e150  cc_HeapCompact        "HeapCompact"     (tail `jmp` to
//               0x00428ed0)
void cc_ListInputDevices(int argc, char** argv);            // 0x0041e090
void cc_ModelAdd(int argc, char** argv);                    // 0x0041d640
void cc_ModelDirAdd(int argc, char** argv);                 // 0x0041d740
void cc_AddAction(int argc, char** argv);                   // 0x0041dde0
void cc_RenderCommand(int argc, char** argv);               // 0x0041de80

// 0x0042e6a0 -- clientdsi.cpp's sub_42e6a0: post the quit, with an optional
// message (null here).
void sub_42e6a0(char* pMsg);

// 0x00428ed0 -- server/dsi.cpp's client-only dsi_HeapCompact.
void dsi_HeapCompact();

// 0x00404470 -- "BlastServer": a bare `ret` (the linker folded it with the
// other empty functions at that address).
void cc_BlastServer(int argc, char** argv)
{
}

// 0x0041e080 -- "quit".
void cc_Quit(int argc, char** argv)
{
	sub_42e6a0(LTNULL);
}

// 0x0041dea0 -- "RestartConsole": a tail `jmp` to 0x004105c0 on the
// console's CClientMgr (clientrender.cpp's CClientMgrView::Sub4105c0).
void cc_RestartConsole(int argc, char** argv)
{
	g_pClientMgr->Sub4105c0();
}

// 0x0041e150 -- "HeapCompact": a tail `jmp` to 0x00428ed0.
void cc_HeapCompact(int argc, char** argv)
{
	dsi_HeapCompact();
}

// 0x00438d80, the other config-file writer: the input manager's saver
// ("AddAction %s %d\n" and the binding lines), defined in clientinput.cpp.
void cc_SaveInputBindings(FILE* pFile);                     // 0x00438d80


// 0x004a8828 -- two entries, in this order.
void (*g_ClientConsoleSavers[2])(FILE* pFile) =
{
	cc_SaveInputBindings,
	cc_SaveModelVars
};

// 0x004a8830 -- thirty-three {name, fn, user} triples, the user field zero
// throughout.  "RenderCommand" and "RCom" share one handler.
ConsoleCommandDef g_ClientConsoleCommands[33] =
{
	{ "BlastServer",      (void*)cc_BlastServer,      LTNULL },
	{ "ListInputDevices", (void*)cc_ListInputDevices, LTNULL },
	{ "quit",             (void*)cc_Quit,             LTNULL },
	{ "serv",             (void*)cc_Serv,             LTNULL },
	{ "ModelAdd",         (void*)cc_ModelAdd,         LTNULL },
	{ "ModelDirAdd",      (void*)cc_ModelDirAdd,      LTNULL },
	{ "World",            (void*)cc_World,            LTNULL },
	{ "TimeDemo",         (void*)cc_TimeDemo,         LTNULL },
	{ "EnableDevice",     (void*)cc_EnableDevice,     LTNULL },
	{ "RangeBind",        (void*)cc_RangeBind,        LTNULL },
	{ "Bind",             (void*)cc_Bind,             LTNULL },
	{ "Scale",            (void*)cc_Scale,            LTNULL },
	{ "RangeScale",       (void*)cc_RangeScale,       LTNULL },
	{ "AddAction",        (void*)cc_AddAction,        LTNULL },
	{ "SSFile",           (void*)cc_Screenshot,       LTNULL },
	{ "UpdateServer",     (void*)cc_Sub41de60,        LTNULL },
	{ "RenderCommand",    (void*)cc_RenderCommand,    LTNULL },
	{ "RCom",             (void*)cc_RenderCommand,    LTNULL },
	{ "Set",              (void*)cc_ShowGameVars,     LTNULL },
	{ "ListCommands",     (void*)cc_ListCommands,     LTNULL },
	{ "RestartConsole",   (void*)cc_RestartConsole,   LTNULL },
	{ "RestartRender",    (void*)cc_RestartRender,    LTNULL },
	{ "ResizeScreen",     (void*)cc_ResizeScreen,     LTNULL },
	{ "EnvMap",           (void*)cc_EnvMap,           LTNULL },
	{ "RebindTextures",   (void*)cc_Sub41e130,        LTNULL },
	{ "HeapCompact",      (void*)cc_HeapCompact,      LTNULL },
	{ "ConsoleHistory",   (void*)cc_ListHistory,      LTNULL },
	{ "ClearHistory",     (void*)cc_ClearHistory,     LTNULL },
	{ "WriteHistory",     (void*)cc_WriteHistory,     LTNULL },
	{ "ReadHistory",      (void*)cc_ReadHistory,      LTNULL },
	{ "Exec",             (void*)cc_Exec,             LTNULL },
	{ "MoveConsole",      (void*)cc_MoveConsole,      LTNULL },
	{ "ShowTicks",        (void*)cc_ShowTicks,        LTNULL }
};


// 0x0041e890 -- bring the client's ConsoleState up.
//
// This is src/engine/server/console.cpp's cc_InitState with the client's
// halves of the contract plugged in, and it is written in that function's
// shape because the store order is the same: the 0x44-byte memset (an inline
// `rep stos` of 0x11 dwords), then savers, bindings, built-ins, then the
// three callbacks, then con_Init.
//
// What the client does NOT set is as telling as what it does: m_pOnCreate,
// m_pOnChange and m_pServerMgr are left as the memset found them, so the
// console-variable hooks are a server-only thing.
//
// The last line is not part of the state at all: it points the console
// window's name source at the completion enumerator at 0x004b2ee8, whose own
// dynamic initialiser (0x0041d610) constructed it.
void cc_InitClientState()
{
	memset(&g_ClientConsoleState, 0, 0x44);

	g_ClientConsoleState.m_pSavers = (void (**)(void*))g_ClientConsoleSavers;
	g_ClientConsoleState.m_nSavers = 2;

	g_ClientConsoleState.m_pBindings = dsi_GetConsoleBindings();
	g_ClientConsoleState.m_nBindings = dsi_GetNumConsoleBindings();

	g_ClientConsoleState.m_pBuiltins = g_ClientConsoleCommands;
	g_ClientConsoleState.m_nBuiltins = 33;

	g_ClientConsoleState.m_pPrint = con_Print;
	g_ClientConsoleState.m_pAlloc = dalloc;
	g_ClientConsoleState.m_pFree = dfree;

	con_Init(&g_ClientConsoleState);

	g_Console.SetNameList(&g_ConsoleCompletion);
}


// 0x0041e930, 19 bytes -- declared above.  0x004105f2 takes its address.
void cc_RunConsoleString(char* pCommand)
{
	con_RunString(&g_ClientConsoleState, pCommand);
}


// ======================================================================= //
//  ADDRESS MAP -- address, size, symbol
// ======================================================================= //
//
//   0x0041da40  281  cc_TimeDemo        6 calls  (__chkstk, StartGame,
//                                                 print, sprintf, send, print)
//   0x0041dba0  125  cc_Bind            3 calls  (2 indirect + 1 direct)
//   0x0041dc20  179  cc_RangeBind       5 calls  (2 indirect + 3 direct)
//   0x0041dce0  104  cc_Scale           3 calls  (1 indirect + 2 direct)
//   0x0041dd50  134  cc_RangeScale      6 calls  (1 indirect + 5 direct)
//   0x0041deb0  112  cc_RestartRender   5 calls  (all direct)
//   0x0041df20  171  cc_ResizeScreen    7 calls  (all direct)
//   0x0041dfd0  171  cc_Serv            2 calls  (all direct)
//   0x0041e0b0  128  cc_EnvMap          3 calls  (all direct)
//   0x0041e160  474  cc_ShowTicks      18 calls  (all direct)
//   0x0041e3d0  210  cc_WriteHistory   12 calls  (4 indirect + 8 direct)
//   0x0041e4b0  223  cc_ReadHistory     7 calls  (all direct)
//   0x0041e590   89  cc_Exec            4 calls  (all direct)
//   0x0041e5f0  421  cc_MoveConsole    18 calls  (all direct)
//   0x0041e890  135  cc_InitClientState 4 calls  (all direct)
//
//   (cc_Serv's and cc_TimeDemo's send to the server is a direct
//    `call 0x00413c60`: a non-virtual thiscall, so it is direct on both
//    sides.  cc_TimeDemo's six include __chkstk, which our build emits too.)
//
//  Every one of those figures was counted on the shipped
//  individual extents and they agree with the enumeration's per-function call
//  counts, which is the first check that the decode is the right shape.
