// =====================================================================
//  w8_454000.cpp  --  cluster 0x00454000..0x00458000
//
//  This file carries ONE function: the netmgr packet tracer that the
//  enumeration lists at 0x00456400.  Everything else listed for
//  this region is already written; see the notes at the bottom of this
//  file for the evidence on each.
//
//  The tracer belongs in src/engine/server/netmgr.cpp -- it is a shared-TU
//  function, `static`/file-scope and __cdecl, and its server.dll twin is
//  sub_10021cbe (2,823 bytes, `todo`).  It is
//  reproduced here standalone so it can be measured without touching the
//  repo; the view classes below are copies of the ones netmgr.cpp already
//  declares, so on integration they come out of the file and the function
//  drops in unchanged.
//
//  Suffix for every new symbol: Q454.
// =====================================================================

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <new.h>
#include "servermgr.h"
#include "servermsg.h"
#include "packet.h"
#include "de_error.h"


// ---------------------------------------------------------------------
//  Views netmgr.cpp already carries (netmgr.cpp:1524-1545).  Repeated
//  here only so this file compiles alone; delete on integration.
// ---------------------------------------------------------------------
class ServerMessageTypeViewQ454
{
public:
	// KEPT, and deliberately OUT OF CLASS. A non-template member declared
	// out-of-class is the one construct /Ob1 never expands, and the first
	// seven reads of the tracer need exactly that -- see the note below.
	uint16 ReadWord(uint16* pDummy);         // 0x0042f950 / 0x1000ebd0
	uint32 ReadDWord(uint32* pDummy);        // 0x0042f9d0 / 0x100240f8
	//
	// The template route, used by the last nine sites, was ReadWord / They were declarations of
	// CPacket::ReadType<uint16> (0x0042f950) and ReadType<uint32>
	// (0x0042f9d0) under other names, and a non-template member declared
	// OUT OF CLASS is the one construct /Ob1 never expands -- which cost
	// this function 964 bytes across nine sites the shipped build inlines.
	// Every read now goes through packet.h's member template, which is
	// defined in the class body and is an expansion candidate with or
	// without the keyword, and VC6 picks the split itself.
	// The length is a *uint16*.  0x0045c2d0 masks its second argument with
	// `and $0xffff,%ecx` before using it, and 0x100260d9 does the same with
	// `movzwl %di,%ebx` -- and every call site in the tracer pushes the raw
	// dword its 16-bit value sits in (0x004567d2 `push %edi` straight off a
	// `movzbw`), which a uint32 parameter would have had to widen first.
	void   ReadData(void* pData, uint16 nBytes);
};


// ---------------------------------------------------------------------
//  0x00456400 (lithtech.exe, /O2) = 0x10021cbe (server.dll, /O1)
//
//  WHAT THE ROW IS.  The function enumeration says 5,176 bytes and
//  names it CPacket::ReadType; both are wrong.  There are
//  fourteen branch targets outside the range and a last instruction that
//  overruns, and the reason is the two switch tables: the function body is
//  0x00456400..0x00457731 (`ret`) plus a two-byte `mov %edi,%edi` pad, i.e.
//  4,916 bytes, and 0x00457734..0x0045783d is 260 bytes of jump tables --
//
//      0x00457734  28 dwords   the message-id switch targets
//      0x004577a4  0x39 bytes  its index table (ids 0x00..0x38)
//      0x004577e0   4 dwords   the internal-packet sub-type switch
//      0x004577f0  18 dwords   the MSG/PIC sub-type switch
//      0x00457834  18 bytes    its index table
//
//  It is ONE function: a sweep of every e8/e9 in .text for a displacement
//  landing inside 0x00456401..0x00457734 returns 49 hits and every one is
//  an internal jump of its own (to the shared strcat block at 0x00457686,
//  the shared sprintf+strcat at 0x00457298/0x0045729d, the BAD arm at
//  0x00457563, the loop head at 0x00457001 and the print tail at
//  0x004576ab).  The only calls INTO it from outside are to its entry:
//  0x004578aa (CNetMgrSendView, the send path) and three tail-recursive
//  calls of its own at 0x004567ed, 0x004569f5 and 0x0045723b.
//
//  WHAT IT DOES.  It renders a packet as one line of text and prints it to
//  the console when the trace variable allows.  Layout of the line:
//
//      <G|g|U|u> <packet name> [<decoded sub-packets> ...]
//
//  upper case when the caller says the packet is outgoing, lower when not,
//  and G/g when the packet is guaranteed (payload byte 0 bit 7).  The
//  caller at 0x0045789d only calls it when g_NetLogPackets is nonzero.
//
//  The argument shapes are read off the frame:  arg1 at S+4 is the packet
//  (`mov 0x2138(%esp),%ebp` after two pushes), arg2 at S+8 is the byte
//  count (`mov 0x2144(%esp),%ebx`), arg3 at S+0xc is a BYTE (`mov
//  0x2148(%esp),%al ; test %al,%al`, and `cmpb $0x0,0x10(%ebp)` in
//  server.dll).  Caller-cleaned (`add $0xc,%esp` at 0x004578af), so
//  __cdecl, so file-scope.
//
//  RECURSION.  Three of the cases carve sub-packets out of the payload,
//  copy each into a fresh ServerMessage off msg_AllocPacket and call this
//  function again on it with bOut = 0.  All three use servermsg.h's
//  CMessageRef, whose Init the /O2 build expands (0x00456741
//  `mov (%eax),%esi ; test ; call *0x4(%eax)` is the body with the
//  freshly-constructed null propagated through the first guard) and whose
//  destructor is the `if (p) p->Release()` at 0x004567f5.  ServerMessage::
//  Init is expanded too (0x0045675e..0x004567cf), which is what the
//  header's inline already promises.
//
//  TWO LENGTH TESTS, TWO SIGNEDNESSES.  Every read is preceded by the
//  same pair of guards, and they are NOT the unsigned pair sub_100235e8
//  uses:  `cmp %edi,%ebx ; jb` is uint32 nLen against the count, and
//  `movzwl 0x34 ; movzwl 0x38 ; sub ; cmp ; jl` is the *int* difference of
//  two uint16 members -- the default integral promotions, not a cast.
//
//  ODDITY KEPT: the GRP loop never subtracts the sub-packet length from
//  nLen (no `sub` of it anywhere between 0x004566e6 and 0x00456800) while
//  the UPD and UUP loops both do (0x004569d6 / 0x0045714x).  That is in
//  the shipped bytes of both builds and is source, not scheduling.
// ---------------------------------------------------------------------
// NOT `static` here, though the shipped function is file-scope: with no
// caller in this probe TU VC6 drops a static function entirely (the object
// came out 236 bytes with no COMDAT at all). Linkage does not change the
// body, and byte matching masks the relocation, so external linkage is the
// measurable spelling. Restore `static` when this lands in a TU that calls it.
void net_TracePacketQ454(ServerMessage* pMsg, uint32 nLen, uint8 bOut)
{
	char         temp[0x100];
	char         str[0x2000];
	uint32       nFlags;
	uint32       nCount;
	uint32       nSubLen;
	uint16       nSubWord;
	uint8        nSubByte;
	uint16       nFlagWord;
	uint8        nFlagByte;
	uint32       nFlagLong;
	uint8        nMsgClass;

	memset(str, 0, sizeof(str));

	// 0x00456428 `testb $0x80,(%edx)` -- payload byte 0, bit 7.
	if (pMsg->m_pData[0] & 0x80)
	{
		strcat(str, bOut ? "G " : "g ");
		nLen -= 4;
	}
	else
	{
		strcat(str, bOut ? "U " : "u ");
	}

	// 0x004564bb -- the payload byte is re-read, not kept from above.
	switch (pMsg->m_pData[0] & 0x3f)
	{
		case 0:
			strcat(str, "TCP ");
			break;

		// ------------------------------------------------------------
		//  1 -- the engine's own packet.  Sub-type in the next byte.
		// ------------------------------------------------------------
		case 1:
			strcat(str, "NM  ");

			if (nLen < 1)
				goto bad;
			if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
				goto bad;

			nLen--;

			switch (pMsg->ReadByte(0))
			{
				case 0:
					strcat(str, "ACK ");

					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					nLen--;

					if (pMsg->ReadByte(0))
						strcat(str, "PRQ ");

					if (nLen < 8)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 8)
						goto bad;

					nLen -= 8;
					nCount = 0;

					((ServerMessageTypeViewQ454*)pMsg)->ReadDWord(0);
					((ServerMessageTypeViewQ454*)pMsg)->ReadDWord(0);

					// 0x0045662a: the peeled guard jumps straight to the
					// print tail -- VC6 threads it through the `if (nCount)`
					// below, which it knows is still zero on that edge.
					while (pMsg->m_Unknown34 - pMsg->m_Unknown38 > 4)
					{
						if (nLen <= 4)
							break;

						nCount++;
						nLen -= 4;

						((ServerMessageTypeViewQ454*)pMsg)->ReadDWord(0);
					}

					if (nCount)
					{
						sprintf(temp, "REQ %d", nCount);
						strcat(str, temp);
					}

					goto done;

				case 1:
					strcat(str, "DIS ");
					break;

				case 2:
					strcat(str, "PNG ");
					break;

				// --------------------------------------------------
				//  3 -- a group packet: a run of length-prefixed
				//  sub-packets, each traced on its own.
				// --------------------------------------------------
				case 3:
					strcat(str, "GRP ");

					nCount = 0;

					while (nLen >= 2)
					{
						// `< 2`, NOT `== 0`. Both images: server 0x10021df3
						// and lithtech 0x004566d9 are `cmp $0x2,%eax ; jl`.
						// The UPD/UUP loops genuinely ARE `== 0` (`je` at
						// 0x00456a01 etc.), so this is not a blanket change.
						if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 2)
							break;

						nLen -= 2;

						nSubWord = ((ServerMessageTypeViewQ454*)pMsg)->ReadWord(0);

						if (nSubWord)
						{
							nSubLen = nSubWord;

							if (nLen < nSubLen)
							{
								strcat(str, "BAD ");
								break;
							}
							if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < (int)nSubLen)
							{
								strcat(str, "BAD ");
								break;
							}

							nCount++;

							{
								CMessageRef  ref;
								CAbstractIO* pIO;

								ref.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
								if (pIO)
									pIO->Release();

								ref.m_pMsg->Init(nSubWord, 0x1000);

								((ServerMessageTypeViewQ454*)pMsg)->ReadData(
									ref.m_pMsg->m_pData, nSubWord);

								ref.m_pMsg->m_Unknown34 = nSubWord;
								ref.m_pMsg->m_Unknown38 = 1;

								net_TracePacketQ454(ref.m_pMsg, nSubLen, 0);
							}
						}
					}

					sprintf(temp, "%d ", nCount);
					strcat(str, temp);
					goto done;

				default:
					strcat(str, "UNK ");
					break;
			}
			break;

		case 4:
			strcat(str, "NPV ");
			break;

		case 5:
			strcat(str, "ULW ");
			break;

		case 6:
			strcat(str, "LW  ");
			break;

		case 7:
			strcat(str, "COI ");
			break;

		// ------------------------------------------------------------
		//  8 -- the object update packet.  A run of byte-length-prefixed
		//  sub-messages first, then the update records themselves.
		// ------------------------------------------------------------
		case 8:
			strcat(str, "UPD ");

			nLen--;

			while (nLen)
			{
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 == 0)
					break;

				nLen--;

				nSubByte = pMsg->ReadByte(0);

				if (nSubByte == 0)
					break;

				nSubLen = nSubByte;

				if (nLen < nSubLen)
				{
					strcat(str, "BAD ");
					break;
				}
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < (int)nSubLen)
				{
					strcat(str, "BAD ");
					break;
				}

				{
					CMessageRef  ref;
					CAbstractIO* pIO;

					ref.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
					if (pIO)
						pIO->Release();

					ref.m_pMsg->Init(nSubByte, 0x1000);

					nLen -= nSubLen;

					((ServerMessageTypeViewQ454*)pMsg)->ReadData(
						ref.m_pMsg->m_pData, nSubByte);

					ref.m_pMsg->m_Unknown34 = nSubByte;
					ref.m_pMsg->m_Unknown38 = 1;

					net_TracePacketQ454(ref.m_pMsg, nSubLen, 0);
				}
			}

			// The record loop.  Every `continue` in it jumps back to this
			// test (0x00457001 in lithtech, 0x100220d6 in server.dll);
			// there is no duplicated bottom test.
			while (pMsg->m_Unknown34 - pMsg->m_Unknown38 > 0 && nLen > 0)
			{
				// THE GUARD IS NOT NEGATED. Both images print when bit 2
				// is SET: lithtech 0x00456a61 is
				// `test BYTE PTR ds:0x4b32c8,0x4 ; je` PAST the block.
				// It reads correctly too -- bit 2 means "expand UPD", and
				// the tail filter drops the line unless the same bit is set.
				if (g_NetLogPackets & 4)
				{
					con_Printf(str);
					strcpy(str, "      ");
				}

				nLen--;

				nFlags = ((CPacket*)pMsg)->ReadType((uint8*)0);

				if (nFlags & 0x80)
				{
					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					nLen--;

					nFlags |= (uint32)((CPacket*)pMsg)->ReadType((uint8*)0) << 8;
				}

				if (nFlags == 0)
				{
					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					nLen--;

					switch (((CPacket*)pMsg)->ReadType((uint8*)0))
					{
						case 0:
							sprintf(temp, "SND (!%d)", nLen);
							break;
						case 1:
							sprintf(temp, "STK (!%d)", nLen);
							break;
						case 3:
							sprintf(temp, "REM (!%d)", nLen);
							break;
						default:
							sprintf(temp, "UNK (!%d)", nLen);
							break;
					}

					strcat(str, temp);

					nLen = 0;
					continue;
				}

				if (nLen < 2)
					goto bad;
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 2)
					goto bad;

				nLen -= 2;

				sprintf(temp, "%08x ",
				        ((CPacket*)pMsg)->ReadType((uint16*)0));
				strcat(str, temp);

				if (nFlags & 1)
				{
					strcat(str, "NEW ");
					continue;
				}

				if (nFlags & 0x20)
				{
					sprintf(temp, "MI  (!%d)", nLen);
					strcat(str, temp);
					goto done;
				}

				if (nFlags & 8)
				{
					strcat(str, "FLG ");

					if (nLen < 7)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 7)
						goto bad;

					nLen -= 7;

					nFlagWord = ((CPacket*)pMsg)->ReadType((uint16*)0);
					nFlagByte = ((CPacket*)pMsg)->ReadType((uint8*)0);
					nFlagLong = ((CPacket*)pMsg)->ReadType((uint32*)0);

					sprintf(temp, "%04hx %02hx %08x ",
					        nFlagWord, nFlagByte, nFlagLong);
					strcat(str, temp);
				}

				if (nFlags & 0x40)
				{
					sprintf(temp, "CLR (!%d)", nLen);
					strcat(str, temp);
					goto done;
				}

				if (nFlags & 0x10)
				{
					sprintf(temp, "SCL (!%d)", nLen);
					strcat(str, temp);
					goto done;
				}

				if (nFlags & 0x202)
				{
					sprintf(temp, "POS (!%d)", nLen);
					strcat(str, temp);
					goto done;
				}

				if (nFlags & 0x404)
				{
					sprintf(temp, "ROT (!%d)", nLen);
					strcat(str, temp);
					goto done;
				}

				if (nFlags & 0x100)
				{
					sprintf(temp, "ATT (!%d)", nLen);
					strcat(str, temp);
					goto done;
				}
			}

			goto done;

		// ------------------------------------------------------------
		//  0x0a -- the unguaranteed update packet.  The same sub-packet
		//  loop as UPD, and then just the leftover count.
		// ------------------------------------------------------------
		case 0x0a:
			strcat(str, "UUP ");

			nLen--;

			while (nLen)
			{
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 == 0)
					break;

				nLen--;

				nSubByte = ((CPacket*)pMsg)->ReadType((uint8*)0);

				if (nSubByte == 0)
					break;

				nSubLen = nSubByte;

				if (nLen < nSubLen)
				{
					strcat(str, "BAD ");
					break;
				}
				if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < (int)nSubLen)
				{
					strcat(str, "BAD ");
					break;
				}

				{
					CMessageRef  ref;
					CAbstractIO* pIO;

					ref.Init(msg_AllocPacket(&pIO, 0x1000, 0x1000));
					if (pIO)
						pIO->Release();

					ref.m_pMsg->Init(nSubByte, 0x1000);

					nLen -= nSubLen;

					((ServerMessageTypeViewQ454*)pMsg)->ReadData(
						ref.m_pMsg->m_pData, nSubByte);

					ref.m_pMsg->m_Unknown34 = nSubByte;
					ref.m_pMsg->m_Unknown38 = 1;

					net_TracePacketQ454(ref.m_pMsg, nSubLen, 0);
				}
			}

			sprintf(temp, "(!%d) ", nLen);
			strcat(str, temp);
			goto done;

		case 0x0c:
			strcat(str, "YID ");
			break;

		// ------------------------------------------------------------
		//  0x0d -- a game message.  The LAST payload byte is the class,
		//  and for class 'g' the next byte read is the sub-type.
		// ------------------------------------------------------------
		case 0x0d:
			strcat(str, "MSG ");

			if (nLen < 1)
				goto bad;
			if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
				goto bad;

			nLen--;

			// 0x004572f4: `mov 0x54(%ebp),%eax ; dec %ebx ;
			// movzbl (%ebx,%eax,1),%eax` -- indexed with the DECREMENTED
			// count, so it is the last unread byte.
			nMsgClass = pMsg->m_pData[nLen];

			switch (nMsgClass)
			{
				case 0x67:
					strcat(str, "PIC ");

					if (nLen < 1)
						goto bad;
					if (pMsg->m_Unknown34 - pMsg->m_Unknown38 < 1)
						goto bad;

					// No decrement here -- the shipped code reads the
					// sub-type without touching nLen.
					switch (((CPacket*)pMsg)->ReadType((uint8*)0))
					{
						case 1:  strcat(str, "AMO "); break;
						case 2:  strcat(str, "HEA "); break;
						case 3:  strcat(str, "ARM "); break;
						case 4:  strcat(str, "WEA "); break;
						case 5:  strcat(str, "WPI "); break;
						case 6:  strcat(str, "AIR "); break;
						case 7:  strcat(str, "ROC "); break;
						case 8:  strcat(str, "OUT "); break;
						case 9:  strcat(str, "OBJ "); break;
						case 10: strcat(str, "MOD "); break;
						case 11: strcat(str, "FAD "); break;
						case 12: strcat(str, "INV "); break;
						case 13: strcat(str, "MTX "); break;
						case 14: strcat(str, "MFA "); break;
						case 15: strcat(str, "MXH "); break;
						case 16: strcat(str, "MXA "); break;
						case 17: strcat(str, "WOI "); break;
						case 18: strcat(str, "INT "); break;
						default: strcat(str, "UNK "); break;
					}
					break;

				case 0xd4:
					strcat(str, "PHU ");
					break;

				default:
					sprintf(temp, "%2x LEN %d ", nMsgClass, nLen);
					strcat(str, temp);
					goto done;
			}
			break;

		case 0x0e:
			strcat(str, "GRP ");
			break;

		case 0x0f:
			strcat(str, "COF ");
			break;

		case 0x10:
			strcat(str, "CV  ");
			break;

		case 0x11:
			strcat(str, "SKY ");
			break;

		case 0x12:
			strcat(str, "ISE ");
			break;

		case 0x13:
			strcat(str, "PF  ");
			break;

		case 0x15:
			strcat(str, "PLL ");
			break;

		case 0x17:
			strcat(str, "THL ");
			break;

		case 0x18:
			strcat(str, "UL  ");
			break;

		case 0x19:
			strcat(str, "LMA ");
			break;

		case 0x1a:
			strcat(str, "GL  ");
			break;

		case 0x32:
			strcat(str, "FD  ");
			break;

		case 0x33:
			strcat(str, "ST  ");
			break;

		case 0x34:
			strcat(str, "CFT ");
			break;

		case 0x35:
			strcat(str, "FB  ");
			break;

		case 0x36:
			strcat(str, "FS  ");
			break;

		case 0x38:
			strcat(str, "GYS ");
			break;

		default:
			strcat(str, "UNK ");
			break;
	}

	goto done;

bad:
	strcat(str, "BAD ");

done:
	// Three sequential filters over the name that was just built, each one
	// skipped when its bit is set in the trace variable.  &str[2] steps
	// past the direction pair.
	if (!(g_NetLogPackets & 2) && strncmp(&str[2], "UUP", 3) == 0)
		return;

	if (!(g_NetLogPackets & 4) && strncmp(&str[2], "UPD", 3) == 0)
		return;

	if (!(g_NetLogPackets & 8) && strncmp(&str[2], "NM", 2) == 0)
		return;

	con_Printf(str);
}


// =====================================================================
//  THE OTHER FOUR ROWS IN THIS CLUSTER ARE ALREADY WRITTEN
//
//  All four are netmgr/servermsg functions in SHARED translation units.
//  Shared TUs are annotated with SERVER addresses (0x100xxxxx), so a
//  lithtech address never grep-matches them and an address scan plus a
//  status filter cannot see the coverage.  Nothing else is going on: each
//  one is findable in seconds by NAME.
//
//  0x004551c0 / 253 b = NetConnection::Term
//      src/engine/server/netmgr.cpp:786, written against server 0x1002126a.
//      Blocked on CNetRecvPacket::Term, which servermgr.h declares and
//      nothing defines.
//
//  0x004561e0 / 315 b = CNetMgr::RemoveDriver
//      src/engine/server/netmgr.cpp:882.  netmgr.cpp:2853 already records
//      that lithtech.exe expands collection.h's Remove2 whole into it at
//      this exact address.  Blocked on collection.h's BaseNew.
//
//  0x00457c40 / 215 b = ServerMessage::ServerMessage()
//      src/engine/server/servermsg.cpp:434, written against server
//      0x1002417f (62 b, `identical`).  The
//      lithtech row is the same constructor with ServerMessage::Init and
//      the CMsgPacketData construction EXPANDED into it, which is why it
//      is 215 b rather than 62.  Proof of identity: it stores the base's
//      vtable 0x004a2b44 and the refcount, the LMessageImpl vtable
//      0x004a2b68 at +0x08, the packet data's vptr 0x004a1ab8 at +0x50,
//      then the derived vtable 0x004a2b58 -- whose slot 3 is 0x0045c1d0,
//      which servermsg.cpp already names as ServerMessage's own _v0c --
//      and finishes with Init(0x1000, 0x1000) inline.  Its one caller is
//      0x00457af7, inside CNetMgrSendView::SendPacket (0x00457ab0), where
//      CNetBankAllocView::Allocate has been inlined; that is netmgr.cpp's
//      `pRec->m_Packet.ServerMessage::ServerMessage()`.
//
//      NOW COVERED: the CServerMessageQ18 view at the end of this file
//      is byte-identical at 0x00457c40, and its ??_G at 0x00457d20.
//      The original note follows.
//
//      WHY IT IS NOT COVERED, AND WHAT WOULD COVER IT: the constructor is
//      `inline` in servermsg.cpp, so netmgr.obj cannot see a body and
//      cannot emit the COMDAT.  Moving the body into servermsg.h, below
//      ServerMessage::Init (VC6's inliner is forward-only), is what would
//      let netmgr.obj emit it.  That needs CMsgPacketData's constructor
//      visible and inline too -- 0x00457c61..0x00457ca5 is its five-word
//      construction expanded, with one out-of-line SetSize call at
//      0x00457c75 and a second SetSize expanded after it -- so it is a
//      header job with a cross-TU blast radius, not a body to write.
//
//  0x00457e40 / 481 b = CNetMgrSendView::AddConnection
//      src/engine/server/netmgr.cpp:2104, written against server
//      0x10022ade.  Blocked on collection.h's BaseNew.
//
//  Two neighbours worth recording while the region was open, both real
//  remaining work and neither written here:
//      0x00457d20 /  90 b  ServerMessage's scalar deleting destructor
//                          (slot 0 of 0x004a2b58; frees m_pData through
//                          0x004152b0 with (m_DataSize + m_DataSpare) as
//                          the size, then runs the base's at 0x00455c30).
//      0x00457840 / 531 b  the send-queue pump that calls the tracer at
//                          0x004578aa; it carries "Resolving guaranteed
//                          packet %d" and is netmgr.cpp's, unwritten.
// =====================================================================


// =====================================================================
//  0x00457d20 (lithtech.exe, /O2) -- ServerMessage's scalar deleting
//  destructor, slot 0 of its vftable 0x004a2b58.
//
//  Compiler-generated, so the source is the CLASS SHAPE, not a body: a
//  base with a virtual destructor (CAbstractIO, vftable 0x004a2b44,
//  refcount at +0x04), no user destructor on the derived class -- which
//  is why the shipped bytes carry no store of 0x004a2b58 before the
//  teardown -- and the +0x50 member that msgterm.cpp already proved is
//  CMoArray<uint8, DefaultCache> (vftable 0x004a1ab8, SetSize2(0,
//  &g_DefAlloc) expanded around the out-of-line free at 0x004152b0).
//  The tail is `if (flags & 1) operator delete(this)` -- 0x004290c0.
//
//  It is the same body as 0x00455bc0 (~ServerMessage, msgterm.cpp) with
//  the delete appended.  servermsg.h's ServerMessage has no destructor,
//  and giving it one is a six-TU header change, so this view is what
//  asks for the COMDAT.  On integration: `virtual ~CAbstractIO()` in the
//  base and the CMoArray member on ServerMessage, and this can go.
//
//  The out-of-line constructor emits the vftable, which is what makes VC6
//  emit the ??_G it points at -- and it is 0x00457c40 itself, below.
// =====================================================================
class CAbstractIOQ18
{
public:
	CAbstractIOQ18() { m_nRefCount = 0; }
	virtual ~CAbstractIOQ18() {}

	uint32 m_nRefCount;                          // +0x04
};

// +0x08 .. +0x34 -- the embedded LMessageImpl (vftable 0x004a2b68).  No
// destructor: 0x00455bc0 never touches +0x08.
class CMsgWriteQ18
{
public:
	virtual void _v00();

	uint32                   m_Unknown04;        // +0x04
	class CServerMessageQ18* m_pOwner;           // +0x08 (+0x10 in the message)
	uint8                    m_pad_0c[0x2c - 0x0c];
};

class CServerMessageQ18 : public CAbstractIOQ18
{
public:
	CServerMessageQ18();                         // 0x00457c40

	inline void Init(uint16 nMaxDataLen, uint16 nGrowSize);

	CMsgWriteQ18                  m_Write;       // +0x08
	uint16                        m_Unknown34;   // +0x34
	uint16                        m_MaxDataLen;  // +0x36
	uint16                        m_Unknown38;   // +0x38
	uint16                        m_Unknown3a;   // +0x3a
	void*                         m_pConnection; // +0x3c
	uint32                        m_MsgFlags;    // +0x40
	uint8                         m_pad_44[0x50 - 0x44];
	CMoArray<uint8, DefaultCache> m_PacketData;  // +0x50 .. +0x64
};

// servermsg.h's ServerMessage::Init, against the CMoArray member.  Three
// spellings here differ from the server.dll source and each was measured
// on 0x00457c40 (/O2):
//   * m_MsgFlags = 0 LAST of the scalar stores: shipped stores +0x40
//     after +0x3a (0x00457cc8).  First, it lands before +0x34.
//   * the bound through GetSize(): shipped compares straight against
//     memory, `cmp %ebp,0x58(%edi)` (0x00457cce).  The bare m_nElements
//     and the server's volatile read both load it into %eax first.
//   * SetSize(0), the one-argument wrapper, not SetSize2(0, &g_DefAlloc):
//     the extra inline level is what leaves it a CALL (0x00457cdb) while
//     Init's own Term() and _InitArray expand as shipped.  SetSize2
//     written directly expanded it and left _InitArray called (248 b).
// The constructor's own `m_MsgFlags = 0; m_Unknown3a = 1;` (servermsg.cpp)
// are dead under Init and made no difference; dropped.
inline void CServerMessageQ18::Init(uint16 nMaxDataLen, uint16 nGrowSize)
{
	m_Unknown34  = 1;
	m_MaxDataLen = nMaxDataLen;
	m_Unknown38  = 1;
	m_Unknown3a  = 1;
	m_MsgFlags   = 0;

	m_Write.m_pOwner = this;

	if (nMaxDataLen > m_PacketData.GetSize())
	{
		m_PacketData.SetSize(0);
		m_PacketData.Init(nMaxDataLen, nGrowSize);
	}
	else
	{
		m_PacketData.SetCacheSize(nGrowSize);
	}
}

// 0x00457c40 -- servermsg.cpp's ServerMessage::ServerMessage (server
// 0x1002417f) with every constructor and Init expanded: base vptr and
// refcount, the LMessageImpl vptr at +0x08, CMoArray() at +0x50 (Clear,
// Term called, SetSize(0) expanded), the derived vptr, then Init.
// IDENTICAL, 215 b.
CServerMessageQ18::CServerMessageQ18()
{
	Init(0x1000, 0x1000);
}


// ---------------------------------------------------------------------
//  0x0045a210 (36 bytes) -- lthread.h's CMessageQueueBase::InsertBefore,
//  out of line.  Not a function of this file: a driver for one.
//
//  The shipped image has the out-of-line copy because ONE caller declined
//  the expansion: netmgr.cpp's SendPacketData (0x004582f0) expands
//  Prepend but calls both of its halves, `call 0x0045a210` for the splice
//  and `call 0x0045a1c0` for Append, at 0x004583f9.  Our cl 8447 has one
//  unit of inline budget less there than the shipped compiler (the
//  finding collection.cpp records for 0x1001a6dd) and expands the splice,
//  so nothing of ours emits the COMDAT, and InsertBefore cannot move out
//  of the class: server.dll's Prepend (0x10015ad0) expands it.  A
//  pointer-to-member needs a real function to point at, so taking the
//  address emits the body the image carries -- sdkforce.cpp's lever, as
//  lt2new1.cpp uses it for three ltvector.h/ltrotation.h inlines.  It is
//  here, in a client-only TU, so that server.dll does not gain it.
//  IDENTICAL, 36 bytes.
// ---------------------------------------------------------------------
LThreadMessage* (CMessageQueueBase::*g_pfnQueueInsertBefore)(LThreadMessage*, LThreadMessage*) =
	&CMessageQueueBase::InsertBefore;
