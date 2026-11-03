//------------------------------------------------------------------
//
//	FILE	  : CompressedIO.cpp
//
//	PURPOSE	  : 
//
//	CREATED	  : February 15 1997
//
//	COPYRIGHT : Microsoft 1997 All Rights Reserved
//
//------------------------------------------------------------------

// Includes....
#include "zlib.h"
#include "compressedio.h"



// Allocators for ZLib's allocation stuff.
//
// The allocator catches its own throw. That is not as odd as it reads: zlib
// is C and cannot be unwound through, so the exception has to stop here and
// the failure has to come back as a null pointer. The shipped code says so
// plainly — it registers an exception frame, and the funclet at the end is a
// `catch(...)` whose body is `return NULL`, reached by the throw two
// instructions earlier.
//
// NEAR (114 of 115 bytes, ONE instruction): the shipped frame is `sub
// $0xc,%esp` where ours is `sub $0x8` — one unused dword. Every other byte
// agrees, and the mnemonic diff is empty.
//
// Where that dword goes, read off the shipped frame: -0x10 holds the EH esp
// save, the dword at -0x14 carries the one-byte exception temporary at its
// top (`lea -0x11(%ebp),%edx` at +0x3f — CLithMemException is an empty class,
// so sizeof is 1), and -0x18 is the extra one. Nothing reads or writes it.
//
// FOUR spellings ruled out, each measured:
//   * a named `CLithMemException e; throw e;`        — two bytes wrong, not one
//   * `ptr = NULL` in the catch instead of `return NULL` — spills and reloads ptr
//   * a named `uInt nBytes = items * size;`          — FIVE instructions wrong
//   * `return ptr;` INSIDE the try, catch returning NULL, no trailing return
//                                                    — byte-identical to this
//
// That last one is worth keeping: a return inside a try with an active EH
// frame does NOT cost a temporary here, which was the most plausible source
// of a spare dword. Whatever reserves it is not the shape of the control
// flow, the number of named locals, or where the throw's operand lives.

void* CustomZAlloc( voidpf opqaud, uInt items, uInt size )
{
	void		*ptr;

	LithTry
	{
		ptr = malloc( items*size );
		if( !ptr )
		{
			throw CLithMemException();
		}
	}
	LithCatch( ... )
	{
		return NULL;
	}

	return ptr;
}

void CustomZFree( voidpf opaque, voidpf address )
{
	free( address );
}




CCompressedIO::CCompressedIO()
{
	m_pReadStream = NULL;
	m_iCurLoadedChunk = (DWORD)-1;
	EnableExceptions( TRUE );
}


CCompressedIO::~CCompressedIO()
{
	Term();
}


BOOL CCompressedIO::WriteStream( CAbstractIO *pInStream, CAbstractIO *pOutStream, DWORD chunkSize, DWORD sizeToWrite )
{
	CAbstractIO		&inStream = *pInStream;
	CAbstractIO		&outStream = *pOutStream;
	
	CMoByteArray	compressed;
	CMoByteArray	uncompressed;
	
	CMoDWordArray	chunkSizes;
	CMoDWordArray	uncompressedChunkSizes;
	
	DWORD			descriptorPos;

	DWORD			nChunks;
	DWORD			nFullChunks = sizeToWrite / chunkSize;
	DWORD			chunkLen;
	
	DWORD			chunkStartPos, chunkEndPos, endPos;
	DWORD			i;

	
	// Find out how many total chunks there are.
	nChunks = nFullChunks;
	if( chunkSize*nFullChunks != sizeToWrite )
		++nChunks;

	
	// Start the header off.
	outStream << nChunks;
	outStream << chunkSize;

	descriptorPos =	outStream.GetCurPos();


// This causes errors in CMemoryIO .. so now it just writes garbage..
//	outStream.SeekTo( outStream.GetCurPos() + sizeof(DWORD)*nChunks + sizeof(DWORD)*nChunks );
	for( i=0; i < nChunks*2; i++ )
		outStream << (DWORD)0;

	
	compressed.SetSize( (chunkSize*2) + 8 );	
	uncompressed.SetSize( chunkSize );

		
	// Write all the full chunks.
	for( i=0; i < nFullChunks; i++ )
	{
		chunkStartPos = outStream.GetCurPos();
		WriteChunk( inStream, outStream, compressed, uncompressed, chunkSize );
		chunkEndPos = outStream.GetCurPos();

		chunkSizes.Append( chunkEndPos - chunkStartPos );
		uncompressedChunkSizes.Append( chunkSize );
	}

	// Write an extra one if necessary.
	if( nChunks != nFullChunks )
	{
		chunkLen = WriteChunk( inStream, outStream, compressed, uncompressed, sizeToWrite - (nFullChunks*chunkSize) );

		chunkSizes.Append( chunkLen );
		uncompressedChunkSizes.Append( sizeToWrite - (nFullChunks*chunkSize) );
	}


	// Go back and fill in the header.
	ASSERT( chunkSizes.GetSize() == nChunks );
	
	endPos = outStream.GetCurPos();

	outStream.SeekTo( descriptorPos );
	
	// Write the compressed chunk sizes.
	for( i=0; i < nChunks; i++ )
		outStream << chunkSizes[i];

	// Write the uncompressed chunk sizes.
	for( i=0; i < nChunks; i++ )
		outStream << uncompressedChunkSizes[i];

	outStream.SeekTo( endPos );

	return TRUE;
}


DWORD CCompressedIO::WriteChunk( CAbstractIO &inStream, CAbstractIO &outStream, CMoByteArray &compressed, CMoByteArray &uncompressed, DWORD size )
{
    z_stream		stream;
	int				err;


	inStream.Read( uncompressed.GetArray(), size );

    stream.next_in = (Bytef*)uncompressed.GetArray();
    stream.avail_in = (uInt)size;

    stream.next_out = compressed.GetArray();
    stream.avail_out = (uInt)compressed.GetSize();

    stream.zalloc = CustomZAlloc;
    stream.zfree = CustomZFree;
    stream.opaque = (voidpf)0;

    err = deflateInit( &stream, Z_DEFAULT_COMPRESSION );
    if( err != Z_OK )
		throw CLithMemException();

    err = deflate( &stream, Z_FINISH );
    deflateEnd(&stream);

    if( err != Z_STREAM_END )
		throw CLithMemException();

	outStream.Write( compressed.GetArray(), stream.total_out );
	return (DWORD)stream.total_out;
}


BOOL CCompressedIO::InitRead( CAbstractIO *pStreamToUse )
{
	DWORD			i;
	DWORD			curOffset;
	DWORD			maxCompressedChunkSize;

	
	try
	{

	Term();


	// Store the stream pointer.
	m_pReadStream = pStreamToUse;


	// Read the header.
	Stream() >> m_nChunks >> m_ChunkSize;

	// Read the compressed chunk sizes and offsets.
	m_ChunkOffsets.SetSize( m_nChunks );
	m_ChunkSizes.SetSize( m_nChunks );

	curOffset = 0;
	maxCompressedChunkSize = 0;

	for( i=0; i < m_nChunks; i++ )
	{
		Stream() >> m_ChunkSizes[i];
		m_ChunkOffsets[i] = curOffset;

		curOffset += m_ChunkSizes[i];

		if( m_ChunkSizes[i] > maxCompressedChunkSize )
			maxCompressedChunkSize = m_ChunkSizes[i];
	}

	// Read the uncompressed chunk sizes.
	m_UncompressedChunkSizes.SetSize( m_nChunks );
	
	m_FileLen = 0;
	for( i=0; i < m_nChunks; i++ )
	{
		Stream() >> m_UncompressedChunkSizes[i];
		m_FileLen += m_UncompressedChunkSizes[i];
	}


	// Setup space for the compressed data to be read into.
	m_CompressedChunkSpace.SetSize( maxCompressedChunkSize );

	// Setup space for the current uncompressed chunk.
	m_UncompressedChunk.SetSize( m_ChunkSize );

	
	// Set it up on the first byte.
	SwitchToChunk( 0, 0 );


	// Store the base file position (where the actual data starts).
	m_BaseFilePos = Stream().GetCurPos();

	}
	catch( CLithException &e )
	{
		Term();
		throw e;
	}
		
	return TRUE;
}


void CCompressedIO::Term()
{
	if( m_pReadStream )
	{
		m_pReadStream = NULL;
		
		m_ChunkOffsets.Term();
		m_ChunkSizes.Term();
		m_UncompressedChunkSizes.Term();
		m_CompressedChunkSpace.Term();
		m_UncompressedChunk.Term();

		m_iCurChunk = (DWORD)-1;
		m_CurChunkPos = 0;
	}
}


BOOL CCompressedIO::Read( void *pData, DWORD size )
{
	DWORD			i;
	BYTE			*pByteData = (BYTE*)pData;


	ASSERT( m_pReadStream );

	// Make sure the current chunk is loaded.
	if( m_iCurLoadedChunk != m_iCurChunk )
		LoadCurChunk();

	for( i=0; i < size; i++ )
	{
		// Move to the next chunk if necessary.
		if( m_CurChunkPos >= m_CurChunkSize )
		{
			// Make sure the even is a next chunk.
			if( (m_iCurChunk+1) >= m_nChunks )
				throw CLithIOException(MoReadError);

			SwitchToChunk( m_iCurChunk+1, 0 );
			LoadCurChunk();
		}

		pByteData[i] = m_UncompressedChunk[m_CurChunkPos];
		++m_CurChunkPos;
	}
	
	return TRUE;
}


DWORD CCompressedIO::GetCurPos()
{
	ASSERT( m_pReadStream );
	
	
	return (m_iCurChunk*m_ChunkSize) + m_CurChunkPos;
}


DWORD CCompressedIO::GetLen()
{
	ASSERT( m_pReadStream );
	
	return m_FileLen;
}


BOOL CCompressedIO::SeekTo( DWORD pos )
{
	if( pos > m_FileLen )
		MaybeThrowIOException( MoSeekError );

	SwitchToChunk( pos/m_ChunkSize, pos%m_ChunkSize );	
	return TRUE;
}


void CCompressedIO::SwitchToChunk( DWORD iChunk, DWORD offset )
{
	ASSERT( iChunk < m_nChunks );
	
	m_iCurChunk = iChunk;
	m_CurChunkPos = offset;

	m_CurChunkSize = m_UncompressedChunkSizes[iChunk];
}


void CCompressedIO::LoadCurChunk()
{
    z_stream		stream;
    int				err;


	ASSERT( m_pReadStream && (m_iCurChunk != (DWORD)-1) );

	// Read in the compressed data.
	Stream().SeekTo( m_BaseFilePos + m_ChunkOffsets[m_iCurChunk] );
	Stream().Read( m_CompressedChunkSpace.GetArray(), m_ChunkSizes[m_iCurChunk] );


    stream.next_in = (Bytef*)m_CompressedChunkSpace.GetArray();
    stream.avail_in = (uInt)m_ChunkSizes[m_iCurChunk];
    
    stream.next_out = m_UncompressedChunk.GetArray();
    stream.avail_out = (uInt)m_ChunkSize;

    stream.zalloc = (alloc_func)CustomZAlloc;
    stream.zfree = (free_func)CustomZFree;

    err = inflateInit( &stream );
    if( err != Z_OK )
		throw CLithMemException();

    err = inflate( &stream, Z_FINISH );
    inflateEnd(&stream);

    if( err != Z_STREAM_END )
		throw CLithMemException();

	// Ok .. now m_iCurChunk is loaded.
	m_iCurLoadedChunk = m_iCurChunk;
}









