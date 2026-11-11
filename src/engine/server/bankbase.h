// *********************************************************************** //
//
//  MODULE:   BANKBASE (.H)
//
//  PURPOSE:  The struct banks' common base. Moved out of servermgr.h so
//            that every bank, server and client side, derives from the
//            SAME class: the shipped engines have one base vftable for all
//            of them (Primal Hunt lithtech.exe 0x004d4514, AvP2 1.0.9.6
//            lithtech.exe 0x004a14c8, server.dll 0x1004d260), whose three
//            slots after the destructor are the CRT's _purecall, and one
//            out-of-line destructor (Primal Hunt lithtech.exe 0x004044e0,
//            the vptr restore alone), where private views of the base each
//            compiled a destructor and a table of their own.
//
// *********************************************************************** //

#ifndef __BANKBASE_H__
#define __BANKBASE_H__

// A bank hands out fixed-size records from a StructBank. The slots are
// stdlith object_bank.h's shape: one-line virtual wrappers over each bank's
// non-virtual workers (`virtual void* AllocVoid() {return Allocate();}`),
// so a holder of the base can allocate and free without knowing the record
// type. Slot 3 is Term, which CServerMgr::Term calls virtually on the
// message bank (name ours).
class CBankBase
{
public:
	virtual ~CBankBase() {}

	virtual void* AllocVoid() = 0;
	virtual void  FreeVoid(void* pRec) = 0;
	virtual void  Term() = 0;
};

#endif  // __BANKBASE_H__
