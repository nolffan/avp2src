// *********************************************************************** //
//
//  MODULE:   q438000 (.CPP)   -- lithtech.exe only, scratch reconstruction
//
//  PURPOSE:  Two unrelated pieces of the client that happen to share the
//            0x00438000..0x0043c000 address window:
//
//              * 0x00438470 -- the DirectInput device-object binder.  This
//                belongs with src/engine/lithtech/clientinput.cpp; every
//                type and global below is that file's, re-declared here with
//                a Q438 suffix so the two objects cannot collide at link
//                time.
//
//              * 0x0043a0e0 -- the scaled-blit rectangle clipper of the
//                client's surface module (the "cis_DrawWarp" cluster that
//                runs 0x00439fc0..0x0043c440).  Nothing of ours covers it.
//
//  EVERY constant below is read off the retail lithtech.exe and cited by
//  the address it was read from.  Names that are not pinned by a literal or
//  by an SDK header say so.
//
// *********************************************************************** //

#include <windows.h>
#include <dinput.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ltbasedefs.h"
#include "ltrect.h"

// windows.h renames engine methods out from under the SDK headers.
#undef PostMessage
#undef GetMessage
#undef PeekMessage
#undef CopyFile
#undef PlaySound
#undef GetClassName


// ======================================================================= //
//  PART ONE -- 0x00438470, the device-object binder
//
//  WHAT IDENTIFIES IT
//
//  The function is reached from exactly one call site, 0x0043821c, inside
//  the 0x004381a0 entry that prints "Bound %s (on %s) to action %s".  That
//  caller first tries di_FindDeviceObject (0x00438140, already written in
//  clientinput.cpp) with the same two arguments and only calls this when
//  the lookup fails -- so this is "find or create", and this half is the
//  create.
//
//  It enumerates the DirectInput device's objects through a matcher
//  callback at 0x00438ad0 (NOT reconstructed here) that leaves its answer in
//  a block of file-scope globals, then allocates a 0x144-byte record, copies
//  the matched DIDEVICEOBJECTINSTANCE into it and pushes it onto the
//  device's circular object list.
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  The record.  0x144 bytes, proven by the `operator new` argument at
//  0x00438926 and by the `rep stos` of 0x51 dwords at 0x0043894e.
//
//  Offsets, every one of them a load or a store in the shipped image:
//
//    +0x000  m_pPrev            0x00438963 / 0x0043898f
//    +0x004  m_pNext            0x00438965 / 0x0043898c, and the walk in
//                               di_FindDeviceObject (0x00438140)
//    +0x008  m_Name[0x64]       strncpy target, count 0x64, 0x004389b9
//    +0x06c  m_TriggerName[0x64] strncpy target, count 0x64, 0x004389af
//    +0x0d0  m_dwType           0x004389fb, the DIDEVICEOBJECTINSTANCE
//                               dwType, re-read as DIPROPHEADER::dwObj
//    +0x0d4  m_guidType         four-dword copy at 0x004389d9..0x004389f3
//    +0x0e4  m_ObjectType       0x004389c4, the 0..4 code below
//    +0x0e8  read at 0x00437cc3 / written at 0x00437cd2 -- unnamed
//    +0x0ec  the current value  (fildl at 0x004374f7)
//    +0x0f0  set to 1 by the constructor (0x0043893d)
//    +0x108  final scale        fmuls at 0x00437617
//    +0x10c  \ pair, defaults (0,0)
//    +0x110  /
//    +0x114  \ scale/offset pair applied third   (0x0043760b/0x00437611)
//    +0x118  /
//    +0x11c  \ scale/offset pair applied first   (0x004375ef/0x004375f5)
//    +0x120  /
//    +0x124  \ scale/offset pair applied second  (0x004375fd/0x00437603)
//    +0x128  /
//    +0x12c  \ compared against each other at 0x004375cf/0x004375e2
//    +0x130  /
//    +0x134  m_pDevice          0x00438a06
//    +0x138  the bound action   0x0043839b, 0x00437624
//    +0x13c  m_fMin             0x00438aab
//    +0x140  m_fMax             0x00438ab5
//
//  The names of the three scale/offset pairs are NOT pinned -- only the
//  order they are applied in at 0x004375ef..0x0043761d is -- so they are
//  numbered by that order and the comment says which offset each is.
// ----------------------------------------------------------------------- //

class CDeviceObjectQ438;

// The object list embedded at InputDevice+0x84: vptr, count, head.  Exactly
// the class clientinput.cpp declares; repeated here for this TU.
class CDeviceObjectListQ438
{
public:
	CDeviceObjectListQ438();

	virtual void GetHead(void* pIterOut);    // 0x00439fc0

	uint32             m_nCount;             // +0x04
	CDeviceObjectQ438* m_pHead;              // +0x08
};

class CDeviceObjectQ438
{
public:
	CDeviceObjectQ438();

	CDeviceObjectQ438* m_pPrev;              // 0x000
	CDeviceObjectQ438* m_pNext;              // 0x004
	char               m_Name[0x64];         // 0x008
	char               m_TriggerName[0x64];  // 0x06c
	uint32             m_dwType;             // 0x0d0
	GUID               m_guidType;           // 0x0d4
	uint32             m_ObjectType;         // 0x0e4
	uint32             m_Unknown0e8;         // 0x0e8
	int32              m_nValue;             // 0x0ec
	uint32             m_Unknown0f0;         // 0x0f0
	uint8              m_pad_0f4[0x108 - 0x0f4];
	float              m_fScale;             // 0x108
	float              m_fUnknown10c;        // 0x10c
	float              m_fUnknown110;        // 0x110
	float              m_fScale3;            // 0x114   applied third
	float              m_fOffset3;           // 0x118
	float              m_fScale1;            // 0x11c   applied first
	float              m_fOffset1;           // 0x120
	float              m_fScale2;            // 0x124   applied second
	float              m_fOffset2;           // 0x128
	float              m_fUnknown12c;        // 0x12c
	float              m_fUnknown130;        // 0x130
	struct InputDeviceQ438* m_pDevice;       // 0x134
	void*              m_pAction;            // 0x138
	float              m_fMin;               // 0x13c
	float              m_fMax;               // 0x140
};

// The constructor is inlined at 0x00438933..0x00438943 and, exactly as with
// InputDevice's in clientinput.cpp, the memset that follows wipes it again.
// That is shipped behaviour, not a misreading.
inline CDeviceObjectQ438::CDeviceObjectQ438()
{
	m_nValue     = 0;
	m_Unknown0f0 = 1;
}

struct InputDeviceQ438
{
	LPDIRECTINPUTDEVICE2A  m_pDevice;        // 0x000
	char*                  m_pAliasName;     // 0x004
	uint32                 m_Unknown008;     // 0x008
	char                   m_Name[0x64];     // 0x00c
	GUID                   m_Guid;           // 0x070
	uint32                 m_dwDevType;      // 0x080
	CDeviceObjectListQ438  m_Objects;        // 0x084
	uint8                  m_pad_090[0x304 - 0x090];
};


// ----------------------------------------------------------------------- //
//  The matcher's parameter block.
//
//  These are file-scope globals, not locals: 0x00438470 writes them and the
//  DIEnumDeviceObjects callback at 0x00438ad0 reads them.  What each one is
//  comes from the callback:
//
//    0x004b352c  tested first; when zero the callback matches on the
//                object's tszName instead (0x00438ad0 / 0x00438b67).
//    0x004b3530  when set, the callback `repz cmpsl`s four dwords of
//                lpddoi->guidType against 0x004b34e8 (0x00438af1).
//    0x004b34e4  compared against lpddoi->dwOfs, -1 meaning "any"
//                (0x00438b03 / 0x00438b0e -- +0x14 is dwOfs).
//    0x004b3524  compared against DIDFT_GETINSTANCE(lpddoi->dwType), i.e.
//                `(dwType >> 8) & 0xffff`, -1 meaning "any" (0x00438b44).
//    0x004b3528  the callback's "I found one" flag (0x00438b23).
//    0x004b3538  the 0x4f-dword (316-byte) copy of the matched
//                DIDEVICEOBJECTINSTANCEA (0x00438b2d).  316 is
//                sizeof(DIDEVICEOBJECTINSTANCEA) for DirectX 7, and the
//                member offsets the two functions use line up exactly:
//                guidType 0x004b353c, dwOfs 0x004b354c, dwType 0x004b3550,
//                tszName 0x004b3558.
//
//  0x004b3524 sits 0x2c bytes past g_ActionList (0x004b34f8), which is the
//  size clientinput.cpp already derived for CInputAction -- a free
//  consistency check on this whole block.
// ----------------------------------------------------------------------- //

// DEFINED HERE: clientinput.cpp (their real home, the 0x004b34e4..0x004b368c
// .bss run) references none of them -- only this file's binder and matcher
// do.  Uninitialised in the image.
int32                    g_MatchOfsQ438;        // 0x004b34e4
GUID                     g_MatchGuidQ438;       // 0x004b34e8
int32                    g_MatchInstanceQ438;   // 0x004b3524
LTBOOL                   g_bMatchFoundQ438;     // 0x004b3528
LTBOOL                   g_bMatchByTypeQ438;    // 0x004b352c
LTBOOL                   g_bHaveMatchGuidQ438;  // 0x004b3530
DIDEVICEOBJECTINSTANCEA  g_MatchInfoQ438;       // 0x004b3538

// 0x00438ad0 -- the matcher callback, defined below di_AddDeviceObjectQ438.
BOOL PASCAL di_MatchObjectCallbackQ438(LPCDIDEVICEOBJECTINSTANCEA lpddoi,
                                       LPVOID pvRef);


// ----------------------------------------------------------------------- //
//  0x00438470 -- bind one DirectInput object on one device.
//
//  WHAT PINS THE SIGNATURE
//    * `add $0x8,%esp` at the call site 0x00438223 -> __cdecl, two args.
//    * arg1 (0x24(%esp) at 0x0043847c) is the string: it is the stricmp
//      subject, the EnumObjects pvRef, and the strncpy source for +0x6c.
//    * arg0 (0x2c(%esp) at 0x004388be) is dereferenced at +0x00 for the
//      IDirectInputDevice2 and at +0x88/+0x8c for the object list, so it is
//      the InputDevice.
//    * the return is the new record in eax (0x00438abb), zero on every
//      failure.
//
//  WHAT PINS THE "##" PARSE
//    The two byte loads at 0x00438482/0x0043848d and the `cmp $0x23,%al`
//    at 0x00438498 are the four tests, in that order.  The second one is
//    `cmp %al,%cl` -- register against register -- so the source compares
//    pName[1] against pName[0] and not against a second '#' literal.  (If
//    VC6 propagated the known 0x23 into the second compare the other
//    spelling would produce the same byte; this is the literal reading.)
//
//  WHAT PINS THE GUIDS
//    0x004a3b0c/0x004a3afc/0x004a3aec/0x004a3aac/0x004a3a9c/0x004a3a7c hold
//    {A36D02E0,E1,E2,E4,F0,F2}-C9F3-11CF-BFC7-444553540000, i.e.
//    GUID_XAxis, GUID_YAxis, GUID_ZAxis, GUID_Slider, GUID_Button and
//    GUID_POV as dinput.h declares them.
//
//  WHAT PINS THE OFFSETS
//    0xc/0xd/0xe are DIMOFS_BUTTON0..2 (FIELD_OFFSET(DIMOUSESTATE,
//    rgbButtons)+n = 12+n); 0x18/0x1c are DIJOFS_SLIDER(0)/(1)
//    (FIELD_OFFSET(DIJOYSTATE, rglSlider) = 24, stride 4); 0x20..0x2c are
//    DIJOFS_POV(0..3) (FIELD_OFFSET(DIJOYSTATE, rgdwPOV) = 32, stride 4).
//    The three axis names set no offset at all -- it stays at the -1 the
//    prologue wrote -- which is why the arms are not uniform.
//
//  WHAT PINS THE LAST ARM
//    0x004935bc masks the ctype table with 0x107 == _ALPHA|_DIGIT, so it is
//    `isalnum`, not `isdigit`; 0x004937a7 is the one-argument forwarder to
//    0x0049371c that ltnew2.cpp and lt2new3.cpp already identified as
//    `atoi`.
//
//  WHAT PINS THE 0..4 CODE
//    The byte at 0x004b3550 is lpddoi->dwType and the five tests at
//    0x004388f3..0x0043891b are DIDFT_PSHBUTTON (4), DIDFT_TGLBUTTON (8),
//    DIDFT_RELAXIS (1), DIDFT_ABSAXIS (2) and DIDFT_POV (0x10) in that
//    order, yielding 0, 1, 3, 2 and 4.  The names of the five codes are NOT
//    pinned by anything in the image -- they are not the SDK's CONTROLTYPE_
//    values, which run 0..10 -- so they are left as literals with this
//    comment.  0x00437a99 compares +0xe4 against 3, which is consistent
//    with 3 being the relative axis.
//
//  WHAT PINS THE RANGE QUERY
//    DIPROPRANGE is 24 bytes and DIPROPHEADER 16, which is exactly the
//    0x18/0x10 pair stored at 0x00438a7d/0x00438a85; dwHow 2 is DIPH_BYID,
//    which is why dwObj is the dwType and not the dwOfs; and the rguidProp
//    pushed as the immediate 4 is DIPROP_RANGE == MAKEDIPROP(4).  The
//    0xcf000000/0x4f000000 defaults it overwrites are -2147483648.0f and
//    +2147483648.0f.
//
//  WHERE IT STANDS (row now 1631 bytes: ship 1631 / 386 insns, ours
//  1637 / 385, two mnemonic blocks)
//    Settled by measurement:
//      * each GUID arm raises g_bHaveMatchGuidQ438 BEFORE the GUID copy --
//        shipped slots that store after the first GUID dword (0x004384f6);
//        written after the copy it lands after all four;
//      * the list insert caches the head in a local for its four splice
//        stores (0x00438965 stores %eax, the head, into pNew->m_pNext
//        without re-reading 0x8c(%ebp); only head->m_pPrev is re-read), and
//        re-reads m_pHead for the count/compare -- the inlined
//        "insert before this position" helper, with the head as position;
//      * the empty-list arm sets m_pHead before bumping m_nCount (shipped
//        load/store-head/inc/store at 0x00438991; the other order folds
//        to one `incl 0x88(%ebp)`, 13 bytes shorter and not shipped).
//    Left: ONE materialisation choice.  The shipped prologue puts the
//    constant zero in a register and keeps it:
//        0x00438473  xor  %edx,%edx
//        0x00438475  mov  %edx,0x4b352c      g_bMatchByType   = LTFALSE
//        0x004384c9  mov  %edx,0x4b3530      g_bHaveMatchGuid = LTFALSE
//    where ours spends `movl $0x0,<global>` at both -- 20 bytes against 14,
//    the whole 6-byte gap.  No effect: the order of the four # block stores
//    (VC6 reschedules them to the same slots), `pName[0] &&` spelling,
//    pType initialised at its declaration, the # parse as an inline helper
//    (with or without the first store), cl 8168 and 8966.  A second zero
//    store in the entry block DOES put zero in a register, but a
//    callee-saved one (%ebx, with `cmp %bl,%al` for the NUL tests) and 18
//    bytes long -- shipped's zero is a caller-saved %edx dead at the first
//    stricmp.  Because the gap sits in the first ten bytes, the scorer's
//    positional pairing cannot see this as a near.
// ----------------------------------------------------------------------- //

CDeviceObjectQ438* di_AddDeviceObjectQ438(InputDeviceQ438* pDevice,
                                          char* pName)
{
	CDeviceObjectQ438* pObject;
	char*              pType;
	uint32             nObjectType;
	DIPROPRANGE        range;

	g_bMatchByTypeQ438 = LTFALSE;

	if (pName[0] != 0 && pName[1] != 0 &&
	    pName[0] == '#' && pName[1] == pName[0])
	{
		pType = pName + 2;

		g_bMatchByTypeQ438   = LTTRUE;
		g_MatchOfsQ438       = -1;
		g_MatchInstanceQ438  = -1;
		g_bHaveMatchGuidQ438 = LTFALSE;

		if (stricmp(pType, "x-axis") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_XAxis;
		}
		else if (stricmp(pType, "y-axis") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_YAxis;
		}
		else if (stricmp(pType, "z-axis") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_ZAxis;
		}
		else if (stricmp(pType, "Button 0") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_Button;
			g_MatchOfsQ438       = DIMOFS_BUTTON0;
		}
		else if (stricmp(pType, "Button 1") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_Button;
			g_MatchOfsQ438       = DIMOFS_BUTTON1;
		}
		else if (stricmp(pType, "Button 2") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_Button;
			g_MatchOfsQ438       = DIMOFS_BUTTON2;
		}
		else if (stricmp(pType, "Slider 0") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_Slider;
			g_MatchOfsQ438       = DIJOFS_SLIDER(0);
		}
		else if (stricmp(pType, "Slider 1") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_Slider;
			g_MatchOfsQ438       = DIJOFS_SLIDER(1);
		}
		else if (stricmp(pType, "POV 0") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_POV;
			g_MatchOfsQ438       = DIJOFS_POV(0);
		}
		else if (stricmp(pType, "POV 1") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_POV;
			g_MatchOfsQ438       = DIJOFS_POV(1);
		}
		else if (stricmp(pType, "POV 2") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_POV;
			g_MatchOfsQ438       = DIJOFS_POV(2);
		}
		else if (stricmp(pType, "POV 3") == 0)
		{
			g_bHaveMatchGuidQ438 = LTTRUE;
			g_MatchGuidQ438      = GUID_POV;
			g_MatchOfsQ438       = DIJOFS_POV(3);
		}
		else if (isalnum(pType[0]))
		{
			g_MatchInstanceQ438 = atoi(pType);
		}
	}

	g_bMatchFoundQ438 = LTFALSE;

	if (pDevice->m_pDevice->EnumObjects(di_MatchObjectCallbackQ438,
	                                    pName, 0) != DI_OK)
	{
		return LTNULL;
	}

	if (!g_bMatchFoundQ438)
		return LTNULL;

	// The 0..4 code.  See the header comment: these are the DIDFT_ bits of
	// the matched object's dwType, tested in this order.
	if (g_MatchInfoQ438.dwType & DIDFT_PSHBUTTON)
		nObjectType = 0;
	else if (g_MatchInfoQ438.dwType & DIDFT_TGLBUTTON)
		nObjectType = 1;
	else if (g_MatchInfoQ438.dwType & DIDFT_RELAXIS)
		nObjectType = 3;
	else if (g_MatchInfoQ438.dwType & DIDFT_ABSAXIS)
		nObjectType = 2;
	else if (g_MatchInfoQ438.dwType & DIDFT_POV)
		nObjectType = 4;
	else
		return LTNULL;

	pObject = new CDeviceObjectQ438;

	memset(pObject, 0, sizeof(CDeviceObjectQ438));

	if (pDevice->m_Objects.m_pHead)
	{
		CDeviceObjectQ438* pPos = pDevice->m_Objects.m_pHead;

		pPos->m_pPrev->m_pNext = pObject;
		pObject->m_pPrev = pPos->m_pPrev;
		pObject->m_pNext = pPos;
		pPos->m_pPrev = pObject;

		pDevice->m_Objects.m_nCount++;

		if (pObject->m_pNext == pDevice->m_Objects.m_pHead)
			pDevice->m_Objects.m_pHead = pObject;
	}
	else
	{
		pObject->m_pNext = pObject;
		pObject->m_pPrev = pObject;

		pDevice->m_Objects.m_pHead = pObject;
		pDevice->m_Objects.m_nCount++;
	}

	strncpy(pObject->m_TriggerName, pName, sizeof(pObject->m_TriggerName));
	strncpy(pObject->m_Name, g_MatchInfoQ438.tszName,
	        sizeof(pObject->m_Name));

	pObject->m_ObjectType = nObjectType;
	pObject->m_guidType   = g_MatchInfoQ438.guidType;
	pObject->m_dwType     = g_MatchInfoQ438.dwType;

	pObject->m_pDevice     = pDevice;
	pObject->m_fScale      = 1.0f;
	pObject->m_fUnknown10c = 0.0f;
	pObject->m_fUnknown110 = 0.0f;
	pObject->m_fScale3     = 1.0f;
	pObject->m_fOffset3    = 0.0f;
	pObject->m_fScale2     = 1.0f;
	pObject->m_fOffset2    = 0.0f;
	pObject->m_fScale1     = 1.0f;
	pObject->m_fOffset1    = 0.0f;
	pObject->m_fUnknown12c = 0.0f;
	pObject->m_fUnknown130 = 0.0f;
	pObject->m_fMin        = -2147483648.0f;
	pObject->m_fMax        =  2147483648.0f;

	if (pDevice->m_pDevice && (nObjectType == 2 || nObjectType == 4))
	{
		range.diph.dwSize       = sizeof(DIPROPRANGE);
		range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		range.diph.dwObj        = g_MatchInfoQ438.dwType;
		range.diph.dwHow        = DIPH_BYID;

		if (pDevice->m_pDevice->GetProperty(DIPROP_RANGE,
		                                    &range.diph) == DI_OK)
		{
			pObject->m_fMin = (float)range.lMin;
			pObject->m_fMax = (float)range.lMax;
		}
	}

	return pObject;
}


// ----------------------------------------------------------------------- //
//  0x00438ad0 -- 215 bytes, the DIEnumDeviceObjects matcher.  `ret $8`:
//  PASCAL, (lpddoi, pvRef).  Answers DIENUM_STOP (0) on a hit, after
//  raising g_bMatchFoundQ438 and copying the whole 0x4f-dword instance,
//  and DIENUM_CONTINUE (1) otherwise.
//
//  Read off the bytes:
//    * by name (g_bMatchByTypeQ438 clear): 0x00492f30 is stricmp of
//      lpddoi->tszName (+0x20) against pvRef, the name the binder passed.
//    * with a GUID: `repz cmpsl` of four dwords is the inline memcmp of
//      GUID operator==; the offset test then follows with -1 as "any",
//      and this arm keeps its OWN copy of the hit tail (0x00438b17).
//    * without one: the instance (`(dwType >> 8) & 0xffff`, DIDFT_
//      GETINSTANCE) if given, else the offset, else no match at all.
//    * ONE `return 1` epilogue (0x00438b9d) that every miss jumps to:
//      each arm is a positive guard around its own hit tail with a single
//      DIENUM_CONTINUE after them all.  Early `return 1`s gave five
//      separate epilogues (247 bytes).  VC6 merges the three non-GUID hit
//      tails into one (0x00438b80).
// ----------------------------------------------------------------------- //
BOOL PASCAL di_MatchObjectCallbackQ438(LPCDIDEVICEOBJECTINSTANCEA lpddoi,
                                       LPVOID pvRef)
{
	if (g_bMatchByTypeQ438)
	{
		if (g_bHaveMatchGuidQ438)
		{
			if (lpddoi->guidType == g_MatchGuidQ438)
			{
				if (g_MatchOfsQ438 == -1 || lpddoi->dwOfs == (DWORD)g_MatchOfsQ438)
				{
					g_bMatchFoundQ438 = LTTRUE;
					g_MatchInfoQ438   = *lpddoi;
					return DIENUM_STOP;
				}
			}
		}
		else if (g_MatchInstanceQ438 != -1)
		{
			if (DIDFT_GETINSTANCE(lpddoi->dwType) == (DWORD)g_MatchInstanceQ438)
			{
				g_bMatchFoundQ438 = LTTRUE;
				g_MatchInfoQ438   = *lpddoi;
				return DIENUM_STOP;
			}
		}
		else if (g_MatchOfsQ438 != -1)
		{
			if (lpddoi->dwOfs == (DWORD)g_MatchOfsQ438)
			{
				g_bMatchFoundQ438 = LTTRUE;
				g_MatchInfoQ438   = *lpddoi;
				return DIENUM_STOP;
			}
		}
	}
	else
	{
		if (stricmp(lpddoi->tszName, (char*)pvRef) == 0)
		{
			g_bMatchFoundQ438 = LTTRUE;
			g_MatchInfoQ438   = *lpddoi;
			return DIENUM_STOP;
		}
	}

	return DIENUM_CONTINUE;
}


// ======================================================================= //
//  PART TWO -- 0x0043a0e0, the scaled-blit rectangle clipper
//
//  WHAT IDENTIFIES IT
//
//  One call site, 0x0040ddea, and it settles everything.  It pushes 14
//  dwords (`add $0x38,%esp` cleans 56 bytes) in this order, so the
//  parameters are, reading the pushes back to front:
//
//    arg0,arg1   0x10(%esi) / 0x14(%esi)   -- the SOURCE surface's w/h
//    arg2..arg5  a 16-byte local block the caller filled at 0x0040dd67
//                from its own `pSrcRect` argument, or with
//                (0, 0, 0x10(%esi), 0x14(%esi)) when that argument is null
//    arg6,arg7   0x10(%edi) / 0x14(%edi)   -- the DEST surface's w/h
//    arg8..arg11 the other 16-byte block, filled the same way at
//                0x0040dd2d from `pDestRect` / (0,0,dest w,dest h)
//    arg12       the address of the arg2..arg5 block
//    arg13       the address of the arg8..arg11 block
//
//  which side is which is pinned by 0x0040ddac -- `cmp $0x4b1620,%edi`,
//  edi compared against the screen-surface global -- so edi is the
//  destination and esi the source.  The two out-parameters are therefore
//  "the clipped source rect" and "the clipped dest rect", and the function
//  writes each group back through its own pointer at 0x0043a25a.
//
//  WHY FOURTEEN SCALARS AND NOT TWO RECTS BY VALUE
//    Because nSrcRight and nSrcBottom live in ebp and edi for the whole
//    body and are never written back to their stack slots, the final
//    `*pSrcOut = rect` cannot be a structure copy -- the eight stores at
//    0x0043a260..0x0043a27f are member assignments out of registers.  The
//    stack layout is the same either way, so this is the reading that the
//    stores force rather than a preference.
//
//  WHY THE RATIOS ARE `fild` / `fidiv` AND NOT TWO `fild`s
//    0x0043a146 and 0x0043a154 are `fildl` then `fidivl`: only the
//    NUMERATOR is converted, so the source casts the numerator alone and
//    leaves the denominator an int.  Two casts would have produced a
//    second `fild` and an `fdivp`.
//
//  WHY fXRatio IS NEVER SPILLED
//    It stays in st(0) across two __ftol calls (0x004928c0) and is finally
//    discarded with `fstp %st(0)` at 0x0043a1db/0x0043a1df.  That is
//    allocation, not source: fYRatio, the other one, IS spilled, at
//    0x0043a176.
//
//  THE TRAILING RETURN -- TWO STATEMENTS, and that is the whole fix.
//    `test %esi,%esi; jge; <return 0 inline>; xor %eax,%eax; test %ecx,%ecx;
//    setge %al` at 0x0043a25e..0x0043a292 is `if (nDstRight < 0) return
//    LTFALSE; return nDstBottom >= 0;`.  Spelled as one `a >= 0 && b >= 0`
//    expression, VC6 lowers BOTH operands to jumps into a `return 0` block
//    at the end and then tail-merges all six earlier `return LTFALSE`
//    epilogues into that block (402 bytes against 439).  With the final
//    return a setcc there is no shared constant-return block to merge into,
//    and every refusal keeps its own inline `pop..; xor; ret` as shipped
//    (0x0043a11e, 0x0043a12d, 0x0043a224, 0x0043a233, 0x0043a240,
//    0x0043a251).  The mixed-cl-build theory once recorded here was wrong.
//    Note it runs AFTER the two rects have been written out.
//
//    The one thing the build DID settle is the two overflow clamps, fixed
//    below: shipped computes `nDstRight - nDstWidth` unconditionally and
//    tests the difference, which is the `- ... > 0` spelling and not
//    `nDstRight > nDstWidth`.
// ======================================================================= //

// The name is invented: nothing in the image names this function.  The
// suffix keeps it out of the way of whatever the real one turns out to be.
LTBOOL cis_ClipScaledBlitQ438(int nSrcWidth, int nSrcHeight,
                              int nSrcLeft, int nSrcTop,
                              int nSrcRight, int nSrcBottom,
                              int nDstWidth, int nDstHeight,
                              int nDstLeft, int nDstTop,
                              int nDstRight, int nDstBottom,
                              LTRect* pSrcOut, LTRect* pDstOut)
{
	float fXRatio;
	float fYRatio;

	if (nSrcLeft < 0)
		nSrcLeft = 0;

	if (nSrcRight > nSrcWidth)
		nSrcRight = nSrcWidth;

	if (nSrcTop < 0)
		nSrcTop = 0;

	if (nSrcBottom > nSrcHeight)
		nSrcBottom = nSrcHeight;

	if (nSrcRight < nSrcLeft)
		return LTFALSE;

	if (nSrcBottom < nSrcTop)
		return LTFALSE;

	fXRatio = (float)(nSrcRight - nSrcLeft) / (nDstRight - nDstLeft);
	fYRatio = (float)(nSrcBottom - nSrcTop) / (nDstBottom - nDstTop);

	if (nDstLeft < 0)
	{
		nSrcLeft -= (int)(nDstLeft * fXRatio);
		nDstLeft  = 0;
	}

	if (nDstTop < 0)
	{
		nSrcTop -= (int)(nDstTop * fYRatio);
		nDstTop  = 0;
	}

	// The guard is the DIFFERENCE against zero, not the two operands
	// against each other: 0x0043a1be..0x0043a1c8 is `mov %esi,%eax;
	// sub %edx,%eax; test %eax,%eax; mov %eax,<temp>; jle`, with the
	// subtraction done on BOTH paths and commoned with the `fildl` source.
	// Writing `nDstRight > nDstWidth` gives `cmp` and a subtraction that
	// happens only inside the body -- which is what our first build did.
	if (nDstRight - nDstWidth > 0)
	{
		nSrcRight -= (int)((nDstRight - nDstWidth) * fXRatio);
		nDstRight  = nDstWidth;
	}

	// Same, 0x0043a1e9..0x0043a1f1.
	if (nDstBottom - nDstHeight > 0)
	{
		nSrcBottom -= (int)((nDstBottom - nDstHeight) * fYRatio);
		nDstBottom  = nDstHeight;
	}

	if (nSrcRight > nSrcWidth)
		nSrcRight = nSrcWidth;

	if (nSrcBottom > nSrcHeight)
		nSrcBottom = nSrcHeight;

	if (nSrcLeft >= nSrcRight)
		return LTFALSE;

	if (nSrcTop >= nSrcBottom)
		return LTFALSE;

	if (nDstLeft >= nDstWidth)
		return LTFALSE;

	if (nDstTop >= nDstHeight)
		return LTFALSE;

	pSrcOut->left   = nSrcLeft;
	pSrcOut->top    = nSrcTop;
	pSrcOut->right  = nSrcRight;
	pSrcOut->bottom = nSrcBottom;

	pDstOut->left   = nDstLeft;
	pDstOut->top    = nDstTop;
	pDstOut->right  = nDstRight;
	pDstOut->bottom = nDstBottom;

	if (nDstRight < 0)                       // 0x0043a25e
		return LTFALSE;

	return nDstBottom >= 0;                  // 0x0043a28a setge
}


// ======================================================================= //
//  PART TWO-B -- 0x0043a2a0, the unscaled blit clipper (239 bytes)
//
//  The sibling of 0x0043a0e0 immediately below it, with no ratios: twelve
//  cdecl dwords, (srcW, srcH, src rect, dstW, dstH, dstX, dstY, pSrcOut,
//  pDstOut).  Callers: cis_DoDrawSurfaceToSurface (0x0040d2f1/0x0040d325),
//  0x0040c2df/0x0040c32d and the font blitter at 0x00488fc8.  Signed
//  compares throughout (jge/jle/jl).  A negative source edge moves the
//  destination origin and vice versa; the destination overflow is the
//  `- ... > 0` difference test (0x0043a2ec, 0x0043a302); then the source
//  rect is clamped, four refusals, the two rects written, and the same
//  `right < 0` / `bottom >= 0` tail as 0x0043a0e0.  Name ours.
// ======================================================================= //
LTBOOL cis_ClipBlitQ438(int nSrcWidth, int nSrcHeight,
                        int nSrcLeft, int nSrcTop,
                        int nSrcRight, int nSrcBottom,
                        int nDstWidth, int nDstHeight,
                        int nDstX, int nDstY,
                        LTRect* pSrcOut, LTRect* pDstOut)
{
	int nDiff;

	if (nSrcLeft < 0)
	{
		nDstX -= nSrcLeft;
		nSrcLeft = 0;
	}

	if (nSrcTop < 0)
	{
		nDstY -= nSrcTop;
		nSrcTop = 0;
	}

	if (nDstX < 0)
	{
		nSrcLeft -= nDstX;
		nDstX = 0;
	}

	if (nDstY < 0)
	{
		nSrcTop -= nDstY;
		nDstY = 0;
	}

	nDiff = (nSrcRight - nSrcLeft) - nDstWidth + nDstX;
	if (nDiff > 0)
		nSrcRight -= nDiff;

	nDiff = (nSrcBottom - nSrcTop) - nDstHeight + nDstY;
	if (nDiff > 0)
		nSrcBottom -= nDiff;

	if (nSrcRight > nSrcWidth)
		nSrcRight = nSrcWidth;

	if (nSrcBottom > nSrcHeight)
		nSrcBottom = nSrcHeight;

	if (nSrcLeft >= nSrcRight)
		return LTFALSE;

	if (nSrcTop >= nSrcBottom)
		return LTFALSE;

	if (nDstX >= nDstWidth)
		return LTFALSE;

	if (nDstY >= nDstHeight)
		return LTFALSE;

	pSrcOut->left   = nSrcLeft;
	pSrcOut->top    = nSrcTop;
	pSrcOut->right  = nSrcRight;
	pSrcOut->bottom = nSrcBottom;

	pDstOut->left   = nDstX;
	pDstOut->top    = nDstY;
	pDstOut->right  = nSrcRight - nSrcLeft + nDstX;
	pDstOut->bottom = nSrcBottom - nSrcTop + nDstY;

	if (pDstOut->right < 0)
		return LTFALSE;

	return pDstOut->bottom >= 0;
}


// ======================================================================= //
//  PART THREE -- 0x0043b270, one plane of the warp-blit polygon clipper
//
//  WHAT IDENTIFIES IT
//
//  The vertex is LTWarpPt, straight out of src/LT2/sdk/inc/ltbasetypes.h:
//
//      struct LTWarpPt { float source_x, source_y; float dest_x, dest_y; };
//
//  Sixteen bytes, which is the stride every walk in this function uses
//  (`add $0x10`, `shl $0x4`, `sar $0x4`), and iltclient.h's comment on
//  WarpSurfaceToSurface says in so many words that "the warp is clipped
//  against the destination rectangle" -- which is what this does, on the
//  field at +0x0c, i.e. dest_y.
//
//  It is one of FOUR identical passes.  0x0043a4f0, its only caller, takes
//  the four plane constants as parameters, publishes them into globals
//  (0x004b3e18 <- arg2, 0x004b3690 <- arg3, 0x004b3e1c <- arg4,
//  0x004b3e24 <- arg5) and then runs the same loop four times.  Three of
//  the four are INLINED into 0x0043a4f0 -- the first is at 0x0043a54d and
//  tests `pt->dest_x < 0x004b3e18` with the sense INVERTED (`test $0x1,%ah;
//  jne` there against `je` here) -- and this fourth one is the out-of-line
//  copy.  So the sense here is `dest_y < g_fWarpClipBottom`: `fnstsw` then
//  `test $0x1,%ah` reads C0, "less than", and the `je` at 0x0043b2a3 takes
//  the ZERO arm to the `esi = 0` block.
//
//  THE PING-PONG BUFFER
//    `lea (%eax,%eax,2); lea (%ecx,%ecx,4); shl $0x6` is a multiply by 960
//    on a 0/1 index, and 960 == 60 * sizeof(LTWarpPt), so 0x004b3698 is
//    `LTWarpPt [2][60]`.  The index global at 0x004b3e20 is read once,
//    used for THIS pass's output, and then set to `!itself` (0x0043b2f8:
//    `xor %edx,%edx; test %eax,%eax; sete %dl`) so the next pass reads from
//    what this one wrote and writes the other half.
//
//  WHY THE TEST IS TWO CONVERSIONS -- see "THE DOUBLE BOOLEAN CONVERSION"
//    at the bottom of this block.  The first reading of it (that %esi has
//    no byte-addressable low half, so VC6 was forced through `test`/`setne`
//    to widen a bool living there) is DISPROVED: our build put the same
//    one-statement comparison in %eax and emitted no second conversion at
//    all.
//
//  WHY THE POINTERS ARE RE-READ
//    `mov (%ebx),%ebp` at 0x0043b331 and `mov 0xf4(%esp),%ebx` at
//    0x0043b3b7 reload *ppPts and pnPts on every iteration, because the
//    struct copy into the output buffer could alias them.  bInside, by
//    contrast, is a local array whose address never escapes, so its two
//    reads per iteration ARE common-subexpressioned (the value is spilled
//    to 0x14(%esp) across the copy rather than re-loaded).
//
//  THE ARRAY SIZE, MEASURED
//    Frame `sub $0xd4,%esp` with four pushes below it, so the locals are
//    [esp_entry-0xd4, esp_entry) and `lea 0x1c(%esp),%ecx` at 0x0043b28d
//    puts the array base at esp_entry-0xc8.  0xc8 == 200 == 50 dwords and
//    the array runs to the TOP of the frame exactly, with three dwords
//    (-0xd4, -0xd0, -0xcc) below it for the two spills the body uses.
//    0x0043a4f0 agrees to the byte: frame 0xdc, four pushes, array at
//    0x24(%esp) -- the same esp_entry-0xc8, the same 50 dwords to the top,
//    and two extra dwords below for the master's own spills.
//
//    This was WRITTEN as 46 and the first build measured it: our frame came
//    out `sub $0xc4`, exactly 0x10 == four dwords short.  50 is therefore
//    read off the difference, not guessed.
//
//  THE DOUBLE BOOLEAN CONVERSION, AND WHAT SPELLING FORCES IT
//    A plain `bInside[i] = (pt->dest_y < g_fWarpClip);` compiles to the
//    branchy 0/1 alone -- our first build put it straight in %eax, stored
//    it and added it, and the `xor %eax,%eax; test %esi,%esi; setne %al`
//    at 0x0043b2ae..0x0043b2b2 had nothing to correspond to.  The shipped
//    sequence is TWO conversions: the comparison materialised as 0/1 in a
//    register, then that register converted to a boolean again.  Both
//    shipped copies do it -- 0x0043b2a5 here and 0x0043a55d in the master's
//    first inlined pass -- so it is source, not luck.  The one-statement
//    ternary `bInside[i] = (cmp) ? LTTRUE : LTFALSE` does NOT produce it
//    (measured: VC6 folds that into one 0/1).  Naming the comparison first,
//    `LTBOOL bIn = (cmp); bInside[i] = bIn ? LTTRUE : LTFALSE;`, does.
//
//    The rest of the old 27-block diff was the second loop's registers.
//    Writing the output pointer first and copying it to pOutStart
//    (`pOut = g_Buf[g_n]; pOutStart = pOut;`) instead of the other way
//    round matches 0x0043b2e2..0x0043b310, where the buffer address is
//    built in %ecx (pOut) and only then copied to %ebp (pOutStart) and
//    spilled.  With both edits the function is byte-identical.
// ======================================================================= //

// The warp clipper's .bss, 0x004b3690..0x004b3e27, DEFINED HERE (this
// cluster owns it; warpblit.cpp's two clip passes extern the top and right
// planes).  Uninitialised in the image.
float g_fWarpClipTopQ438;                                  // 0x004b3690
float g_fWarpClipRightQ438;                                // 0x004b3e1c

// 0x004b3e24 -- the clip plane for this pass; 0x0043a527 stores
// 0x0043a4f0's sixth argument into it.
float g_fWarpClipBottomQ438;                               // 0x004b3e24

// 0x004b3e20 -- the 0/1 output-buffer selector.
int g_nWarpClipBufferQ438;                                 // 0x004b3e20

// 0x004b3698 -- 2 * 60 * 16 bytes.  0x0043a688 stores the ADDRESS
// 0x004b3698 back through the caller's LTWarpPt** when buffer 0 is the one
// that was written, which is what says the two halves are a single array
// and not two variables.
LTWarpPt g_WarpClipBufferQ438[2][60];                      // 0x004b3698

// The first two parameters are never read.  The sole call site,
// 0x0043a93d, loads the SAME stack slot into two registers for them, so
// they are two arguments of a signature the other three passes share and
// this pass has no use for.  Their types are not recoverable.
LTBOOL cis_ClipWarpDestBottomQ438(int nUnused1Q438, int nUnused2Q438,
                                  LTWarpPt** ppPts, int* pnPts)
{
	LTBOOL bInside[50];
	int nInside;
	int nPrev;
	int i;

	LTWarpPt* pOutStart;
	LTWarpPt* pOut;
	LTWarpPt* pPrev;
	LTWarpPt* pCur;

	float fT;

	nInside = 0;

	for (i = 0; i < *pnPts; i++)
	{
		LTBOOL bIn = ((*ppPts)[i].dest_y < g_fWarpClipBottomQ438);
		bInside[i] = bIn ? LTTRUE : LTFALSE;

		nInside += bInside[i];
	}

	if (!nInside)
		return LTFALSE;

	if (nInside != *pnPts)
	{
		pOut = g_WarpClipBufferQ438[g_nWarpClipBufferQ438];
		pOutStart = pOut;

		g_nWarpClipBufferQ438 = !g_nWarpClipBufferQ438;

		nPrev = *pnPts - 1;
		pPrev = &(*ppPts)[nPrev];

		for (i = 0; i < *pnPts; i++)
		{
			pCur = &(*ppPts)[i];

			if (bInside[nPrev])
			{
				*pOut = *pPrev;

				pOut++;
			}

			if (bInside[nPrev] != bInside[i])
			{
				fT = (g_fWarpClipBottomQ438 - pPrev->dest_y) /
				     (pCur->dest_y - pPrev->dest_y);

				pOut->dest_y   = g_fWarpClipBottomQ438;
				pOut->dest_x   = (pCur->dest_x - pPrev->dest_x) * fT +
				                 pPrev->dest_x;
				pOut->source_x = (pCur->source_x - pPrev->source_x) * fT +
				                 pPrev->source_x;
				pOut->source_y = (pCur->source_y - pPrev->source_y) * fT +
				                 pPrev->source_y;

				pOut++;
			}

			nPrev = i;
			pPrev = pCur;
		}

		*pnPts = pOut - pOutStart;
		*ppPts = pOutStart;
	}

	return LTTRUE;
}


// ======================================================================= //
//  PART FOUR -- 0x0043b410, 0x0043b6a0 and 0x0043b940, the three 32-bit
//               span fillers of cis_DrawWarp
//
//  WHAT THEY ARE
//
//  Each is the 32-bits-per-pixel tail of one of the three warp entries at
//  0x0043aa20, 0x0043ad40 and 0x0043aff0 -- the SDK's WarpSurfaceToSurface,
//  ...Transparent and ...SolidColor (iltclient.h, 0x004a8248/0x004a828c/
//  0x004a82ac name all three).  Each entry opens with
//
//      mov  0x004b175c,%eax ; cmp $0x2,%eax ; jne ...   -- 16-bit inline
//      cmp  $0x3,%eax       ; jne ...                   -- 32-bit, tail call
//      mov  $0x1,%eax                                   -- neither: LT_ERROR
//
//  and the 32-bit arm is the out-of-line call (0x0043ac98 / 0x0043afa1 /
//  0x0043b257) with the entry's own six arguments plus two literal zeros.
//  The 16-bit arm is the SAME body inlined into the entry with `mov
//  0x0(%ebp,%edx,2),%dx` and `add $0x2` in place of the dword forms, which
//  is what makes the three functions below transcriptions of a body that
//  can be read twice.
//
//  THE SURFACE LOCK
//
//  The two "lock this surface or the screen" blocks are not two different
//  pieces of source.  They are two inlinings of ONE three-argument helper
//  which survives OUT OF LINE at 0x0043acb0, called from all three entries
//  (0x0043aabc, 0x0043addc, 0x0043b08c) and never from anywhere else.  The
//  call at 0x0043aab8..0x0043aac1 settles the signature outright:
//
//      lea  0x1c(%esp),%eax
//      push $0x1            ; arg2  -- the literal 1
//      push %eax            ; arg1  -- &pitch, a stack local
//      push %esi            ; arg0  -- the surface
//      call 0x0043acb0
//      add  $0xc,%esp       ; __cdecl, three dwords
//
//  and 0x0043acb0 itself gives the body:
//
//      !g_pRender                      -> return NULL          (0x0043acb8)
//      *pPitch = 0                     unconditionally         (0x0043accc)
//      pSurface == &g_ScreenSurface    -> LockScreen(0,0,w,h,&pBits,pPitch)
//                                         and return pBits or NULL -- that
//                                         is the `neg/sbb/and` at 0x0043acfe
//      otherwise                       -> *pPitch = pSurface->m_nPitch,
//                                         pBits = LockSurface(+0x0c), then
//                                         `if (arg2 && pBits && pSurface)
//                                          pSurface->m_nFlags |= 4`
//                                         (0x0043ad21/25/29/2d)
//
//  So the third argument is a WRITE flag, and the whole difference between
//  the two inlined blocks in each function below is that one is called with
//  LTFALSE and the other with LTTRUE:
//
//    * the SOURCE lock (0x0043b430..0x0043b484) has no `|= 4` and none of
//      the three tests, because `bWrite` folded to 0 and killed the block;
//    * the DEST lock (0x0043b49c..0x0043b4ff) keeps `cmp %ebx,%eax;
//      cmp %ebx,%esi; orl $0x4,0x1c(%esi)` -- the two remaining conjuncts
//      of the SAME `&&`, with `bWrite` folded to 1 and dropped.
//
//  Nothing else differs: same `*pPitch = 0` first (0x0043b430 / 0x0043b49c),
//  same screen test, same six-argument LockScreen, same `mov 0x18(%reg)`
//  pitch and `mov 0x0c(%reg)` render data.  The "different local count" is
//  register allocation -- one keeps the screen path's pBits in the slot the
//  common test reads, the other needs a second slot because its screen arm
//  jumps PAST the `|= 4` block (0x0043b4d7) and has to bring the value with
//  it.
//
//  WHICH ARGUMENT IS WHICH
//    arg1 (edi) is locked FIRST with bWrite LTFALSE and its bits are the
//    ones READ (`mov 0x0(%ebp,%edx,4),%edx`, 0x0043b5c3), so arg1 is the
//    SOURCE.  arg0 (esi) is locked SECOND with bWrite LTTRUE, gets the
//    `|= 4`, and its bits are the ones WRITTEN -- the DESTINATION.  The two
//    restores at 0x0043b5d3 / 0x0043b602 (`mov 0x34(%esp),%esi` and
//    `mov 0x38(%esp),%edi`, i.e. arg0 and arg1) confirm it a second time.
//
//  WHY THE NESTING IS `if (bits) { ... }` AND NOT EARLY RETURNS
//    Three jumps out of the dest lock go to three DIFFERENT places and only
//    this shape explains all three:
//      * 0x0043b4ff (`pDestBits == 0`) and 0x0043b4cd (LockScreen failed)
//        jump to 0x0043b62f, which unlocks the SOURCE -- so the dest unlock
//        at 0x0043b606 is INSIDE `if (pDestBits)`;
//      * 0x0043b492 (`!g_pRender`) jumps to 0x0043b634, four bytes later,
//        skipping exactly the `mov 0x4b17a4,%eax` that 0x0043b62f begins
//        with -- because on THAT path eax already holds g_pRender from
//        0x0043b48a.  A redundant-load entry point, not a different
//        statement;
//      * the two first-lock failures (0x0043b426, 0x0043b484) jump straight
//        to the error tail at 0x0043b65a, because bFailed is still the
//        constant 1 there and VC6 threads the test away.
//
//  THE FLAG
//    `movl $0x1,<slot>` in the prologue (0x0043b41e), `mov %ebx,<slot>` (a
//    zero) once both locks are held (0x0043b516), and `mov <slot>,%eax;
//    test; je` before the error tail (0x0043b652).  One `LTBOOL bFailed`,
//    set true at the top and cleared inside the body.  Where inside is NOT
//    pinned: the store is scheduled into the shadow of the `divl` above it,
//    so "first statement of the block" and "just after nSrcPitchPixels" are
//    indistinguishable.  Written as the first statement.
//
//  THE EDGE RECORD
//    Twelve bytes, walked with `add $0xc`: a SHORT at +0 (`mov (%eax),%cx`
//    then `movswl`) and two 32-bit 16.16 fixed-point fields at +4 and +8.
//    +8 is the one multiplied by the source pitch-in-pixels (0x0043b5b0),
//    so +8 is v (the row) and +4 is u (the column).
//
//  THE SPAN
//    `cmp %dx,%cx` at 0x0043b551 compares the two SHORTS before either is
//    widened, so `pRight->x != pLeft->x` is a statement of its own and the
//    subtraction below it is a second expression, not the same one reused.
//    `jle` at 0x0043b564 puts the `> 0` arm in the fall-through.
//    `test %esi,%esi; je` then `dec %esi; jne` is the rotation of
//    `while (nWidth--)`.
//
//  WHAT THE THREE DIFFER BY -- eight instructions in one loop, nothing else
//    0x0043b410  *pOut = pixel;                           (0x0043b5c7)
//    0x0043b6a0  if (pixel != g_TransColorQ438)           (0x0043b858)
//                    *pOut = pixel;
//    0x0043b940  if (pixel != g_TransColorQ438)           (0x0043baf8)
//                    *pOut = g_FillColorQ438;             (0x0043bafc)
//    0x004b161c and 0x004b1750 are the two colours clientinterface.cpp
//    already writes through 0x0045f5e0 as g_Sub4b161c (hTransColor) and
//    g_Sub4b1750 (hFillColor) -- see its lines 2018/2019.
//
//  ARGUMENTS 6 AND 7
//    Pushed as literal zeros by all three entries (0x0043ac82, 0x0043af8b,
//    0x0043b241) and read by none of the three bodies -- there is no
//    0x4c(%esp) or 0x50(%esp) anywhere in the 1989 bytes.  Named by
//    position; their types are not recoverable, only their dword width.
// ======================================================================= //

// The surface, as this cluster reaches it.  Same offsets clientblit.cpp
// proved: +0x0c render data, +0x18 pitch, +0x1c flags with bit 2 meaning
// "the renderer has to optimize me again".
class CSurfaceQ438
{
public:
	uint8    m_pad_00[0x0c];
	void*    m_pRenderData;                     // 0x00c
	uint32   m_nWidth;                          // 0x010
	uint32   m_nHeight;                         // 0x014
	uint32   m_nPitch;                          // 0x018
	uint32   m_nFlags;                          // 0x01c
};

// The four render slots this cluster uses, with clientblit.cpp's names and
// prototypes.  All four are plain function pointers reached through the
// 0x004b17a4 pointer, never virtuals.
class RenderStructWarpQ438
{
public:
	uint8   m_pad_00[0xd4];
	void*  (*LockSurface)(void* pRenderData);                       // 0x0d4
	void   (*UnlockSurface)(void* pRenderData);                     // 0x0d8
	uint8   m_pad_dc[0xe4 - 0xdc];
	LTBOOL (*LockScreen)(int nLeft, int nTop, int nRight, int nBottom,
	                     void** ppPixels, uint32* pPitch);          // 0x0e4
	void   (*UnlockScreen)();                                       // 0x0e8
};

extern RenderStructWarpQ438* g_pRenderQ438;            // 0x004b17a4
extern CSurfaceQ438          g_ScreenSurfaceQ438;      // 0x004b1620
// The screen size (0x004b1630 / 0x004b1634) is the screen surface's own
// m_nWidth / m_nHeight (+0x10 / +0x14), not a global of its own.

// 0x004b1794 -- bytes per pixel.  `divl` (UNSIGNED) at 0x0043b50b turns a
// pitch into a pitch in pixels and `imul` at 0x0043b591 turns an x into a
// byte offset.  Uninitialised in the image, so .bss.
extern uint32                g_nBytesPerPixel;         // 0x004b1794

// The two colours.  clientinterface.cpp fills both through 0x0045f5e0:
// hTransColor -> 0x004b161c, hFillColor -> 0x004b1750.
extern uint32                g_TransColorQ438;         // 0x004b161c
extern uint32                g_FillColorQ438;          // 0x004b1750

// de_error.h's RETURN_ERROR, the same expansion binkvideo.cpp wrote out.
// The stringifier is double-expanding: 0x004a7494 is the string "1", not
// "LT_ERROR", so the macro sees LT_ERROR already replaced by 1.
extern "C" void dsi_OnReturnErrorQ438(int code);            // 0x0042d760
extern int      g_DebugLevelQ438;                           // 0x004b0ab0
extern char*    g_pErrorFormatQ438;                         // 0x004a7110
void            con_ConsolePrintQ438(char* pFormat, ...);   // 0x0042e6f0

#define Q_ERRSTR_(x) #x
#define Q_ERRSTR(x)  Q_ERRSTR_(x)
#define Q_ERRDESC    " "

#define Q_RETURN_ERROR(level, fnName, code) \
	{ \
		dsi_OnReturnErrorQ438(code); \
		if (g_DebugLevelQ438 >= level) \
			con_ConsolePrintQ438(g_pErrorFormatQ438, fnName, \
			                     Q_ERRSTR(code), Q_ERRDESC); \
		return code; \
	}


// ----------------------------------------------------------------------- //
//  0x0043acb0 -- lock a surface, or the screen if that is what it is.
//
//  Reconstructed here because the three functions below INLINE it six
//  times and nothing else in the reconstruction has it; the out-of-line
//  body is a free extra if it lands.  Every line is cited in PART FOUR's
//  header above.
//
//  The two g_pRender reloads (0x0043ace6, 0x0043ad0e) are forced, not
//  chosen: the store through pPitch that precedes each of them could alias
//  the global.
// ----------------------------------------------------------------------- //

// The shipped image has BOTH an out-of-line copy of this pair (called from
// the three warp entries) AND expansions inside the three cis_DrawWarp32*
// clones.  Marking these two `inline` loses the out-of-line bodies, so the
// clones call the separate inline copies below instead (measured: all
// five identical that way).
void* cis_LockSurfaceQ438(CSurfaceQ438* pSurface, uint32* pPitch,
                          LTBOOL bWrite)
{
	void* pBits;

	if (!g_pRenderQ438)
		return LTNULL;

	*pPitch = 0;

	if (pSurface == &g_ScreenSurfaceQ438)
	{
		if (!g_pRenderQ438->LockScreen(0, 0, g_ScreenSurfaceQ438.m_nWidth,
		                               g_ScreenSurfaceQ438.m_nHeight, &pBits, pPitch))
		{
			return LTNULL;
		}

		return pBits;
	}

	*pPitch = pSurface->m_nPitch;

	pBits = g_pRenderQ438->LockSurface(pSurface->m_pRenderData);

	if (bWrite && pBits && pSurface)
		pSurface->m_nFlags |= 4;

	return pBits;
}


// ----------------------------------------------------------------------- //
//  The partner.  src/engine/lithtech/lt2new3.cpp already reconstructs this
//  one out of line, at 0x0043afc0, under the name r_UnlockSurface -- with
//  the same two arms, the same fall-through on the screen case and the same
//  per-arm reload of g_pRender.  Repeated here (renamed) only because the
//  three functions below inline it six times and VC6 cannot inline across
//  translation units.
// ----------------------------------------------------------------------- //

void cis_UnlockSurfaceQ438(CSurfaceQ438* pSurface)
{
	if (pSurface == &g_ScreenSurfaceQ438)
		g_pRenderQ438->UnlockScreen();
	else
		g_pRenderQ438->UnlockSurface(pSurface->m_pRenderData);
}


// The copies the three clones expand.  The screen arm is spelled positively
// here: the out-of-line body folds either spelling into the same
// `neg/sbb/and`, but expanded inline the negative form comes out as
// `je fail ; jmp ok` where 0x0043b45f..0x0043b463 has `jne ok ; jmp fail`
// (one byte).
inline void* cis_LockSurfaceInlQ438(CSurfaceQ438* pSurface, uint32* pPitch,
                                    LTBOOL bWrite)
{
	void* pBits;

	if (!g_pRenderQ438)
		return LTNULL;

	*pPitch = 0;

	if (pSurface == &g_ScreenSurfaceQ438)
	{
		if (g_pRenderQ438->LockScreen(0, 0, g_ScreenSurfaceQ438.m_nWidth,
		                              g_ScreenSurfaceQ438.m_nHeight, &pBits, pPitch))
		{
			return pBits;
		}

		return LTNULL;
	}

	*pPitch = pSurface->m_nPitch;

	pBits = g_pRenderQ438->LockSurface(pSurface->m_pRenderData);

	if (bWrite && pBits && pSurface)
		pSurface->m_nFlags |= 4;

	return pBits;
}

inline void cis_UnlockSurfaceInlQ438(CSurfaceQ438* pSurface)
{
	if (pSurface == &g_ScreenSurfaceQ438)
		g_pRenderQ438->UnlockScreen();
	else
		g_pRenderQ438->UnlockSurface(pSurface->m_pRenderData);
}


// ----------------------------------------------------------------------- //
//  The edge record the two walkers step through, 12 bytes.
// ----------------------------------------------------------------------- //

struct SWarpEdgeQ438
{
	int16  x;                  // 0x00  `mov (%eax),%cx` / `movswl`
	int16  m_pad02;            // 0x02  never touched
	int32  u;                  // 0x04  16.16, the column
	int32  v;                  // 0x08  16.16, the row
};


// ----------------------------------------------------------------------- //
//  0x0043b410 -- WarpSurfaceToSurface's 32-bit span fill.
//
//  The names are invented; nothing in the image names any of the three.
//  "cis_DrawWarp" is only the string the error tail reports, and it is the
//  same string in all three, so it names the family and not the function.
// ----------------------------------------------------------------------- //

LTRESULT cis_DrawWarp32Q438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
                            SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
                            uint32 nYStart, uint32 nYEnd,
                            uint32 nUnused6Q438, uint32 nUnused7Q438)
{
	uint32*        pSrcBits;
	uint8*         pDestBits;
	uint32         nSrcPitch;
	uint32         nDestPitch;
	uint32         nSrcPitchPixels;
	uint32         nY;
	uint32*        pOut;
	SWarpEdgeQ438* pEdgeLeft;
	SWarpEdgeQ438* pEdgeRight;
	SWarpEdgeQ438* pStart;
	SWarpEdgeQ438* pEnd;
	int            nWidth;
	int32          u, v, du, dv;
	LTBOOL         bFailed;

	bFailed = LTTRUE;

	pSrcBits = (uint32*)cis_LockSurfaceInlQ438(pSrc, &nSrcPitch, LTFALSE);

	if (pSrcBits)
	{
		pDestBits = (uint8*)cis_LockSurfaceInlQ438(pDest, &nDestPitch, LTTRUE);

		if (pDestBits)
		{
			bFailed = LTFALSE;

			nSrcPitchPixels = nSrcPitch / g_nBytesPerPixel;

			pEdgeLeft  = &pLeft[nYStart];
			pEdgeRight = &pRight[nYStart];

			for (nY = nYStart; nY < nYEnd; pEdgeLeft++, pEdgeRight++, nY++)
			{
				if (pEdgeRight->x != pEdgeLeft->x)
				{
					nWidth = pEdgeRight->x - pEdgeLeft->x;

					if (nWidth > 0)
					{
						pStart = pEdgeLeft;
						pEnd   = pEdgeRight;
					}
					else
					{
						pStart = pEdgeRight;
						pEnd   = pEdgeLeft;

						nWidth = -nWidth;
					}

					u = pStart->u;
					v = pStart->v;

					du = (pEnd->u - u) / nWidth;
					dv = (pEnd->v - v) / nWidth;

					pOut = (uint32*)(pDestBits +
					                 pStart->x * g_nBytesPerPixel +
					                 nY * nDestPitch);

					while (nWidth)
					{
						*pOut = pSrcBits[(v >> 16) * nSrcPitchPixels +
						                 (u >> 16)];

						pOut++;

						u += du;
						v += dv;

						nWidth--;
					}
				}
			}

			cis_UnlockSurfaceInlQ438(pDest);
		}

		cis_UnlockSurfaceInlQ438(pSrc);
	}

	if (bFailed)
		Q_RETURN_ERROR(1, "cis_DrawWarp", LT_ERROR);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0043b6a0 -- WarpSurfaceToSurfaceTransparent's 32-bit span fill.
//
//  Identical to the above but for the one test in the inner loop
//  (0x0043b852..0x0043b85c).  `cmp %ebp,%edx` is nPixel against the colour,
//  in that order, and the EQUAL case jumps over the store.
// ----------------------------------------------------------------------- //

LTRESULT cis_DrawWarp32TransparentQ438(CSurfaceQ438* pDest,
                                       CSurfaceQ438* pSrc,
                                       SWarpEdgeQ438* pLeft,
                                       SWarpEdgeQ438* pRight,
                                       uint32 nYStart, uint32 nYEnd,
                                       uint32 nUnused6Q438,
                                       uint32 nUnused7Q438)
{
	uint32*        pSrcBits;
	uint8*         pDestBits;
	uint32         nSrcPitch;
	uint32         nDestPitch;
	uint32         nSrcPitchPixels;
	uint32         nY;
	uint32*        pOut;
	uint32         nPixel;
	SWarpEdgeQ438* pEdgeLeft;
	SWarpEdgeQ438* pEdgeRight;
	SWarpEdgeQ438* pStart;
	SWarpEdgeQ438* pEnd;
	int            nWidth;
	int32          u, v, du, dv;
	LTBOOL         bFailed;

	bFailed = LTTRUE;

	pSrcBits = (uint32*)cis_LockSurfaceInlQ438(pSrc, &nSrcPitch, LTFALSE);

	if (pSrcBits)
	{
		pDestBits = (uint8*)cis_LockSurfaceInlQ438(pDest, &nDestPitch, LTTRUE);

		if (pDestBits)
		{
			bFailed = LTFALSE;

			nSrcPitchPixels = nSrcPitch / g_nBytesPerPixel;

			pEdgeLeft  = &pLeft[nYStart];
			pEdgeRight = &pRight[nYStart];

			for (nY = nYStart; nY < nYEnd; pEdgeLeft++, pEdgeRight++, nY++)
			{

				if (pEdgeRight->x != pEdgeLeft->x)
				{
					nWidth = pEdgeRight->x - pEdgeLeft->x;

					if (nWidth > 0)
					{
						pStart = pEdgeLeft;
						pEnd   = pEdgeRight;
					}
					else
					{
						pStart = pEdgeRight;
						pEnd   = pEdgeLeft;

						nWidth = -nWidth;
					}

					u = pStart->u;
					v = pStart->v;

					du = (pEnd->u - u) / nWidth;
					dv = (pEnd->v - v) / nWidth;

					pOut = (uint32*)(pDestBits +
					                 pStart->x * g_nBytesPerPixel +
					                 nY * nDestPitch);

					while (nWidth)
					{
						nPixel = pSrcBits[(v >> 16) * nSrcPitchPixels +
						                  (u >> 16)];

						if (nPixel != g_TransColorQ438)
							*pOut = nPixel;

						pOut++;

						u += du;
						v += dv;

						nWidth--;
					}
				}
			}

			cis_UnlockSurfaceInlQ438(pDest);
		}

		cis_UnlockSurfaceInlQ438(pSrc);
	}

	if (bFailed)
		Q_RETURN_ERROR(1, "cis_DrawWarp", LT_ERROR);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0043b940 -- WarpSurfaceToSurfaceSolidColor's 32-bit span fill.
//
//  The same test as the transparent one, with the SOURCE pixel replaced by
//  the fill colour in the store (0x0043bafc: `mov 0x4b1750,%edx` between
//  the test and the store, where the transparent form stores %edx itself).
// ----------------------------------------------------------------------- //

LTRESULT cis_DrawWarp32SolidColorQ438(CSurfaceQ438* pDest,
                                      CSurfaceQ438* pSrc,
                                      SWarpEdgeQ438* pLeft,
                                      SWarpEdgeQ438* pRight,
                                      uint32 nYStart, uint32 nYEnd,
                                      uint32 nUnused6Q438,
                                      uint32 nUnused7Q438)
{
	uint32*        pSrcBits;
	uint8*         pDestBits;
	uint32         nSrcPitch;
	uint32         nDestPitch;
	uint32         nSrcPitchPixels;
	uint32         nY;
	uint32*        pOut;
	uint32         nPixel;
	SWarpEdgeQ438* pEdgeLeft;
	SWarpEdgeQ438* pEdgeRight;
	SWarpEdgeQ438* pStart;
	SWarpEdgeQ438* pEnd;
	int            nWidth;
	int32          u, v, du, dv;
	LTBOOL         bFailed;

	bFailed = LTTRUE;

	pSrcBits = (uint32*)cis_LockSurfaceInlQ438(pSrc, &nSrcPitch, LTFALSE);

	if (pSrcBits)
	{
		pDestBits = (uint8*)cis_LockSurfaceInlQ438(pDest, &nDestPitch, LTTRUE);

		if (pDestBits)
		{
			bFailed = LTFALSE;

			nSrcPitchPixels = nSrcPitch / g_nBytesPerPixel;

			pEdgeLeft  = &pLeft[nYStart];
			pEdgeRight = &pRight[nYStart];

			for (nY = nYStart; nY < nYEnd; pEdgeLeft++, pEdgeRight++, nY++)
			{

				if (pEdgeRight->x != pEdgeLeft->x)
				{
					nWidth = pEdgeRight->x - pEdgeLeft->x;

					if (nWidth > 0)
					{
						pStart = pEdgeLeft;
						pEnd   = pEdgeRight;
					}
					else
					{
						pStart = pEdgeRight;
						pEnd   = pEdgeLeft;

						nWidth = -nWidth;
					}

					u = pStart->u;
					v = pStart->v;

					du = (pEnd->u - u) / nWidth;
					dv = (pEnd->v - v) / nWidth;

					pOut = (uint32*)(pDestBits +
					                 pStart->x * g_nBytesPerPixel +
					                 nY * nDestPitch);

					while (nWidth)
					{
						nPixel = pSrcBits[(v >> 16) * nSrcPitchPixels +
						                  (u >> 16)];

						if (nPixel != g_TransColorQ438)
							*pOut = g_FillColorQ438;

						pOut++;

						u += du;
						v += dv;

						nWidth--;
					}
				}
			}

			cis_UnlockSurfaceInlQ438(pDest);
		}

		cis_UnlockSurfaceInlQ438(pSrc);
	}

	if (bFailed)
		Q_RETURN_ERROR(1, "cis_DrawWarp", LT_ERROR);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x0043aa20 / 0x0043ad40 / 0x0043aff0 -- the three warp entries: the SDK's
//  WarpSurfaceToSurface, ...Transparent and ...SolidColor, the callbacks
//  clientinterface.cpp hands to 0x0040e120 as sub_43aa20 / sub_43ad40 /
//  sub_43aff0.  `cmp $0x2` / `cmp $0x3` on the screen bit depth at
//  0x004b175c; neither answers `mov $0x1,%eax` (LT_ERROR) with no message.
//
//  The 32-bit arm is the call to the clone above with two literal zeros.
//  The 16-bit arm is the SAME body at uint16 width (`mov %dx`, `add $0x2`,
//  `cmp 0x4b161c,%dx`), reached through an `inline` function that VC6
//  expands.  Written straight into the entry instead, the same statements
//  come out 639/616/623 against 653/630/636: the expanded return merges
//  the two epilogues differently and the screen arm of the source lock
//  lays out `jne ok` where the expansion gives `je fail` (0x0043aa7f).
//
//  WHICH HELPERS ARE EXPANDED.  VC6's per-function inline budget runs out
//  in these three:
//    0x0043aa20  source lock expanded, dest lock CALLED (0x0043aabc), both
//                unlocks expanded;
//    0x0043ad40  source lock expanded, dest lock CALLED (0x0043addc), dest
//    0x0043aff0  unlock expanded, source unlock CALLED (0x0043af25 /
//                0x0043b1db).
//  The clones expand all four.  So, as for the clones, each site names the
//  copy it gets: the `...InlQ438` pair where shipped expands, and the out-
//  of-line pair (0x0043acb0 / 0x0043afc0) where it calls.  (Measured the
//  other way too: with ONE inline lock/unlock pair and the three bodies as
//  templates, our budget expands both locks and calls both unlocks in all
//  three entries; it takes ~32 dead statements of padding in each body to
//  reproduce shipped's split, so the true original is a body that costs
//  VC6 more than this one.  Naming the copies is the honest stand-in.)
// ----------------------------------------------------------------------- //

// 0x004b175c is +0x04 of the screen PFormat at 0x004b1758 (cissurf.cpp's
// g_ScreenFormat), its bit-depth code -- a field, not a global of its own.
class PFormatQ438
{
public:
	void*  m_pVtable;                        // 0x00
	uint32 m_nBitDepth;                      // 0x04  2 == 16-bit, 3 == 32-bit
	uint8  m_pad_08[0x38 - 0x08];
};

extern PFormatQ438 g_ScreenFormatQ438;                 // 0x004b1758

inline LTRESULT cis_DrawWarp16Q438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
	SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
	uint32 nYStart, uint32 nYEnd)
{
	uint16*        pSrcBits;
	uint8*         pDestBits;
	uint32         nSrcPitch;
	uint32         nDestPitch;
	uint32         nSrcPitchPixels;
	uint32         nY;
	uint16*        pOut;
	uint16         nPixel;
	SWarpEdgeQ438* pEdgeLeft;
	SWarpEdgeQ438* pEdgeRight;
	SWarpEdgeQ438* pStart;
	SWarpEdgeQ438* pEnd;
	int            nWidth;
	int32          u, v, du, dv;
	LTBOOL         bFailed;

	bFailed = LTTRUE;

	pSrcBits = (uint16*)cis_LockSurfaceInlQ438(pSrc, &nSrcPitch, LTFALSE);

	if (pSrcBits)
	{
		pDestBits = (uint8*)cis_LockSurfaceQ438(pDest, &nDestPitch, LTTRUE);

		if (pDestBits)
		{
			bFailed = LTFALSE;

			nSrcPitchPixels = nSrcPitch / g_nBytesPerPixel;

			pEdgeLeft  = &pLeft[nYStart];
			pEdgeRight = &pRight[nYStart];

			for (nY = nYStart; nY < nYEnd; pEdgeLeft++, pEdgeRight++, nY++)
			{
				if (pEdgeRight->x != pEdgeLeft->x)
				{
					nWidth = pEdgeRight->x - pEdgeLeft->x;

					if (nWidth > 0)
					{
						pStart = pEdgeLeft;
						pEnd   = pEdgeRight;
					}
					else
					{
						pStart = pEdgeRight;
						pEnd   = pEdgeLeft;

						nWidth = -nWidth;
					}

					u = pStart->u;
					v = pStart->v;

					du = (pEnd->u - u) / nWidth;
					dv = (pEnd->v - v) / nWidth;

					pOut = (uint16*)(pDestBits +
					                 pStart->x * g_nBytesPerPixel +
					                 nY * nDestPitch);

					while (nWidth)
					{
						nPixel = pSrcBits[(v >> 16) * nSrcPitchPixels +
						                  (u >> 16)];

						*pOut = nPixel;

						pOut++;

						u += du;
						v += dv;

						nWidth--;
					}
				}
			}

			cis_UnlockSurfaceInlQ438(pDest);
		}

		cis_UnlockSurfaceInlQ438(pSrc);
	}

	if (bFailed)
		Q_RETURN_ERROR(1, "cis_DrawWarp", LT_ERROR);

	return LT_OK;
}

LTRESULT cis_WarpSurfaceToSurfaceQ438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
	SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
	uint32 nYStart, uint32 nYEnd)
{
	if (g_ScreenFormatQ438.m_nBitDepth == 2)
	{
		return cis_DrawWarp16Q438(pDest, pSrc, pLeft, pRight, nYStart, nYEnd);
	}
	else if (g_ScreenFormatQ438.m_nBitDepth == 3)
	{
		return cis_DrawWarp32Q438(pDest, pSrc, pLeft, pRight, nYStart, nYEnd,
			0, 0);
	}

	return LT_ERROR;
}


inline LTRESULT cis_DrawWarp16TransparentQ438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
	SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
	uint32 nYStart, uint32 nYEnd)
{
	uint16*        pSrcBits;
	uint8*         pDestBits;
	uint32         nSrcPitch;
	uint32         nDestPitch;
	uint32         nSrcPitchPixels;
	uint32         nY;
	uint16*        pOut;
	uint16         nPixel;
	SWarpEdgeQ438* pEdgeLeft;
	SWarpEdgeQ438* pEdgeRight;
	SWarpEdgeQ438* pStart;
	SWarpEdgeQ438* pEnd;
	int            nWidth;
	int32          u, v, du, dv;
	LTBOOL         bFailed;

	bFailed = LTTRUE;

	pSrcBits = (uint16*)cis_LockSurfaceInlQ438(pSrc, &nSrcPitch, LTFALSE);

	if (pSrcBits)
	{
		pDestBits = (uint8*)cis_LockSurfaceQ438(pDest, &nDestPitch, LTTRUE);

		if (pDestBits)
		{
			bFailed = LTFALSE;

			nSrcPitchPixels = nSrcPitch / g_nBytesPerPixel;

			pEdgeLeft  = &pLeft[nYStart];
			pEdgeRight = &pRight[nYStart];

			for (nY = nYStart; nY < nYEnd; pEdgeLeft++, pEdgeRight++, nY++)
			{
				if (pEdgeRight->x != pEdgeLeft->x)
				{
					nWidth = pEdgeRight->x - pEdgeLeft->x;

					if (nWidth > 0)
					{
						pStart = pEdgeLeft;
						pEnd   = pEdgeRight;
					}
					else
					{
						pStart = pEdgeRight;
						pEnd   = pEdgeLeft;

						nWidth = -nWidth;
					}

					u = pStart->u;
					v = pStart->v;

					du = (pEnd->u - u) / nWidth;
					dv = (pEnd->v - v) / nWidth;

					pOut = (uint16*)(pDestBits +
					                 pStart->x * g_nBytesPerPixel +
					                 nY * nDestPitch);

					while (nWidth)
					{
						nPixel = pSrcBits[(v >> 16) * nSrcPitchPixels +
						                  (u >> 16)];

						if (nPixel != (uint16)g_TransColorQ438)
							*pOut = nPixel;

						pOut++;

						u += du;
						v += dv;

						nWidth--;
					}
				}
			}

			cis_UnlockSurfaceInlQ438(pDest);
		}

		cis_UnlockSurfaceQ438(pSrc);
	}

	if (bFailed)
		Q_RETURN_ERROR(1, "cis_DrawWarp", LT_ERROR);

	return LT_OK;
}

LTRESULT cis_WarpSurfaceToSurfaceTransparentQ438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
	SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
	uint32 nYStart, uint32 nYEnd)
{
	if (g_ScreenFormatQ438.m_nBitDepth == 2)
	{
		return cis_DrawWarp16TransparentQ438(pDest, pSrc, pLeft, pRight, nYStart, nYEnd);
	}
	else if (g_ScreenFormatQ438.m_nBitDepth == 3)
	{
		return cis_DrawWarp32TransparentQ438(pDest, pSrc, pLeft, pRight, nYStart, nYEnd,
			0, 0);
	}

	return LT_ERROR;
}


inline LTRESULT cis_DrawWarp16SolidColorQ438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
	SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
	uint32 nYStart, uint32 nYEnd)
{
	uint16*        pSrcBits;
	uint8*         pDestBits;
	uint32         nSrcPitch;
	uint32         nDestPitch;
	uint32         nSrcPitchPixels;
	uint32         nY;
	uint16*        pOut;
	uint16         nPixel;
	SWarpEdgeQ438* pEdgeLeft;
	SWarpEdgeQ438* pEdgeRight;
	SWarpEdgeQ438* pStart;
	SWarpEdgeQ438* pEnd;
	int            nWidth;
	int32          u, v, du, dv;
	LTBOOL         bFailed;

	bFailed = LTTRUE;

	pSrcBits = (uint16*)cis_LockSurfaceInlQ438(pSrc, &nSrcPitch, LTFALSE);

	if (pSrcBits)
	{
		pDestBits = (uint8*)cis_LockSurfaceQ438(pDest, &nDestPitch, LTTRUE);

		if (pDestBits)
		{
			bFailed = LTFALSE;

			nSrcPitchPixels = nSrcPitch / g_nBytesPerPixel;

			pEdgeLeft  = &pLeft[nYStart];
			pEdgeRight = &pRight[nYStart];

			for (nY = nYStart; nY < nYEnd; pEdgeLeft++, pEdgeRight++, nY++)
			{
				if (pEdgeRight->x != pEdgeLeft->x)
				{
					nWidth = pEdgeRight->x - pEdgeLeft->x;

					if (nWidth > 0)
					{
						pStart = pEdgeLeft;
						pEnd   = pEdgeRight;
					}
					else
					{
						pStart = pEdgeRight;
						pEnd   = pEdgeLeft;

						nWidth = -nWidth;
					}

					u = pStart->u;
					v = pStart->v;

					du = (pEnd->u - u) / nWidth;
					dv = (pEnd->v - v) / nWidth;

					pOut = (uint16*)(pDestBits +
					                 pStart->x * g_nBytesPerPixel +
					                 nY * nDestPitch);

					while (nWidth)
					{
						nPixel = pSrcBits[(v >> 16) * nSrcPitchPixels +
						                  (u >> 16)];

						if (nPixel != (uint16)g_TransColorQ438)
							// STAND-IN.  0x0043b167 loads the fill as a DWORD
							// (`mov 0x4b1750,%edx`) and stores %dx; every
							// direct spelling -- cast, implicit, int/volatile/
							// __int64 declarations, an inline getter or a
							// uint32 parameter -- narrows the load to `mov
							// 0x4b1750,%dx` (one byte longer).  Only a select
							// VC6 folds AFTER choosing the load width keeps it
							// whole, which suggests the original chose the
							// stored value with a runtime-looking select.
							// cissurf.cpp's 0x0040d230 has the same load.
							*pOut = (uint16)(nPixel ? g_FillColorQ438
							                        : g_FillColorQ438);

						pOut++;

						u += du;
						v += dv;

						nWidth--;
					}
				}
			}

			cis_UnlockSurfaceInlQ438(pDest);
		}

		cis_UnlockSurfaceQ438(pSrc);
	}

	if (bFailed)
		Q_RETURN_ERROR(1, "cis_DrawWarp", LT_ERROR);

	return LT_OK;
}

LTRESULT cis_WarpSurfaceToSurfaceSolidColorQ438(CSurfaceQ438* pDest, CSurfaceQ438* pSrc,
	SWarpEdgeQ438* pLeft, SWarpEdgeQ438* pRight,
	uint32 nYStart, uint32 nYEnd)
{
	if (g_ScreenFormatQ438.m_nBitDepth == 2)
	{
		return cis_DrawWarp16SolidColorQ438(pDest, pSrc, pLeft, pRight, nYStart, nYEnd);
	}
	else if (g_ScreenFormatQ438.m_nBitDepth == 3)
	{
		return cis_DrawWarp32SolidColorQ438(pDest, pSrc, pLeft, pRight, nYStart, nYEnd,
			0, 0);
	}

	return LT_ERROR;
}



// ======================================================================= //
//  NOT WRITTEN -- what the rest of the enumeration is
//
//  All of them are real, single functions: there is no
//  branch target leaving any of them, no second prologue, no trailing
//  padding and no overrun, and the extra `ret`s are all duplicated /O2
//  epilogues reached from inside.  They are left alone for reasons of
//  confidence, not of doubt about the extents.
//
//  0x0043a4f0  (1130)  cis_ClipWarpPoly -- the master of PART THREE.
//      `(pCoords, pnCoords, fLeft, fRight, fTop, fBottom)`: arg0 is an
//      LTWarpPt**, arg1 an int*, and args 2..5 are the four plane
//      constants, published into 0x004b3e18 / 0x004b3690 / 0x004b3e1c /
//      0x004b3e24 at 0x0043a502..0x0043a527.  It then runs the SAME loop
//      PART THREE reconstructs four times: three of the four are inlined
//      (0x0043a54d tests `dest_x < 0x004b3e18` with the sense inverted,
//      and two more follow at 0x0043a6fc and 0x0043a849), and the fourth
//      is the call to 0x0043b270 at 0x0043a941.  Its answer is squeezed to
//      0/1 by `neg/sbb/neg` at the call site.  Not written because getting
//      three inlined copies of a loop to come out in the right order is a
//      different job from getting one out-of-line copy right, and the one
//      copy should be measured first.
//
//  0x0043b410 (655), 0x0043b6a0 (664), 0x0043b940 (670)
//      WRITTEN -- see PART FOUR above.  The block that follows this
//      one is what is left of the survey.
//
//  0x0043beb0 (1106)  RECURSIVE, and that is the first thing to know about
//      it: of its two call sites, 0x0043c09c is INSIDE its own body and
//      only 0x0043c34a (in the 68-byte wrapper at 0x0043c310) is external.
//      It is a BSP walk, not part of the warp blitter at all -- `testb
//      $0x3,0x16(%edi)` on the node, then `(%edi) -> +0x28 -> +0x34` to
//      reach a four-float plane and two `n.x*a + n.y*b + n.z*c - d` dot
//      products at 0x0043bedd..0x0043bf19, each compared against the
//      constant at 0x004a1e08.  Eight dwords of arguments (`add
//      $0x20,%esp`): the node, one more pointer, and two LTVectors BY
//      VALUE (0x68/0x6c/0x70 and 0x74/0x78/0x7c), which 0x0043c310 builds
//      with two `sub $0xc,%esp` plus three-dword copies.  Not examined
//      further -- it wants to be read next to src/engine/lithtech/
//      worldtree.cpp, not next to the surface code above.
// ======================================================================= //
