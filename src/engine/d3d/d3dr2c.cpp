// *********************************************************************** //
//
//  MODULE:   BATCH_C (.CPP)  --  d3d.ren, batch C
//
//  PURPOSE:  Reconstructed source for the 25 rows of batch_c.txt.
//            Intel C/C++ 4.0, per-file /O2 or /Os.  Every offset, constant
//            and branch below was read off the retail d3d.ren.
//
//  THE BIG FIND IN THIS BATCH: SEVEN of the twenty-five rows are the SAME
//  THREE FUNCTIONS.  They are one-line member forwarders that supply a
//  trailing constant argument -- the address of the global at 0x10091df0 --
//  to another member function.  Within a group the shipped bytes are
//  IDENTICAL except for the call displacement (a relocation, which
//  the byte comparison masks), so one correct source shape lands three functions:
//
//    group 1 (one argument,  ret $4):  0x1001d340  0x1007ef40  0x1007c9d0
//    group 2 (two arguments, ret $8):  0x10009ec0  0x1001e010
//    group 3 (arg + NULL,    ret $4):  0x1001c7b0  0x1005a390
//
//  The tell for the shape is the pointless `mov %ecx,%eax; mov %eax,%ecx`
//  pair: icl parks `this` in %eax for the body and hands it back to %ecx
//  for the nested thiscall.  Every one of the seven has it, and so does
//  every neighbouring forwarder in the module that is not on the list
//  (0x1001d370, 0x1005a3c0, 0x10037570, 0x10037590 ...).
//
//  A SECOND FIND, which is why two dead-looking stores are written out
//  longhand below: ICL 4.0 DOES NOT DO DEAD-STORE ELIMINATION THROUGH
//  MEMORY.  0x10037550 writes a vtable pointer to (%eax) and then
//  immediately overwrites (%eax) with a list link, and 0x100646d0 loads
//  (%eax) into %edx and stores %edx straight back to (%eax).  Both stores
//  survived /O2 in the shipped image, so the same source survives here.
//
//  TWO SDK HEADER FUNCTIONS turn up again, exactly as batch D found:
//    0x10023350  _CVector<T>::operator-()   ltvector.h:54
//    0x1004e300  MatVMul_InPlace            ltmatrix.h:213
//  Both are copied character for character from src/LT2/sdk/inc.  MatVMul
//  itself (0x10014940) is CALLED, not inlined -- it is over icl's /Ob1
//  size budget -- so it is declared here and left out of line.
//
//  SIX ROWS ARE NOT WRITTEN.  Five of them are the register-ABI class the
//  filter was supposed to remove and one is a custom-convention CALL; the
//  reason for each is at the bottom of this file.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  Shared vector / matrix types.
//
//  Named LTVecC / LTMatC rather than LTVector / LTMatrix so that this file
//  cannot collide with the hand-rolled copies batch D already put in
//  d3dbatchd.cpp -- the bodies are the SDK's, the names are local.
// ----------------------------------------------------------------------- //

class LTVecC
{
public:
			LTVecC() {}
			LTVecC(float mx, float my, float mz) {x=mx; y=my; z=mz;}

	LTVecC	sub_10023350() const;                       // operator - ()

	float	x;
	float	y;
	float	z;
};

struct LTMatC
{
	float m[4][4];
};

// ltmatrix.h:206.  Out of line: the shipped module calls it.
void sub_10014940(LTVecC *pDest, LTMatC *pMat, LTVecC *pSrc);


// ----------------------------------------------------------------------- //
//  0x10023350 -- 32 bytes (the row's 48 counts the lea-nop padding),
//  2 callers.  _CVector<T>::operator-(), ltvector.h:54.
//
//  WHAT PINS IT: a 12-byte struct return through the hidden pointer at
//  0x4(%esp) with `this` still in %ecx, `ret $4`; three `flds ; fchs` pairs
//  pushed x, y, z in THAT order, then one `fxch %st(2)` to bring -x back to
//  the top before the three `fstps` walk +0, +4, +8.  That fxch is what the
//  header's `_CVector<T>(-x, -y, -z)` costs: the constructor stores in
//  declaration order but the arguments were evaluated onto the x87 stack
//  last-first.
// ----------------------------------------------------------------------- //
LTVecC LTVecC::sub_10023350() const
{
	return LTVecC(-x, -y, -z);
}


// ----------------------------------------------------------------------- //
//  0x1004e300 -- 50 bytes, 2 callers.  MatVMul_InPlace, ltmatrix.h:213.
//
//  WHAT PINS IT: the callee 0x10014940 is MatVMul itself -- it reads
//  m[0][0..3] against (%edx),4(%edx),8(%edx) and adds m[0][3], row by row,
//  writing (%ecx),4(%ecx),8(%ecx) -- so the three pushed arguments are
//  (&temp, pMat, pSrc) in the header's order.  The `rep movsl` with
//  %ecx = 3 straight afterwards, into the SAME pointer that was argument
//  three, is `*pSrc = temp`.  Both callers push (pMat, &vec).
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  THE FORWARDER FAMILY.
//
//  0x10091df0 is uninitialised data -- a global object whose ADDRESS is the
//  constant these three shapes append to their argument list.  Every one of
//  the seven builds its outgoing frame the same way (`sub` for the fixed
//  local area, then `add $0xffffffXX,%esp` for the arguments) and hands
//  `this` through %eax.
// ----------------------------------------------------------------------- //

struct C10091df0;                                       // opaque
extern C10091df0 g_10091df0;                            // 0x10091df0


// ---- group 1: f(a) -> g(a, &g_10091df0) ---- ret $4 ------------------- //

class C1001d2a0
{
public:
	int32	sub_1001d2a0(void *pA, C10091df0 *pAlloc);
	int32	sub_1001d340(void *pA);
	uint32	m_00;
};

// 0x1001d340 -- 36 bytes, 1 caller.
int32 C1001d2a0::sub_1001d340(void *pA)
{
	return sub_1001d2a0(pA, &g_10091df0);
}


class C1007eea0
{
public:
	int32	sub_1007eea0(void *pA, C10091df0 *pAlloc);
	int32	sub_1007ef40(void *pA);
	uint32	m_00;
};

// 0x1007ef40 -- 36 bytes, 1 caller.  Byte-for-byte 0x1001d340.
int32 C1007eea0::sub_1007ef40(void *pA)
{
	return sub_1007eea0(pA, &g_10091df0);
}


class C1007c890
{
public:
	int32	sub_1007c890(void *pA, C10091df0 *pAlloc);
	int32	sub_1007c9d0(void *pA);
	uint32	m_00;
};

// 0x1007c9d0 -- 36 bytes, 1 caller.  Byte-for-byte 0x1001d340.
int32 C1007c890::sub_1007c9d0(void *pA)
{
	return sub_1007c890(pA, &g_10091df0);
}


// ---- group 2: f(a,b) -> g(a, b, &g_10091df0) ---- ret $8 -------------- //

class C10009d60
{
public:
	int32	sub_10009d60(void *pA, void *pB, C10091df0 *pAlloc);
	int32	sub_10009ec0(void *pA, void *pB);
	uint32	m_00;
};

// 0x10009ec0 -- 45 bytes, 0 direct callers (reached through a table).
int32 C10009d60::sub_10009ec0(void *pA, void *pB)
{
	return sub_10009d60(pA, pB, &g_10091df0);
}


class C1001dee0
{
public:
	int32	sub_1001dee0(void *pA, void *pB, C10091df0 *pAlloc);
	int32	sub_1001e010(void *pA, void *pB);
	uint32	m_00;
};

// 0x1001e010 -- 45 bytes, 1 caller.  Byte-for-byte 0x10009ec0.
int32 C1001dee0::sub_1001e010(void *pA, void *pB)
{
	return sub_1001dee0(pA, pB, &g_10091df0);
}


// ---- group 3: f(a) -> g(a, LTNULL, &g_10091df0) ---- ret $4 ----------- //

class C1001c2c0
{
public:
	int32	sub_1001c2c0(void *pA, void *pB, C10091df0 *pAlloc);
	int32	sub_1001c7b0(void *pA);
	uint32	m_00;
};

// 0x1001c7b0 -- 44 bytes, 1 caller.
int32 C1001c2c0::sub_1001c7b0(void *pA)
{
	return sub_1001c2c0(pA, LTNULL, &g_10091df0);
}


class C10009f10
{
public:
	int32	sub_10009f10(void *pA, void *pB, C10091df0 *pAlloc);
	int32	sub_1005a390(void *pA);
	uint32	m_00;
};

// 0x1005a390 -- 44 bytes, 1 caller.  Byte-for-byte 0x1001c7b0.
int32 C10009f10::sub_1005a390(void *pA)
{
	return sub_10009f10(pA, LTNULL, &g_10091df0);
}


// ----------------------------------------------------------------------- //
//  0x10014550 -- 43 bytes, 2 callers.  The same idea one level out: a plain
//  __cdecl free function that appends a fourth argument of 0.
//
//  WHAT PINS IT: `sub $0x14` is sixteen bytes of outgoing arguments plus
//  icl's dead four; the three incoming arguments are re-laid at (%esp),
//  4(%esp), 8(%esp) IN ORDER and 0xc(%esp) is written with an immediate 0.
//  Both callers push (&pArray, &nCount, pContext) and test the result.
// ----------------------------------------------------------------------- //
int32 sub_1000f2c0(void *pA, void *pB, void *pC, void *pD);

// (body moved to d3dr2c_os.cpp: at /O2 icl pads the 43 bytes out to 48
// with `nop; lea 0(%esi,%eiz),%esi`, where the shipped image has 0x90 fill
// -- i.e. the shipped COMDAT carried at most /Os's 4-byte pad and the linker
// filled to 16.  /Os gives the same 43 bytes plus one 0x90.)


// ----------------------------------------------------------------------- //
//  THE POOL / FREE-LIST FAMILY.
//
//  0x1000a6a0 pops the pool's free list (`mov 0x18(%ecx),%eax; test; mov
//  (%eax),%edx; mov %edx,0x18(%ecx)`), so it is a one-argument __cdecl
//  allocator taking the pool by address.  Both banks below hold their pool
//  at +0x04, which is why both say `add $0x4,%ecx`.
// ----------------------------------------------------------------------- //

void *sub_1000a6a0(void *pPool);


// -- bank A, the polymorphic node -------------------------------------- //

// The node's first dword is its vtable pointer, and the free list threads
// THROUGH that same dword -- 0x10037550 writes the vtable there and then
// overwrites it with the list link.  It is spelled as a union because that
// is what the memory does; the vtable pointer is a plain data member so
// that the class stays non-polymorphic and icl does not add one of its own.
struct C1003abe4;

extern void *g_vtbl_100894d0[];                         // 0x100894d0

struct C1003abe4
{
	union
	{
		void       *m_pVTable;                          // +0x00
		C1003abe4  *m_pNextFree;                        // +0x00
	};
	uint8	m_pad_04[0x38];

	void	sub_1003abe4();                             // the initialiser
};

class C10037520Bank
{
public:
	C1003abe4  *sub_10037520();
	void        sub_10037550(C1003abe4 *pNode);

	uint32      m_00;                                   // +0x00
	uint8       m_Pool[0x18];                           // +0x04
	C1003abe4  *m_pFreeHead;                            // +0x1c
};

// 0x10037520 -- 48 bytes, 1 caller.  Allocate: pop the pool, and if that
// produced anything run the node's initialiser on it.
//
// WHAT PINS IT: the null test between the two calls is the source's, not
// `new`'s -- the allocator is 0x1000a6a0, an ordinary function taking the
// pool by address, and its result goes to %ebx which is both the argument
// of the second call and the return value.
C1003abe4 *C10037520Bank::sub_10037520()
{
	C1003abe4 *pNode;

	pNode = (C1003abe4 *)sub_1000a6a0(&m_Pool);
	if (pNode)
		pNode->sub_1003abe4();

	return pNode;
}

// 0x10037550 -- 32 bytes counting the padding, 1 caller.  Free: reset the
// node's vtable pointer, then push it on the bank's free list.
//
// WHAT PINS IT: 0x100894d0 is a vtable -- its slot 0 is 0x100374a0, which
// is the scalar-deleting destructor (`test $0x1,%al; call 0x10080479`).  So
// the first store is the vtable reset an inlined destructor emits, and the
// second store, to the same dword, is the free-list link.  icl kept both.
void C10037520Bank::sub_10037550(C1003abe4 *pNode)
{
	pNode->m_pVTable = g_vtbl_100894d0;
	pNode->m_pNextFree = m_pFreeHead;
	m_pFreeHead = pNode;
}


// -- bank B, the same source, the other translation unit ---------------- //

struct C10050d00;

class C10050d30Bank
{
public:
	C10050d00  *sub_10050d30();

	uint32      m_00;
	uint8       m_Pool[0x18];
	C10050d00  *m_pFreeHead;
};

struct C10050d00
{
	void	sub_10050d00();
	uint8	m_pad[0x28];
};

// 0x10050d30 -- 41 bytes, 1 caller.  The same source as 0x10037520; the two
// differ only in how icl spilled %ebx and whether it pushed or stored the
// outgoing argument, which is the /O2-vs-/Os split this module has.  Both
// are written so whichever flag the file lands on takes one of them.


// ----------------------------------------------------------------------- //
//  0x10035bb4 -- 24 bytes, 1 caller.  The initialiser of a 0x10-byte
//  polymorphic object: the caller is `operator new(0x10); if (p) <this>;
//  store p`, so in the original this was the constructor.  A constructor
//  carries no `sub_<addr>` in its mangled name, so it is written here as a
//  member that assigns the vtable pointer itself and returns `this` -- the
//  same bytes, a name that can be placed.
//
//  WHAT PINS IT: 0x10089480 is the class's vtable (its slot 0, 0x10035a70,
//  is the destructor and re-stores 0x10089480 into (%ebx)); the three zero
//  stores walk +0x04, +0x08, +0x0c in that order out of a single
//  `xor %eax,%eax`; and `mov %ecx,%eax` before the `ret` is the
//  constructor's this-return.
// ----------------------------------------------------------------------- //

extern void *g_vtbl_10089480[];                         // 0x10089480

class C10035bb4
{
public:
	C10035bb4  *sub_10035bb4();

	void       *m_pVTable;                              // +0x00
	uint32      m_04;                                   // +0x04
	uint32      m_08;                                   // +0x08
	uint32      m_0c;                                   // +0x0c
};

C10035bb4 *C10035bb4::sub_10035bb4()
{
	m_pVTable = g_vtbl_10089480;
	m_04 = 0;
	m_08 = 0;
	m_0c = 0;
	return this;
}


// ----------------------------------------------------------------------- //
//  0x100647c0 -- 8 bytes plus padding, 1 caller.  An out-of-line float
//  accessor.
//
//  WHAT PINS IT: `push %esi; add $0x4,%esp` is icl's dead four-byte slot on
//  a leaf, and the whole body is `flds 0x18(%ecx)`.  The caller walks an
//  array of stride 0x28 calling this, then 0x100647d0 (the same thing at
//  +0x1c), then 0x100647e0 (which stores both back), so +0x18 and +0x1c are
//  a float pair inside a 0x28-byte element.  It is defined OUT OF LINE
//  because /Ob1 would fold an in-class definition into the caller and there
//  would then be no function here at all.
// ----------------------------------------------------------------------- //

class C100647c0
{
public:
	float	sub_100647c0();

	uint8	m_pad_00[0x18];
	float	m_18;
	float	m_1c;
	uint8	m_pad_20[8];
};

float C100647c0::sub_100647c0()
{
	return m_18;
}


// ----------------------------------------------------------------------- //
//  0x1007a180 -- 71 bytes, 2 callers.  A table dispatch.
//
//  WHAT PINS IT: `ret $0xc` with `this` in %ecx is three stack arguments;
//  the handler is fetched with `mov 0x100899b0(,%eax,4),%eax` where %eax
//  came from 0x4(%edx), i.e. a type word at +0x04 of argument one; and the
//  third thing pushed is `lea 0x20(%esp)` -- the ADDRESS OF THE SECOND
//  PARAMETER, which is why the parameter has to be named and passed by
//  address rather than forwarded.  The two exits are separate epilogues
//  with the failure arm placed last, which is the one-positive-condition
//  shape, not a guarded early return.
//
//  Table 0x100899b0: [0] is null, [1] = 0x100799b0, [2] = 0x10079aa0.
// ----------------------------------------------------------------------- //

struct C1007a180Req
{
	uint32	m_00;
	uint32	m_nType;                                    // +0x04
};

class C1007a180Disp;

typedef void (*PFN1007a180)(C1007a180Disp *pThis, C1007a180Req *pReq,
                            uint32 *pParam, void *pData);

extern PFN1007a180 g_100899b0[];                        // 0x100899b0

class C1007a180Disp
{
public:
	int32	sub_1007a180(C1007a180Req *pReq, uint32 nParam, void *pData);
	uint32	m_00;
};



// ----------------------------------------------------------------------- //
//  0x1000da50 -- 79 bytes, 2 callers.  Recomputes four globals from the
//  object at 0x1008beb8 and the two-float table at 0x1008e664.
//
//  WHAT PINS IT: `flds 0xc(%eax); fdivrs table(,%edx,8)` is table-over-
//  member, not member-over-table -- icl loads the divisor and uses the
//  reversed divide so the dividend can stay a memory operand.  Both
//  multiplies go through a pooled 1.0f (0x100a458c and 0x100a4594 both hold
//  0x3f800000), so the source really does multiply by one and icl really
//  does not fold it.  The last two members move with `mov`, so they are
//  copied as bit patterns and are written as uint32 here.  One `ret`, so
//  the whole body is inside one positive test.
// ----------------------------------------------------------------------- //

struct C1000da50Data
{
	uint32	m_00;
	uint32	m_04;
	uint32	m_08;
	float	m_0c;
	float	m_10;
};

extern C1000da50Data   *g_1008beb8;                     // 0x1008beb8
extern uint32           g_1008d7cc;                     // 0x1008d7cc
extern float            g_1008e664[][2];                // 0x1008e664
extern float            g_1008ad94;                     // 0x1008ad94
extern float            g_1008ad98;                     // 0x1008ad98
extern uint32           g_1008ad9c;                     // 0x1008ad9c
extern uint32           g_1008ada0;                     // 0x1008ada0

void sub_1000da50()
{
	C1000da50Data *pData = g_1008beb8;

	if (pData)
	{
		g_1008ad94 = g_1008e664[g_1008d7cc][0] / pData->m_0c * 1.0f;
		g_1008ad98 = g_1008e664[g_1008d7cc][1] / pData->m_10 * 1.0f;
		g_1008ad9c = pData->m_04;
		g_1008ada0 = pData->m_08;
	}
}


// ----------------------------------------------------------------------- //
//  0x1005db10 and 0x100646d0 -- two subsystem terminators, called eleven
//  instructions apart from the same shutdown routine (0x1004fd34 and
//  0x1004fd6d).
//
//  WHAT PINS THEM: 0x1007b8d0 is `mov $0x10091a10,%eax; ret`, a getter for
//  one global object, and each terminator adds a fixed member offset to its
//  result before using it as `this` -- +0x284 for one and +0x1a4 for the
//  other.  0x1007b8e0 walks `this->m_pArray[0 .. this->m_nCount)`, so the
//  subobject is an eight-byte array-and-count.  The guard is a GUARDED
//  EARLY RETURN, not a wrapping if: the flag test jumps FORWARD over the
//  epilogue into the body, so the `return` arm is the one that falls
//  through and it has its own epilogue.
// ----------------------------------------------------------------------- //

class C1007b8e0
{
public:
	void	sub_1007b8e0(void *pParam, void (*pfn)());

	void  **m_pArray;                                   // +0x00
	uint32	m_nCount;                                   // +0x04
};

struct C1007b8d0Thing
{
	uint8       m_pad_000[0x1a4];
	C1007b8e0   m_1a4;                                  // +0x1a4
	uint8       m_pad_1ac[0x284 - 0x1a4 - 8];
	C1007b8e0   m_284;                                  // +0x284
};

C1007b8d0Thing *sub_1007b8d0();

struct C1008b880;
extern C1008b880    g_1008b880;                         // 0x1008b880
extern uint32       g_100893d8;                         // 0x1005db10's flag
extern uint32       g_100893d4;                         // 0x100646d0's flag

void sub_1005dae0(void* pCtx, void* pItem);             // 0x1005dae0  (w27_1005dae0.cpp)
void sub_100646a0(void* pCtx, void* pItem);             // 0x100646a0  (w27_1005dae0.cpp)

// 0x1005db10 -- 58 bytes, 1 caller.
void sub_1005db10()
{
	if (!g_100893d8)
		return;

	sub_1007b8d0()->m_284.sub_1007b8e0(&g_1008b880, (void (*)())sub_1005dae0);
}


// 0x100646d0 -- 70 bytes, 1 caller.  The same shape plus a trailing
// four-instruction idiom that also closes 0x10064720 in the same
// translation unit:
//
//      mov 0x1008beb0,%eax ; mov 0xc(%eax),%eax ; mov (%eax),%edx
//      mov %edx,(%eax)
//
// -- a dword loaded out of a location and stored straight back to it.  It
// is written below as a copy between two members of a union, i.e. two
// different designators for the same dword, which is what keeps it from
// reading as a self-assignment.  This is the LEAST certain line in the
// file; if 0x100646d0 misses and 0x1005db10 matches, this idiom is the
// reason and the fix is here, not in the guard above it.
struct C1008beb0Node
{
	union
	{
		C1008beb0Node  *m_pA;                           // +0x00
		C1008beb0Node  *m_pB;                           // +0x00
	};
};

struct C1008beb0Thing
{
	uint8           m_pad_00[0x0c];
	C1008beb0Node  *m_p0c;                              // +0x0c
};

extern C1008beb0Thing *g_1008beb0;                      // 0x1008beb0

void sub_100646d0()
{
	if (!g_100893d4)
		return;

	sub_1007b8d0()->m_1a4.sub_1007b8e0(&g_1008b880, (void (*)())sub_100646a0);

	g_1008beb0->m_p0c->m_pA = g_1008beb0->m_p0c->m_pB;
}


// *********************************************************************** //
//
//  THE SIX ROWS THAT ARE NOT WRITTEN
//
//  FIVE ARE REGISTER-ABI BODIES.  The filter is heuristic and these got
//  through it: each takes its arguments in %eax / %edx with no stack shim
//  in front, which no C source produces in isolation -- icl only picks that
//  convention for a function whose callers it can see, and our file
//  boundaries are not the shipped ones.
//
//    0x1003b6f4   64 bytes, 2 callers.  Argument in %eax, result in %eax.
//                 Fully decoded: a five-arm identity map --
//                 5->5, 6->6, 3->3, 4->4, 1->1, everything else 2 -- tested
//                 in exactly that order, with `xor %eax,%eax; inc %eax` for
//                 the 1 arm and %eax preloaded with 2 before the first
//                 test.  The callers (0x1003d3ed, 0x1003d41c) feed it %edi
//                 and hand the answer to SetRenderState slot +0x50 as
//                 states 0x13 and 0x14, so it is the src/dest blend-mode
//                 translation.  Ready to write the moment it can be paired
//                 with its callers in one file.
//
//    0x1000f270   68 bytes, 1 caller.  Argument in %eax.  Bounds-checks
//                 g_1008d520 + n against g_1008d51c, grows through
//                 0x10023fb0 with `g_1008d51c + 0x5dc + n`, and returns
//                 `g_1008d524 + (g_1008d520 << 5)` -- a 32-byte-element
//                 array reserve.  Returns 0 on failure.
//
//    0x1006a5f8   56 bytes, 1 caller.  Arguments in %eax and %edx.  Copies
//                 two 12-byte vectors out of (%edx) and 0x4(%edx) into
//                 locals and calls 0x1006a1b0 with (%eax, %edx, v1, v2).
//
//    0x10021078   84 bytes, 1 caller.  Arguments in %eax and %edx.  Zeroes
//                 a 0x20-byte DDSURFACEDESC-shaped local, sets its dwSize
//                 to 0x20, calls the COM slot at +0x54 through (%eax), and
//                 on success passes the local plus %edx to 0x10021018.
//                 Returns 0 / 1.
//
//    0x100230d4   92 bytes, 1 caller.  This one is a proper thiscall body,
//                 but the function it calls is not callable from source:
//                 0x10022e9c takes arguments in %ecx AND %edx AND at
//                 0xc(%esp) and 0x10(%esp), a private convention icl chose
//                 for a callee it could see.  The body itself is plain --
//                 `if (!m_104) return 0; if (sub_10022e9c(this+0x108,
//                 this+0x104, this, this+0x100)) return 1; m_104 = 0;
//                 return 1;` -- and becomes writable as soon as 0x10022e9c
//                 is in the same file.
//
//  ONE IS AN UNRELIABLE DECODE:
//
//    0x10050d00   36 bytes, 1 caller (0x10050d30, above).  A node
//                 initialiser: three self-pointing pairs at +0x00, +0x10
//                 and +0x1c, plus `this` stored at +0x18 and +0x24 -- two
//                 twelve-byte {prev, next, owner} links at +0x10 and +0x1c
//                 (which is exactly what 0x10050d60 later destroys, calling
//                 0x1000a6e0 on node+0x10 and node+0x1c) behind a
//                 self-linked pair at +0x00.  The problem is the stores:
//                 icl grouped them BY SOURCE REGISTER -- all four %ecx
//                 stores, then both %eax stores, then both %edx stores --
//                 so the order in the image is the scheduler's and not the
//                 source's, and there is nothing left to pin the source
//                 order with.  It is declared above so 0x10050d30 can call
//                 it, and left undefined.
//
// *********************************************************************** //
