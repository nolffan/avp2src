// *********************************************************************** //
//
//  d3dinit.cpp -- d3d.ren's module initialisers, put where the CRT runs them.
//
//  The shipped image's static-initialiser table (__xc_a 0x10089000 ..
//  __xc_z 0x10089224, the .CRT$XC section merged into .data, one 16-byte
//  slot each, run by _initterm from _CRT_INIT) holds 34 entries:
//
//      0x10009860  0x1000eeb0  0x10014600  0x1001bfa0  0x1001eab0
//      0x10021580  0x10021f90  0x10023200  0x1002db00  0x10069c90
//      0x10033070  0x1003ac30  0x1003b1c0  0x1003b3b0  0x1003d910
//      0x10047b00  0x1004b670  0x1004faf0  0x10050330  0x10050c60
//      0x10059c80  0x1005db90  0x1005dbf0  0x1005f730  0x1000eeb0
//      0x10068c50  0x10069c90  0x1006aca0  0x1006c4a0  0x1006d500
//      0x1007a1d0  0x1007bb20  0x1007e830  0x1007faa0
//
//  Nine of them our C++ already produces as real static objects, so icl
//  emits their __sti__ functions into .CRT$XCU itself: 0x10009860,
//  0x1001eab0 (d3dblit.cpp), 0x10021580 (w26_10020fd0), 0x1002db00,
//  0x1003b1c0, 0x1006d500, 0x1007bb20, 0x1007e830 and 0x1007faa0
//  (w12_7fae0, LAlloc's vtable).  The other 25 are written as plain
//  `void sub_<addr>()` functions -- console-variable registration, list
//  heads, the pixel-format manager (0x1003ac30 -> FormatMgr 0x1007a0d0 at
//  0x1008daa0), the frame-buffer tables (0x10059c80), ... -- that nothing
//  called, so the linker dropped them and they never ran.  The first
//  symptom: FormatMgr's "n bits -> 8 bits" table pointers stayed NULL and
//  the first 16-bit texture conversion (0x10075920) read through them
//  (page fault at 0x00000000 once a world loaded).
//
//  This table puts them in .CRT$XCU, in the shipped order.  Within one
//  section name LINK orders contributions by object order, and
//  build/engine.sh links Release/*.obj alphabetically, so this table runs
//  after d3dblit.obj's initialiser and before the eight w*.obj ones.  Of
//  the shipped order that moves only those eight relative to these; none
//  of the 25 reads anything those eight set (checked against the shipped
//  bodies: they prepend console variables to the list at 0x1008bf90, tie
//  off their own list heads and fill their own tables), so the visible
//  difference is the order of the console-variable list.
//
//  0x1000eeb0 and 0x10069c90 are empty initialisers (a bare `ret`) that
//  appear twice each in the shipped table; they are kept, as shipped.
//
// *********************************************************************** //

typedef void (__cdecl* PFV)();

void sub_1000eeb0();
void sub_10014600();
void sub_1001bfa0();
void sub_10021f90();
void sub_10023200();
void sub_10069c90();
void sub_10033070();
void sub_1003ac30();
void sub_1003b3b0();
void sub_1003d910();
void sub_10047b00();
void sub_1004b670();
void sub_1004faf0();
void sub_10050330();
void sub_10050c60();
void sub_10059c80();
void sub_1005db90();
void sub_1005dbf0();
void sub_1005f730();
void sub_10068c50();
void sub_1006aca0();
void sub_1006c4a0();
void sub_1007a1d0();

#pragma data_seg(".CRT$XCU")
PFV g_d3dModuleInits[] =
{
	sub_1000eeb0,           // 0x10089020
	sub_10014600,           // 0x10089030
	sub_1001bfa0,           // 0x10089040
	sub_10021f90,           // 0x10089070
	sub_10023200,           // 0x10089080
	sub_10069c90,           // 0x100890a0
	sub_10033070,           // 0x100890b0
	sub_1003ac30,           // 0x100890c0
	sub_1003b3b0,           // 0x100890e0
	sub_1003d910,           // 0x100890f0
	sub_10047b00,           // 0x10089100
	sub_1004b670,           // 0x10089110
	sub_1004faf0,           // 0x10089120
	sub_10050330,           // 0x10089130
	sub_10050c60,           // 0x10089140
	sub_10059c80,           // 0x10089150
	sub_1005db90,           // 0x10089160
	sub_1005dbf0,           // 0x10089170
	sub_1005f730,           // 0x10089180
	sub_1000eeb0,           // 0x10089190
	sub_10068c50,           // 0x100891a0
	sub_10069c90,           // 0x100891b0
	sub_1006aca0,           // 0x100891c0
	sub_1006c4a0,           // 0x100891d0
	sub_1007a1d0,           // 0x100891f0
};
#pragma data_seg()
