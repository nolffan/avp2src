// *********************************************************************** //
//
//  MODULE:   Q454530 (.CPP)   — staging translation unit (name ours)
//
//  PURPOSE:  The eight enumerated rows from 0x00454530 forward, read off
//            the retail lithtech.exe ONLY.  They are NOT one translation
//            unit: the cluster straddles a module boundary at 0x00454f20.
//            The per-block banners say where each body really belongs.
//            Everything here compiles at /O2 (cl 8447), which is what
//            src/engine/lithtech gets.
//
//  All type names in this file carry the suffix `Q4545` so the file can be
//  compiled beside the real headers without colliding with them.
//
//
//  THE THREE CHEAP CHECKS
//  ----------------------
//
//  1. NOT A PREBUILT LIBRARY.  rezmgr.lib's window is 0x0048df10..0x00490b43
//     and none of these rows is in it.  Three relocation-free windows —
//     0x004548ac (14 bytes), 0x0045486c (12) and 0x00454913 (16) — were
//     searched byte-for-byte through the WHOLE of the retail server.dll,
//     at every offset, not just the rezmgr delta +0x0FBAC620.  Zero hits.
//     That is expected even for the two rows that ARE shared source, because
//     server.dll compiles this TU at /O1 and lithtech.exe at /O2: the byte
//     test can only ever prove "prebuilt object, identical compile", and it
//     says no.
//
//  2. TWO OF THE SIX REAL FUNCTIONS ARE ALREADY RECONSTRUCTED — and this is
//     exactly the shared-TU blind spot an address search cannot see, because the
//     file that holds them is annotated with server.dll addresses:
//
//       * 0x00454a40 (row 0x00454a40 + row 0x00454c49, ONE function of
//         1211 bytes) is wm_RotatePushObject, written in full in
//         src/engine/server/commonphysics.cpp against its server address
//         0x1002115c.  Verified statement by statement against the client
//         bytes below — every call, constant and branch lines up.  NOT
//         rewritten here.
//       * 0x00454f00 was written as `sub_454f00` in lt2new1.cpp and is now
//         commonphysics.cpp's wm_RotatePushFilter.  It is the per-object filter
//         wm_RotateWorldModel hands to the world tree, and its two "View"
//         classes are LTObject and WMMoveState seen through a keyhole; the
//         BODY is right and byte-identical work on it is already banked.
//         NOT rewritten here; the naming correction is noted in its banner
//         below and nothing else about it changes.
//
//  3. THE MODULE, from data and from the call graph.
//
//     0x00454530..0x00454efb is the tail of the WORLD-MODEL MOVEMENT
//     translation unit that src/engine/server/commonphysics.cpp already
//     covers.  Every helper it reaches is a named member of that file's own
//     twelve-function cluster, at the client addresses commonphysics.cpp's
//     index table lists:
//
//         0x00451000  wm_MoveObject            0x00451690  wm_BoxesOverlap
//         0x00451d70  wm_NotifyContact         0x004524b0  CWMStateInit::Init
//         0x004524e0  wm_MoveThroughSim        0x00453ef0  CWMStateInit::InitFrom
//
//     and the counter it bumps on the way in, 0x004b4070, is the one
//     src/engine/lithtech/clientphysics.h already names g_nMoveObjectCount.
//     servermgr.h declares the head of this run — wm_RotateWorldModel,
//     server 0x10020a53 — and nothing had ever read its body.
//
//     0x00454f70..0x004550b0 is a DIFFERENT module: the CD-audio / music DLL
//     loader.  What names it is the .data string pool, in address order:
//     "CClientMgr::AppInitMusic" (0x004a8460) is the one caller, the DLL it
//     loads is "cdaudio.dll" (0x004a8480), the entry point it asks for is
//     "MusicDLLSetup" (0x004ac440), and the three failure returns 1/2/3 are
//     mapped by that caller onto ltcodes.h's LT_MISSINGMUSICDLL (36),
//     LT_INVALIDMUSICDLL (37) and LT_UNABLETOINITMUSICDLL (38) — the only
//     three music codes the SDK header has, in that order.
//
//     0x00454f20 belongs to NEITHER: it is an SDK template COMDAT,
//     _CVector<float>::operator/(float), sitting in the gap because the
//     linker parked it there.  Its two callers (0x004538e9, 0x00453b75) are
//     both inside the physics TU.
//
//
//  TRUE EXTENTS — four of the eight rows are wrong, in two shapes
//  --------------------------------------------------------------
//
//    row         stated  real function                       why
//    ----------  ------  ---------------------------------   --------------
//    0x00454530     298  0x00454530..0x00454a35  (1285)      row ends MID
//    0x0045465a     987  ^ same function                     INSTRUCTION at
//                                                            0x00454652
//    0x00454a40     521  0x00454a40..0x00454efb  (1211)      three branches
//    0x00454c49     690  ^ same function                     leave the row
//    0x00454f00      31  0x00454f00..0x00454f1f  correct
//    0x00454f20      67  0x00454f20..0x00454f62  correct
//    0x00454f70      58  0x00454f70..0x00454fa9  correct
//    0x00454fb0     176  0x00454fb0..0x0045505f  correct
//
//  Row 0x00454530 has ZERO rets and 1 byte follows that is NOT
//  padding: the row cuts the `movl $0x3f800000,0xe8(%esp)` at 0x00454652 in
//  half.  Row 0x00454a40 has six branch targets outside
//  the range — 0x00454ef0 (x4) and 0x00454ee2 (x2) — which are the shared
//  `fail:` epilogue and the not-solid arm, both inside the row below it.
//  Both defects are shape (a)/(b) of the seven: a success arm and a shared
//  fail block enumerated as functions of their own.
//
//  IN THE GAP, and written here because they are the same module and were
//  unclaimed: 0x00455060 music_Term (61 bytes) and the five-byte tail `jmp`
//  thunk at 0x004550a0 onto the bare `ret` at 0x004550b0.  0x004550a0 is
//  taken by address from .data at 0x004a70b0 — a vtable slot — which is why
//  the pair exists at all.
//
//  NOT CLAIMED: 0x004550c0 (97 bytes) is a 16-bit bit walk with one caller
//  at 0x00458866 and belongs to whatever module that is; it is past the
//  music loader and is left alone.
//
// *********************************************************************** //

#include <stdio.h>            // _vsnprintf
#include <stdarg.h>
#include <windows.h>

#include "ltbasedefs.h"


// ======================================================================= //
//  A.  0x00454f20, 67 bytes — _CVector<float>::operator / (float)
//
//  BELONGS IN: nowhere new.  It is src/LT2/sdk/inc/ltvector.h line 65, verbatim,
//  as a template COMDAT:
//
//      _CVector<T> operator / (T v) const
//      { return _CVector<T>(x / v, y / v, z / v); }
//
//  WHAT PINS IT.
//   * `ret $0x8` with `this` in %ecx: a thiscall member that pops TWO stack
//     arguments.  For a member returning a class by value those two are the
//     hidden return-slot pointer (0x10(%esp) after the `sub $0xc`, i.e. the
//     FIRST stack argument) and the one declared argument (0x14(%esp)).
//   * that declared argument is loaded as a FLOAT and used as the divisor of
//     all three components (`fdivs 0x14(%esp)` three times, re-read each
//     time — it is a parameter in memory, not a cached register), so the
//     operator is the scalar one at line 65 and not the vector one at 60.
//   * `sub $0xc,%esp` is a twelve-byte TEMPORARY that all three quotients
//     are stored into and then copied out of, three dwords at a time, into
//     the return slot.  That is the signature of `return _CVector<T>(a,b,c)`
//     — construct into a temporary, copy to the NRV slot — and not of
//     writing the members of the return slot directly, which would need no
//     temporary at all.
//   * the copy-out is INTERLEAVED with the arithmetic (x and y are copied
//     out before z's `fstps`), which is scheduling, not source order.
//   * %eax is loaded with the return slot at 0x00454f29 and never clobbered,
//     so it is still the return slot at the `ret`: the required convention.
//
//  WHY IT IS OUT OF LINE AT ALL: it is a template member, so it gets a
//  COMDAT whether or not it is defined in the class body, and VC6 declines
//  to expand it at /Ob1 because of the twelve-byte return.  Compare
//  0x004316b0, _CVector<float>::MagSqr, which src/engine/lithtech/lt2new1.cpp
//  already reports in the same position.
//
//  HOW TO BUILD IT FOR REAL: do not compile the class below — force the
//  SDK's own header instead, exactly as lt2new1.cpp forces MagSqr:
//      static LTVector (LTVector::*s_pDiv)(float) const = &LTVector::operator/;
//  The stand-in here exists so this file compiles on its own.
// ======================================================================= //

template <class T>
class _QVector4545
{
public:
	_QVector4545() {}
	_QVector4545(T _x, T _y, T _z) { x = _x; y = _y; z = _z; }

	_QVector4545<T> operator / (T v) const
	{
		return _QVector4545<T>(x / v, y / v, z / v);
	}

	T x, y, z;
};

typedef _QVector4545<float> LTVectorQ4545;

// Forced out of line the way the shipped image has it.
static LTVectorQ4545 (LTVectorQ4545::*s_pDivQ4545)(float) const =
	&LTVectorQ4545::operator/;


// ======================================================================= //
//  B.  THE MUSIC DLL LOADER — 0x00454f70, 0x00454fb0, 0x00455060,
//      0x004550a0/0x004550b0
//
//  BELONGS IN: a client TU of its own — the engine's cdaudio.dll shim.  Its
//  only entry points into the rest of the client are CClientMgr::AppInitMusic
//  (the caller at 0x004103d1, named by the literal at 0x004a8460) and the
//  five-byte tail jump at 0x00410500 onto music_Term.
//
//  WHAT THE STRUCT IS.  The thing AppInitMusic passes is CClientMgr+0x0f84,
//  and the DLL name it passes is the buffer at CClientMgr+0x0ff4 that
//  AppInitMusic has just filled with "cdaudio.dll" (0x004a8480) or the
//  console override.  Four of its fields are reachable, all four off these
//  three functions and nothing else:
//
//    +0x00  int  (*Init)(MusicStruct*)   `push %esi; call *(%esi)`, cdecl,
//                                        NONZERO means it came up.
//    +0x04  void (*Term)(void)           `call *0x4(%eax)` with NO push and
//                                        no cleanup — genuinely no argument.
//    +0x58  the printf the DLL is given: `movl $0x454f70,0x58(%esi)`, set
//           between the setup call and Init, i.e. so Init can already use it.
//    +0x5c  cleared to zero twice: on the way into a load, and again in
//           music_Term AFTER Term() has run.  Nothing reads it here.
//
//  Everything between 0x08 and 0x54 is the DLL's own half of the contract
//  and is padding as far as this file can see.
//
//  THE TWO GLOBALS, both zeroed .data (.bss):
//    0x004b40c0  the HINSTANCE of the loaded DLL
//    0x004b40c4  the struct currently installed
//
//  The other four of the eleven byte-visible facts are in the per-function
//  banners below.
// ======================================================================= //

struct MusicStructQ4545;

typedef int  (*TMusicInitQ4545)(MusicStructQ4545* pStruct);
typedef void (*TMusicTermQ4545)(void);
typedef void (*TMusicPrintQ4545)(char* pMsg, ...);
typedef void (*TMusicSetupQ4545)(MusicStructQ4545* pStruct);

struct MusicStructQ4545
{
	TMusicInitQ4545  m_pInit;                    // 0x00
	TMusicTermQ4545  m_pTerm;                    // 0x04
	uint8            m_pad_08[0x58 - 0x08];
	TMusicPrintQ4545 m_pPrint;                   // 0x58
	void*            m_pUnknown5c;               // 0x5c
};

// Return codes.  They are locals of this module, not LTRESULTs: the caller
// at 0x004103d9 turns 1/2/3 into ltcodes.h's LT_MISSINGMUSICDLL (36),
// LT_INVALIDMUSICDLL (37) and LT_UNABLETOINITMUSICDLL (38) with three
// consecutive `dec %eax; je`, and answers LT_OK on the fall-through.  The
// spellings are ours; only the VALUES are attested.
#define MUSICDLL_OK          0
#define MUSICDLL_MISSING     1
#define MUSICDLL_INVALID     2
#define MUSICDLL_NOINIT      3

// 0x0041b880 — the client console's print entry, __cdecl (colour first, a
// word that is zero at every site, the line).  src/engine/lithtech's
// clientdsi.cpp, cltnew.cpp and clientshell.cpp all already declare it at
// this address with this shape.
void sub_41b880(uint32 nColor, int nUnknown, char* pMsg);

// Defined here: nothing outside this module touches either address, and
// both are zero-initialised (.bss past the end of the raw .data).
HINSTANCE         g_hMusicDLLQ4545;              // 0x004b40c0
MusicStructQ4545* g_pMusicStructQ4545;           // 0x004b40c4

void music_TermQ4545(void);


// ---- 0x00454f70, 58 bytes -------------------------------------------- //
//
// The printf the loader hands the DLL.  Instruction for instruction the same
// thirteen as CLTClient::CPrint (0x00407890, written in cltnew.cpp) and
// CLTServer's twin (0x0046e820, written in servermessage.cpp): same
// `sub $0x1f4` 500-byte buffer, same `push $0x1f3` one-less limit, same
// `lea 0x1fc(%esp)` va_start, same 0x0049318e, same absent va_end, same
// `add $0x210,%esp` == 0x1f4 + four _vsnprintf arguments + three of the
// printer's.
//
// TWO DIFFERENCES, and both are load-bearing:
//  * it is a FREE function, not a member: `mov 0x4(%esp),%ecx` reads the
//    format off the FIRST stack slot, where CLTClient::CPrint reads it off
//    the second because `this` is ahead of it.
//  * the colour pushed is 0x0064ff64 and not CPrint's 0xffffff.  That is
//    (0x64, 0xff, 0x64) — the green the music shim's own chatter comes out
//    in.  It is not a relocation, so the value is the source's.
void music_PrintQ4545(char* pMsg, ...)
{
	char    msg[500];
	va_list marker;

	// No va_end: neither this nor either of its two twins has one.
	va_start(marker, pMsg);
	_vsnprintf(msg, sizeof(msg) - 1, pMsg, marker);

	sub_41b880(0x0064ff64, 0, msg);
}


// ---- 0x00454fb0, 176 bytes ------------------------------------------- //
//
// WHAT PINS IT.
//  * __cdecl, two arguments, `ret` bare.  %esi is loaded from 0x0c(%esp)
//    with ONE push outstanding — argument TWO — and %edi from 0x0c(%esp)
//    with TWO outstanding — argument ONE.  So the struct is the second
//    argument and the DLL name the first, which is also the order the call
//    site pushes them in (0x004103cf `push %eax` = CClientMgr+0xf84, then
//    0x004103d0 `push %esi` = CClientMgr+0xff4).
//  * BOTH null tests jump to the SAME `mov $0x3,%eax` tail at 0x0045504d,
//    which is also the tail of the Init-failed arm — one `return` reached
//    three ways, not three returns.  So the whole load sits inside one
//    `if (pStruct && pDLLName)` and the Init-failed arm FALLS OUT of it onto
//    the single `return MUSICDLL_NOINIT`.  Two early `return 3`s (the first
//    spelling) gave two extra epilogues; an `if (Init) {return 0}` with the
//    failure after it put the success tail inline — the success arm has to
//    be the `else` of `if (!Init)` to land last, as 0x00455055 does.
//  * `call 0x00455060` before anything else is done: a load always tears the
//    previous one down first.
//  * the zeroing of +0x5c is scheduled INTO the LoadLibraryA argument push
//    (0x00454fcf `push %edi`, 0x00454fd0 the store, 0x00454fd7 the call),
//    which is scheduling; in source it precedes the load.
//  * GetLastError's result is DISCARDED — `call *0x4a10b4` then straight to
//    `mov $0x1,%eax`.  A bare call statement, kept for the debugger.
//  * the handle is stored to the global BEFORE it is tested
//    (`test %eax,%eax` at 0x00454fdd, `mov %eax,0x4b40c0` at 0x00454fdf, the
//    branch at 0x00454fe4), so the source assigns and then tests the global,
//    not a local.  Every later reader re-reads the global — 0x00455004 and
//    0x00455036 both `mov 0x4b40c0,...` — which is "an
//    expression used twice with a call between is REPEATED in source".
//  * the GetProcAddress result is never spilled: `call *%eax` at 0x00455023
//    is the same %eax GetProcAddress returned.
//  * the two failure arms that own a loaded DLL both FreeLibrary and then
//    clear the global, in that order, and both write the constant zero
//    rather than reusing a register.
//  * `test %eax,%eax; jne` on Init's answer: nonzero is success, so the
//    source tests for failure and the success arm is the one that falls out
//    of the bottom.
int music_InitQ4545(char* pDLLName, MusicStructQ4545* pStruct)
{
	TMusicSetupQ4545 pSetup;

	if (pStruct && pDLLName)
	{
		music_TermQ4545();

		pStruct->m_pUnknown5c = LTNULL;

		g_hMusicDLLQ4545 = LoadLibrary(pDLLName);

		if (!g_hMusicDLLQ4545)
		{
			GetLastError();
			return MUSICDLL_MISSING;
		}

		pSetup = (TMusicSetupQ4545)GetProcAddress(g_hMusicDLLQ4545, "MusicDLLSetup");

		if (!pSetup)
		{
			FreeLibrary(g_hMusicDLLQ4545);
			g_hMusicDLLQ4545 = LTNULL;
			return MUSICDLL_INVALID;
		}

		pSetup(pStruct);

		pStruct->m_pPrint = music_PrintQ4545;

		if (!pStruct->m_pInit(pStruct))
		{
			FreeLibrary(g_hMusicDLLQ4545);
			g_hMusicDLLQ4545 = LTNULL;
		}
		else
		{
			g_pMusicStructQ4545 = pStruct;
			return MUSICDLL_OK;
		}
	}

	return MUSICDLL_NOINIT;
}


// ---- 0x00455060, 61 bytes -------------------------------------------- //
//
// IN THE GAP: the enumeration has this row, but it is past the eight rows
// above and nothing else defines it.  It is music_Init's other half and could
// not be written without it.
//
// WHAT PINS IT.
//  * no arguments, no return, `ret` bare, frameless, no saved registers: one
//    `%eax` does the whole job.
//  * the struct global is read, tested, used — and then READ AGAIN
//    (0x00455060 and 0x0045506c are the same `mov 0x4b40c4,%eax`) across the
//    Term call.  An expression used twice with a call between is
//    repeated in source.  Written as two reads of the global, not a local.
//  * `call *0x4(%eax)` with NO argument pushed and no stack cleanup, so slot
//    +0x04 really takes none.  Contrast +0x00 in music_Init, which is pushed
//    the struct.
//  * the two teardowns are SEPARATE `if`s, not one: the DLL handle is freed
//    even when no struct was installed, and the struct's is a `je` past its
//    whole block to the handle test rather than to the `ret`.
void music_TermQ4545(void)
{
	if (g_pMusicStructQ4545)
	{
		g_pMusicStructQ4545->m_pTerm();

		g_pMusicStructQ4545->m_pUnknown5c = LTNULL;
		g_pMusicStructQ4545 = LTNULL;
	}

	if (g_hMusicDLLQ4545)
	{
		FreeLibrary(g_hMusicDLLQ4545);
		g_hMusicDLLQ4545 = LTNULL;
	}
}


// ---- 0x004550a0 / 0x004550b0, 5 + 1 bytes ---------------------------- //
//
// IN THE GAP, and a PAIR: 0x004550a0 is a five-byte `jmp 0x004550b0` plus
// eleven alignment nops, and 0x004550b0 is a bare `ret` plus fifteen.
//
// WHAT THEY ARE.  0x004550a0 is this TU's DYNAMIC-INITIALIZER thunk.  The
// dword at 0x004a70b0 that points at it is slot 44 of the table that starts
// at 0x004a7004 and ends at 0x004a70d8 — and 0x004a7000 and 0x004a70d8 are
// both ZERO, which is .CRT$XCA and .CRT$XCZ bracketing .CRT$XC.  Every one
// of the 53 live entries points at a five-byte `jmp` with eleven nops after
// it, which is what VC6's `??__E<name>` looks like at /O2 when it tail-calls
// the real initializer.  The one already read elsewhere proves the shape:
// src/engine/lithtech/lt2new1.cpp's 0x0045a450 is slot 45, and it jumps to
// 0x0045a460, which ties off the LTLink at 0x004b40c8 — a file-scope list
// head's constructor.
//
// So the music TU has exactly one file-scope object needing dynamic
// initialization, and its initializer is EMPTY.
//
// UNDECODED: which object, and what its class is.  The thunk carries no
// `mov $<addr>,%ecx` ahead of the jump, so the body it reaches takes no
// `this`, and the body itself is one `ret` — there is nothing in the bytes
// to name either.  Both are written empty below and only the PAIRING and the
// .CRT$XC membership are claimed.  They must stay in this TU and in this
// order for the table slot to land at 0x004a70b0.
void music_InitSegQ4545(void)
{
}

// ??__E for the object above.  Writing it as a call is what produces the
// tail `jmp`; VC6 does not expand it because the initializer table takes its
// address.
void music_DynInitQ4545(void)
{
	music_InitSegQ4545();
}


// ======================================================================= //
//  C.  0x00454530..0x00454a35, 1285 bytes — wm_RotateWorldModel
//      (rows 0x00454530 and 0x0045465a are ONE function)
//
//  NOW WRITTEN IN src/engine/server/commonphysics.cpp as wm_RotateWorldModel
//  (byte-identical on server.dll 0x10020a53; the counter question below is
//  settled there by `#ifdef LT_SERVER_TIMERS` round the CountAdder).  The
//  staged Q4545 copy that stood here was removed: against the true extent
//  (--end=0x454a35) it measured 1240 bytes / 30 mnemonic blocks, where
//  commonphysics.cpp's body built for lithtech measures 1278 / 25 against
//  the shipped 1285, so nothing was lost.  Neither pairs: the row for
//  0x00454530 in the function enumeration says 298 bytes (see the
//  extents table).  The remaining gap on the client is the two expansions
//  noted below (CWMMoveSetup::Setup and obj_MaybeRetransformWorldModel);
//  marking them `inline` in commonphysics.cpp was measured and costs
//  server.dll six functions, so the client's inlining has another cause.
//  The analysis below is kept as the lithtech-side record.
//
//  servermgr.h already DECLARES it, from the server's address:
//      void wm_RotateWorldModel(WMMoveState*, LTRotation*, int);  // 0x10020a53
//  and nothing had ever read the body.  Every one of the six helpers it
//  reaches is a named member of commonphysics.cpp's own cluster, at the
//  client addresses that file's index table lists.
//
//  WHAT IT DOES: turn a world model to a new rotation and take everything it
//  was carrying, and everything it now overlaps, round with it.  The object
//  is re-rotated first (its dims re-derived from the transformed geometry and
//  its box re-seated in the container), then the rotation is expressed as the
//  4x4
//       T(+pos) * R(new) * R(old)^T * T(-pos)
//  and every object standing on it is moved to where that 4x4 puts it.
//
//  HOW IT WAS READ.  Instruction by instruction against the server's /O1 copy
//  at 0x10020a53, which keeps an %ebp frame; every statement below is visible
//  in both.  Six things the client INLINES that the server calls out of line —
//  all six are /Ot-vs-/Os, and all six are already-reconstructed bodies, so
//  each one is a cross-check rather than a guess:
//
//    server 0x1001efd8  CWMMoveSetup::Setup  -> client `call *0x24(%eax)`
//    server 0x10011dc5  LTMatrix::Init       -> client 16 constant stores
//    server 0x10020e08  MatTranspose3x3      -> client six loads/stores
//    server 0x1001eb0c  obj_MaybeRetransform -> client `cmp $2/$9` + 0x450c30
//    server 0x1003e280  memset               -> client `rep stos` of 9 dwords
//    server 0x1000f878  MatVMul_H            -> client the whole fmul chain
//
//  FRAME ARITHMETIC.  `sub $0x420,%esp` = 1056, four pushes below it.  Write
//  C for the esp the two outstanding quaternion-call pushes leave (E-0x438);
//  every displacement in the banner is C-relative.  It decomposes with
//  NOTHING left over:
//
//     C+0x018  LTVector vPos          (12; later reused for vDest)
//     C+0x024  void*    pSavedRequest (4)
//     C+0x028  LTLink*  the list head spill (4)
//     C+0x02c  LTMatrix matRotation   (0x40)
//     C+0x06c  LTMatrix matFromOrigin (0x40)
//     C+0x0ac  LTMatrix matToOrigin   (0x40)
//     C+0x0ec  LTMatrix matObjRot     (0x40)
//     C+0x12c  WMMoveState state      (0x68)
//     C+0x194  CWMRequestRec rec      (0x24)
//     C+0x1b8  LTMatrix matNewRot     (0x40)
//     C+0x1f8  LTMatrix matTemp       (0x40)
//     C+0x238  CWMStandRec found[0x20](0x200, ending exactly at E)
//
//  THE COUNTER — READ THIS BEFORE MOVING THE BODY INTO commonphysics.cpp.
//  The server's copy opens with BOTH halves of commonphysics.cpp's counted-
//  move idiom:
//
//      g_Unknown58728++;
//      CountAdder cntAdder((uint32*)&g_Unknown5872c);
//
//  — `incl 0x10058728` at 0x10020a60, then CBaseCounter(0) at 0x10020a80,
//  `m_pTotal = 0x1005872c` at 0x10020a88, cnt_StartCounter at 0x10020a90 and
//  ~CountAdder at 0x10020dfe.  THE CLIENT HAS THE COUNTER AND NOT THE
//  CountAdder: 0x00454536..0x0045454e is `mov 0x4b4070,%ecx; inc %ecx;
//  mov %ecx,0x4b4070` and there is no constructor, no cnt_StartCounter and no
//  destructor anywhere in the 1285 bytes — the single epilogue at 0x00454a2a
//  is four pops and an `add`.
//
//  This is not an optimisation difference: cnt_StartCounter reaches
//  QueryPerformanceCounter and cannot be optimised away.  And it is not
//  isolated — the client's twin of sm_MoveObject, 0x0047c5b0, does exactly the
//  same thing: it increments 0x004b4070 (and 0x004b4074 is its CountAdder
//  total, cleared beside it at 0x004742ef) and then goes straight to the six
//  field stores and wm_MoveObject with a `sub $0x68` frame that has no room
//  for a counter.  commonphysics.cpp's already-written sm_MoveObject HAS the
//  CountAdder.
//
//  So the shared TU is compiled into the two targets with the scoped timer
//  present in one and absent in the other, and whatever selects it is a
//  preprocessor condition this reconstruction has not found.  Until that is
//  settled, this body is the CLIENT's and is written WITHOUT the CountAdder;
//  adding the line gives the server and loses lithtech.exe.
//
//  EVERYTHING ELSE THE BYTES PIN.
//   * __cdecl, three arguments, `ret` bare.  arg1 is read at 0x0045453d as
//     0x428(%esp) with one push outstanding (= E+4); arg3 at 0x004547ad as
//     0x43c(%esp) with the frame restored (= E+0xc); arg2 at 0x0045474e as
//     0x440(%esp) with two outstanding (= E+8).
//   * THE TWO EARLY-OUTS ARE AFTER THE ROTATION, not before it.  BumpObject
//     is called at 0x004547a0 and only then are `bUnknown` and the flag
//     tested — so the object really is re-rotated and re-seated whatever the
//     third argument says, and the argument only gates carrying things round
//     with it.
//   * 0x4000 is FLAG_BOXPHYSICS.  ltbasedefs.h gives the same bit two names
//     (FLAG_NOGLOBALLIGHTSCALE is particle-system-only) and the immediate is
//     not a relocation, so the bytes cannot separate them — but the same bit
//     of the same word is what obj_RetransformWorldModel (0x00450c30) shifts
//     out with `shr $0xe; and $1` to choose its own box-vs-geometry path two
//     statements earlier, which is FLAG_BOXPHYSICS's whole meaning.
//   * the three MatMuls' argument order is fixed by the push order and gives
//     matRotation = T(+pos) * R(new) * R(old)^T * T(-pos), with matRotation
//     used as both destination and source of the third — which is why it is
//     one variable and not three.
//   * _v04() is CALLED TWICE back to back (0x0045481c and 0x0045482a), once
//     to read the slot and once to write it, and a THIRD time at 0x00454a1c
//     to put it back.  Three calls, so three spellings of the same chain in
//     source; caching it in a local gives one.
//   * the carry list is the same loop wm_MoveObject already has, cap and all,
//     INCLUDING THE SHIPPED OFF-BY-ONE: `cmp $0x20,%ebp; jge` admits an index
//     of 0x20 into an array of 0x20.  The frame proves the size — found[]
//     runs C+0x238..C+0x438 and C+0x438 is E, the return address.  Written as
//     shipped, exactly as commonphysics.cpp writes it in wm_MoveObject.
//   * the record's offset here is the object's POSITION, not a delta:
//     `add $0x18c,%eax` then three plain dword copies, where wm_MoveObject's
//     twin subtracts the mover's position first.
//   * the push list is a caller-supplied ARRAY: pState+0x10 is tested for
//     null and then indexed `(%edx,%esi,4)` against the unsigned count at
//     pState+0x14, both re-read from pState every iteration.  When it is null
//     the container's own box query runs instead with 0x00454f00 as the
//     per-object callback.  commonphysics.h has both fields as uint32 because
//     everything that had been read so far only ever zeroed them.
//   * the carry loop is a DOWN-COUNTING SUBSCRIPT (`lea 0x1(%ebp),%edi` for
//     the trip count and `shl $4` for the cursor, both computed before the
//     first iteration), the same form commonphysics.cpp had to use for
//     wm_MoveObject's carry loop and for the same reason.
//   * 0.5f (0x004a1a74) and 1.0f (0x004a1a08) are the only two constants; the
//     0.5 comes off the y of the rotated position, so the carried object is
//     dropped half a unit before the move is asked for.
// ======================================================================= //

// ======================================================================= //
//  D.  0x00454a40..0x00454efb, 1211 bytes — ALREADY RECONSTRUCTED
//      (rows 0x00454a40 and 0x00454c49 are ONE function)
//
//  This is wm_RotatePushObject, server 0x10020e31, and its body is written in
//  full in src/engine/server/commonphysics.cpp (search for
//  `void wm_RotatePushObject`).  NOT rewritten here.  It is the shared-TU
//  blind spot an address search cannot see: commonphysics.cpp compiles
//  into lithtech.exe through `..\server\commonphysics.cpp` and is annotated
//  with server.dll addresses throughout, so no 0x0045xxxx citation exists for
//  it anywhere in the tree.
//
//  VERIFIED against the client bytes statement by statement, not assumed.
//  Eleven checks, every one of them passing:
//
//    0x00454a73  `call *0x34(%eax)`      TakesPartInPush -> return
//    0x00454a91  `push $0x3c23d70a`      0.01f, the wm_BoxesOverlap tolerance
//    0x00454aac  call 0x00451690         wm_BoxesOverlap
//    0x00454acd  call 0x00417840         bsp_IntersectBox (GetTransformedBsp
//                                        through m_pWMIface slot 1 first)
//    0x00454ae0  `mov $0x2000,%eax`      FLAG_SOLID, tested on BOTH ends
//    0x00454ee8  call 0x00451d70         wm_NotifyContact (the not-solid arm,
//                                        which is the row 0x00454c49 defect:
//                                        two branches from the row above land
//                                        here and four more land on the shared
//                                        epilogue at 0x00454ef0)
//    0x00454c13  `fcomps 0x004a1a54`     0.001f, the no-rotation early-out
//    0x00454c7a  call 0x00436760         _CVector<float>::Norm, then
//                                        `fcoms 0.0f` / `fdivrs 1.0f`
//    0x00454cb2  `mov $0xc7,%ebp`        199 steps
//    0x00454e32  call 0x004524e0         wm_MoveThroughSim
//    0x00454e70  call 0x004524b0         CWMStateInit::Init, then
//    0x00454e98  call 0x00451000         wm_MoveObject(&move, vObjPos, 7)
//    0x00454ed4  `call *0x1c(%eax)`      _v1c(pObj, pState->m_pObj)
//
//  commonphysics.cpp records it as a NEAR against the server's copy (799
//  bytes of ours against 811 shipped, one twelve-byte local missing).  The
//  client's copy is 1211 bytes because /O2 inlines what /O1 calls; nothing
//  about the source changes.
//
//
//  E.  0x00454f00, 31 bytes — ALREADY RECONSTRUCTED
//
//  Now commonphysics.cpp's wm_RotatePushFilter (byte-identical on both
//  images); lt2new1.cpp's `sub_454f00` duplicate has been removed.  Its body
//  was:
//
//      if (!pItem->m_n54 && pItem != pHolder->m_pCurrent)
//          sub_454a40(pHolder, pItem);
//
//  NOT rewritten here.  Two NAMING corrections now that its caller has been
//  read — neither changes a byte:
//
//   * it is not an anonymous pair of views.  Argument 1 is an LTObject and
//     the +0x54 it gates on is ltobject_internal.h's `m_Unknown54`
//     ("nonzero excludes the object from SFX broadcasts"); argument 2 is a
//     WMMoveState and the +0x08 it compares against is m_pObj.  So the whole
//     function is "push this object unless it is excluded, or unless it is
//     the world model doing the pushing".
//   * servermgr.h's WMMoveState carries the comment "0x54 — a gate the
//     per-object push filter (0x1002115c) tests for zero before it will push
//     anything at all", attributing the field to the MOVE STATE.  The bytes
//     say otherwise: `mov 0x4(%esp),%eax; mov 0x54(%eax),%ecx` reads +0x54
//     off argument ONE, and argument one is the object the container's box
//     query hands the callback.  WMMoveState::m_nUnknown54 may well exist,
//     but this function is not the evidence for it.
//
//  Its true home is this physics TU, not lt2new1.cpp: it is the callback
//  0x004548ed pushes.  Name, once moved: wm_RotatePushFilter (ours).
//
//
// ======================================================================= //
//  INDEX AND EXTENTS
//
//  addr        stated  true    what it is                        status
//  ----------  ------  ------  --------------------------------  ----------
//  0x00454530     298    1285  wm_RotateWorldModel               commonphysics
//  0x0045465a     987      --  NOT A FUNCTION: the row cuts the
//                              store at 0x00454652 in half
//  0x00454a40     521    1211  wm_RotatePushObject               ALREADY, D
//  0x00454c49     690      --  NOT A FUNCTION: the not-solid arm
//                              and shared epilogue of the above
//  0x00454f00      31      31  wm_RotatePushFilter               ALREADY, E
//  0x00454f20      67      67  _CVector<float>::operator/(float) NEW, A
//  0x00454f70      58      58  music_Print                       NEW, B
//  0x00454fb0     176     176  music_Init                        NEW, B
//  ---- in the gap, unclaimed, written here --------------------------------
//  0x00455060      61      61  music_Term                        NEW, B
//  0x004550a0       5       5  .CRT$XC dynamic-initializer thunk NEW, B
//  0x004550b0       1       1  its empty initializer             NEW, B
//  ---- in the gap, NOT claimed --------------------------------------------
//  0x004550c0      97      97  a 16-bit bit walk, one caller at
//                              0x00458866; a different module
//
//  Six real functions in the eight rows, not eight; two of the six were
//  already written.
//
//
//  COIN FLIPS I COULD NOT SETTLE
//
//  1. wm_RotateWorldModel's two early-outs.  Written as two `return`s,
//     because /O2 merges the one epilogue and both `je`/`jne` land on it.
//     The alternative — one `if (bUnknown && !(flags & FLAG_BOXPHYSICS))`
//     wrapping the whole second half — gives the same two branches to the
//     same label and is what the SERVER's copy needs, because its CountAdder
//     destructor forces a common exit.  Nothing in the client bytes chooses.
//
//  2. wm_RotateWorldModel's object pointer.  Written as `pState->m_pObj`
//     everywhere, which is the house style commonphysics.cpp already uses in
//     wm_MoveObject.  The shipped code keeps it in %esi across seven uses and
//     then RE-READS it at 0x004547a0 and again at 0x004548dd — consistent
//     with the repeated chain, but a local `LTObject* pObj` would also be
//     kept in a callee-saved register. If this comes back a near with the
//     loads in the wrong places, the local is the probe.
//
//  3. Where `vDest.y -= 0.5f` sits.  Written immediately after MatVMul_H.
//     The shipped schedule has the subtraction AFTER all six state stores
//     (0x004549ee) in the client and BEFORE them (0x10020d95) in the server,
//     so it is being scheduled in both and neither position is the source's.
//     The alternative is to put it just above the wm_MoveObject call.
//
//  4. 0x00004000.  FLAG_BOXPHYSICS and FLAG_NOGLOBALLIGHTSCALE are the same
//     bit in ltbasedefs.h and the immediate is not a relocation.  Written as
//     FLAG_BOXPHYSICS; see the banner for why.  Alternative, byte-identical:
//         if (pState->m_pObj->m_Flags & FLAG_NOGLOBALLIGHTSCALE) return;
//
//  5. music_Init's two null guards.  Written as two `if`s because the shipped
//     order is argument TWO then argument ONE.  `if (!pStruct || !pDLLName)`
//     gives the same bytes; `if (!pDLLName || !pStruct)` does not.
//
//  6. _CVector<float>::operator/ is a template COMDAT, so the class below is
//     a stand-in.  What should actually be compiled is the SDK's own
//     ltvector.h line 65, forced out of line by taking its address, the way
//     lt2new1.cpp forces MagSqr (0x004316b0).  Unlike MagSqr, its 67 bytes
//     are NOT present verbatim in the retail server.dll -- searched whole-image,
//     zero hits -- because the server's copy of the same template keeps an
//     %ebp frame at /O1.  Same source, different compile.
// ======================================================================= //
