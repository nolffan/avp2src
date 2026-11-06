// ----------------------------------------------------------------------- //
//
// MODULE  : MultiFileImpl.h
//
// PURPOSE : Primal Hunt's two CMultiFile classes (MultiFile.h,
//           MultiFile.cpp): CMultiFile_FILE and CMultiFile_LITH.
//
// ----------------------------------------------------------------------- //

#ifndef __MULTI_FILE_IMPL_H__
#define __MULTI_FILE_IMPL_H__

#ifdef PRIMAL_HUNT

#include "MultiFile.h"

// ----------------------------------------------------------------------- //
// A file of the C runtime's: what WriteCache writes (PHObject.lto
// 0x10154b10..0x10154ea0, vtable 0x1028b250)

class CMultiFile_FILE : public CMultiFile
{
	public :

		CMultiFile_FILE();
		virtual ~CMultiFile_FILE();

		virtual LTBOOL		Open(const char* szFile, int nMode);
		virtual LTBOOL		Close();

		virtual void		WriteDWord(uint32 dwVal);
		virtual void		WriteInt(int nVal);
		virtual void		WriteString(const char* szVal);
		virtual void		WriteFloat(LTFLOAT fVal);
		virtual void		WriteByte(uint8 nVal);
		virtual void		WriteBool(LTBOOL bVal);
		virtual void		WriteRaw(const void* pData, uint32 nSize);
		virtual void		WriteVector(const LTVector & vVal);

		virtual uint32		ReadDWord();
		virtual int			ReadInt();
		virtual void		ReadString(char* szVal, uint16 nMaxLen);
		virtual LTFLOAT		ReadFloat();
		virtual uint8		ReadByte();
		virtual LTBOOL		ReadBool();
		virtual LTBOOL		ReadRaw(void* pData, uint32 nSize);

		virtual LTBOOL		IsEOF();
		virtual void		Seek(uint32 nOffset, int nOrigin);
		virtual uint32		GetPos();
		virtual uint32		GetLen();
		virtual LTBOOL		IsOpen();

	protected :

		FILE*		m_pFile;
};

// ----------------------------------------------------------------------- //
// A file through the engine (ILTCSBase::OpenFile, so in the rez files too):
// what ReadCache reads (PHObject.lto 0x10154ee0..0x101552d0, vtable
// 0x1028b2cc; PHCShell.dll 0x10136230..0x10136bc0)

class CMultiFile_LITH : public CMultiFile
{
	public :

		CMultiFile_LITH();
		virtual ~CMultiFile_LITH();

		virtual LTBOOL		Open(const char* szFile, int nMode);
		virtual LTBOOL		Close();

		virtual void		WriteDWord(uint32 dwVal);
		virtual void		WriteInt(int nVal);
		virtual void		WriteString(const char* szVal);
		virtual void		WriteFloat(LTFLOAT fVal);
		virtual void		WriteByte(uint8 nVal);
		virtual void		WriteBool(LTBOOL bVal);
		virtual void		WriteRaw(const void* pData, uint32 nSize);
		virtual void		WriteVector(const LTVector & vVal);

		virtual uint32		ReadDWord();
		virtual int			ReadInt();
		virtual void		ReadString(char* szVal, uint16 nMaxLen);
		virtual LTFLOAT		ReadFloat();
		virtual uint8		ReadByte();
		virtual LTBOOL		ReadBool();
		virtual LTBOOL		ReadRaw(void* pData, uint32 nSize);

		virtual LTBOOL		IsEOF();
		virtual void		Seek(uint32 nOffset, int nOrigin);
		virtual uint32		GetPos();
		virtual uint32		GetLen();
		virtual LTBOOL		IsOpen();

	protected :

		ILTStream*	m_pStream;
};

#endif // PRIMAL_HUNT

#endif // __MULTI_FILE_IMPL_H__
