// *********************************************************************** //
//
//  MODULE:   BINKW32 (.C)  -- the modern build's binkw32.dll
//
//  PURPOSE:  Plays the game's Bink movies (Movies/*.bik) with FFmpeg's Bink
//            decoders, in place of RAD's proprietary binkw32.dll. Only the
//            modern build ships it (build/ext.sh builds it, with the static
//            FFmpeg libraries from ext/ffmpeg); the release build keeps
//            the retail DLL.
//
//  THE INTERFACE.  lithtech.exe loads "Binkw32.dll" itself and looks up
//  thirteen entry points by their decorated __stdcall names
//  (src/engine/lithtech/binkvideo.cpp, BinkVideoMgr::Init); it refuses
//  the DLL if any of them but _BinkRestoreCursor@4 is missing. Those
//  thirteen are all this file exports (binkw32.def), with the meanings
//  the engine relies on:
//
//    * BinkOpen returns a BINK whose first four fields the engine reads
//      directly: Width, Height, Frames and FrameNum (1 = the first frame,
//      as in RAD's bink.h). The rest of the structure is ours.
//    * the engine's frame loop is Wait -> CopyToBuffer -> DoFrame ->
//      NextFrame. Wait answers nonzero while the current frame's time has
//      not come yet. CopyToBuffer converts the current frame into the
//      caller's surface (decoding it first if DoFrame has not), in one of
//      the four surface formats the engine asks for.
//    * the movie is done when FrameNum reaches Frames - 1 (the engine's
//      IsDone), so NextFrame never needs to wrap.
//    * texture videos are opened with BINKSNDTRACK after
//      BinkSetSoundTrack(-1): no sound. On-screen videos play their first
//      audio track.
//
//  SOUND.  The engine hands Miles' digital driver to BinkSetSoundSystem;
//  this player does not use it. A movie's whole audio track is decoded
//  when it opens (a few MB for the game's movies) and played with one
//  waveOut buffer, queued with the first frame; video frames are timed
//  from the same moment by the performance counter, and skip ahead when
//  the game falls behind.
//
//  The cursor entry points are for a software mouse cursor drawn over the
//  movie; no answer from them changes what is shown, so they are no-ops.
//
// *********************************************************************** //

#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#define BINKSNDTRACK        0x00004000

#define BINKSURFACE32       3       // 0x00RRGGBB dwords: B, G, R, x in memory
#define BINKSURFACE32R      4       // R, G, B, x in memory
#define BINKSURFACE555      9
#define BINKSURFACE565      10
#define BINKSURFACEMASK     15

typedef struct BinkPlayer BinkPlayer;

// The engine reads the first four fields; the layout of the first seven
// is RAD's.
typedef struct BINK
{
	unsigned int Width;                     // 0x00
	unsigned int Height;                    // 0x04
	unsigned int Frames;                    // 0x08
	unsigned int FrameNum;                  // 0x0c, 1 = the first frame
	unsigned int LastFrameNum;              // 0x10
	unsigned int FrameRate;                 // 0x14, frames per second is
	unsigned int FrameRateDiv;              // 0x18  FrameRate / FrameRateDiv
	BinkPlayer*  pPlayer;
} BINK;

struct BinkPlayer
{
	AVFormatContext*   pFormat;
	AVCodecContext*    pVideo;
	int                nVideoStream;
	AVPacket*          pPacket;
	AVFrame*           pFrame;
	unsigned int       nDecoded;            // frames decoded so far (pFrame is the last)
	int                bEOF;

	struct SwsContext* pScale;

	LARGE_INTEGER      nStart;              // when the first frame was shown
	LARGE_INTEGER      nFreq;
	int                bStarted;

	HWAVEOUT           hWave;
	WAVEHDR            WaveHdr;
	short*             pAudio;
};

static unsigned int g_nSoundTrack = 0;

static void CloseSound(BinkPlayer* pPlayer);


// ----------------------------------------------------------------------- //
//  Sound: the whole first (or selected) audio track, as 16-bit PCM.
// ----------------------------------------------------------------------- //

static short ToS16(float f)
{
	if (f > 1.0f)
		f = 1.0f;
	else if (f < -1.0f)
		f = -1.0f;
	return (short)(f * 32767.0f);
}

// Appends one decoded frame to *ppPCM (interleaved 16-bit), growing it.
static int AppendAudio(AVFrame* pFrame, short** ppPCM, size_t* pnSamples, size_t* pnAlloc)
{
	int    nChannels = pFrame->ch_layout.nb_channels;
	int    nPlanar = av_sample_fmt_is_planar((enum AVSampleFormat)pFrame->format);
	size_t nNeed = *pnSamples + (size_t)pFrame->nb_samples * nChannels;
	int    i, c;

	if (nNeed > *pnAlloc)
	{
		size_t nNew = *pnAlloc ? *pnAlloc * 2 : 1 << 20;
		short* pNew;

		while (nNew < nNeed)
			nNew *= 2;
		pNew = (short*)realloc(*ppPCM, nNew * sizeof(short));
		if (!pNew)
			return 0;
		*ppPCM = pNew;
		*pnAlloc = nNew;
	}

	for (i = 0; i < pFrame->nb_samples; ++i)
	{
		for (c = 0; c < nChannels; ++c)
		{
			short s;

			switch (av_get_packed_sample_fmt((enum AVSampleFormat)pFrame->format))
			{
			case AV_SAMPLE_FMT_FLT:
				s = nPlanar ? ToS16(((float*)pFrame->extended_data[c])[i])
				            : ToS16(((float*)pFrame->extended_data[0])[i * nChannels + c]);
				break;
			case AV_SAMPLE_FMT_S16:
				s = nPlanar ? ((short*)pFrame->extended_data[c])[i]
				            : ((short*)pFrame->extended_data[0])[i * nChannels + c];
				break;
			default:
				return 0;
			}
			(*ppPCM)[(*pnSamples)++] = s;
		}
	}
	return 1;
}

// Decodes the audio track into one buffer and opens waveOut for it; the
// buffer is queued with the first frame (StartSound). No sound (and no
// error) if anything fails.
static void OpenSound(BinkPlayer* pPlayer, const char* pFilename, unsigned int nTrack)
{
	AVFormatContext* pFormat = NULL;
	AVCodecContext*  pAudio = NULL;
	AVPacket*        pPacket = NULL;
	AVFrame*         pFrame = NULL;
	short*           pPCM = NULL;
	size_t           nSamples = 0, nAlloc = 0;
	int              nStream = -1, nRate = 0, nChannels = 0;
	unsigned int     i, nSeen = 0;
	WAVEFORMATEX     wfx;

	if (avformat_open_input(&pFormat, pFilename, NULL, NULL) < 0)
		return;

	for (i = 0; i < pFormat->nb_streams; ++i)
	{
		if (pFormat->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
			continue;
		if (nSeen++ == nTrack)
		{
			nStream = (int)i;
			break;
		}
	}
	if (nStream < 0)
		goto done;

	{
		AVCodecParameters* pPar = pFormat->streams[nStream]->codecpar;
		const AVCodec*     pCodec = avcodec_find_decoder(pPar->codec_id);

		if (!pCodec || !(pAudio = avcodec_alloc_context3(pCodec)) ||
		    avcodec_parameters_to_context(pAudio, pPar) < 0 ||
		    avcodec_open2(pAudio, pCodec, NULL) < 0)
			goto done;
	}

	pPacket = av_packet_alloc();
	pFrame = av_frame_alloc();
	if (!pPacket || !pFrame)
		goto done;

	for (;;)
	{
		int bEOF = av_read_frame(pFormat, pPacket) < 0;

		if (!bEOF && pPacket->stream_index != nStream)
		{
			av_packet_unref(pPacket);
			continue;
		}
		avcodec_send_packet(pAudio, bEOF ? NULL : pPacket);
		av_packet_unref(pPacket);
		while (avcodec_receive_frame(pAudio, pFrame) == 0)
		{
			nRate = pFrame->sample_rate;
			nChannels = pFrame->ch_layout.nb_channels;
			if (!AppendAudio(pFrame, &pPCM, &nSamples, &nAlloc))
				goto done;
		}
		if (bEOF)
			break;
	}

	if (!nSamples || nRate <= 0 || nChannels < 1 || nChannels > 2)
		goto done;

	memset(&wfx, 0, sizeof(wfx));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = (WORD)nChannels;
	wfx.nSamplesPerSec = (DWORD)nRate;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = (WORD)(nChannels * 2);
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	if (waveOutOpen(&pPlayer->hWave, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
	{
		pPlayer->hWave = NULL;
		goto done;
	}

	// Prepared now, queued by the first BinkWait (StartSound): a buffer
	// queued here would start playing at once -- waveOutPause on a device
	// with nothing queued is not reliably honoured -- and the sound would
	// run ahead of the picture by however long the engine takes to show
	// the first frame.
	memset(&pPlayer->WaveHdr, 0, sizeof(pPlayer->WaveHdr));
	pPlayer->WaveHdr.lpData = (LPSTR)pPCM;
	pPlayer->WaveHdr.dwBufferLength = (DWORD)(nSamples * sizeof(short));
	if (waveOutPrepareHeader(pPlayer->hWave, &pPlayer->WaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		waveOutClose(pPlayer->hWave);
		pPlayer->hWave = NULL;
		goto done;
	}
	pPlayer->pAudio = pPCM;
	pPCM = NULL;

done:
	free(pPCM);
	av_frame_free(&pFrame);
	av_packet_free(&pPacket);
	avcodec_free_context(&pAudio);
	avformat_close_input(&pFormat);
}

// With the first frame: the decoded track starts playing.
static void StartSound(BinkPlayer* pPlayer)
{
	if (pPlayer->hWave &&
	    waveOutWrite(pPlayer->hWave, &pPlayer->WaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		CloseSound(pPlayer);
}

static void CloseSound(BinkPlayer* pPlayer)
{
	if (pPlayer->hWave)
	{
		waveOutReset(pPlayer->hWave);
		waveOutUnprepareHeader(pPlayer->hWave, &pPlayer->WaveHdr, sizeof(WAVEHDR));
		waveOutClose(pPlayer->hWave);
		pPlayer->hWave = NULL;
	}
	free(pPlayer->pAudio);
	pPlayer->pAudio = NULL;
}


// ----------------------------------------------------------------------- //
//  Video
// ----------------------------------------------------------------------- //

// Decodes until frame FrameNum is in pFrame. At the end of the file the
// last frame decoded stays.
static void DecodeCurrent(BINK* pBink)
{
	BinkPlayer* pPlayer = pBink->pPlayer;

	while (pPlayer->nDecoded < pBink->FrameNum)
	{
		int nResult = avcodec_receive_frame(pPlayer->pVideo, pPlayer->pFrame);

		if (nResult == 0)
		{
			++pPlayer->nDecoded;
			continue;
		}
		if (nResult != AVERROR(EAGAIN))
			return;                         // drained: nothing more to show

		if (pPlayer->bEOF)
			return;
		if (av_read_frame(pPlayer->pFormat, pPlayer->pPacket) < 0)
		{
			pPlayer->bEOF = 1;
			avcodec_send_packet(pPlayer->pVideo, NULL);
			continue;
		}
		if (pPlayer->pPacket->stream_index == pPlayer->nVideoStream)
			avcodec_send_packet(pPlayer->pVideo, pPlayer->pPacket);
		av_packet_unref(pPlayer->pPacket);
	}
}

static void FreePlayer(BinkPlayer* pPlayer)
{
	CloseSound(pPlayer);
	sws_freeContext(pPlayer->pScale);
	av_frame_free(&pPlayer->pFrame);
	av_packet_free(&pPlayer->pPacket);
	avcodec_free_context(&pPlayer->pVideo);
	avformat_close_input(&pPlayer->pFormat);
	free(pPlayer);
}


// ----------------------------------------------------------------------- //
//  The exports (binkw32.def gives them RAD's decorated names)
// ----------------------------------------------------------------------- //

BINK* __stdcall BinkOpen(const char* pFilename, unsigned int nFlags)
{
	BINK*              pBink;
	BinkPlayer*        pPlayer;
	AVStream*          pStream;
	const AVCodec*     pCodec;
	AVRational         fps;
	unsigned int       nTrack = g_nSoundTrack;

	g_nSoundTrack = 0;
	av_log_set_level(AV_LOG_QUIET);

	pBink = (BINK*)calloc(1, sizeof(BINK));
	pPlayer = (BinkPlayer*)calloc(1, sizeof(BinkPlayer));
	if (!pBink || !pPlayer)
		goto fail;
	pBink->pPlayer = pPlayer;

	if (avformat_open_input(&pPlayer->pFormat, pFilename, NULL, NULL) < 0)
		goto fail;
	pPlayer->nVideoStream = av_find_best_stream(pPlayer->pFormat, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (pPlayer->nVideoStream < 0)
		goto fail;
	pStream = pPlayer->pFormat->streams[pPlayer->nVideoStream];

	pCodec = avcodec_find_decoder(pStream->codecpar->codec_id);
	if (!pCodec || !(pPlayer->pVideo = avcodec_alloc_context3(pCodec)) ||
	    avcodec_parameters_to_context(pPlayer->pVideo, pStream->codecpar) < 0 ||
	    avcodec_open2(pPlayer->pVideo, pCodec, NULL) < 0)
		goto fail;

	pPlayer->pPacket = av_packet_alloc();
	pPlayer->pFrame = av_frame_alloc();
	if (!pPlayer->pPacket || !pPlayer->pFrame)
		goto fail;

	// The Bink demuxer times the video stream in frames: its time base is
	// the inverse of the frame rate.
	fps = av_inv_q(pStream->time_base);
	if (fps.num <= 0 || fps.den <= 0)
		fps = av_make_q(15, 1);

	pBink->Width = (unsigned int)pStream->codecpar->width;
	pBink->Height = (unsigned int)pStream->codecpar->height;
	pBink->Frames = (unsigned int)(pStream->nb_frames > 0 ? pStream->nb_frames : pStream->duration);
	if ((int)pBink->Frames < 1)
		pBink->Frames = 1;
	pBink->FrameNum = 1;
	pBink->FrameRate = (unsigned int)fps.num;
	pBink->FrameRateDiv = (unsigned int)fps.den;

	QueryPerformanceFrequency(&pPlayer->nFreq);

	if (!((nFlags & BINKSNDTRACK) && nTrack == 0xFFFFFFFF))
		OpenSound(pPlayer, pFilename, (nFlags & BINKSNDTRACK) ? nTrack : 0);

	DecodeCurrent(pBink);
	return pBink;

fail:
	if (pPlayer)
		FreePlayer(pPlayer);
	free(pBink);
	return NULL;
}

void __stdcall BinkClose(BINK* pBink)
{
	if (!pBink)
		return;
	FreePlayer(pBink->pPlayer);
	free(pBink);
}

// Nonzero while it is too early to show frame FrameNum.
int __stdcall BinkWait(BINK* pBink)
{
	BinkPlayer*   pPlayer;
	LARGE_INTEGER nNow;
	double        fElapsed, fDue;

	if (!pBink)
		return 0;
	pPlayer = pBink->pPlayer;

	if (!pPlayer->bStarted)
	{
		pPlayer->bStarted = 1;
		QueryPerformanceCounter(&pPlayer->nStart);
		StartSound(pPlayer);
		return 0;
	}

	QueryPerformanceCounter(&nNow);
	fElapsed = (double)(nNow.QuadPart - pPlayer->nStart.QuadPart) / (double)pPlayer->nFreq.QuadPart;
	fDue = (double)(pBink->FrameNum - 1) * pBink->FrameRateDiv / pBink->FrameRate;
	return fElapsed < fDue;
}

int __stdcall BinkDoFrame(BINK* pBink)
{
	if (pBink)
		DecodeCurrent(pBink);
	return 0;
}

// Advances to the next frame -- or, when the game has fallen behind the
// clock (a stall while it loads, a slow machine), straight to the frame due
// now, so the picture catches up with the sound as RAD's player does. The
// frames skipped are still decoded (Bink frames depend on the previous
// one), just not converted or shown. Never past Frames - 1, the frame the
// engine's IsDone stops at.
void __stdcall BinkNextFrame(BINK* pBink)
{
	BinkPlayer*   pPlayer;
	LARGE_INTEGER nNow;
	double        fElapsed;
	unsigned int  nDue;

	if (!pBink)
		return;
	pPlayer = pBink->pPlayer;
	pBink->LastFrameNum = pBink->FrameNum;
	if (pBink->FrameNum < pBink->Frames)
		++pBink->FrameNum;

	if (pPlayer->bStarted)
	{
		QueryPerformanceCounter(&nNow);
		fElapsed = (double)(nNow.QuadPart - pPlayer->nStart.QuadPart) / (double)pPlayer->nFreq.QuadPart;
		nDue = (unsigned int)(fElapsed * pBink->FrameRate / pBink->FrameRateDiv) + 1;
		if (nDue > pBink->Frames - 1)
			nDue = pBink->Frames - 1;
		if (nDue > pBink->FrameNum)
			pBink->FrameNum = nDue;
	}
}

int __stdcall BinkCopyToBuffer(BINK* pBink, void* pDest, int nPitch, unsigned int nDestHeight,
                               unsigned int nDestX, unsigned int nDestY, unsigned int nFlags)
{
	BinkPlayer*        pPlayer;
	AVFrame*           pFrame;
	enum AVPixelFormat nFormat;
	int                nBytes, nHeight;
	uint8_t*           pDst[4] = { NULL, NULL, NULL, NULL };
	int                nDstPitch[4] = { 0, 0, 0, 0 };

	if (!pBink || !pDest)
		return 0;
	pPlayer = pBink->pPlayer;
	DecodeCurrent(pBink);
	pFrame = pPlayer->pFrame;
	if (!pPlayer->nDecoded || !pFrame->data[0])
		return 0;

	switch (nFlags & BINKSURFACEMASK)
	{
	case BINKSURFACE32:  nFormat = AV_PIX_FMT_BGR0;      nBytes = 4; break;
	case BINKSURFACE32R: nFormat = AV_PIX_FMT_RGB0;      nBytes = 4; break;
	case BINKSURFACE555: nFormat = AV_PIX_FMT_RGB555LE;  nBytes = 2; break;
	case BINKSURFACE565: nFormat = AV_PIX_FMT_RGB565LE;  nBytes = 2; break;
	default:             return 0;
	}

	nHeight = pFrame->height;
	if (nDestHeight && (int)(nDestHeight - nDestY) < nHeight)
		nHeight = (int)(nDestHeight - nDestY);
	if (nHeight <= 0)
		return 0;

	pPlayer->pScale = sws_getCachedContext(pPlayer->pScale,
		pFrame->width, pFrame->height, (enum AVPixelFormat)pFrame->format,
		pFrame->width, pFrame->height, nFormat, SWS_POINT, NULL, NULL, NULL);
	if (!pPlayer->pScale)
		return 0;

	pDst[0] = (uint8_t*)pDest + nDestY * nPitch + nDestX * nBytes;
	nDstPitch[0] = nPitch;

	// Scale the whole picture when it fits; otherwise only the rows that do
	// (sws_scale writes as many rows as it is given source rows).
	sws_scale(pPlayer->pScale, (const uint8_t* const*)pFrame->data, pFrame->linesize,
	          0, nHeight == pFrame->height ? pFrame->height : nHeight & ~1,
	          pDst, nDstPitch);
	return 0;
}

void __stdcall BinkSetSoundTrack(unsigned int nTrack)
{
	g_nSoundTrack = nTrack;
}

// The engine passes one of the two "open" entries below and, for Miles, its
// digital driver. Sound goes to waveOut regardless (see the header).
int __stdcall BinkSetSoundSystem(void* pOpen, void* pParam)
{
	(void)pOpen;
	(void)pParam;
	return 1;
}

int __stdcall BinkOpenMiles(void* pParam)
{
	(void)pParam;
	return 1;
}

int __stdcall BinkOpenDirectSound(void* pParam)
{
	(void)pParam;
	return 1;
}

int __stdcall BinkIsSoftwareCursor(void* pSurface, HCURSOR hCursor)
{
	(void)pSurface;
	(void)hCursor;
	return 0;
}

int __stdcall BinkCheckCursor(HWND hWnd, int x, int y, int w, int h)
{
	(void)hWnd; (void)x; (void)y; (void)w; (void)h;
	return 0;
}

void __stdcall BinkRestoreCursor(int bForce)
{
	(void)bForce;
}
