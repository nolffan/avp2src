// ----------------------------------------------------------------------- //
//
// MODULE  : MultiFile.h
//
// PURPOSE : CMultiFile - Primal Hunt's file interface.
//
//           Primal Hunt (Third Law) caches the attribute files the bute
//           managers parse: each manager can write what it loaded to a
//           CMultiFile and read it back instead of parsing the text
//           (CGameButeMgr's WriteCache and ReadCache). Nothing of this is
//           in the AvP2 1.0.9.6 release; the interface is reconstructed
//           from PHCShell.dll: CMultiFile_LITH (its vtable at 0x101b1854,
//           the functions at 0x10136230..0x10136bc0) wraps an ILTStream.
//           The slot order is the vtable's (VC6 lays out a virtual's
//           overloads in reverse: ReadInt(int*) is the slot before
//           ReadInt()); the names are ours.
//
// ----------------------------------------------------------------------- //

#ifndef __MULTI_FILE_H__
#define __MULTI_FILE_H__

#ifdef PRIMAL_HUNT

class CMultiFile
{
	public :

		virtual ~CMultiFile() {}

		virtual LTBOOL		Open(const char* szFile, int nMode) = 0;
		virtual LTBOOL		Close() = 0;

		virtual LTRESULT	WriteDWord(uint32 dwVal) = 0;
		virtual LTRESULT	WriteInt(int nVal) = 0;
		virtual LTRESULT	WriteString(const char* szVal) = 0;
		virtual LTRESULT	WriteFloat(LTFLOAT fVal) = 0;
		virtual LTRESULT	WriteByte(uint8 nVal) = 0;
		virtual LTRESULT	WriteBool(LTBOOL bVal) = 0;
		virtual LTRESULT	WriteRaw(const void* pData, uint32 nSize) = 0;
		virtual LTRESULT	WriteVector(const LTVector & vVal) = 0;

		virtual uint32		ReadDWord() = 0;
		virtual LTRESULT	ReadDWord(uint32* pVal) = 0;
		virtual int			ReadInt() = 0;
		virtual LTRESULT	ReadInt(int* pVal) = 0;
		virtual LTRESULT	ReadString(char* szVal, uint16 nMaxLen) = 0;
		virtual LTFLOAT		ReadFloat() = 0;
		virtual LTRESULT	ReadFloat(LTFLOAT* pVal) = 0;
		virtual uint8		ReadByte() = 0;
		virtual LTRESULT	ReadByte(uint8* pVal) = 0;
		virtual LTBOOL		ReadBool() = 0;
		virtual LTRESULT	ReadBool(LTBOOL* pVal) = 0;
		virtual LTRESULT	ReadRaw(void* pData, uint32 nSize) = 0;
		virtual LTVector	ReadVector() = 0;
		virtual LTRESULT	ReadVector(LTVector* pVal) = 0;

		virtual LTBOOL		IsEOF() = 0;
		virtual LTRESULT	Seek(uint32 nOffset, int nOrigin) = 0;
		virtual uint32		GetPos() = 0;
		virtual uint32		GetLen() = 0;
		virtual LTBOOL		IsOpen() = 0;
};

#endif // PRIMAL_HUNT

#endif // __MULTI_FILE_H__
