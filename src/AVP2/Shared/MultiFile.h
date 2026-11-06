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
//           the functions at 0x10136230..0x10136bc0) wraps an ILTStream,
//           and from PHObject.lto, which has CMultiFile_FILE (a FILE*)
//           too (MultiFile.cpp). The class names are the RTTI's; the slot
//           order is the vtable's (VC6 lays out a virtual's overloads in
//           reverse: ReadInt(int*) is the slot before ReadInt()); the
//           function names are ours.
//
// ----------------------------------------------------------------------- //

#ifndef __MULTI_FILE_H__
#define __MULTI_FILE_H__

#ifdef PRIMAL_HUNT

// The open modes and seek origins (CMultiFile_FILE::Open, PHObject.lto
// 0x10154ba0: 0 opens "rb", 1 "wb"; Seek's origins are stdio's)
#define MULTIFILE_READ				0
#define MULTIFILE_WRITE				1

#define MULTIFILE_SEEK_SET			0
#define MULTIFILE_SEEK_CUR			1
#define MULTIFILE_SEEK_END			2

class CMultiFile
{
	public :

		CMultiFile();
		virtual ~CMultiFile();

		virtual LTBOOL		Open(const char* szFile, int nMode) = 0;
		virtual LTBOOL		Close() = 0;

		virtual void		WriteDWord(uint32 dwVal) = 0;
		virtual void		WriteInt(int nVal) = 0;
		virtual void		WriteString(const char* szVal) = 0;
		virtual void		WriteFloat(LTFLOAT fVal) = 0;
		virtual void		WriteByte(uint8 nVal) = 0;
		virtual void		WriteBool(LTBOOL bVal) = 0;
		virtual void		WriteRaw(const void* pData, uint32 nSize) = 0;
		virtual void		WriteVector(const LTVector & vVal) = 0;

		// The pointer overloads are the base class's (PHObject.lto
		// 0x101549e0..0x10154ad0): each reads through its value overload
		virtual uint32		ReadDWord() = 0;
		virtual LTBOOL		ReadDWord(uint32* pVal);
		virtual int			ReadInt() = 0;
		virtual LTBOOL		ReadInt(int* pVal);
		virtual void		ReadString(char* szVal, uint16 nMaxLen) = 0;
		virtual LTFLOAT		ReadFloat() = 0;
		virtual LTBOOL		ReadFloat(LTFLOAT* pVal);
		virtual uint8		ReadByte() = 0;
		virtual LTBOOL		ReadByte(uint8* pVal);
		virtual LTBOOL		ReadBool() = 0;
		virtual LTBOOL		ReadBool(LTBOOL* pVal);
		virtual LTBOOL		ReadRaw(void* pData, uint32 nSize) = 0;
		virtual LTVector	ReadVector();
		virtual LTBOOL		ReadVector(LTVector* pVal);

		virtual LTBOOL		IsEOF() = 0;
		virtual void		Seek(uint32 nOffset, int nOrigin) = 0;
		virtual uint32		GetPos() = 0;
		virtual uint32		GetLen() = 0;
		virtual LTBOOL		IsOpen() = 0;
};

#endif // PRIMAL_HUNT

#endif // __MULTI_FILE_H__
