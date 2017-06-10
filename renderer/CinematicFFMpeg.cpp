/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

$Revision$ (Revision of last commit)
$Date$ (Date of last commit)
$Author$ (Author of last commit)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "CinematicFFMpeg.h"
#include "Image.h"
#include "tr_local.h"

idCVar r_cinematic_log("r_cinematic_log", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Dump logs from cinematic into \"log_cinematics.txt\" file."
);
idCVar r_cinematic_log_ffmpeg("r_cinematic_log_ffmpeg", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Dump logs from FFmpeg libraries into \"log_cinematics.txt\" file."
);
idCVar r_cinematic_log_flush("r_cinematic_log_flush", 0, CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, 
	"Flush log file for cinematics: make sure all messages are written on crash."
);


//note: global variables are used only for developer-level logging
static idFile *logFile = NULL;
static double InvClockTicksPerSecond = -1.0;
static const int MAX_LOG_LEN = 1024;

static void LogPostMessage(const char *message) {
	if (!logFile)
		return;

	static double StartClockTicks = idLib::sys->GetClockTicks();
	double timestamp = (idLib::sys->GetClockTicks() - StartClockTicks) * InvClockTicksPerSecond;
	int64_t microsecs = int64_t(timestamp * 1e+6);
	logFile->Printf("%4d.%03d.%03d: %s\n",
		int(microsecs / 1000000), int(microsecs / 1000 % 1000), int(microsecs % 1000),
		message
	);

	if (r_cinematic_log_flush.GetBool())
		logFile->Flush();
}

void idCinematicFFMpeg::InitCinematic( void ) {
	InvClockTicksPerSecond = 1.0 / idLib::sys->ClockTicksPerSecond();
	//Note: we cannot init logfile, because we cannot read cvars yet
}

void idCinematicFFMpeg::ShutdownCinematic( void ) {
	if (logFile) {
		fileSystem->CloseFile(logFile);
		logFile = NULL;
	}
}

static void LogVPrintf(const char *format, va_list args) {
	char messageBuf[MAX_LOG_LEN];
	idStr::vsnPrintf(messageBuf, MAX_LOG_LEN, format, args);
	LogPostMessage(messageBuf);
}

static void LogPrintf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	LogVPrintf(format, args);
	va_end(args);
}
//in each idCinematic call, this macro is used to start logging 
static void LogCallStart(const char *format, ...) {
	char formatBuf[MAX_LOG_LEN];
	idStr::snPrintf(formatBuf, MAX_LOG_LEN, "\n\n   ====== %s ======", format);
	va_list args;
	va_start(args, format);
	LogVPrintf(formatBuf, args);
	va_end(args);
}

//for logging various timings:
#define TIMER_START(name) double timerStart_##name = idLib::sys->GetClockTicks()
#define TIMER_END(name) ((idLib::sys->GetClockTicks() - timerStart_##name) * InvClockTicksPerSecond)
#define TIMER_END_LOG(name, description) LogPrintf(description " in %0.3lf ms", TIMER_END(name) * 1000.0);

//making sections for calls
#define CALL_START(...) TIMER_START(CALL); LogCallStart(__VA_ARGS__);
#define CALL_END_LOG() TIMER_END_LOG(CALL, "Call in total:");

void idCinematicFFMpeg::LogCallback(void* avcl, int level, const char *fmt, va_list vl) {
	static const char PREFIX[] = "[FFmpeg]   ";
	static const int PREFIX_LEN = sizeof(PREFIX) - 1;
	char messageBuf[MAX_LOG_LEN];
	strcpy(messageBuf, PREFIX);
	vsnprintf(messageBuf + PREFIX_LEN, MAX_LOG_LEN - PREFIX_LEN, fmt, vl);
	int len = strlen(messageBuf);
	if (messageBuf[len-1] == '\n')
		messageBuf[--len] = 0;
	Sys_EnterCriticalSection(CRITICAL_SECTION_THREE);
	LogPostMessage(messageBuf);
	Sys_LeaveCriticalSection(CRITICAL_SECTION_THREE);
}

idCinematicFFMpeg::idCinematicFFMpeg() :
	_looping(false),
	_file(NULL),
	_duration(0),
	_frameRate(0),
	_startTime(-1),
	_status(FMV_EOF),
	_bufferSize(0),
	_formatContext(NULL),
	_videoDecoderContext(NULL),
	_videoStreamIndex(-1),
	_packetTimeOffset(0),
	_highestNextPacketTime(-1),
	_tempFrame(NULL),
	_swsContext(NULL)
{
	// Make sure all codecs are registered
	ExtLibs::av_register_all();
	// Create log file if requested by cvar (only once)
	if (!logFile) {
		if (r_cinematic_log.GetBool() || r_cinematic_log_ffmpeg.GetBool()) {
			logFile = fileSystem->OpenFileWrite("log_cinematics.txt", "fs_devpath", "");
		}
	}
}

idCinematicFFMpeg::~idCinematicFFMpeg()
{}

// Custom AVIOContext wrapper to allow loading directly from an idFile
class idCinematicFFMpeg::VFSIOContext
{
private:
	idFile* _file;
	int _bufferSize;

	AVIOContext* _context;

	// Noncopyable
	VFSIOContext(const VFSIOContext&);
	VFSIOContext& operator=(const VFSIOContext&);

public:
	VFSIOContext(idFile* file) :
		_file(file),
		_bufferSize(4096),
		_context(NULL)
	{
		unsigned char* buffer = static_cast<unsigned char*>(ExtLibs::av_malloc( _bufferSize ));
		_context = ExtLibs::avio_alloc_context(buffer, _bufferSize, 0, this,
			&VFSIOContext::read, NULL, &VFSIOContext::seek);
	}

	~VFSIOContext()
	{
		ExtLibs::av_free( _context->buffer );
		ExtLibs::av_free( _context );
	}

	static int read(void* opaque, unsigned char* buf, int buf_size)
	{
		VFSIOContext* self = static_cast<VFSIOContext*>(opaque);

		return self->_file->Read(buf, buf_size);
	}

	static int64_t seek(void *opaque, int64_t offset, int whence)
	{
		VFSIOContext* self = static_cast<VFSIOContext*>(opaque);

		switch (whence)
		{
		case AVSEEK_SIZE:
			return self->_file->Length();
		case SEEK_SET:
			return self->_file->Seek(offset, FS_SEEK_SET);
		case SEEK_CUR:
			return self->_file->Seek(offset, FS_SEEK_CUR);
		case SEEK_END:
			return self->_file->Seek(offset, FS_SEEK_END);
		default:
			return AVERROR(EINVAL);
		};
	}

	AVIOContext* getContext()
	{
		return _context;
	}
};


bool idCinematicFFMpeg::InitFromFile(const char *qpath, bool looping)
{
	CALL_START("InitFromFile(%s, %d)", qpath, int(looping));
	_path = qpath;
	_packetTimeOffset = 0;
	_looping = looping;

	bool res = OpenAVDecoder();
	CALL_END_LOG();
	return res;
}

bool idCinematicFFMpeg::OpenAVDecoder()
{
	_file = fileSystem->OpenFileRead(_path.c_str());
	if (_file == NULL) {
		common->Warning("Couldn't open video file: %s", _path.c_str());
		return false;
	}
	LogPrintf("Opened file %s", _path.c_str());

	if (r_cinematic_log_ffmpeg.GetBool())
		ExtLibs::av_log_set_callback( idCinematicFFMpeg::LogCallback );

	TIMER_START(ctxAlloc);
	// Use libavformat to detect the video type and stream
	_formatContext = ExtLibs::avformat_alloc_context();
	// To use the VFS we need to set up a custom AV I/O context
	_customIOContext.reset(new VFSIOContext(_file));
	_formatContext->pb = _customIOContext->getContext();
	TIMER_END_LOG(ctxAlloc, "AVFormat context allocated");

	TIMER_START(formatOpen);
	if (ExtLibs::avformat_open_input( &_formatContext, _path.c_str(), NULL, NULL ) < 0) {
		common->Warning("Could not open %s\n", _path.c_str());
		return false;
	}
	TIMER_END_LOG(formatOpen, "AVFormat input opened");

	TIMER_START(findStream);
	if (ExtLibs::avformat_find_stream_info( _formatContext, NULL ) < 0) {
		common->Warning("Could not find stream info %s\n", _path.c_str());
		return false;
	}
	TIMER_END_LOG(findStream, "Found stream info");
	// Find the most suitable video stream 
	_videoStreamIndex = FindBestStreamByType(AVMEDIA_TYPE_VIDEO);
	if (_videoStreamIndex < 0) {
		common->Warning("Could not find video stream in %s\n", _path.c_str());
		return false;
	}

	AVStream* videoStream = _formatContext->streams[_videoStreamIndex];
	_videoDecoderContext = videoStream->codec;

	// Some video formats (like the beloved ROQ) don't provider a sane duration value, so let's check
	if (videoStream->duration != AV_NOPTS_VALUE && videoStream->duration >= 0) {
		// Calculate duration in milliseconds
		_duration = static_cast<int>(videoStream->duration * av_q2d(videoStream->time_base) * 1000);
	}
	else {
		_duration = 100000; // use a hardcoded value, just like the good old idCinematicLocal
	}

	_frameRate = static_cast<float>(av_q2d(_videoDecoderContext->framerate));

	// Allocate target buffer for RGBA data
	_bufferSize = _videoDecoderContext->width * _videoDecoderContext->height * 4;

	LogPrintf("Found video stream %d of duration %d ms", _videoStreamIndex, _duration);
	LogPrintf("Video has resolution %d x %d and framerate %0.2f", _videoDecoderContext->width, _videoDecoderContext->height, _frameRate);

	TIMER_START(allocTemp);
	// Allocate the temporary frame for decoding
	_tempFrame = ExtLibs::av_frame_alloc();
	if (!_tempFrame) {
		common->Warning("Could not allocate video frame\n");
		return false;
	}
	ExtLibs::av_init_packet( &_packet );
	_packet.data = NULL;
	_packet.size = 0;
	TIMER_END_LOG(allocTemp, "Alloc frame & init packet");

	_status = FMV_PLAY;

	TIMER_START(createSwsCtx);
	// Set up the scaling context used to re-encode the images
	_swsContext = ExtLibs::sws_getContext(
		_videoDecoderContext->width, _videoDecoderContext->height,
		_videoDecoderContext->pix_fmt,
		_videoDecoderContext->width, _videoDecoderContext->height,
		AV_PIX_FMT_RGBA,
		SWS_BICUBIC, NULL, NULL, NULL
	);
	TIMER_END_LOG(createSwsCtx, "Created sws context");

	return true;
}

void idCinematicFFMpeg::CloseAVDecoder()
{
	TIMER_START(closeAll);
	_status = FMV_IDLE;

	_duration = 0;
	_frameRate = 0;
	_bufferSize = 0;

	if (_tempFrame != NULL)
		ExtLibs::av_frame_free( &_tempFrame );
	_tempFrame = NULL;

	if (_swsContext != NULL)
		ExtLibs::sws_freeContext( _swsContext );
	_swsContext = NULL;

	_videoStreamIndex = -1;

	if (_videoDecoderContext != NULL)
		ExtLibs::avcodec_close( _videoDecoderContext );
	_videoDecoderContext = NULL;

	if (_formatContext != NULL)
		ExtLibs::avformat_close_input(&_formatContext);
	_formatContext = NULL;

	_customIOContext.reset();

	ExtLibs::av_log_set_callback( ExtLibs::av_log_default_callback );

	if (_file != NULL)
		fileSystem->CloseFile(_file);
	_file = NULL;
	TIMER_END_LOG(closeAll, "Freed all FFmpeg resources");
}

int idCinematicFFMpeg::FindBestStreamByType(AVMediaType type)
{
	TIMER_START(findBestStream);
	int streamIndex = ExtLibs::av_find_best_stream( _formatContext, type, -1, -1, NULL, 0 );
	if (streamIndex < 0) {
		common->Warning( "Could not find %s stream in input.\n", ExtLibs::av_get_media_type_string( type ) );
		return -1;
	}
	TIMER_END_LOG(findBestStream, "Found best stream");
	AVStream* st = _formatContext->streams[streamIndex];

	AVCodecID codecId = st->codec->codec_id;
	LogPrintf( "Stream %d is encoded with codec %d: %s", streamIndex, codecId, ExtLibs::avcodec_get_name( codecId ) );
	// find decoder for the stream
	TIMER_START(findDecoder);
	AVCodec* dec = ExtLibs::avcodec_find_decoder( codecId );
	if (!dec) {
		common->Warning( "Failed to find %s:%s decoder\n", ExtLibs::av_get_media_type_string( type ), ExtLibs::avcodec_get_name( codecId ) );
		return AVERROR(EINVAL);
	}
	TIMER_END_LOG(findDecoder, "Found decoder");

	TIMER_START(openCodec);
	AVDictionary *opts = NULL;
	// Use API_MODE_NEW_API_REF_COUNT
	ExtLibs::av_dict_set( &opts, "refcounted_frames", "1", 0 );
	if (ExtLibs::avcodec_open2( st->codec, dec, &opts ) < 0) {
		common->Warning( "Failed to open %s:%s codec\n", ExtLibs::av_get_media_type_string( type ), ExtLibs::avcodec_get_name( codecId ) );
		return -1;
	}
	TIMER_END_LOG(openCodec, "Opened decoder");

	// success
	return streamIndex;
}

bool idCinematicFFMpeg::ReadPacket(AVPacket& avpkt) {
	TIMER_START(readPacket);
	int ret = ExtLibs::av_read_frame( _formatContext, &_packet );

	AVStream* st = _formatContext->streams[_packet.stream_index];
	AVMediaType type = ExtLibs::avcodec_get_type( st->codec->codec_id );
	TIMER_END_LOG(readPacket, "Read packet");
	LogPrintf( "Packet: stream = %d (%s)  size = %d", _packet.stream_index, ExtLibs::av_get_media_type_string( type ), _packet.size );
	LogPrintf("  DTS = %lld  PTS = %lld  dur = %d", _packet.dts, _packet.pts, _packet.duration);
	return ret >= 0;
}

int idCinematicFFMpeg::DecodePacket(AVPacket& avpkt, byte* targetRGBA, bool& frameDecoded)
{
	int decoded = avpkt.size;
	frameDecoded = false;

	TIMER_START(decode);
	// Decode the packet
	int got_frame = 0;
	int ret = ExtLibs::avcodec_decode_video2( _videoDecoderContext, _tempFrame, &got_frame, &avpkt );
	if (ret < 0) {
		common->Warning("Error decoding video frame (%d)\n", ret);
		return ret;
	}
	TIMER_END_LOG(decode, "Packet decoded");

	if (got_frame) {
		frameDecoded = true;

		TIMER_START(swsScale);
		// Note: AV_PIX_FMT_RGBA format is non-planar
		// So all the colors are stored in interleaved way: R, G, B, A, R, G, ...
		// That's why swscale expects only single destination pointer + stride
		uint8_t* const dstPtr[1] = { targetRGBA };
		int lineWidth[1] = { _videoDecoderContext->width * 4 };
		ExtLibs::sws_scale(
			_swsContext,
			_tempFrame->data, _tempFrame->linesize, 0, _tempFrame->height,
			dstPtr, lineWidth
		);
		TIMER_END_LOG(swsScale, "Converted to RGBA");
	}

	return decoded;
}

cinData_t idCinematicFFMpeg::ImageForTime(int milliseconds) {
	CALL_START("ImageForTime(%d)", milliseconds);
	cinData_t res = GetFrame(milliseconds);
	CALL_END_LOG();
	return res;
}

cinData_t idCinematicFFMpeg::GetFrame(int milliseconds) {
	if (milliseconds < 0)
		milliseconds = 0;

	if (_startTime < 0) {
		LogPrintf("Start time is not set, setting it to now (%d)", milliseconds);
		_startTime = milliseconds;
	}

	cinData_t data;
	memset(&data, 0, sizeof(data));

	int requestedVideoTime = milliseconds - _startTime;
	LogPrintf("Video time: %d", requestedVideoTime);

	// Ensure we have at least the first buffer decoded
	if (_buffer.timeStamp == -1) {
		if (!ReadFrame(_buffer)) {
			common->Printf("No more frames available.\n");
			return data; // out of frames
		}
	}

	// Requests for frames before the buffered ones are not served right now
	if (requestedVideoTime < _buffer.timeStamp) {
		//common->Printf("Waiting to get in sync, requested time is too small.\n");
		return data;
	}

	// Requested video time >= _buffer.timeStamp past this point

	// Also load the second buffer, we need it to check whether it is closer
	if (_bufferNext.timeStamp == -1) {
		ReadFrame(_bufferNext);
	}

	//common->Printf("BEFORE: Reqested Time: %d ms, buf: %d ms, delta: %d, next: %d\n",
	//               requestedVideoTime, _buffer.timeStamp, requestedVideoTime - _buffer.timeStamp, _bufferNext.timeStamp);

	// Keep shifting buffers to the front if they are more suitable
	while (_bufferNext.timeStamp != -1 && requestedVideoTime >= _bufferNext.timeStamp) {
		// buffernext will take the place of buffer, which can be discarded
		_buffer = _bufferNext;
		_bufferNext.timeStamp = -1;

		// Attempt to load the next buffer
		if (!ReadFrame(_bufferNext))
			break;
	}

	// At this point the first buffer should be aligned to be <= requestedTime
	assert(requestedVideoTime >= _buffer.timeStamp);

	if (_buffer.timeStamp != -1) {
		data.imageWidth = _videoDecoderContext->width;
		data.imageHeight = _videoDecoderContext->height;
		data.status = FMV_PLAY;

		// Check which frame is closest to the requested time
		if (_bufferNext.timeStamp != -1) {
			int delta = idMath::Abs(requestedVideoTime - _buffer.timeStamp);
			int deltaNext = idMath::Abs(requestedVideoTime - _bufferNext.timeStamp);

			if (deltaNext < delta) {
				// Next frame is closer
				data.image = _bufferNext.rgbaImage.get();

				//common->Printf("SERVENEXT: Reqested Time: %d ms, buf: %d ms, delta: %d, next: %d\n",
				//               requestedVideoTime, _buffer.timeStamp, requestedVideoTime - _buffer.timeStamp, _bufferNext.timeStamp);

				return data;
			}
		}

		// Return this frame
		data.image = _buffer.rgbaImage.get();

		//common->Printf("SERVE: Reqested Time: %d ms, buf: %d ms, delta: %d, next: %d\n", 
		//               requestedVideoTime, _buffer.timeStamp, requestedVideoTime - _buffer.timeStamp, _bufferNext.timeStamp);

		return data;
	}

	// We don't have a valid _buffer anymore, return an empty frame
	return data;
}

bool idCinematicFFMpeg::ReadFrame(FrameBuffer& targetBuffer)
{
	// Invalidate the buffer whatever comes
	targetBuffer.timeStamp = -1;
	targetBuffer.duration = 0;

	if (_status == FMV_EOF) {
		LogPrintf("Video ended: no more frames");
		if (!_looping)
			return false;

		// Add one full video duration as offset
		if (_highestNextPacketTime > 0)
			_packetTimeOffset += _highestNextPacketTime;

		LogPrintf("Looped video: start from the beginning");
		// EOF, so let's rewind the whole thing
		CloseAVDecoder();
		OpenAVDecoder();
	}

	bool frameDecoded = false;

	while (ReadPacket(_packet)) {
		if (_packet.stream_index != _videoStreamIndex) {
			ExtLibs::av_free_packet( &_packet );
			continue;
		}

		do {
			// Ensure that the target buffer has an RGBA plane allocated
			if (!targetBuffer.rgbaImage) {
				TIMER_START(allocImage);
				targetBuffer.rgbaImage = std::shared_ptr<byte>(static_cast<byte*>(Mem_Alloc(_bufferSize)), Mem_Free);
				TIMER_END_LOG(allocImage, "Allocated RGBA image buffer");
			}

			int ret = DecodePacket(_packet, targetBuffer.rgbaImage.get(), frameDecoded);
			if (ret < 0)
				break;

			_packet.data += ret;
			_packet.size -= ret;

			if (frameDecoded) {
				// Save the time stamp into the buffer
				targetBuffer.timeStamp = CalculatePacketTime();
				targetBuffer.duration = _packet.duration * av_q2d(_formatContext->streams[_videoStreamIndex]->time_base) * 1000;
				LogPrintf("Decoded frame with timestamp: %d + %d", targetBuffer.timeStamp, targetBuffer.duration);

				ExtLibs::av_free_packet( &_packet );
				return true;
			}
		} while (_packet.size > 0);
	}

	LogPrintf("Flushing codec internal buffer...");

	// flush cached frames
	_packet.data = NULL;
	_packet.size = 0;

	DecodePacket(_packet, targetBuffer.rgbaImage.get(), frameDecoded);

	if (frameDecoded) {
		// Save the time stamp, we might re-use this buffer
		targetBuffer.timeStamp = CalculatePacketTime();
		targetBuffer.duration = _packet.duration * av_q2d(_formatContext->streams[_videoStreamIndex]->time_base) * 1000;
		LogPrintf("Decoded frame with timestamp: %d + %d", targetBuffer.timeStamp, targetBuffer.duration);

		ExtLibs::av_free_packet( &_packet );
		return true;
	}

	ExtLibs::av_free_packet( &_packet );

	// We seem to be out of frames
	_status = FMV_EOF;

	return false;
}

int idCinematicFFMpeg::CalculatePacketTime()
{
	//TODO: we rely here on the fact that _tempFrame is still current
	int64_t pts = ExtLibs::av_frame_get_best_effort_timestamp( _tempFrame );

	double timeBase = av_q2d(_formatContext->streams[_videoStreamIndex]->time_base);
	double frameTime = pts * timeBase;

	// Calculate the time in msecs
	int packetTime = static_cast<int>(frameTime * 1000);

	// Estimate the time of the next packet, even if there would be none
	int estimatedNextPacketTime = packetTime + (_packet.duration * timeBase * 1000);

	// Keep track of the highest packet time so far
	if (estimatedNextPacketTime > _highestNextPacketTime)
		_highestNextPacketTime = estimatedNextPacketTime;

	// For looping, we just add an offset to packets such that their time is ever-ascending
	return packetTime + _packetTimeOffset;
}

int idCinematicFFMpeg::AnimationLength()
{
	return _duration;
}

void idCinematicFFMpeg::Close()
{
	CALL_START("Close");
	CloseAVDecoder();

	_packetTimeOffset = 0;
	_highestNextPacketTime = -1;
	_startTime = -1;
	CALL_END_LOG();
}

void idCinematicFFMpeg::ResetTime(int time)
{
	CALL_START("ResetTime(%d)", time);

	// Even though a time is passed to this function, it seems we're just ditching it
	// and revert the start time to the reference time in the render backend, since
	// this backend time is the one that is passed to ImageForTime()
	_startTime = (backEnd.viewDef) ? 1000 * backEnd.viewDef->floatTime : -1;
	LogPrintf("Backend time: %d", _startTime);

	// Reset the loop time offset
	_packetTimeOffset = 0;
	_highestNextPacketTime = -1;

	//common->Printf("Resetting time to %d, startTime is now %d\n", time, _startTime);

#if ENABLE_AV_SEEKING
	if (av_seek_frame(_formatContext, _videoStreamIndex, 0, AVSEEK_FLAG_BYTE) < 0) {
		common->Warning("Cannot seek in video stream.");
	}

	// Flush buffers before seeking
	avcodec_flush_buffers(_videoDecoderContext);
#else
	// Just re-init the whole stuff, don't bother seeking
	// as it doesn't seem to work reliably with ROQs.
	CloseAVDecoder();
	OpenAVDecoder();

	// CloseAVDecoder won't touch the buffers, invalidate them
	_buffer.timeStamp = -1;
	_buffer.duration = 0;
	_bufferNext.timeStamp = -1;
	_bufferNext.duration = 0;
#endif
	CALL_END_LOG();
}
