// ======================================================================= //
//  w27_1005dae0.cpp -- d3d.ren: two deferred-draw callbacks.
//  /O2 (the `lea` padding after each `ret`).
//
//  0x1005dae0 and 0x100646a0 are the same 38-byte function with a
//  different draw callback, the shape of d3dr3c.cpp's 0x100688e0 and
//  d3dr4b.cpp's 0x1003b760: a cdecl (pCtx, pItem) whose first argument
//  is dead, that queues pItem on the deferred-draw queue at 0x1008ef3c
//  through 0x100500a0 (w27_1004fbc0_qipni.cpp) with its draw function.
//  Neither has a direct caller; both are reached as function pointers.
//
//      55 8b ec 83 ec 08 83 c4 f8     frame, 8-byte outgoing area
//      8b 45 0c                       pItem (the second argument)
//      8b 0d 3c ef 08 10              this = g_p1008ef3c
//      89 04 24 / c7 44 24 04 <fn>    (pItem, fn)
//      e8 <0x100500a0>
//
//  0x10050bf0 is a third copy (callback 0x10050900), NOT written: no
//  source defines 0x10050900 yet (739 bytes), and referencing it would
//  add an unresolved external to the link.
// ======================================================================= //
#include "ltbasedefs.h"

class C100500a0
{
public:
	void	sub_100500a0(void* pItem, void (*pfnDraw)(void* pCtx, void* pItem));   // 0x100500a0
};

extern C100500a0*	g_p1008ef3c;                        // 0x1008ef3c  the queue

struct CtxPG5a960;
class  ObjPG5a960;
extern void sub_1005a960(CtxPG5a960* pCtx, ObjPG5a960* pObj);       // 0x1005a960  (big5a960.cpp)

struct Ctx61a30;
class  Obj61a30;
extern void sub_10061a30(Ctx61a30* pCtx, Obj61a30* pObj);           // 0x10061a30  (big61a30.cpp)


// ======================================================================= //
//  0x1005dae0 -- 38 bytes.  Queue with 0x1005a960.
// ======================================================================= //
void sub_1005dae0(void* pCtx, void* pItem)
{
	g_p1008ef3c->sub_100500a0(pItem, (void (*)(void*, void*))sub_1005a960);
}


// ======================================================================= //
//  0x100646a0 -- 38 bytes.  Queue with 0x10061a30.
// ======================================================================= //
void sub_100646a0(void* pCtx, void* pItem)
{
	g_p1008ef3c->sub_100500a0(pItem, (void (*)(void*, void*))sub_10061a30);
}
