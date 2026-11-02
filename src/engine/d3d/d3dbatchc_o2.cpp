// *********************************************************************** //
//
//  MODULE:   BATCH_C_O2 (.CPP)
//
//  PURPOSE:  The rows of batch C whose shipped bodies were compiled at /O2,
//            split out of d3dbatchc.cpp (an /Os file in build/engine.sh).
//
//  WHY A SEPARATE FILE. icl 4.0 at /Os aligns function starts to FOUR bytes;
//  at /O2 it aligns them to SIXTEEN and fills the gap with lea-nops. The
//  shipped image pads all six of these to sixteen:
//
//      0x10009cc0   42b body + 6b  lea 0x0(%esi),%esi      (48)
//      0x10009ef0   17b body + 15b lea-nop run            (32)
//      0x10023e50   81b body + 15b nop/lea/lea            (96)
//      0x1007c140  257b body + 15b                        (272)
//      0x10009c90   33b body + 15b  (and see its block: it must not see
//                  0x1000a690's body, which icl would inline)
//      0x1007b8e0  124b body + 4b, and 10b of lea-nop INSIDE, aligning
//                  its else-arm to sixteen (branch-target alignment is
//                  also /O2-only)
//
//  Measured by compiling d3dbatchc.cpp both ways: at /Os the first three
//  have the same body and a 2/3/3-byte pad (`mov %esi,%esi`, `lea
//  0x0(%esi),%esi`), so placement fails on the padding
//  alone; at /O2 they are byte-identical INCLUDING the fill. 0x1007c140 is
//  179 bytes and 152 differing at /Os, 257 bytes and 2 differing at /O2 (a
//  store-order swap, fixed in its body below). 0x1007b8e0 and 0x10009c90
//  needed source changes as well (their blocks say what). Every other row
//  of batch C scores the same or better at /Os and stays there.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <stdlib.h>
#include <string.h>

// ======================================================================= //
//  THE ENGINE'S RenderStruct AT 0x1008cbcc
//
//  d3dmid.cpp already pins +0x34 dalloc and +0x38 dfree and the struct below
//  keeps them at those offsets so the two translation units agree.
//
//  THREE NEW SLOTS, and they are the console-variable accessors:
//
//    +0x18  takes ONE argument and 0x10032a50 passes it the string
//           "ForceMode" (0x100a8b80); the answer is null-tested.
//    +0x1c  takes the +0x18 answer and returns a DOUBLE -- 0x1007c1b3
//           spills it with `fstpl`, not `fstps`.
//    +0x20  takes the +0x18 answer and returns something that 0x10032a35
//           hands straight to the CRT's _stricmp at 0x100874b0, so it is a
//           string.
//
//  Every one of the three is __cdecl: 0x1007c196 and 0x1007c1b0 restore the
//  outgoing area themselves.
// ======================================================================= //

struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x18];                        // +0x00..+0x17
	void*   (*GetConsoleVariable)(const char* pszName); // +0x18
	double  (*GetVarValueFloat)(void* pVar);            // +0x1c
	char*   (*GetVarValueString)(void* pVar);           // +0x20
	uint8    m_Reserved24[0x10];                        // +0x24..+0x33
	void*   (*dalloc)(uint32 nSize);                    // +0x34
	void    (*dfree)(void* pMem);                       // +0x38
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ======================================================================= //
//  THE D3D7 DEVICE AT 0x1008d988 -- copied verbatim from d3dopt2d.cpp so the
//  two files agree on the one slot this file needs (+0x50 SetRenderState).
// ======================================================================= //

struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

#define D3DRENDERSTATE_ZWRITEENABLE          14         // 0x0e
#define D3DRENDERSTATE_ALPHABLENDENABLE      27         // 0x1b


// ======================================================================= //
//  THE ALLOCATOR INTERFACE
//
//  0x10009cc0 and 0x1007eb80 both take a pointer whose vtable slot +0x04 is
//  called with ONE pushed argument and %ecx holding the object -- an
//  ordinary virtual __thiscall. Slot +0x00 is never touched here and is only
//  present to put Alloc at +0x04.
// ======================================================================= //

class IAllocator
{
public:
	virtual void* Slot00() = 0;                         // +0x00, unused here
	virtual void* Alloc(uint32 nBytes) = 0;             // +0x04
};


// ======================================================================= //
//  0x10009ef0 -- see d3dbatchc.cpp's 0x1001c790 block for the decode; the
//  two are one function with the element width changed (0x20 here).
// ======================================================================= //

struct Elem10009ef0                                     // 0x20 bytes
{
	uint8 m_Reserved00[0x20];
};

class C10009ef0
{
public:
	Elem10009ef0* sub_10009ef0(uint32 nIndex);          // 0x10009ef0

	uint32         m_n00;                               // +0x00
	Elem10009ef0*  m_pElements;                         // +0x04
};

Elem10009ef0* C10009ef0::sub_10009ef0(uint32 nIndex)
{
	return &m_pElements[nIndex];
}


// ======================================================================= //
//  0x10009cc0   36 bytes of body, then a six-byte lea-nop. Four callers.
//
//      push %ebp ; mov %esp,%ebp ; sub $0xc,%esp
//      mov 0x8(%ebp),%eax ; add %eax,%eax
//      mov 0xc(%ebp),%ecx
//      add %eax,%eax                       <- eax = n * 4
//      push %eax
//      mov (%ecx),%eax ; mov 0x4(%eax),%eax ; call *%eax
//      test %eax,%eax ; je -> xor %edx,%edx
//      mov %eax,%edx
//      mov %edx,%eax ; mov %ebp,%esp ; pop %ebp ; ret $0x8
//
//  WHAT PINS IT:
//   * %ecx is LOADED from 0xc(%ebp), so this is not __thiscall; `ret $0x8`
//     makes it __stdcall with two arguments.
//   * the second argument goes into %ecx and its vtable slot +0x04 is called
//     with one pushed dword -- a virtual __thiscall Alloc(size).
//   * `add %eax,%eax` twice is n * 4, so the element is four bytes wide.
//   * the null test SURVIVES even though both arms return the same value.
//     That is the whole reason this function has a branch at all, and it is
//     what says the source really does test the result before handing it
//     back. Its 16-byte sibling 0x1007eb80 below has the identical test in
//     front of a per-element initialiser, which is where the shape comes
//     from: these are two instantiations of one array-allocating template
//     whose element here has nothing to initialise.
// ======================================================================= //

class C10009cc0
{
public:
	void* sub_10009cc0(uint32 nCount, IAllocator* pAllocator);

	uint32 m_nUnknown00;                                // +0x00, never read
};

void* C10009cc0::sub_10009cc0(uint32 nCount, IAllocator* pAllocator)
{
	void** pArray = (void**)pAllocator->Alloc(nCount * 4);
	void** pRet;

	if (pArray)
		pRet = pArray;
	else
		pRet = LTNULL;

	return pRet;
}


// ======================================================================= //
//  0x10023e50   81 bytes of body, then padding. Three callers.
//
//  WHAT IT IS: a two-line render-state helper on the DX7 device --
//  alpha blending off, and Z writes on when asked.
//
//  WHAT PINS IT:
//   * a bare `ret` and arguments read from %ebp: __cdecl. The body reads
//     only 0x8(%ebp), but all three call sites lay down TWO dwords
//     (0x10030cbd `push %edi; push $0x0`, 0x10030dac the same, and
//     0x1004fe21 writes only (%esp) of an eight-byte outgoing area), so
//     there are two parameters and the second is dead.
//   * both calls go through the device's vtable +0x50 with three dwords and
//     no stack restore afterwards -- __stdcall, i.e. COM. d3d.cpp pins +0x50
//     as IDirect3DDevice7::SetRenderState.
//   * state 0x1b is D3DRENDERSTATE_ALPHABLENDENABLE and it is set to 0
//     UNCONDITIONALLY, before the test.
//   * state 0x0e is D3DRENDERSTATE_ZWRITEENABLE, set to 1 only when the
//     first argument is non-zero. There is no else arm: the fall-through
//     from `je 0x10023e9d` is the epilogue.
// ======================================================================= //

void sub_10023e50(uint32 bZWriteEnable, void* pUnused)
{
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		D3DRENDERSTATE_ALPHABLENDENABLE, 0);

	if (bZWriteEnable)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
			D3DRENDERSTATE_ZWRITEENABLE, 1);
	}
}


// ======================================================================= //
//  0x1007c140   239 bytes of body (0x1007c140..0x1007c22e), then padding.
//               FIFTEEN callers, all of them in the block
//               0x1007bebc..0x1007c10c, each one a
//               `pThis->sub_1007c140(pOwner, "<NAME>", 0x80)`.
//
//  WHAT IT IS: (re)allocate a counted dword table whose length comes from a
//  console variable, falling back to a caller-supplied default.
//
//  WHAT PINS IT:
//   * `ret $0xc` with %ecx read and never loaded: __thiscall, three
//     arguments, at 0x8(%ebp), 0xc(%ebp) and 0x10(%ebp).
//   * 0x1007bea2 sets up a call as
//         %ecx = pOwner + 0x144, (%esp) = pOwner,
//         0x4(%esp) = "VS_MODELS", 0x8(%esp) = 0x80
//     so `this` is a sub-object of the first argument, the second argument
//     is a name and the third a count. 0x1007bee7 repeats it with
//     "VS_MODELS_TRANSLUCENT", and thirteen more follow.
//   * 0x10080479 is the CRT free (it tail-calls 0x10080b8e through a single
//     forwarded argument) and 0x10081222 is the CRT malloc (it is
//     `push $1; push size; call _nh_malloc`).
//   * the same second argument -- the name -- is used TWICE: once as the
//     second argument to 0x1007c250 and once as the console variable name.
//     %esi holds it across both.
//   * 0x1007c250 is __thiscall on the same `this` with two arguments (it
//     ends `ret $0x8`) and its answer is discarded; the outgoing area is not
//     restored after it, which is what says it pops.
//   * the +0x1c console accessor's answer is spilled with `fstpl`, so it is
//     a DOUBLE, and 0x1007c1b6..0x1007c1f8 is the compiler's own inline
//     double-to-int32 expansion: pull the exponent with
//     `shr $0x14 ; and $0x7ff`, bail to zero when it is <= 0x3fe (|x| < 1),
//     else `shld $0xb` the mantissa, set the implicit bit, shift right by
//     0x41e - exponent and negate on the sign. Nothing in the source
//     produces that except a plain `(int32)` cast.
//   * the fallback arm is out of line at 0x1007c22d and jumps BACK into the
//     common tail, so the console-variable arm is the fall-through.
//   * `mov %eax,-0x28(%ebp)` at 0x1007c1fc is a store of the count to a
//     frame slot that is never read again -- icl keeping a local in memory
//     as well as in %esi. It is not a second variable.
//   * malloc's answer is STORED into +0x00 before it is tested
//     (`test %eax,%eax ; mov %eax,(%ebx) ; je`), and +0x14 is only written
//     on the success path.
//
//  CONFIDENCE: this is the one function in this file that is INFERRED
//  rather than fully DECODED. Every instruction is read, but two things
//  depend on icl's behaviour rather than on the source: the inline
//  double-to-int expansion, and the dead frame store at -0x28(%ebp).
// ======================================================================= //

class C1007c140
{
public:
	LTBOOL sub_1007c140(void* pOwner, const char* pszVarName,
	                    int32 nDefaultCount);           // 0x1007c140
	LTBOOL sub_1007c250(void* pOwner, const char* pszVarName);

	uint32* m_pTable;                                   // +0x00
	uint32  m_n04;                                      // +0x04
	uint32  m_n08;                                      // +0x08
	uint32  m_n0c;                                      // +0x0c
	uint32  m_n10;                                      // +0x10
	int32   m_nCount14;                                 // +0x14
};

LTBOOL C1007c140::sub_1007c140(void* pOwner, const char* pszVarName,
                               int32 nDefaultCount)
{
	void* pVar;
	int32 nCount;

	if (m_pTable)
	{
		free(m_pTable);
		m_pTable = LTNULL;
	}

	// icl emits these two stores in the REVERSE of source order: written
	// +0x04 then +0x14 they came out +0x14 first, and the shipped bytes
	// store +0x04 first (0x1007c171).
	m_nCount14 = 0;
	m_n04 = 0;

	sub_1007c250(pOwner, pszVarName);

	pVar = g_pStruct->GetConsoleVariable(pszVarName);
	if (pVar)
		nCount = (int32)g_pStruct->GetVarValueFloat(pVar);
	else
		nCount = nDefaultCount;

	m_pTable = (uint32*)malloc(nCount * 4);
	if (m_pTable == LTNULL)
		return LTFALSE;

	m_nCount14 = nCount;
	return LTTRUE;
}


// ======================================================================= //
//  0x1007b8e0   122 bytes of body (0x1007b8e0..0x1007b95b), ten of which are
//               lea-nop padding INSIDE the function at 0x1007b926 aligning
//               the else-arm to sixteen -- /O2 branch-target alignment,
//               which is why this lives here. Eleven callers.
//
//  WHAT IT IS: walk an array of object pointers and invoke a caller-supplied
//  callback on the ones that pass a two-flag filter.
//
//      mov 0x20(%esp),%esi         <- entry+4 : the context
//      call *0x24(%esp)            <- entry+8 : the callback
//      ret $0x8                    <- __thiscall, two arguments
//
//      edi = this->+0x04           <- the count; `test ; jbe` -> unsigned
//   L: eax = this->+0x00           <- the base, RELOADED every iteration
//      edx = base[i]
//      eax = edx->+0x88
//      if (eax & 1) {
//          if (ctx->+0x4d4 == 0)          -> call
//          else if (edx->+0x8c & 1)       -> skip
//          else                           -> call
//      } else {
//          if (ctx->+0x4d4 == 0)          -> skip
//          else if (!(eax & 0x400))       -> skip     (`test $4,%ah`)
//          else                           -> call
//      }
//      call: (%esp)=ctx ; 0x4(%esp)=edx ; call *0x24(%esp)
//            edi = this->+0x04             <- the count is reloaded after it
//      i++ ; cmp %edi,%ebx ; jb L
//
//  WHAT PINS IT:
//   * `ret $0x8` with %ecx read and never loaded: __thiscall, two arguments.
//   * the array is `Elem**` -- `mov (%eax,%ebx,4),%edx` then dereferences
//     %edx -- so +0x00 holds pointers, four bytes apart, and +0x04 the count.
//   * the outgoing area is opened ONCE by the prologue's `sub $0xc,%esp` and
//     never restored around the call, so the callback is __cdecl.
//   * the callback takes (context, element) in that order: the context is at
//     (%esp), the element at 0x4(%esp).
//   * `test $0x4,%ah` on a dword loaded from +0x88 is bit 10, i.e. 0x400.
//   * both the base and the count are reloaded after the callback returns,
//     which is why they are read inside the loop below rather than hoisted.
//   * there is ONE call site (0x1007b93f), and both arms reach it: the
//     flag-1 arm jumps to it, the else arm falls into it. So the source
//     filters with `continue` and calls once at the bottom. Two calls, one
//     per arm, came out 140 bytes at /O2 (114 at /Os); one call is
//     byte-identical at /O2.
// ======================================================================= //

struct Ctx1007b8e0
{
	uint8  m_Reserved00[0x4d4];                         // +0x000..+0x4d3
	uint32 m_n4d4;                                      // +0x4d4
};

struct Elem1007b8e0
{
	uint8  m_Reserved00[0x88];                          // +0x00..+0x87
	uint32 m_nFlags88;                                  // +0x88
	uint32 m_nFlags8c;                                  // +0x8c
};

typedef void (*Fn1007b8e0)(Ctx1007b8e0* pContext, Elem1007b8e0* pElem);

class C1007b8e0
{
public:
	void sub_1007b8e0(Ctx1007b8e0* pContext, Fn1007b8e0 pfn);   // 0x1007b8e0

	Elem1007b8e0** m_pElements;                         // +0x00
	uint32         m_nCount;                            // +0x04
};

void C1007b8e0::sub_1007b8e0(Ctx1007b8e0* pContext, Fn1007b8e0 pfn)
{
	uint32        i;
	Elem1007b8e0* pElem;

	for (i = 0; i < m_nCount; i++)
	{
		pElem = m_pElements[i];

		if (pElem->m_nFlags88 & 1)
		{
			if (pContext->m_n4d4 != 0 && (pElem->m_nFlags8c & 1) != 0)
				continue;
		}
		else
		{
			if (pContext->m_n4d4 == 0 || (pElem->m_nFlags88 & 0x400) == 0)
				continue;
		}

		pfn(pContext, pElem);
	}
}


// ======================================================================= //
//  0x1000a690's class -- declared only. The getter itself stays in
//  d3dbatchc.cpp (/Os); see its block there.
// ======================================================================= //

class C1000a690
{
public:
	uint32 sub_1000a690();                              // 0x1000a690

	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
};


// ======================================================================= //
//  0x10009c90   33 bytes of body (0x10009c90..0x10009cb0 inclusive), then
//               nine bytes of padding. Twelve callers.
//
//  WHAT PINS IT:
//   * bare `ret`, %ecx read and never loaded: __thiscall, no arguments.
//   * `mov $0xc,%ecx ; add %ebx,%ecx ; call 0x1000a690` -- the callee is
//     invoked on `this + 0x0c`, so there is a C1000a690 sub-object at +0x0c.
//   * `add 0x8(%ebx),%eax` AFTER the call: the sum is (sub-object's first
//     dword) + (this->+0x08), in that order. The call result is the left
//     operand, which is why the add is the memory form and not a reload.
//   * the callee is CALLED, not inlined, on all twelve of this function's
//     paths and at all fifty-eight sites elsewhere.
//
//  WHY IT IS IN THIS FILE, measured: icl inlines across a whole
//  translation unit regardless of definition order -- with 0x1000a690
//  defined anywhere in the same file, even after this caller and not
//  `inline`, this came out as `mov 0xc(%ecx),%eax ; add 0x8(%ecx),%eax`
//  (11 bytes, 10 differing). The shipped function CALLS it, so the getter
//  was in another TU; here only its declaration is visible. The 16-byte
//  lea-nop fill after the `ret` is /O2's.
//
//  0x1001c3e6 shows what the answer means: `Get() + this->m_n08` is compared
//  against a running pointer, so +0x08 is a base or offset and the
//  sub-object's first dword is a length added to it.
// ======================================================================= //

class C10009c90
{
public:
	uint32 sub_10009c90();                              // 0x10009c90

	uint32    m_n00;                                    // +0x00
	uint32    m_n04;                                    // +0x04
	uint32    m_n08;                                    // +0x08
	C1000a690 m_Sub0c;                                  // +0x0c
};

uint32 C10009c90::sub_10009c90()
{
	return m_Sub0c.sub_1000a690() + m_n08;
}
