// *********************************************************************** //
//
//  w12_36320_os.cpp -- 0x10036320 (88 bytes): C10036320's constructor.
//  IDENTICAL at /Os (86 bytes + pad; /O2 is 112). The singleton
//  is typed as a POINTER here (d3dneutral.cpp defines it as uint32): icl
//  predicts a pointer test taken and hosts the store out of line, as
//  shipped; typed uint32 the store stays inline.
//
//  d3dr3a.cpp's 0x10033120 does `new C10036320;` and discards the result:
//  the object registers itself. Read off the shipped body:
//    * `movl $0x100894a0,(%ebx)` -- its vtable (d3ddata.cpp's
//      g_vtbl_100894a0: 0x10035b40, 0x10036380, 0x10036420);
//    * the first one constructed becomes the singleton at 0x100911b8
//      (d3dneutral.cpp's g_100911b8; the store is hosted out of line,
//      `je 0x1003636e` ... `jmp 0x10036339`);
//    * +0x04 = `new` of a twelve-byte node (operator new, 0x10081222) built
//      by its constructor 0x10036254 with (0, 0) -- or 0 if new failed.
//  Returns this.
//
//  The node's constructor is spelled as d3dr5c.cpp spells it
//  (??0Node10036380, already aliased onto d3dr2b.obj's
//  Node10036254::sub_10036254, identical).
//
// *********************************************************************** //

typedef unsigned long uint32;

extern void* g_vtbl_100894a0[];                         // 0x100894a0

struct Node10036380
{
	Node10036380(void* pValue, Node10036380* pNext);    // 0x10036254
	void*			m_pValue;                           // +0x00
	Node10036380*	m_pNext;                            // +0x04
	uint32			m_n08;                              // +0x08
};

class C10036320;
extern C10036320* g_100911b8;                           // 0x100911b8

class C10036320
{
public:
			C10036320();                                // 0x10036320

	void	**m_pVTable;                                // +0x00
	Node10036380* m_pList;                              // +0x04
};

C10036320::C10036320()
{
	m_pVTable = g_vtbl_100894a0;
	if (!g_100911b8)
		g_100911b8 = this;
	m_pList = new Node10036380(0, 0);
}
