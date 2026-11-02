// *********************************************************************** //
//
//  MODULE:   D3DQIP4 (.CPP)  --  d3d.ren 0x1007a470 / 0x1007a580 / 0x1007a680
//
//  PURPOSE:  The attachment-draw family: an object's own draw callback,
//            then a walk over its attachment list drawing every child
//            once per frame, recursing to a depth of 32.  Moved out of
//            d3dr5b.cpp (0x1007a680), where it could not match: its call
//            to 0x1007a470 hands the object over in %eax, which only a
//            /Qip TU that also holds the callee produces.
//
//  THIS FILE MUST BE COMPILED /O2 /Qip /Qip_no_inlining (D3D_QIP_NI_TUS in
//  build/engine.sh).  Measured, all three functions:
//
//      /O2                         0x1007a680 near, callee not in the TU
//      /O2 /Qip                    0x1007a470 INLINED into 0x1007a680
//                                  (670 bytes), 0x1007a580 304b near
//      /Os /Qip                    same inlining, 560 bytes
//      /O2 /Qip /Qip_no_inlining   all three byte-identical
//
//  Why the extra flag is honest: in the shipped image 0x1007a470 has TWO
//  callers, 0x1007a680 here and the 1644-byte 0x1007aeb4 (call at
//  0x1007b4fd), so icl had no reason to fold it into either.  With only
//  one caller in our TU, /Qip folds it; /Qip_no_inlining stops /Qip from
//  inlining functions that are not marked inline while leaving the
//  `inline` predicate below expanded, which is exactly what the shipped
//  bytes show (the predicate is expanded at all three sites, 0x1007a470
//  is not).  If 0x1007aeb4 is ever written it belongs in this file, and
//  the flag can then be re-measured without the no-inlining switch.
//
//  REGISTER ABI.  0x1007a470 and 0x1007a580 have no stack shim (five 0x90
//  and a lea-nop respectively in front of their first `push %edi`), so
//  they are internal linkage under /Qip: object in %eax, depth in %edx.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// The attachment list node: only the link at +0x24 is walked here.
struct Node1007a470
{
	uint8               m_pad_00[0x24];                 // +0x00
	Node1007a470*       m_pNext;                        // +0x24
};

// The drawn object.  +0x58 is the frame stamp compared against the
// current frame at 0x1008bea4, so a child reached twice is drawn once.
struct Obj1007a680
{
	uint8         m_pad_00[0x58];                       // +0x00
	uint32        m_nFrame58;                           // +0x58
	uint8         m_pad_5c[0x88 - 0x5c];                // +0x5c
	uint32        m_nFlags88;                           // +0x88
	uint32        m_nFlags8c;                           // +0x8c
	uint8         m_pad_90[0x98 - 0x90];                // +0x90
	Node1007a470* m_pNodes98;                           // +0x98
	uint8         m_pad_9c[0xbc - 0x9c];                // +0x9c
	char          m_cTypeBC;                            // +0xbc
	uint8         m_pad_bd[0x188 - 0xbd];               // +0xbd
	uint32        m_n188;                               // +0x188 (0x1007aeb4)
};

// 0x100895c0: twenty-byte records indexed by the object's type byte
// (`lea (%eax,%eax,4)` doubled, `*(base,%edx,2)`); +0x00 is the type's
// draw function, called cdecl with the object as its one argument.
struct Rec1007a680
{
	void   (*m_p00)(Obj1007a680* pObj);                 // +0x00
	uint8  m_pad_04[0x10];                              // +0x04
};

// 0x1008cbcc: the same pointer d3dr5b.cpp's 0x10059590 reads +0x4c of.
// Slot +0x00 is `mov 0x1008cbcc,%eax ; call *(%eax)` with (object, node)
// planted at (%esp)/0x4(%esp): a plain function pointer that resolves an
// attachment node to the object it attaches.
struct Counters10059590
{
	Obj1007a680* (*m_pfnGetChild)(Obj1007a680* pObj, Node1007a470* pNode); // +0x00
	uint8  m_pad_04[0x48];                              // +0x04
	uint32 m_n4c;                                       // +0x4c
};

extern Rec1007a680       g_100895c0[];                  // 0x100895c0
extern uint32            g_1008bd54;                    // 0x1008bd54
extern uint32            g_1008bea4;                    // 0x1008bea4
extern Counters10059590* g_1008cbcc;                    // 0x1008cbcc


// ---------------------------------------------------------------------- //
//  The draw predicate, expanded at all three sites.  Inside the two walks
//  it is materialised as a BOOL in %al (`mov $0x1,%al` / `xor %al,%al`,
//  then `test %al,%al`), so it is a function returning a value, not a
//  macro; one expression, because `m_88 & 1` is computed once and tested
//  twice and 0x1008bd54 is loaded twice, one per mention.
// ---------------------------------------------------------------------- //
inline bool CanDraw1007a680(Obj1007a680* pObj)
{
	return g_100895c0[pObj->m_cTypeBC].m_p00 &&
	    pObj->m_cTypeBC != 7 &&
	    !(pObj->m_nFlags8c & 0x40) &&
	    (((pObj->m_nFlags88 & 1) &&
	      (!g_1008bd54 || !(pObj->m_nFlags8c & 1))) ||
	     (!(pObj->m_nFlags88 & 1) && g_1008bd54 &&
	      (pObj->m_nFlags88 & 0x400)));
}

static void sub_1007a580(Obj1007a680* pObj, uint32 nDepth);


// ======================================================================= //
//  0x1007a470 -- 262 bytes.  Draw the object, then each attached child
//  not yet drawn this frame (its own attachments first, depth 1).
//
//  The list head is tested TWICE (`test %eax,%eax ; je ; mov %eax,%ebx ;
//  je`): an `if` around the `for`, whose own entry test icl keeps.
// ======================================================================= //
static void sub_1007a470(Obj1007a680* pObj)
{
	Node1007a470* pNode;
	Obj1007a680*  pChild;

	g_100895c0[pObj->m_cTypeBC].m_p00(pObj);

	if (pObj->m_pNodes98)
	for (pNode = pObj->m_pNodes98; pNode; pNode = pNode->m_pNext)
	{
		pChild = g_1008cbcc->m_pfnGetChild(pObj, pNode);
		if (pChild && pChild->m_nFrame58 != g_1008bea4)
		{
			sub_1007a580(pChild, 1);
			if (CanDraw1007a680(pChild))
			{
				g_100895c0[pChild->m_cTypeBC].m_p00(pChild);
				pChild->m_nFrame58 = g_1008bea4;
			}
		}
	}
}


// ======================================================================= //
//  0x1007a580 -- 246 bytes.  The recursive half: children of children,
//  bounded at `cmp $0x20 ; ja <out>`.
// ======================================================================= //
static void sub_1007a580(Obj1007a680* pObj, uint32 nDepth)
{
	Node1007a470* pNode;
	Obj1007a680*  pChild;

	if (nDepth > 0x20)
		return;

	for (pNode = pObj->m_pNodes98; pNode; pNode = pNode->m_pNext)
	{
		pChild = g_1008cbcc->m_pfnGetChild(pObj, pNode);
		if (pChild && pChild->m_nFrame58 != g_1008bea4)
		{
			sub_1007a580(pChild, nDepth + 1);
			if (CanDraw1007a680(pChild))
			{
				g_100895c0[pChild->m_cTypeBC].m_p00(pChild);
				pChild->m_nFrame58 = g_1008bea4;
			}
		}
	}
}


// ======================================================================= //
//  0x1007a680 -- 119 bytes.  The guard in front of 0x1007a470.
//
//      53 / 8b 44 24 08 / 83 ec 10        the object into %eax up front
//      ... the predicate, branching directly (not through %al) ...
//      83 c4 10 / 5b / c3                 <- epilogue copy 1 (return)
//      e8 <0x1007a470> / 83 c4 10 / 5b / c3   <- copy 2, after the call
//
//  WHAT PINS IT:
//    * the call pushes NOTHING: the object is already in %eax, the
//      register entry's first argument.
//    * the RETURN falls through and the call sits out of line with its
//      own epilogue: `if (!(...)) return;` then the call.  Written as
//      `if (...) call;` the call falls through (14 bytes differ), and
//      through the inline predicate the BOOL is materialised in %dl.
// ======================================================================= //
void sub_1007a680(Obj1007a680* pObj)
{
	if (!(g_100895c0[pObj->m_cTypeBC].m_p00 &&
	    pObj->m_cTypeBC != 7 &&
	    !(pObj->m_nFlags8c & 0x40) &&
	    (((pObj->m_nFlags88 & 1) &&
	      (!g_1008bd54 || !(pObj->m_nFlags8c & 1))) ||
	     (!(pObj->m_nFlags88 & 1) && g_1008bd54 &&
	      (pObj->m_nFlags88 & 0x400)))))
		return;

	sub_1007a470(pObj);
}


// *********************************************************************** //
//
//  THE REST OF THE SHIPPED TU: 0x1007a7b0, 0x1007aeb0/0x1007aeb4,
//  0x1007b580 and 0x1007b880 -- the world draw that feeds the family above.
//  They belong in this file for the register ABI:
//    * 0x1007a7b0 (a leaf visitor, reached only as a function pointer)
//      calls 0x1007a580 with the object in %eax and the depth in %edx
//      (0x1007ae09);
//    * 0x1007aeb4 is the register entry of the external pair at
//      0x1007aeb0 (`mov 0x4(%esp),%eax`) and calls 0x1007a470 with the
//      object in %eax (0x1007b4fd) -- 0x1007a470's second caller, the one
//      the banner above predicted;
//    * 0x1007b580 has no stack entry (a lea-nop in front of its `push
//      %ebp`): static, its one caller 0x1007b880 hands the flag over in
//      %eax (0x1007b89b / 0x1007b8a9), and it calls 0x1007aeb4 with the
//      world in %eax straight from GetVisBSP's return.
//  /Qip_no_inlining still holds, re-measured with the whole TU present:
//  under plain /O2 /Qip icl expands 0x1007a470 into all three callers and
//  folds 0x1007b580 into 0x1007b880, and the shipped image keeps both out
//  of line.  0x1007a7b0's copy of the 0x1007a470 body is therefore
//  written out in the source (an `inline` 0x1007a470 would expand at
//  every site).
//
//  RESULTS: 0x1007aeb0 (the stack entry) and 0x1007b880 IDENTICAL;
//  0x1007aeb4 near (1644 vs 1660), 0x1007b580 near (768 vs 759),
//  0x1007a7b0 near (1796 vs 1792) -- 0x1007aeb4 and 0x1007a7b0 grew to
//  that since QueuePoly's arms were nested the shipped way round (see
//  QueuePoly1007aeb4).  The residue in all three is block
//  placement -- icl here sinks the arm the shipped code keeps inline and
//  vice versa (the translucent-queue arm, the object-draw arm, the walk
//  arm of 0x1007b580), plus one extra frame slot in 0x1007b580 (0x48 vs
//  0x68 of locals).  The first-queue arms are kept apart, as shipped.
//
//  THE QUEUES.  The renderer root at 0x10091a10 (0x1007b8d0 answers its
//  address): +0x00 an LTLink list whose records' +0x04 is reset per
//  frame, +0x0c its flags, +0x10 and +0x28 two polygon queues -- a
//  CMoArray<Poly*> (vptr, m_pArray, m_nElements, the DefaultCache pair)
//  and a use count -- +0x40 the 32-entry lightmap-animation table and its
//  count at +0x140 (w8_10066d1c.cpp's LMTable).  A queue reuses its
//  array's slots up to m_nElements and grows it through dynarray.h's
//  Insert2 (inlined, `&g_10091df0` the allocator) past that.
// *********************************************************************** //

struct Plane1007aeb4
{
	float			m_fX, m_fY, m_fZ;                   // +0x00
	float			m_fDist;                            // +0x0c
};

// w8_10066d1c.cpp's surface and polygon, the fields read here.
struct Surf1007aeb4
{
	uint8			m_pad_00[0x30];
	void*			m_pTexture30;                       // +0x30
	Plane1007aeb4*	m_pPlane34;                         // +0x34
	uint32			m_nFlags38;                         // +0x38
	uint8			m_pad_3c[0x42 - 0x3c];
	uint16			m_wLMAnim42;                        // +0x42
};

struct Poly1007aeb4
{
	uint8			m_pad_00[0x18];
	float			m_fX18, m_fY18, m_fZ18;             // +0x18 centre
	float			m_fRadius24;                        // +0x24
	Surf1007aeb4*	m_pSurf28;                          // +0x28
	uint8			m_pad_2c[0x42 - 0x2c];
	uint16			m_wFrameCode42;                     // +0x42
};

class LAlloc1007aeb4
{
public:
	virtual void	Slot00();
	virtual void*	Alloc(uint32 nBytes);               // +0x04
	virtual void	Free(void* p);                      // +0x08
};

extern LAlloc1007aeb4 g_10091df0;                       // 0x10091df0

// l_allocator.h's BaseNew / BaseDelete for a pointer element.
inline Poly1007aeb4** QBaseNew(LAlloc1007aeb4* pAlloc, uint32 nElements)
{
	Poly1007aeb4** ptr;

	ptr = (Poly1007aeb4**)pAlloc->Alloc(sizeof(Poly1007aeb4*) * nElements);
	if (!ptr)
		return LTNULL;

	return ptr;
}

inline void QBaseDelete(LAlloc1007aeb4* pAlloc, Poly1007aeb4** ptr)
{
	if (!ptr)
		return;

	pAlloc->Free(ptr);
}

struct CQipPolyQueue
{
	void*			m_pVtbl;                            // +0x00
	Poly1007aeb4**	m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	uint32			m_nCacheSize;                       // +0x0c
	uint32			m_nWantedCache;                     // +0x10
	uint32			m_nUsed;                            // +0x14

	inline void _DeleteAndDestroyArray(LAlloc1007aeb4* pAlloc)
	{
		if (m_pArray)
		{
			QBaseDelete(pAlloc, m_pArray);
			m_pArray = LTNULL;
		}

		m_nCacheSize = 0;
	}

	inline LTBOOL Insert2(uint32 index, Poly1007aeb4* const& toInsert,
	               LAlloc1007aeb4* pAlloc)
	{
		Poly1007aeb4**	pNewArray;
		uint32			newSize, i;

		newSize = m_nElements + 1;

		if (m_nCacheSize == 0)
		{
			pNewArray = QBaseNew(pAlloc, newSize + m_nWantedCache);
			if (!pNewArray)
				return LTFALSE;

			for (i = 0; i < index; i++)
				pNewArray[i] = m_pArray[i];

			for (i = index; i < m_nElements; i++)
				pNewArray[i + 1] = m_pArray[i];

			pNewArray[index] = toInsert;

			if (m_pArray)
				_DeleteAndDestroyArray(pAlloc);

			m_nCacheSize = m_nWantedCache;
			m_pArray = pNewArray;
		}
		else
		{
			for (i = m_nElements; i > index; i--)
				m_pArray[i] = m_pArray[i - 1];

			m_nCacheSize--;

			m_pArray[index] = toInsert;
		}

		++m_nElements;

		return LTTRUE;
	}

	// The grow arm sunk (switch-default), as shipped: compare, store,
	// count, jmp back -- Insert2 below the function's epilogue.
	inline void Add(Poly1007aeb4* pPoly)
	{
		switch (m_nUsed >= m_nElements)
		{
		case 0:
			m_pArray[m_nUsed] = pPoly;
			m_nUsed++;
			break;

		default:
			if (Insert2(m_nElements, pPoly, &g_10091df0))
				m_nUsed++;
		}
	}
};

struct LMEntry1007aeb4
{
	Poly1007aeb4*	m_pPoly;                            // +0x00
	void*			m_pMat;                             // +0x04
};

struct Link1007aeb4
{
	Link1007aeb4*	m_pPrev;                            // +0x00
	Link1007aeb4*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08
};

struct Root1007aeb4
{
	Link1007aeb4		m_Head;                         // +0x00
	uint32				m_nFlags0c;                     // +0x0c
	CQipPolyQueue	m_Opaque;                       // +0x10
	CQipPolyQueue	m_Translucent;                  // +0x28
	LMEntry1007aeb4		m_Entries[0x20];                // +0x40
	uint32				m_nEntries;                     // +0x140
};

extern Root1007aeb4		g_10091a10;                     // 0x10091a10
extern uint16			g_1008bea8;                     // 0x1008bea8 frame code
extern int32			g_CV_DrawFlat;                  // 0x1008caa4
extern uint32			g_1008d958;                     // 0x1008d958
extern uint32			g_1008ba9c;                     // 0x1008ba9c world xform


// ---------------------------------------------------------------------- //
//  Queue a world polygon once per frame: invisible ones are only stamped;
//  translucent (0x10) ones go on the second queue unless DrawFlat;
//  lightmap-animated ones into the table (once, up to 32) when 0x1008d958
//  is set; the rest on the first queue.  Expanded in 0x1007a7b0 and
//  0x1007aeb4 alike, with its two first-queue arms kept apart.
// ---------------------------------------------------------------------- //
inline void QueuePoly1007aeb4(Poly1007aeb4* pPoly)
{
	Surf1007aeb4*	pSurf;
	uint32			nEntries, j;

	if (pPoly->m_wFrameCode42 == g_1008bea8)
		return;

	pSurf = pPoly->m_pSurf28;
	if (!(pSurf->m_nFlags38 & 4))
	{
		// The tests nested with the lightmap-table arm innermost and the
		// three Adds as the else arms: icl lays the table search inline and
		// the Adds after it in reverse (the two opaque ones kept apart, the
		// translucent one falling into the frame-code store), as shipped.
		// As an else-if chain in the other order it merged the two opaque
		// Adds and laid the translucent one inline (1680 -> 1792 bytes of
		// 0x1007a7b0's shipped 1796; 0x1007aeb4 1532 -> 1660 of 1644).
		if (!(pSurf->m_nFlags38 & 0x10) || g_CV_DrawFlat)
		{
			if (pSurf->m_wLMAnim42 != 0x7fff)
			{
				if (g_1008d958)
				{
					nEntries = g_10091a10.m_nEntries;
					if (nEntries < 0x20)
					{
						for (j = 0; j < nEntries; j++)
						{
							if (g_10091a10.m_Entries[j].m_pPoly == pPoly)
								break;
						}

						if (j >= nEntries)
						{
							g_10091a10.m_Entries[nEntries].m_pPoly = pPoly;
							g_10091a10.m_Entries[nEntries].m_pMat = &g_1008ba9c;
							g_10091a10.m_nEntries++;
						}
					}
				}
				else
					g_10091a10.m_Opaque.Add(pPoly);
			}
			else
				g_10091a10.m_Opaque.Add(pPoly);
		}
		else
			g_10091a10.m_Translucent.Add(pPoly);
	}

	pPoly->m_wFrameCode42 = g_1008bea8;
}


// ======================================================================= //
//  0x1007a7b0 -- 1788 bytes.  The visible-leaf callback of the world walk
//  (0x1007b580 installs it at +0x20 of the walk record): stamp the leaf,
//  count it, draw the leaf's type-7 objects that are visible (flag 1)
//  and not yet drawn this frame together with their attachments (the
//  0x1007a470 body, written out), then queue the leaf's polygons.
// ======================================================================= //
struct ObjRef1007a7b0
{
	uint8			m_pad_00[0x18];
	Obj1007a680*	m_pObj18;                           // +0x18
};

struct Leaf1007a7b0
{
	uint8			m_pad_00[0x14];
	Link1007aeb4	m_Objs14;                           // +0x14
	Poly1007aeb4**	m_pPolys20;                         // +0x20
	uint32			m_nPolys24;                         // +0x24
	uint8			m_pad_28[0x2c - 0x28];
	uint16			m_wFrameCode2c;                     // +0x2c
};

extern int32 g_nVisibleLeaves;                          // 0x1008be40

void sub_1007a7b0(Leaf1007a7b0* pLeaf)
{
	Link1007aeb4*	pLink;
	Obj1007a680*	pObj;
	Node1007a470*	pNode;
	Obj1007a680*	pChild;
	Poly1007aeb4**	ppPoly;
	uint32			nPolys;

	pLeaf->m_wFrameCode2c = g_1008bea8;
	g_nVisibleLeaves++;

	for (pLink = pLeaf->m_Objs14.m_pNext; pLink != &pLeaf->m_Objs14;
	     pLink = pLink->m_pNext)
	{
		pObj = ((ObjRef1007a7b0*)pLink->m_pData)->m_pObj18;

		// The switch-default sink: the shipped loop keeps the not-type-7
		// `continue` inline and the type-7 body out of line (`je` to it).
		switch (pObj->m_cTypeBC == 7)
		{
		case 0:
			break;

		default:
			if (!(pObj->m_nFlags88 & 1) || pObj->m_nFrame58 == g_1008bea4)
				continue;

			pObj->m_nFrame58 = g_1008bea4;
			g_100895c0[pObj->m_cTypeBC].m_p00(pObj);

			if (pObj->m_pNodes98)
			for (pNode = pObj->m_pNodes98; pNode; pNode = pNode->m_pNext)
			{
				pChild = g_1008cbcc->m_pfnGetChild(pObj, pNode);
				if (pChild && pChild->m_nFrame58 != g_1008bea4)
				{
					sub_1007a580(pChild, 1);
					if (CanDraw1007a680(pChild))
					{
						g_100895c0[pChild->m_cTypeBC].m_p00(pChild);
						pChild->m_nFrame58 = g_1008bea4;
					}
				}
			}
		}
	}

	ppPoly = pLeaf->m_pPolys20;
	for (nPolys = pLeaf->m_nPolys24; nPolys; nPolys--)
	{
		QueuePoly1007aeb4(*ppPoly);
		ppPoly++;
	}
}


// ======================================================================= //
//  0x1007aeb0 (stack entry) / 0x1007aeb4 (register body) -- 1648 bytes.
//  Draw a whole BSP without the visibility walk: for every leaf, its
//  object list from the tail, stopping at the first object with +0x188
//  set, drawing (and stamping) each drawable one through 0x1007a470;
//  then every polygon of the BSP queued.
// ======================================================================= //
struct Leaf1007aeb4
{
	uint8			m_pad_00[0x0c];
	Link1007aeb4	m_Objs0c;                           // +0x0c
};

struct BSP1007aeb4
{
	uint8			m_pad_00[0x70];
	Leaf1007aeb4*	m_pLeaves70;                        // +0x70
	uint32			m_nLeaves74;                        // +0x74
	uint8			m_pad_78[0xa0 - 0x78];
	Poly1007aeb4**	m_pPolysA0;                         // +0xa0
	uint32			m_nPolysA4;                         // +0xa4
};

void sub_1007aeb0(BSP1007aeb4* pBSP)
{
	uint32			i;
	Link1007aeb4*	pHead;
	Link1007aeb4*	pLink;
	Obj1007a680*	pObj;

	for (i = 0; i < pBSP->m_nLeaves74; i++)
	{
		pHead = &pBSP->m_pLeaves70[i].m_Objs0c;
		for (pLink = pHead->m_pPrev; pLink != pHead; pLink = pLink->m_pPrev)
		{
			pObj = (Obj1007a680*)pLink->m_pData;
			if (pObj->m_n188)
				break;

			if (!(g_100895c0[pObj->m_cTypeBC].m_p00 &&
			    pObj->m_cTypeBC != 7 &&
			    !(pObj->m_nFlags8c & 0x40) &&
			    (((pObj->m_nFlags88 & 1) &&
			      (!g_1008bd54 || !(pObj->m_nFlags8c & 1))) ||
			     (!(pObj->m_nFlags88 & 1) && g_1008bd54 &&
			      (pObj->m_nFlags88 & 0x400)))))
				continue;

			pObj->m_nFrame58 = g_1008bea4;
			sub_1007a470(pObj);
		}
	}

	for (i = 0; i < pBSP->m_nPolysA4; i++)
		QueuePoly1007aeb4(pBSP->m_pPolysA0[i]);
}


// ======================================================================= //
//  0x1007b580 -- 768 bytes (register ABI: the flag in %eax; static).
//  Draw the world for a frame.  Unless LockPVS, when the root's flag 1 is
//  set the queues are emptied and the records on its list reset, then
//  either (bAll) every leaf of GetVisBSP's BSP is drawn by 0x1007aeb4, or
//  the world's tree (+0x6c, virtual +0x14) walks the visible leaves from
//  the camera (the portal view's when 0x1008bd54, else the main one) with
//  the visitor record: objects 0x1007a680, leaves 0x1007a7b0, the node
//  test 0x1001e770, the object list 0x1007a700, 0x1007b520, and a
//  far distance of 10000.  Then the sky (0x1005f580) and 0x10040dd0, and
//  -- DrawWorld -- the first queue is drawn: each polygon facing the
//  camera (Dot > 0.01) and not outside the six frustum planes, its clip
//  flags cleared for each plane it is wholly inside, through the draw
//  entry its surface selects.
// ======================================================================= //
extern void   sub_1006c670();                           // 0x1006c670
extern int32  sub_1006c680();                           // 0x1006c680
struct Obj1001e770;
extern LTBOOL sub_1001e770(Obj1001e770* pObject);       // 0x1001e770
struct C1007b520;
extern int32  sub_1007b520(const C1007b520* pObj);      // 0x1007b520
struct Item1007a700;
class LTLink;
extern void   sub_1007a700(LTLink* pHead, Item1007a700*** ppArray,
                           uint32* pnCount);            // 0x1007a700 (d3dr5c.cpp)
extern void   sub_1005f580();                           // 0x1005f580
extern void   sub_10040dd0();                           // 0x10040dd0
extern void sub_10021d10(const char* pszMsg, ...);      // 0x10021d10

typedef void (*PFN1007b580)();

// The walk record the tree's visitor takes (constructor inlined: the
// defaults are the do-nothing 0x1006c670 and 0x1006c680).
struct Visit1007b580
{
	uint32			m_n00;                              // +0x00
	float			m_fX04, m_fY04, m_fZ04;             // +0x04 the eye
	float			m_fFar10;                           // +0x10
	PFN1007b580		m_pfnObject14;                      // +0x14
	PFN1007b580		m_pfnObjList18;                     // +0x18
	uint32			m_n1c;                              // +0x1c
	PFN1007b580		m_pfnLeaf20;                        // +0x20
	PFN1007b580		m_pfnNode24;                        // +0x24
	PFN1007b580		m_pfn28;                            // +0x28
	uint8			m_pad_2c[0x48 - 0x2c];      // the shipped frame
												// holds the record at -0x50..-0x8,
												// 0x48 bytes (sub $0x68): pairs as near

	inline Visit1007b580()
	{
		m_pfnObjList18 = LTNULL;
		m_n1c = 0;
		m_n00 = 0;
		m_pfn28 = LTNULL;
		m_fFar10 = 0.0f;
		m_fZ04 = 0.0f;
		m_fY04 = 0.0f;
		m_fX04 = 0.0f;
		m_pfnObject14 = (PFN1007b580)sub_1006c670;
		m_pfnLeaf20 = (PFN1007b580)sub_1006c670;
		m_pfnNode24 = (PFN1007b580)sub_1006c680;
	}
};

class CTree1007b580
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Slot10();
	virtual void	Visit(Visit1007b580* pVisit);       // +0x14
};

class CWorld1007b580
{
public:
	virtual void			Slot00();
	virtual void			Slot04();
	virtual BSP1007aeb4*	GetVisBSP();                // +0x08

	uint8	m_pad_04[0x6c - 0x04];
	uint8	m_Tree6c[4];                                // +0x6c
	uint8	m_pad_70[0x180 - 0x70];
	uint32	m_nFlags180;                                // +0x180
};

struct FrustumPlane1007b580
{
	float			m_fX, m_fY, m_fZ;                   // +0x00
	float			m_fDist;                            // +0x0c
};

extern CWorld1007b580*	g_1008beac;                     // 0x1008beac
extern int32			g_CV_LockPVS;                   // 0x1008cadc
extern int32			g_CV_DrawAll;                   // 0x1008cac8
extern int32			g_CV_DrawWorld;                 // 0x100893a4
extern uint32			g_1008bd54;                     // 0x1008bd54
extern float			g_f1008bd58[3];                 // 0x1008bd58 portal eye
extern float			g_f1008bd34[3];                 // 0x1008bd34 eye
extern FrustumPlane1007b580 g_1008bc50[6];              // 0x1008bc50
extern uint32			g_1008b874;                     // 0x1008b874 clip flags
extern int32			g_nWorldPoliesProcessed;        // 0x1008be58

extern void (*g_1008ce94)(Poly1007aeb4* pPoly);         // 0x1008ce94  textured
extern void (*g_1008ce98)(Poly1007aeb4* pPoly);         // 0x1008ce98  untextured
extern void (*g_1008ce9c)(Poly1007aeb4* pPoly);         // 0x1008ce9c  flag 0x80
extern void (*g_1008cea0)(Poly1007aeb4* pPoly);         // 0x1008cea0  flag 0x8000

inline void DrawQueue1007b580()
{
	Poly1007aeb4**		ppPoly;
	Poly1007aeb4*		pPoly;
	Plane1007aeb4*		pPlane;
	Surf1007aeb4*		pSurf;
	uint32				nPolys;
	uint32				i;
	float				fDist, fRadius;

	ppPoly = g_10091a10.m_Opaque.m_pArray;
	for (nPolys = g_10091a10.m_Opaque.m_nUsed; nPolys; nPolys--, ppPoly++)
	{
		pPoly = *ppPoly;

		pPlane = pPoly->m_pSurf28->m_pPlane34;
		if (pPlane->m_fX * g_f1008bd34[0] + pPlane->m_fY * g_f1008bd34[1] +
		    pPlane->m_fZ * g_f1008bd34[2] - pPlane->m_fDist > 0.01f)
		{
		g_1008b874 = 0x3f;
		for (i = 0; i < 6; i++)
		{
			fDist = g_1008bc50[i].m_fX * pPoly->m_fX18 +
			        g_1008bc50[i].m_fY * pPoly->m_fY18 +
			        g_1008bc50[i].m_fZ * pPoly->m_fZ18 - g_1008bc50[i].m_fDist;
			fRadius = pPoly->m_fRadius24;
			if (fDist < -fRadius)
				goto next;

			if (fDist > fRadius)
				g_1008b874 &= ~(1 << i);
		}

		g_nWorldPoliesProcessed++;

		pSurf = pPoly->m_pSurf28;
		if (pSurf->m_nFlags38 & 4)
			continue;

		if (!pSurf->m_pTexture30)
			g_1008ce98(pPoly);
		else if (pSurf->m_nFlags38 & 0x80)
			g_1008ce9c(pPoly);
		else if (pSurf->m_nFlags38 & 0x8000)
			g_1008cea0(pPoly);
		else
			g_1008ce94(pPoly);
		}
next:
		;
	}
}

static void sub_1007b580(LTBOOL bAll)
{
	Visit1007b580		visit;
	Link1007aeb4*		pLink;
	BSP1007aeb4*		pBSP;

	if (!(g_10091a10.m_nFlags0c & 1) || g_CV_LockPVS)
	{
	}
	else
	{
	{
		g_10091a10.m_nEntries = 0;
		g_10091a10.m_Translucent.m_nUsed = 0;
		g_10091a10.m_Opaque.m_nUsed = 0;

		for (pLink = g_10091a10.m_Head.m_pNext; pLink != &g_10091a10.m_Head;
		     pLink = pLink->m_pNext)
			((uint32*)pLink->m_pData)[1] = 0;

		if (bAll)
		{
			pBSP = g_1008beac->GetVisBSP();
			if (!pBSP)
				sub_10021d10("ERROR: GetVisBSP returned LTNULL!");
			else
				sub_1007aeb0(pBSP);
		}
		else
		{
			visit.m_n00 = 0;
			if (g_1008bd54)
			{
				visit.m_fX04 = g_f1008bd58[0];
				visit.m_fY04 = g_f1008bd58[1];
				visit.m_fZ04 = g_f1008bd58[2];
			}
			else
			{
				visit.m_fX04 = g_f1008bd34[0];
				visit.m_fY04 = g_f1008bd34[1];
				visit.m_fZ04 = g_f1008bd34[2];
			}

			visit.m_pfn28 = (PFN1007b580)sub_1007b520;
			visit.m_pfnNode24 = (PFN1007b580)sub_1001e770;
			visit.m_pfnLeaf20 = (PFN1007b580)sub_1007a7b0;
			visit.m_n1c = 0;
			visit.m_pfnObjList18 = (PFN1007b580)sub_1007a700;
			visit.m_pfnObject14 = (PFN1007b580)sub_1007a680;
			visit.m_fFar10 = 10000.0f;

			((CTree1007b580*)g_1008beac->m_Tree6c)->Visit(&visit);
		}
	}
	}

	sub_1005f580();
	sub_10040dd0();

	if (g_CV_DrawWorld)
		DrawQueue1007b580();
}


void sub_1007b880()
{
	if ((g_1008beac->m_nFlags180 & 2) && !g_CV_DrawAll)
		sub_1007b580(LTFALSE);
	else
		sub_1007b580(LTTRUE);
}
