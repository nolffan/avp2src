// *********************************************************************** //
//
//  MODULE:   LITHTECH (.CPP)
//
//  PURPOSE:  lithtech.exe entry point — the engine core.
//
//  SCAFFOLD: lithtech.exe exports NOTHING (it is the executable, not a DLL),
//            so unlike server.dll / d3d.ren there is no export table to anchor
//            correct names against. Its ~3000 functions are symbol-less; each
//            is reconstructed as a `sub_<rva>` and
//            renamed once its role is understood.
//
//            What IS known up front:
//            - the entry point is WinMain (a windowed app),
//            - the engine populates the public interface structs the game DLLs
//              call through — ILTClient, ILTServer, ILTCommon, ILTMessage,
//              ILTModel, ILTPhysics, ... (src/LT2/sdk/inc/*.h). Those structs are
//              tables of function pointers with known signatures; the engine's
//              internal functions that fill them therefore have known
//              signatures even though their names are not in the binary.
//            - it loads d3d.ren (RenderDLLSetup) and server.dll (CreateServer),
//              and Miles / Bink as middleware.
//
//            Start from the smallest leaf functions
//            and work up. This file holds WinMain and grows from there.
//
// *********************************************************************** //

#include <windows.h>


// The entry point is NOT here any more.  WinMain was reconstructed in
// clientwin.cpp (0x00403e30, the client's Windows entry layer, beside
// RunClientApp).  The empty body this scaffold used to carry was what the
// whole-exe link bound the CRT's WinMainCRTStartup to, so /OPT:REF threw away
// everything the real one reaches: the client manager, interface, sound,
// music and video, ~180 KB of .text.  This translation unit now defines
// nothing; it stays because build/engine.sh compiles it.
