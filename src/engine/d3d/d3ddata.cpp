// *********************************************************************** //
//  d3ddata.cpp -- d3d.ren's globals, ONE definition per shipped address.
//
//  Generated from the retail d3d.ren and the d3d objects.
//
//  The d3d TUs declare these `extern`; this file defines each once, with
//  the shipped initial bytes, in address order within each section. The
//  other spellings of an address (another type, another name) are weak
//  aliases of the definition here -- src/engine/d3d/link/aliases.rsp,
//  the generated d3d.ren-globals block. Each trailing comment gives the address, the
//  extent and where the address came from (the name, the extern's
//  comment, or a placed function's relocation).
//
//  A struct/class-typed global is defined through an OPAQUE stand-in of
//  its tag: the mangled name depends only on the tag and its class-key,
//  and nothing but this TU sees the stand-in's layout. An object spans
//  the whole gap to the next known global (the spelling may be a view --
//  an interface or base -- smaller than the object really there), so a
//  stand-in is as large as the largest gap any global of its tag has,
//  and never smaller than the sizeof the TUs agree on (link/d3dsizes.txt).
//
//  d3dfn_<addr> is the function our objects define at that address; the
//  alias block maps each onto its real name.
// *********************************************************************** //

struct Bucket123a0;
class C1007a180Disp;
struct C1007a180Req;
struct ConvertRequest;
struct Elem24024;
class FormatMgr;
struct W7Poly56c60;

struct Bank10021f40 { public: unsigned char m_Bytes[0x20]; };
struct CVarR5C { public: unsigned char m_Bytes[0x20]; };

extern "C" void d3dfn_10003770();
extern "C" void d3dfn_100037b0();
extern "C" void d3dfn_100037d0();
extern "C" void d3dfn_100037f0();
extern "C" void d3dfn_100038e0();
extern "C" void d3dfn_1001c480();
extern "C" void d3dfn_1001c4c0();
extern "C" void d3dfn_1001c5a0();
extern "C" void d3dfn_1001c680();
extern "C" void d3dfn_1001c6a0();
extern "C" void d3dfn_1001c6c0();
extern "C" void d3dfn_1001c6e0();
extern "C" void d3dfn_1001c720();
extern "C" void d3dfn_1001c760();
extern "C" void d3dfn_1001cf40();
extern "C" void d3dfn_1001cf60();
extern "C" void d3dfn_1001cfa0();
extern "C" void d3dfn_1001d080();
extern "C" void d3dfn_1001d160();
extern "C" void d3dfn_1001d180();
extern "C" void d3dfn_1001d1a0();
extern "C" void d3dfn_1001d1c0();
extern "C" void d3dfn_1001d200();
extern "C" void d3dfn_1001d240();
extern "C" void d3dfn_1001d980();
extern "C" void d3dfn_1001d9c0();
extern "C" void d3dfn_1001da90();
extern "C" void d3dfn_1001db50();
extern "C" void d3dfn_1001db70();
extern "C" void d3dfn_1001db90();
extern "C" void d3dfn_1001dbb0();
extern "C" void d3dfn_1001dbd0();
extern "C" void d3dfn_1001dc00();
extern "C" void d3dfn_10035a70();
extern "C" void d3dfn_10035ae0();
extern "C" void d3dfn_10035b10();
extern "C" void d3dfn_10035b40();
extern "C" void d3dfn_10035bd0();
extern "C" void d3dfn_10035c00();
extern "C" void d3dfn_10035c30();
extern "C" void d3dfn_10035e20();
extern "C" void d3dfn_10036130();
extern "C" void d3dfn_100361c0();
extern "C" void d3dfn_10036380();
extern "C" void d3dfn_10036420();
extern "C" void d3dfn_10036460();
extern "C" void d3dfn_10037470();
extern "C" void d3dfn_100374a0();
extern "C" void d3dfn_100374d0();
extern "C" void d3dfn_10037570();
extern "C" void d3dfn_10037590();
extern "C" void d3dfn_100375b0();
extern "C" void d3dfn_1003aba0();
extern "C" void d3dfn_1003abb0();
extern "C" void d3dfn_1003abc0();
extern "C" void d3dfn_1003abd0();
extern "C" void d3dfn_1003bb10();
extern "C" void d3dfn_1003d350();
extern "C" void d3dfn_1003d5c0();
extern "C" void d3dfn_1003d5f0();
extern "C" void d3dfn_1003d8f0();
extern "C" void d3dfn_100503f0();
extern "C" void d3dfn_10050670();
extern "C" void d3dfn_10050770();
extern "C" void d3dfn_10050cc0();
extern "C" void d3dfn_10050da0();
extern "C" void d3dfn_10050dc0();
extern "C" void d3dfn_10050de0();
extern "C" void d3dfn_10050e10();
extern "C" void d3dfn_10050e40();
extern "C" void d3dfn_10050e60();
extern "C" void d3dfn_100525b0();
extern "C" void d3dfn_10056aa0();
extern "C" void d3dfn_100591a0();
extern "C" void d3dfn_100591e0();
extern "C" void d3dfn_10059210();
extern "C" void d3dfn_10059270();
extern "C" void d3dfn_100592a0();
extern "C" void d3dfn_10059300();
extern "C" void d3dfn_10059380();
extern "C" void d3dfn_10059590();
extern "C" void d3dfn_10059620();
extern "C" void d3dfn_10059870();
extern "C" void d3dfn_10059880();
extern "C" void d3dfn_100598a0();
extern "C" void d3dfn_100598f0();
extern "C" void d3dfn_10059980();
extern "C" void d3dfn_10059c70();
extern "C" void d3dfn_10059f70();
extern "C" void d3dfn_10059fc0();
extern "C" void d3dfn_1005a040();
extern "C" void d3dfn_1005a050();
extern "C" void d3dfn_1005a0b0();
extern "C" void d3dfn_1005a0f0();
extern "C" void d3dfn_1005a1d0();
extern "C" void d3dfn_1005a2b0();
extern "C" void d3dfn_1005a2d0();
extern "C" void d3dfn_1005a2f0();
extern "C" void d3dfn_1005a310();
extern "C" void d3dfn_1005a350();
extern "C" void d3dfn_1006c690();
extern "C" void d3dfn_1006c6c0();
extern "C" void d3dfn_1006c6d0();
extern "C" void d3dfn_10073c00();
extern "C" void d3dfn_10073e00();
extern "C" void d3dfn_100740a0();
extern "C" void d3dfn_10074350();
extern "C" void d3dfn_100746b0();
extern "C" void d3dfn_10074a50();
extern "C" void d3dfn_10074ea0();
extern "C" void d3dfn_100754d0();
extern "C" void d3dfn_10075920();
extern "C" void d3dfn_10075cd0();
extern "C" void d3dfn_10076300();
extern "C" void d3dfn_10076870();
extern "C" void d3dfn_10076df0();
extern "C" void d3dfn_10077110();
extern "C" void d3dfn_100773e0();
extern "C" void d3dfn_10077680();
extern "C" void d3dfn_10078910();
extern "C" void d3dfn_100799b0();
extern "C" void d3dfn_10079aa0();
extern "C" void d3dfn_10079b90();
extern "C" void d3dfn_10079c70();
extern "C" void d3dfn_10079d70();
extern "C" void d3dfn_10079e70();
extern "C" void d3dfn_1007a3f0();
extern "C" void d3dfn_1007c440();
extern "C" void d3dfn_1007c480();
extern "C" void d3dfn_1007c580();
extern "C" void d3dfn_1007c670();
extern "C" void d3dfn_1007c690();
extern "C" void d3dfn_1007c6b0();
extern "C" void d3dfn_1007c6d0();
extern "C" void d3dfn_1007c700();
extern "C" void d3dfn_1007dbd0();
extern "C" void d3dfn_1007dc00();
extern "C" void d3dfn_1007dc40();
extern "C" void d3dfn_1007dd20();
extern "C" void d3dfn_1007dde0();
extern "C" void d3dfn_1007def0();
extern "C" void d3dfn_1007dfa0();
extern "C" void d3dfn_1007e110();
extern "C" void d3dfn_1007e1c0();
extern "C" void d3dfn_1007e270();
extern "C" void d3dfn_1007e2c0();
extern "C" void d3dfn_1007e8b0();
extern "C" void d3dfn_1007ec20();
extern "C" void d3dfn_1007ed00();
extern "C" void d3dfn_1007ede0();
extern "C" void d3dfn_1007ee00();
extern "C" void d3dfn_1007ee20();
extern "C" void d3dfn_1007f560();
extern "C" void d3dfn_1007f5a0();
extern "C" void d3dfn_1007f670();
extern "C" void d3dfn_1007f730();
extern "C" void d3dfn_1007f750();
extern "C" void d3dfn_1007f770();
extern "C" void d3dfn_1007f790();
extern "C" void d3dfn_1007f7b0();
extern "C" void d3dfn_10080905();

extern "C" unsigned char d3dobj_1008e6d0[];   // defined by a TU (alias block)
extern "C" unsigned char d3dobj_1008e6e8[];   // defined by a TU (alias block)

extern "C" unsigned char d3dview_1008bf6c[];   // link/d3dviews.txt


// ======================================================================= //
//  .data, initialised
// ======================================================================= //
void * g_vtbl_10089260[8] = { (void *)(&d3dfn_10003770), (void *)(&d3dfn_1007dd20), (void *)(&d3dfn_1007dde0), (void *)(&d3dfn_1007def0), (void *)(&d3dfn_1007e8b0), (void *)(&d3dfn_1007dfa0), (void *)(&d3dfn_100037b0), (void *)(&d3dfn_100037f0) };   // 0x10089260  0x20  (name)
void * g_vtbl_10089280[8] = { (void *)(&d3dfn_10003770), (void *)(&d3dfn_1007dd20), (void *)(&d3dfn_1007dde0), (void *)(&d3dfn_1007def0), (void *)(&d3dfn_1007e8b0), (void *)(&d3dfn_1007dfa0), (void *)(&d3dfn_100037d0), (void *)(&d3dfn_100038e0) };   // 0x10089280  0x20  (name)
void * g_vtbl_100892a0[8] = { (void *)(&d3dfn_10003770), (void *)(&d3dfn_1007dd20), (void *)(&d3dfn_1007dde0), (void *)(&d3dfn_1007def0), (void *)(&d3dfn_1007e8b0), (void *)(&d3dfn_1007dfa0), (void *)(&d3dfn_100037b0), (void *)(&d3dfn_100037f0) };   // 0x100892a0  0x20  (name)
void * g_vtbl_100892c0[8] = { (void *)(&d3dfn_10003770), (void *)(&d3dfn_1007dd20), (void *)(&d3dfn_1007dde0), (void *)(&d3dfn_1007def0), (void *)(&d3dfn_1007e8b0), (void *)(&d3dfn_1007dfa0), (void *)(&d3dfn_100037d0), (void *)(&d3dfn_100038e0) };   // 0x100892c0  0x20  (name)
extern void * const g_vtbl_100892e0[12] = { (void * const)(&d3dfn_1001d240), (void * const)(&d3dfn_1001c760), (void * const)(&d3dfn_1001dc00), (void * const)(&d3dfn_1001dbd0), (void * const)(&d3dfn_1001dbb0), (void * const)(&d3dfn_1001db90), (void * const)(&d3dfn_1001db70), (void * const)(&d3dfn_1001db50), (void * const)(&d3dfn_1001da90), (void * const)(&d3dfn_1001d9c0), (void * const)(&d3dfn_1001d980), (void * const)(&d3dfn_1001cf40) };   // 0x100892e0  0x30  (name)
extern void * const g_vtbl_10089310[12] = { (void * const)(&d3dfn_1001d240), (void * const)(&d3dfn_1001c760), (void * const)(&d3dfn_1001d200), (void * const)(&d3dfn_1001d1c0), (void * const)(&d3dfn_1001d1a0), (void * const)(&d3dfn_1001d180), (void * const)(&d3dfn_1001d160), (void * const)(&d3dfn_1001db50), (void * const)(&d3dfn_1001d080), (void * const)(&d3dfn_1001cfa0), (void * const)(&d3dfn_1001cf60), (void * const)(&d3dfn_1001cf40) };   // 0x10089310  0x30  (name)
extern void * const g_vtbl_10089340[12] = { (void * const)(&d3dfn_1001d240), (void * const)(&d3dfn_1001c760), (void * const)(&d3dfn_1001c720), (void * const)(&d3dfn_1001c6e0), (void * const)(&d3dfn_1001c6c0), (void * const)(&d3dfn_1001c6a0), (void * const)(&d3dfn_1001c680), (void * const)(&d3dfn_1001db50), (void * const)(&d3dfn_1001c5a0), (void * const)(&d3dfn_1001c4c0), (void * const)(&d3dfn_1001c480), (void * const)(&d3dfn_1001cf40) };   // 0x10089340  0x30  (name)
void * g_vtbl_10089390[1] = { (void *)(&d3dfn_1007a3f0) };   // 0x10089390  0x4  (name; +1 alias)
float g_CV_EnvPanSpeed = 0.0005f;   // 0x100893a0  0x4  (comment)
long g_CV_DrawWorld = 0x1;   // 0x100893a4  0x4  (comment)
float g_CV_ShadowZRange = 17.0f;   // 0x100893a8  0x4  (comment)
float g_CV_SkyScale = 1.0f;   // 0x100893ac  0x4  (comment)
float g_CV_Gamma = 100.0f;   // 0x100893b0  0x4  (comment)
long g_CV_UseDX6Commands = 0x1;   // 0x100893b4  0x4  (comment)
long g_CV_LightAddPoly = 0x1;   // 0x100893b8  0x4  (comment)
long g_CV_FogColor[3] = { 0xff, 0xff, 0xff };   // 0x100893bc  0xc  (comment)
float g_100893c8 = 2000.0f;   // 0x100893c8  0x4  (comment/name; +1 alias)
long g_CV_DrawSky = 0x1;   // 0x100893cc  0x4  (comment)
long g_CV_TextureModels = 0x1;   // 0x100893d0  0x4  (comment)
unsigned long g_100893d4 = 0x1;   // 0x100893d4  0x4  (comment/name; +1 alias)
unsigned long g_100893d8 = 0x1;   // 0x100893d8  0x4  (comment/name; +1 alias)
unsigned long g_100893dc = 0x1;   // 0x100893dc  0x4  (comment/name; +1 alias)
unsigned long g_100893e0 = 0x1;   // 0x100893e0  0x4  (comment/name; +1 alias)
long g_CV_ModelFullbrite = 0x1;   // 0x100893e4  0x4  (comment)
long g_CV_ShadowLodOffset = 0x64;   // 0x100893e8  0x4  (comment)
long g_CV_MaxModelShadows = 0x1;   // 0x100893ec  0x4  (comment/name; +1 alias)
float g_CV_LodScale = 1.0f;   // 0x100893f0  0x4  (comment)
unsigned long g_100893f4 = 0x1;   // 0x100893f4  0x4  (comment/name; +1 alias)
unsigned long g_100893f8 = 0x1;   // 0x100893f8  0x4  (comment/name; +1 alias)
unsigned long g_100893fc = 0x1;   // 0x100893fc  0x4  (comment/name; +1 alias)
long g_CV_LightMap = 0x1;   // 0x10089400  0x4  (comment)
long g_CV_Dither = 0x1;   // 0x10089404  0x4  (comment)
long g_CV_TintModels = 0x1;   // 0x10089408  0x4  (comment/name; +1 alias)
unsigned long g_10089410 = 0x1;   // 0x10089410  0x4  (name)
unsigned long g_10089414 = 0;   // 0x10089414  0x4  (name)
long g_10089420 = 0x1;   // 0x10089420  0x4  (name)
unsigned long g_TextureSizes[6] = { 0x2, 0x4, 0x8, 0x10, 0x20, 0x40 };   // 0x10089430  0x18  (comment)
float g_1008944c = 0.01f;   // 0x1008944c  0x4  (name)
extern void * const g_vtbl_10089450[7] = { (void * const)(&d3dfn_10035ae0), (void * const)(&d3dfn_10080905), (void * const)(&d3dfn_10080905), (void * const)(&d3dfn_10080905), (void * const)(&d3dfn_10080905), (void * const)(&d3dfn_10080905), (void * const)(&d3dfn_10080905) };   // 0x10089450  0x1c  (name)
void * g_vtbl_10089470[3] = { (void *)(&d3dfn_10035b10), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905) };   // 0x10089470  0xc  (name; +1 alias)
void * g_vtbl_10089480[8] = { (void *)(&d3dfn_10035a70), (void *)(&d3dfn_10036460), (void *)(&d3dfn_10035c00), (void *)(&d3dfn_10035c30), (void *)(&d3dfn_10035e20), (void *)(&d3dfn_10036130), (void *)(&d3dfn_100361c0), (void *)(&d3dfn_10035bd0) };   // 0x10089480  0x20  (name)
void * g_vtbl_100894a0[3] = { (void *)(&d3dfn_10035b40), (void *)(&d3dfn_10036380), (void *)(&d3dfn_10036420) };   // 0x100894a0  0xc  (name)
void * g_vtbl_100894c0[4] = { (void *)(&d3dfn_10037470), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905) };   // 0x100894c0  0x10  (name; +1 alias)
void * g_vtbl_100894d0[5] = { (void *)(&d3dfn_100374a0), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905) };   // 0x100894d0  0x14  (name)
void * g_vtbl_100894f0[5] = { (void *)(&d3dfn_100374a0), (void *)(&d3dfn_1003aba0), (void *)(&d3dfn_1003abb0), (void *)(&d3dfn_1003abc0), (void *)(&d3dfn_1003abd0) };   // 0x100894f0  0x14  (name)
void * g_vtbl_10089510[6] = { (void *)(&d3dfn_100374d0), (void *)(&d3dfn_10037570), (void *)(&d3dfn_10037590), (void *)(&d3dfn_100375b0), (void *)(d3dobj_1008e6d0), (void *)(d3dobj_1008e6e8) };   // 0x10089510  0x18  (name; +1 alias)
void * g_10089530[8] = { (void *)(&d3dfn_1003bb10), (void *)(&d3dfn_1003d350), (void *)(&d3dfn_1003d5c0), (void *)(&d3dfn_1003d5f0), (void *)(&d3dfn_1003d8f0) };   // 0x10089530  0x20  (name; +1 alias)
float g_f10089550 = 0.5f;   // 0x10089550  0x4  (name)
float g_f10089554 = 0.5f;   // 0x10089554  0x4  (name)
unsigned long g_10089560[10] = { 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1 };   // 0x10089560  0x28  (name)
unsigned long g_10089588[10] = { 0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };   // 0x10089588  0x28  (name)
void (__cdecl* g_100896f0[2])(struct W7Poly56c60 *, unsigned long *, unsigned long *) = { (void (__cdecl*)(struct W7Poly56c60 *, unsigned long *, unsigned long *))(&d3dfn_10050e10), (void (__cdecl*)(struct W7Poly56c60 *, unsigned long *, unsigned long *))(&d3dfn_10050e40) };   // 0x100896f0  0x8  (name)
void * g_100896f8[4] = { (void *)(&d3dfn_100591a0), (void *)(&d3dfn_100591e0), (void *)(&d3dfn_10059210), (void *)(&d3dfn_10059270) };   // 0x100896f8  0x10  (name)
void * g_10089708[4] = { 0, 0, (void *)(&d3dfn_100592a0), (void *)(&d3dfn_10059300) };   // 0x10089708  0x10  (name)
void * g_10089718[4] = { (void *)(&d3dfn_1005a050), (void *)(&d3dfn_1005a040), (void *)(&d3dfn_10059fc0), (void *)(&d3dfn_10059f70) };   // 0x10089718  0x10  (name; +1 alias)
void * g_10089728[4] = { (void *)(&d3dfn_10059380), (void *)(&d3dfn_10059590), (void *)(&d3dfn_10059620), (void *)(&d3dfn_10059870) };   // 0x10089728  0x10  (name)
void * g_10089738[3] = { (void *)(&d3dfn_10059880), (void *)(&d3dfn_100598a0), (void *)(&d3dfn_100598f0) };   // 0x10089738  0xc  (name)
void (__fastcall* * g_10089744[2])(void *, void *) = { (void (__fastcall* *)(void *, void *))(d3dview_1008bf6c + 0xf20), (void (__fastcall* *)(void *, void *))(d3dview_1008bf6c + 0xf1c) };   // 0x10089744  0x8  (name)
unsigned long g_1008974c[4] = { 0, 0x20, 0x28, 0x30 };   // 0x1008974c  0x10  (name)
long (__fastcall* g_1008975c[4])(unsigned long, unsigned char * *, long *) = { 0, (long (__fastcall*)(unsigned long, unsigned char * *, long *))(&d3dfn_10050e60), (long (__fastcall*)(unsigned long, unsigned char * *, long *))(&d3dfn_100525b0), (long (__fastcall*)(unsigned long, unsigned char * *, long *))(&d3dfn_10056aa0) };   // 0x1008975c  0x10  (name)
void * g_1008977c[3] = { (void *)(&d3dfn_10059c70), (void *)(&d3dfn_10059980), (void *)(&d3dfn_10059c70) };   // 0x1008977c  0xc  (name)
unsigned long g_10089788[4] = { 0, 0x1c4, 0x2c4, 0x3c4 };   // 0x10089788  0x10  (name)
extern void * const g_vtbl_100897a0[12] = { (void * const)(&d3dfn_1001d240), (void * const)(&d3dfn_1001c760), (void * const)(&d3dfn_1005a350), (void * const)(&d3dfn_1005a310), (void * const)(&d3dfn_1005a2f0), (void * const)(&d3dfn_1005a2d0), (void * const)(&d3dfn_1005a2b0), (void * const)(&d3dfn_1001db50), (void * const)(&d3dfn_1005a1d0), (void * const)(&d3dfn_1005a0f0), (void * const)(&d3dfn_1005a0b0), (void * const)(&d3dfn_1001cf40) };   // 0x100897a0  0x30  (name)
extern void * const g_vtbl_100897d0[4] = { (void * const)(&d3dfn_10050cc0), (void * const)(&d3dfn_10050da0), (void * const)(&d3dfn_10050dc0), (void * const)(&d3dfn_10050de0) };   // 0x100897d0  0x10  (name)
extern long const g_a100897e0[4] = { 0x3, 0x2, 0x1 };   // 0x100897e0  0x10  (name)
extern long const g_a100897f0[4] = { 0, 0x1, 0x2, 0x3 };   // 0x100897f0  0x10  (name)
void * g_vtbl_10089800[5] = { (void *)(&d3dfn_1006c690), (void *)(&d3dfn_1006c6c0), (void *)(&d3dfn_1006c6c0), (void *)(&d3dfn_1006c6d0), (void *)(&d3dfn_1006c6d0) };   // 0x10089800  0x14  (name)
unsigned long g_10089820[5] = { 0x4, 0x8, 0x10, 0x20, 0x40 };   // 0x10089820  0x14  (name)
unsigned long g_10089834[5] = { 0x7d, 0x48, 0x18, 0xa, 0xa };   // 0x10089834  0x14  (name)
unsigned long g_10089870[8] = { 0, 0, 0x1, 0x2 };   // 0x10089870  0x20  (name)
unsigned long g_nFormatBytesPerPixel[8] = { 0x1, 0x1, 0x2, 0x4, 0, 0, 0, 0x1 };   // 0x10089890  0x20  (manual)
unsigned long (__cdecl* g_100898b0[8][8])(class FormatMgr *, struct ConvertRequest *) = { { (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10073c00), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10073e00), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_100740a0), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10074350) }, { 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_100746b0), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10074a50), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10074ea0) }, { 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_100754d0), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10075920), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10075cd0) }, { 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10076300), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10076870), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10076df0) }, { 0, 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10077680), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10078910), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10073c00) }, { 0, 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10077680), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10078910), 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10073c00) }, { 0, 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10077680), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10078910), 0, 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10073c00) }, { 0, 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10077110), (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_100773e0), 0, 0, 0, (unsigned long (__cdecl*)(class FormatMgr *, struct ConvertRequest *))(&d3dfn_10073c00) } };   // 0x100898b0  0x100  (name)
void (__cdecl* g_100899b0[16])(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *) = { 0, (void (__cdecl*)(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *))(&d3dfn_100799b0), (void (__cdecl*)(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *))(&d3dfn_10079aa0), (void (__cdecl*)(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *))(&d3dfn_10079b90), 0, 0, 0, 0, 0, (void (__cdecl*)(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *))(&d3dfn_10079c70), (void (__cdecl*)(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *))(&d3dfn_10079d70), (void (__cdecl*)(class C1007a180Disp *, struct C1007a180Req *, unsigned long *, void *))(&d3dfn_10079e70) };   // 0x100899b0  0x40  (name)
void * g_vtbl_100899f0[12] = { (void *)(&d3dfn_10050770), (void *)(&d3dfn_1007c700), (void *)(&d3dfn_1007c6d0), (void *)(&d3dfn_1007f790), (void *)(&d3dfn_1007c6b0), (void *)(&d3dfn_1007c690), (void *)(&d3dfn_1007c670), (void *)(&d3dfn_10050670), (void *)(&d3dfn_1007c580), (void *)(&d3dfn_1007c480), (void *)(&d3dfn_1007c440), (void *)(&d3dfn_100503f0) };   // 0x100899f0  0x30  (name)
void * g_vtbl_10089a20[8] = { (void *)(&d3dfn_1007dbd0), (void *)(&d3dfn_1007dd20), (void *)(&d3dfn_1007dde0), (void *)(&d3dfn_1007def0), (void *)(&d3dfn_1007e8b0), (void *)(&d3dfn_1007dfa0), (void *)(&d3dfn_10080905), (void *)(&d3dfn_10080905) };   // 0x10089a20  0x20  (name; +1 alias)
void * g_vtbl_10089a40[8] = { (void *)(&d3dfn_1007dc00), (void *)(&d3dfn_1007e1c0), (void *)(&d3dfn_1007dde0), (void *)(&d3dfn_1007e270), (void *)(&d3dfn_1007e8b0), (void *)(&d3dfn_1007e110), (void *)(&d3dfn_1007dc40), (void *)(&d3dfn_1007e2c0) };   // 0x10089a40  0x20  (name)
void * g_vtbl_10089a60[12] = { (void *)(&d3dfn_1001d240), (void *)(&d3dfn_1001c760), (void *)(&d3dfn_1007f7b0), (void *)(&d3dfn_1007f790), (void *)(&d3dfn_1007f770), (void *)(&d3dfn_1007f750), (void *)(&d3dfn_1007f730), (void *)(&d3dfn_1001db50), (void *)(&d3dfn_1007f670), (void *)(&d3dfn_1007f5a0), (void *)(&d3dfn_1007f560), (void *)(&d3dfn_1001cf40) };   // 0x10089a60  0x30  (name; +1 alias)
void * g_vtbl_10089a90[12] = { (void *)(&d3dfn_1001d240), (void *)(&d3dfn_1001c760), (void *)(&d3dfn_1005a350), (void *)(&d3dfn_1005a310), (void *)(&d3dfn_1007ee20), (void *)(&d3dfn_1007ee00), (void *)(&d3dfn_1007ede0), (void *)(&d3dfn_1001db50), (void *)(&d3dfn_1007ed00), (void *)(&d3dfn_1007ec20), (void *)(&d3dfn_1005a0b0), (void *)(&d3dfn_1001cf40) };   // 0x10089a90  0x30  (name; +1 alias)
float g_f1008ab94 = 0.0f;   // 0x1008ab94  0x4  (name)
float g_f1008ac34 = 0.0f;   // 0x1008ac34  0x4  (name)
float g_f1008ac54 = 0.0f;   // 0x1008ac54  0x4  (name)
float g_f1008ac74 = 0.0f;   // 0x1008ac74  0x4  (name)
struct Elem24024 * g_1008ad90 = 0;   // 0x1008ad90  0x4  (name)
float g_1008ad94 = 0.0f;   // 0x1008ad94  0x4  (name; +1 alias)
float g_1008ad98 = 0.0f;   // 0x1008ad98  0x4  (name; +1 alias)
unsigned long g_1008ad9c = 0;   // 0x1008ad9c  0x4  (name; +1 alias)
unsigned long g_1008ada0 = 0;   // 0x1008ada0  0x4  (name; +1 alias)
unsigned long g_n1008adb0 = 0;   // 0x1008adb0  0x4  (name)
float g_f1008adb4 = 0.0f;   // 0x1008adb4  0x4  (name)
float g_f1008adb8 = 0.0f;   // 0x1008adb8  0x4  (name)
float g_f1008adbc = 0.0f;   // 0x1008adbc  0x4  (name)
float g_f1008adc0 = 0.0f;   // 0x1008adc0  0x4  (name)
float g_f1008adc4 = 0.0f;   // 0x1008adc4  0x4  (name)
float g_f1008adc8 = 0.0f;   // 0x1008adc8  0x4  (name)
float g_f1008adcc = 0.0f;   // 0x1008adcc  0x4  (name)
float g_1008add4 = 0.0f;   // 0x1008add4  0x4  (name)
float g_1008add8 = 0.0f;   // 0x1008add8  0x4  (name)
float g_1008addc = 0.0f;   // 0x1008addc  0x4  (name)
struct CVarR5C g_1008ade0 = { { 0 } };   // 0x1008ade0  0x20  (name; +1 alias)
struct CVarR5C g_1008ae00 = { { 0 } };   // 0x1008ae00  0x20  (name; +1 alias)
struct CVarR5C g_1008ae20 = { { 0 } };   // 0x1008ae20  0x20  (name; +1 alias)
struct CVarR5C g_1008ae40 = { { 0 } };   // 0x1008ae40  0x20  (name; +1 alias)
struct CVarR5C g_1008ae60 = { { 0 } };   // 0x1008ae60  0x20  (name)
struct CVarR5C g_1008ae80 = { { 0 } };   // 0x1008ae80  0x20  (comment/name; +2 aliases)
struct CVarR5C g_1008aea0 = { { 0 } };   // 0x1008aea0  0x20  (name; +1 alias)
struct Bucket123a0 * g_1008aec0 = 0;   // 0x1008aec0  0x4  (name)
unsigned short g_w1008aed0 = 0;   // 0x1008aed0  0x2  (name)
struct Bank10021f40 g_1008aed4 = { { 0 } };   // 0x1008aed4  0x20  (name; +1 alias)
struct Bank10021f40 g_1008aef4 = { { 0 } };   // 0x1008aef4  0x20  (name; +1 alias)
struct Bank10021f40 g_1008af14 = { { 0 } };   // 0x1008af14  0x20  (name; +1 alias)
struct Bank10021f40 g_1008af34 = { { 0 } };   // 0x1008af34  0x20  (name)
struct Bank10021f40 g_1008af54 = { { 0 } };   // 0x1008af54  0x20  (name; +1 alias)
struct Bank10021f40 g_1008af74 = { { 0 } };   // 0x1008af74  0x20  (name)
struct Bank10021f40 g_1008af94 = { { 0 } };   // 0x1008af94  0x20  (name; +1 alias)
struct Bank10021f40 g_1008afb4 = { { 0 } };   // 0x1008afb4  0x20  (name)
struct Bank10021f40 g_1008afd4 = { { 0 } };   // 0x1008afd4  0x20  (name)
struct Bank10021f40 g_1008aff4 = { { 0 } };   // 0x1008aff4  0x20  (name)

// ======================================================================= //
//  .data1, initialised
// ======================================================================= //
unsigned char g_100a9810[16] = { 0 };   // 0x100a9810  0x10  (name)
extern char const g_sz100a9f70[56] = "Find: %d, BSP: %d, Setup: %d, Draw: %d, Total: %d ticks";   // 0x100a9f70  0x38  (name)
