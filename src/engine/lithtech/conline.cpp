// *********************************************************************** //
//
//  MODULE:   CCLINEKEYS (.CPP) -- lithtech.exe 0x0041bc40..0x0041c007
//
//  PURPOSE:  Three more methods on the CLIENT console's input line: the
//            word-wise delete, the word-wise cursor move, and the keyboard
//            dispatcher that drives both.  (reconstruction; build cl 8447
//            /O2, /I ..\..\LT2\sdk\inc /I ..\server /I .)
//
//  ------------------------------------------------------------------ //
//  HOW THE CLUSTER WAS IDENTIFIED
//  ------------------------------------------------------------------ //
//
//  From the OBJECT, not from a string.  Every one of the three is a thiscall
//  whose `this` is handed straight on to methods src/engine/lithtech/
//  clientconsole.cpp has ALREADY named on CConsoleLine, the sub-object at
//  CConsole+0x10:
//
//      0x0041bb90  CConsoleLine::Clear()              (already reconstructed)
//      0x0041bba0  CConsoleLine::SetText(char*)       (already reconstructed)
//      0x0041bd30  CConsoleLine::MoveCursor(int)      (already reconstructed)
//
//  and to three neighbours in the same run, now written below too:
//
//      0x0041bbf0  delete one character, at m_nCursor + nDelta
//      0x0041bcc0  insert one character, at m_nCursor
//      0x0041be30  ToAscii() wrapper: VK code -> character, 0 for none
//
//  The field offsets below are clientconsole.cpp's, unchanged: m_nLen at
//  +0x04, m_szText[0x100] at +0x08, m_nCursor at +0x12c.  Every access in
//  these three bodies is one of those three and nothing else.
//
//  The literal 0x004a87f0 " .()\"" is the only string in the range, and the
//  .data string pool puts it immediately after console.obj's "%s = %s"
//  (0x004a87e8) -- literal order follows function address order on this
//  project, and these functions sit immediately after console.obj's, so the
//  delimiter set belongs here.
//
//  The dispatcher 0x0041bea0 is pinned by its own jump table.  The switch
//  biases by 8 and range-checks against 0x26, so the labels run VK 0x08 to
//  VK 0x2e; the byte index table at 0x0041bfe0 (39 entries, read out of the
//  image) is
//
//      idx  0 -> 0   VK 0x08  VK_BACK
//      idx 19 -> 1   VK 0x1b  VK_ESCAPE
//      idx 27 -> 2   VK 0x23  VK_END
//      idx 28 -> 3   VK 0x24  VK_HOME
//      idx 29 -> 4   VK 0x25  VK_LEFT
//      idx 31 -> 5   VK 0x27  VK_RIGHT
//      idx 38 -> 6   VK 0x2e  VK_DELETE
//      everything else -> 7   (default)
//
//  and the jump table at 0x0041bfc0 is
//
//      0 0x0041bf46   1 0x0041bec2   2 0x0041bf1d   3 0x0041bf09
//      4 0x0041becd   5 0x0041beeb   6 0x0041bf72   7 0x0041bf95
//
//  VC6 numbers the index values in ASCENDING CASE-LABEL order but lays the
//  bodies out in SOURCE order, so the source order is the address order of
//  the bodies: ESCAPE, LEFT, RIGHT, HOME, END, BACK, DELETE, default.  That
//  is the order written below, and it is what puts the shared
//  `MoveCursor(...); return;` tail at 0x0041bfb5 inside the LAST block, with
//  LEFT's and RIGHT's else-arms cross-jumped into it.
//
//  NAMING.  Everything introduced here carries a `Q418` tag so the object
//  can sit in one build beside clientconsole.cpp and ccline.cpp, which
//  declare their own incompatible views of the same console.
//
// *********************************************************************** //

#include <windows.h>            // GetAsyncKeyState, VK_*
#include <string.h>             // strchr, strcpy, memmove

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  The word delimiters
// ----------------------------------------------------------------------- //

// 0x004a87f0, read out of the image: space, full stop, both parentheses and
// a double quote.  Both word functions push this address to strchr
// (0x00493660, named by src/lithshared/rezmgr/rezdir.cpp) with the character
// as the second argument, so it is strchr(set, c) and not the other way
// round.  (name ours)
#define CON_WORD_DELIMS_Q418   " .()\""


// ----------------------------------------------------------------------- //
//  The console's input line
// ----------------------------------------------------------------------- //

// clientconsole.cpp's CConsoleLine, re-declared under the Q418 tag.  Only
// the three fields these bodies touch are named; the rest is padding, since
// nothing here reads it.  (names ours)
class CConsoleLineQ418
{
public:
	// Already reconstructed in src/engine/lithtech/clientconsole.cpp.
	void Clear();                                      // 0x0041bb90
	void MoveCursor(int nDelta);                       // 0x0041bd30

	// The three neighbours this run calls but does not define.  DeleteChar
	// removes m_szText[m_nCursor + nDelta] and drops m_nLen; InsertChar
	// opens a hole at m_nCursor, writes the byte and raises m_nLen (it
	// reads its argument as `mov 0x8(%esp),%al`, one byte, which is why the
	// parameter is char and not int); TranslateKey is the GetKeyboardState
	// / ToAscii pair at 0x0041be30 -- it never touches `this`, but the
	// caller loads ecx with it, so it is a member.
	void DeleteChar(int nDelta);                       // 0x0041bbf0
	void InsertChar(char c);                           // 0x0041bcc0
	char TranslateKey(uint32 nKey);                    // 0x0041be30

	// This run.
	void DeleteWord(int nDelta);                       // 0x0041bc40
	void MoveCursorWord(int nDelta);                   // 0x0041bd70
	void HandleKey(uint32 nKey);                       // 0x0041bea0

	uint8 m_pad_000[0x004];
	int   m_nLen;                            // 0x004
	char  m_szText[0x100];                   // 0x008
	uint8 m_pad_108[0x12c - 0x108];
	int   m_nCursor;                         // 0x12c
};


// 0x0041bbf0 -- remove the character at m_nCursor + nDelta.
//
// The removal is the inline strcpy (`repnz scas` over the source, then
// `rep movsl` / `rep movsb`) from one past the character onto it.
void CConsoleLineQ418::DeleteChar(int nDelta)
{
	int i = m_nCursor + nDelta;

	if (i >= 0 && i < m_nLen)
	{
		strcpy(&m_szText[i], &m_szText[i + 1]);
		m_nLen--;
	}
}


// 0x0041bc40 -- delete a word in the direction of nDelta.
//
// Two passes over one loop (`mov $0x2,%ebp` ... `dec %ebp; jne`, a counted
// for with the trip count folded, not a `while (i--)`), with a flag that
// says which KIND of character this pass eats and is inverted between them.
// The flag starts at `nDelta < 0` (`xor %ebx,%ebx; test %edi,%edi;
// setl %bl`), so moving left eats the run of delimiters first and then the
// word, and moving right eats the word first and then the delimiters.
//
// The comparison against the flag is `33 c3  xor %eax,%ebx` followed by
// `je`, NOT a `cmp`.  VC6 materialises an integer comparison with CMP or
// TEST, so the xor is the source's own `^` on two 0/1 values rather than a
// `!=` the compiler rewrote.  (If a build comes out with
// `cmp %ebx,%eax` here, `!=` is the one other reading and the only edit
// needed.)
//
// `neg %eax; sbb %eax,%eax; inc %eax` on the strchr result is `== 0`, the
// unsigned-compare-to-zero idiom, so the test is written against NULL and
// not as a bare truth test.
//
// The loop is rotated on its FIRST conjunct only: `m_nCursor + nDelta >= 0`
// appears once as the entry guard at 0x0041bc56 and once as the latch at
// 0x0041bca6, while the length test and the strchr both sit at the top of
// the rotated body at 0x0041bc60.  That is exactly a three-term `&&`.
//
// DeleteChar does the removal and the cursor follows it, which is why
// nDelta == 0 (the DELETE key, below) terminates: the character under the
// cursor goes and m_nLen shrinks even though m_nCursor never moves.
void CConsoleLineQ418::DeleteWord(int nDelta)
{
	int bDelim;
	int i;

	bDelim = (nDelta < 0);

	for (i = 0; i < 2; i++)
	{
		while (m_nCursor + nDelta >= 0 &&
		       m_nCursor + nDelta < m_nLen &&
		       ((strchr(CON_WORD_DELIMS_Q418,
		                m_szText[m_nCursor + nDelta]) == 0) ^ bDelim))
		{
			DeleteChar(nDelta);

			m_nCursor += nDelta;
		}

		bDelim = !bDelim;
	}
}


// 0x0041bcc0 -- insert c at m_nCursor.
//
// A full line drops its last character first.  The shift is a CALL
// (0x004931e0) with overlapping ranges, so memmove, not the memcpy
// intrinsic; at the end of the line it is a NUL store instead.
void CConsoleLineQ418::InsertChar(char c)
{
	if (m_nCursor < 0xff)
	{
		if (m_nLen >= 0xff)
		{
			m_nLen--;
			m_szText[m_nLen] = 0;
		}

		if (m_nCursor < m_nLen)
			memmove(&m_szText[m_nCursor + 1], &m_szText[m_nCursor], m_nLen - m_nCursor + 1);
		else
			m_szText[m_nLen + 1] = 0;

		m_szText[m_nCursor] = c;
		m_nLen++;
	}
}


// 0x0041bd70 -- move the cursor a word in the direction of nDelta.
//
// The first two thirds are DeleteWord's two passes with the delete taken
// out, and with the length test relaxed from `<` to `<=` (`7f  jg` at
// 0x0041bd9e where DeleteWord has `7d  jge`) -- the cursor is allowed to
// land one past the last character, a delete is not.
//
// The third loop at 0x0041bdde is NOT a third pass of the same shape: it
// tests the character at m_nCursor, not at m_nCursor + nDelta (0x0041bdf8
// indexes with %eax, which 0x0041bde8 loaded as m_nCursor, while the bound
// test at 0x0041bdf4 uses %edx = m_nCursor + nDelta), and it tests the
// strchr result directly with `test %eax,%eax` rather than through the
// `neg/sbb/inc` inversion -- so it eats delimiters, unconditionally, with
// no flag involved.
void CConsoleLineQ418::MoveCursorWord(int nDelta)
{
	int bDelim;
	int i;

	bDelim = (nDelta < 0);

	for (i = 0; i < 2; i++)
	{
		while (m_nCursor + nDelta >= 0 &&
		       m_nCursor + nDelta <= m_nLen &&
		       ((strchr(CON_WORD_DELIMS_Q418,
		                m_szText[m_nCursor + nDelta]) == 0) ^ bDelim))
		{
			m_nCursor += nDelta;
		}

		bDelim = !bDelim;
	}

	while (m_nCursor + nDelta >= 0 &&
	       m_nCursor + nDelta <= m_nLen &&
	       strchr(CON_WORD_DELIMS_Q418, m_szText[m_nCursor]) != 0)
	{
		m_nCursor += nDelta;
	}
}


// 0x0041be30 -- VK code -> character via GetKeyboardState / ToAscii; 0 if
// the key makes none (or two).
char CConsoleLineQ418::TranslateKey(uint32 nKey)
{
	BYTE keys[256];
	char buf[2];

	if (GetKeyboardState(keys))
	{
		switch (ToAscii(nKey, 0, keys, (WORD*)buf, 0))
		{
			case 0:
				return 0;

			case 2:
				return buf[1];

			case 1:
				return buf[0];
		}
	}

	return 0;
}


// 0x0041bea0 -- one key, from the console's key handler.
//
// Every arm that asks about CONTROL does it the same way: `push $0x11;
// call *GetAsyncKeyState; test $0x80,%ah` -- bit 15 of the SHORT return,
// i.e. `& 0x8000`, not `& 0x80000000` and not `< 0`.  In LEFT, RIGHT, BACK
// and DELETE the argument to BOTH arms is pushed BEFORE the branch (e.g.
// 0x0041bed8 `push $0xffffffff` ahead of the `je` at 0x0041bedc), which is
// the scheduler hoisting a constant both sides want; it does not mean the
// call was written once.
//
// The `je` in each pair jumps AWAY to the plain arm and the ctrl arm falls
// through, so the source guard is positive: `if (ctrl) word-wise; else
// character-wise`.
//
// HOME is `MoveCursor(-m_nCursor)` (`mov 0x12c(%esi),%eax; neg %eax`) and
// the plain END is `MoveCursor(m_nLen)`.  Ctrl+END calls InsertChar(0) --
// a NUL written at the cursor, which is what the shipped bytes say even
// though it leaves m_nLen one too high.
//
// The default arm's `char` lives in the dead nKey parameter slot: the
// function has no `sub $n,%esp` at all, `mov %al,0x8(%esp)` stores it after
// nKey has been pushed for the last time, and `mov 0x8(%esp),%edx; push
// %edx` reads the whole slot back because only the low byte of an argument
// slot matters to a char parameter.  Declaring it inside the case block is
// what keeps it off a frame of its own.
void CConsoleLineQ418::HandleKey(uint32 nKey)
{
	switch (nKey)
	{
		// 0x0041bec2
		case VK_ESCAPE:
		{
			Clear();
		}
		break;

		// 0x0041becd
		case VK_LEFT:
		{
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				MoveCursorWord(-1);
			else
				MoveCursor(-1);
		}
		break;

		// 0x0041beeb
		case VK_RIGHT:
		{
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				MoveCursorWord(1);
			else
				MoveCursor(1);
		}
		break;

		// 0x0041bf09
		case VK_HOME:
		{
			MoveCursor(-m_nCursor);
		}
		break;

		// 0x0041bf1d
		case VK_END:
		{
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				InsertChar(0);
			else
				MoveCursor(m_nLen);
		}
		break;

		// 0x0041bf46
		case VK_BACK:
		{
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				DeleteWord(-1);
			}
			else
			{
				DeleteChar(-1);
				MoveCursor(-1);
			}
		}
		break;

		// 0x0041bf72
		case VK_DELETE:
		{
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				DeleteWord(0);
			else
				DeleteChar(0);
		}
		break;

		// 0x0041bf95
		default:
		{
			char c = TranslateKey(nKey);

			if (c)
			{
				InsertChar(c);
				MoveCursor(1);
			}
		}
		break;
	}
}
