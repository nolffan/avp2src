//------------------------------------------------------------------
//
//  MODULE  : WORLDTREE.CPP
//
//  PURPOSE : server.dll's TCP query array and engine clock and its
//            ILTTransform: 0x10037b47 .. 0x10038673, which was counted
//            as CRT until the app region was corrected (the static CRT
//            starts at _ftol, 0x1003e0f0).
//
//  The range 0x10037b47 .. 0x1003a54f was once this one file, laid out in
//  address-range sections (A to F) so that functions could be written in
//  parallel. Primal Hunt's server.dll shows it is four units: three of
//  them end in <string>'s guarded initializer of
//  std::ctype<unsigned short>::id (guard 0x1008ec84, atexit of the empty
//  0x1002eaeb) -- 0x1004872c (this file's, after ILTTransform),
//  0x10048f7f (transformmaker.cpp's) and 0x100493e8 (visquery.cpp's) --
//  and one TU can give only one. Sections C to F now live in
//  transformmaker.cpp, visquery.cpp and worldtree2.cpp.
//
//------------------------------------------------------------------

#ifdef PRIMAL_HUNT
// PH: <string>'s guarded ctype initializer ends the unit (0x1004872c).
#include <string>
#endif
#include "worldbsp.h"
#include "servermgr.h"

// ==== SECTION A: 0x10037b47 .. 0x10038000 ================================
//
// NOT WORLD TREE. The whole of section A is the tail of TCPDriver's object
// (src/engine/server/tcpdriver.cpp): TCPDriver::JoinSession runs from
// 0x100375e2 for 1434 bytes to 0x10037b7c (so nothing starts between
// 0x10037b47 and 0x10037b7c), and what follows is the driver's last two
// vtable slots, tcp_Create, and the `CMoArray<TCPQueryResponse,
// DefaultCache>` instantiation the driver carries at +0x50 -- the server
// twins of src/engine/lithtech/tcpqarray.cpp, which matches the client's
// copies. Everything here belongs in tcpdriver.cpp; it is written here with
// `_A` names so that it does not collide with tcpdriver.cpp's declarations.
//
// The element is tcpdriver.cpp's TCPQueryResponse (0xa4 bytes). Server
// allocator spellings: ::operator new (0x1000d25d) and ::operator delete
// (0x10028ace), as every other release of m_pData in tcpdriver.cpp.

#include "de_error.h"

// winsock2.h cannot lead here: worldbsp.h has already pulled windows.h and
// with it winsock.h, whose htons is NOT dllimport and would assemble as a
// five-byte direct call. The shipped call goes through the import table
// (`call *0x1004d1d4`, WSOCK32 ordinal 9), so it is spelled here as a call
// through a pointer, which is the same six bytes. In tcpdriver.cpp, where
// this code belongs, it is plain htons() under winsock2.h.
extern "C" unsigned short (__stdcall* htons_A)(unsigned short);
typedef struct sockaddr_in SockAddr_A;

// 0x10035efb (tcpdriver.cpp's tcp_BuildSockaddrFromString).
int tcp_BuildSockaddrFromString_A(char* pString, SockAddr_A* pAddr);

class TCPQueryResponse_A
{
public:
	TCPQueryResponse_A();                    // 0x1003654a (tcpdriver.cpp)

	// GenGetNext / GenGetAt store 0 to the result's +0x98 and then call
	// operator= (0x10037c6e, 0x10037cf5): this, expanded.
	TCPQueryResponse_A(const TCPQueryResponse_A& other)
	{
		m_pData = 0;
		*this = other;
	}

	~TCPQueryResponse_A() { if (m_pData) ::operator delete(m_pData); }

	// 0x10037c9c -- out of line at /O1; void, eax is left undefined.
	void operator=(const TCPQueryResponse_A& other)
	{
		if (m_pData)
			::operator delete(m_pData);

		memcpy(this, &other, sizeof(TCPQueryResponse_A));

		if (other.m_pData)
		{
			m_pData = (char*)::operator new(strlen(other.m_pData) + 1);
			strcpy(m_pData, other.m_pData);
		}
	}

	char       m_Text[0x80];                 // 0x00
	uint32     m_Unknown80;                  // 0x80
	float      m_Time;                       // 0x84
	SockAddr_A m_Addr;                       // 0x88
	char*      m_pData;                      // 0x98
	float      m_Unknown9c;                  // 0x9c
	float      m_UnknownA0;                  // 0xa0
};

// 0x10037c4c (ctor), 0x10037c6e GenGetNext, 0x10037cf5 GenGetAt,
// 0x10037d1e GenAppend, 0x10037d33 GenRemoveAt, 0x10037d44 GenRemoveAll,
// 0x10037d51 GenGetSize, 0x10037d55 GenCopyList, 0x10037e1a GenAppendList,
// 0x10037eea GenFindElement, 0x10037f14 Init, 0x10037f49 SetSize2,
// 0x10037fb9 InternalNiceSetSize (and 0x100380c9 Insert2 in section B).
#ifdef PRIMAL_HUNT
// tcpdriver.cpp's body (0x10046422), compiled here before the array is.
// Primal Hunt's /GX build has one BaseNew for the response array
// (0x1004841e, 63 bytes, called by this array's Insert2, Remove2 and
// SetSize2) with no EH frame around the constructor calls, which is VC6
// knowing the body cannot throw. Declared only, ours framed them (99 bytes)
// and was a second copy of it.
TCPQueryResponse_A::TCPQueryResponse_A()
{
	m_pData = LTNULL;
	m_Unknown9c = 0.0f;
	m_Unknown80 = 0;
	memset(m_Text, 0, sizeof(m_Text));
	m_UnknownA0 = 0.0f;
	m_Time = 0.0f;
}
#endif

template class CMoArray<TCPQueryResponse_A, DefaultCache>;

// Just enough of TCPDriver (tcpdriver.cpp) for the two slots below: 0xb0
// bytes, m_Socket at +0x90. The slots are virtual in the real class; their
// bodies do not depend on that.
class TCPDriver_A
{
public:
	TCPDriver_A();                           // 0x10036060

	LTRESULT ConnectTCP(char* pAddress);     // slot 0x54
	LTRESULT SendToAddr(void* pData, uint32 nLen, char* sAddr, uint32 port); // 0x58

	LTRESULT JoinSession(SockAddr_A* pHostAddr);                    // 0x100375e2
	LTBOOL   SendTo(uint32 hSocket, void* pData, uint32 nLen,
	                SockAddr_A* pTo);                               // 0x10036278

	uint8  m_pad00[0x90];
	uint32 m_Socket;                         // 0x90
	uint8  m_pad94[0xb0 - 0x94];
};

// 0x10037b7c -- slot 0x54 (lithtech twin 0x004870f0).
LTRESULT TCPDriver_A::ConnectTCP(char* pAddress)
{
	SockAddr_A addr;

	if (!tcp_BuildSockaddrFromString_A(pAddress, &addr))
		RETURN_ERROR(1, "ConnectTCP", LT_CANTBINDTOPORT)

	return JoinSession(&addr);
}

// 0x10037bda -- slot 0x58 (lithtech twin 0x00487160).
LTRESULT TCPDriver_A::SendToAddr(void* pData, uint32 nLen,
                                 char* sAddr, uint32 port)
{
	SockAddr_A addr;

	if (!m_Socket)
		return LT_NOTINITIALIZED;

	if (!tcp_BuildSockaddrFromString_A(sAddr, &addr))
		return LT_ERROR;

	addr.sin_port = htons_A((unsigned short)port);

	SendTo(m_Socket, pData, nLen, &addr);
	return LT_OK;
}

// 0x10037c32 -- servermgr.h's tcp_Create (lithtech twin 0x004871e0).
CBaseDriver* tcp_Create()
{
	return (CBaseDriver*)new TCPDriver_A;
}


// ==== END SECTION A =======================================================

// ==== SECTION B: 0x10038000 .. 0x10038674 ================================
//
// Three unrelated runs share this range:
//   1. 0x100380c9 .. 0x100383a4: four COMDATs of tcpdriver.cpp's
//      CMoArray<TCPQueryResponse> (the 0xa4 element: the constructor it
//      builds with is TCPQueryResponse::TCPQueryResponse, 0x1003654a, and the
//      destructor it expands frees +0x98 with ::operator delete, exactly
//      tcpdriver.cpp's inline ~TCPQueryResponse). They REALLY belong to
//      tcpdriver.cpp (the linker emitted them after 0x10038094,
//      tcp_FreeQueryList, which is also tcpdriver's); written here against a
//      copy of the element type.
//   2. 0x100383a5 .. 0x100383d8: the engine clock (the client twin is
//      0x004895e0 / 0x00489600, directly in front of CLTTransform).
//   3. 0x100383d9 .. 0x10038673: ILTTransform's nine virtuals. The server's
//      vtable 0x1004e660 is ILTTransform's own (CLTServer embeds a bare
//      ILTTransform at +0x228), so the bodies are ILTTransform:: members.
//      Its nine slots are 0x100383d9, 0x1003840a, 0x1003843b, 0x1003844f,
//      0x10038473, 0x100384af, 0x100384e9, 0x10038543, 0x100385d2.

// ---- 1. CMoArray<TCPQueryResponse> ------------------------------------- //
// 0x100380c9 Insert2, 0x1003820d Remove2, 0x10038329 BaseDelete and
// 0x10038366 BaseNew are the tail of section A's explicit instantiation of
// CMoArray<TCPQueryResponse_A, DefaultCache>; nothing is written for them
// here. Insert2 is NEAR (324 vs 322): collection.h's body, the residual is
// the m_pArray + i*0xa4 operand order into the operator= argument (shipped
// `mov 4(%esi),%ecx; add %eax,%ecx`). Register allocation; an out-of-class
// inline operator= made no difference.

// ---- 2. the engine clock ----------------------------------------------- //

extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);

// 0x10058c24 — timeGetTime() at static-initialisation time. Its dynamic
// initialiser is the compiler's: 0x100383a5 (`jmp`) and 0x100383aa (the
// body, `call timeGetTime; mov %eax,0x10058c24; ret`).
static uint32 g_StartTime_B = timeGetTime();

// 0x100383b6 — seconds since startup (saveio.cpp's time_GetTime). The
// unsigned difference converts through a zero-extended fildll.
float time_GetTime()
{
	return (float)(timeGetTime() - g_StartTime_B) * 0.001f;
}

// ---- 3. ILTTransform --------------------------------------------------- //

#include "ilttransform.h"

// 0x10015bcd / 0x10015ce2 (commonmodel.h spells them the same way).
void quat_ConvertToMatrix(LTRotation* pRot, LTMatrix* pMat);
void quat_ConvertFromMatrix(float* pQuat, const float mat[4][4]);

// 0x100384c3 — a matrix back into a transform (name ours). A plain
// function, not inline: every slot below calls it, and it sits between
// FromMatrix and Inverse, so it is defined there.
void trans_FromMatrix_B(LTransform* pTransform, LTMatrix* pMat);

// 0x100383d9 — slot 0.
LTRESULT ILTTransform::Get(LTransform &transform, LTVector &pos, LTRotation &rot)
{
	pos = transform.m_Pos;
	rot = transform.m_Rot;

	return LT_OK;
}

// 0x1003840a — slot 1.
LTRESULT ILTTransform::Set(LTransform &transform, LTVector &pos, LTRotation &rot)
{
	transform.m_Pos = pos;
	transform.m_Rot = rot;

	return LT_OK;
}

// 0x1003843b — slot 2.
LTRESULT ILTTransform::GetPos(LTransform &transform, LTVector &pos)
{
	pos = transform.m_Pos;

	return LT_OK;
}

// 0x1003844f — slot 3.
LTRESULT ILTTransform::GetRot(LTransform &transform, LTRotation &rot)
{
	rot = transform.m_Rot;

	return LT_OK;
}

// 0x10038473 — slot 4. The 0xc frame is SetTranslation's by-value vector.
LTRESULT ILTTransform::ToMatrix(LTransform &transform, LTMatrix &mat)
{
	quat_ConvertToMatrix(&transform.m_Rot, &mat);
	mat.SetTranslation(transform.m_Pos);

	return LT_OK;
}

// 0x100384af — slot 5.
LTRESULT ILTTransform::FromMatrix(LTransform &transform, LTMatrix &mat)
{
	trans_FromMatrix_B(&transform, &mat);

	return LT_OK;
}

void trans_FromMatrix_B(LTransform* pTransform, LTMatrix* pMat)
{
	pTransform->m_Pos.x = pMat->m[0][3];
	pTransform->m_Pos.y = pMat->m[1][3];
	pTransform->m_Pos.z = pMat->m[2][3];

	quat_ConvertFromMatrix(pTransform->m_Rot.m_Quat, pMat->m);
}

// 0x100384e9 — slot 6. Frame 0x8c = vector + two matrices. The inverse is
// computed (Mat_InverseTransformation, out of line at 0x10011d32) and then
// NOT used: the result is built from the original matrix -- a Monolith bug
// (the /O2 client drops the dead inverse altogether, 0x00489780).
LTRESULT ILTTransform::Inverse(LTransform &inverse, LTransform &transform)
{
	LTMatrix mat, matInverse;

	quat_ConvertToMatrix(&transform.m_Rot, &mat);
	mat.SetTranslation(transform.m_Pos);

	Mat_InverseTransformation(&mat, &matInverse);
	trans_FromMatrix_B(&inverse, &mat);

	return LT_OK;
}

// 0x10038543 — slot 7. out = t1 * t2.
LTRESULT ILTTransform::Multiply(LTransform &out, LTransform &t1, LTransform &t2)
{
	LTMatrix mat1, mat2, matOut;

	quat_ConvertToMatrix(&t1.m_Rot, &mat1);
	mat1.SetTranslation(t1.m_Pos);

	quat_ConvertToMatrix(&t2.m_Rot, &mat2);
	mat2.SetTranslation(t2.m_Pos);

	MatMul(&matOut, &mat1, &mat2);
	trans_FromMatrix_B(&out, &matOut);

	return LT_OK;
}

// 0x100385d2 — slot 8. diff = inverse(t2) * t1.
LTRESULT ILTTransform::Difference(LTransform &diff, LTransform &t1, LTransform &t2)
{
	LTMatrix mat1, mat2, mat2Inv, matDiff;

	quat_ConvertToMatrix(&t1.m_Rot, &mat1);
	mat1.SetTranslation(t1.m_Pos);

	quat_ConvertToMatrix(&t2.m_Rot, &mat2);
	mat2.SetTranslation(t2.m_Pos);

	Mat_InverseTransformation(&mat2, &mat2Inv);
	MatMul(&matDiff, &mat2Inv, &mat1);
	trans_FromMatrix_B(&diff, &matDiff);

	return LT_OK;
}

// ==== END SECTION B =======================================================

