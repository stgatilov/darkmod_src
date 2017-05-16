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

#include "precompiled_engine.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "CinematicFFMpeg.h"
#include "Image.h"
#include "tr_local.h"

extern "C"
{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>

}

idCinematicFFMpeg::idCinematicFFMpeg() :
	_looping(false),
	_file(NULL),
	_duration(0),
	_frameRate(0),
	_startTime(0),
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
	av_register_all();
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
		unsigned char* buffer = static_cast<unsigned char*>(av_malloc(_bufferSize));
		_context = avio_alloc_context(buffer, _bufferSize, 0, this,
			&VFSIOContext::read, NULL, &VFSIOContext::seek);
	}

	~VFSIOContext()
	{
		av_free(_context->buffer);
		av_free(_context);
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
	_path = qpath;
	_packetTimeOffset = 0;
	_looping = looping;

	return OpenAVDecoder();
}

bool idCinematicFFMpeg::OpenAVDecoder()
{
	_file = fileSystem->OpenFileRead(_path.c_str());

	if (_file == NULL)
	{
		common->Warning("Couldn't open video file: %s", _path.c_str());
		return false;
	}

#if ENABLE_AV_DEBUG_LOGGING
	av_log_set_callback(idCinematicFFMpeg::LogCallback);
#endif

	// Use libavformat to detect the video type and stream
	_formatContext = avformat_alloc_context();

	// To use the VFS we need to set up a custom AV I/O context
	_customIOContext.reset(new VFSIOContext(_file));

	_formatContext->pb = _customIOContext->getContext();

	if (avformat_open_input(&_formatContext, _path.c_str(), NULL, NULL) < 0)
	{
		common->Warning("Could not open %s\n", _path.c_str());
		return false;
	}

	if (avformat_find_stream_info(_formatContext, NULL) < 0)
	{
		common->Warning("Could not find stream info %s\n", _path.c_str());
		return false;
	}

	// Find the most suitable video stream 
	_videoStreamIndex = FindBestStreamByType(AVMEDIA_TYPE_VIDEO);

	if (_videoStreamIndex < 0)
	{
		common->Warning("Could not find video stream in %s\n", _path.c_str());
		return false;
	}

	AVStream* videoStream = _formatContext->streams[_videoStreamIndex];
	_videoDecoderContext = videoStream->codec;

	// Some video formats (like the beloved ROQ) don't provider a sane duration value, so let's check
	if (videoStream->duration != AV_NOPTS_VALUE && videoStream->duration >= 0)
	{
		// Calculate duration in milliseconds
		_duration = static_cast<int>(videoStream->duration * av_q2d(videoStream->time_base) * 1000);
	}
	else
	{
		_duration = 100000; // use a hardcoded value, just like the good old idCinematicLocal
	}

	_frameRate = static_cast<float>(av_q2d(_videoDecoderContext->framerate));

	// Allocate target buffer for RGBA data
	_bufferSize = _videoDecoderContext->width * _videoDecoderContext->height * 4;

	// Allocate the temporary frame for decoding
	_tempFrame = av_frame_alloc();

	if (!_tempFrame)
	{
		common->Warning("Could not allocate video frame\n");
		return false;
	}

	av_init_packet(&_packet);
	_packet.data = NULL;
	_packet.size = 0;

	_status = FMV_PLAY;

	// Set up the scaling context used to re-encode the images
	_swsContext = sws_getContext(_videoDecoderContext->width, _videoDecoderContext->height,
		_videoDecoderContext->pix_fmt,
		_videoDecoderContext->width, _videoDecoderContext->height,
		AV_PIX_FMT_RGBA,
		SWS_BICUBIC, NULL, NULL, NULL);

	return true;
}

void idCinematicFFMpeg::CloseAVDecoder()
{
	_status = FMV_IDLE;

	_duration = 0;
	_frameRate = 0;
	_bufferSize = 0;

	if (_tempFrame != NULL)
	{
		av_frame_free(&_tempFrame);
		_tempFrame = NULL;
	}

	if (_swsContext != NULL)
	{
		sws_freeContext(_swsContext);
		_swsContext = NULL;
	}

	_videoStreamIndex = -1;

	if (_videoDecoderContext != NULL)
	{
		avcodec_close(_videoDecoderContext);
		_videoDecoderContext = NULL;
	}

	if (_formatContext != NULL)
	{
		avformat_close_input(&_formatContext);
		_formatContext = NULL;
	}

	_customIOContext.reset();

#if ENABLE_AV_DEBUG_LOGGING
	av_log_set_callback(av_log_default_callback);
#endif

	if (_file != NULL)
	{
		fileSystem->CloseFile(_file);
		_file = NULL;
	}
}

#if ENABLE_AV_DEBUG_LOGGING
static idStr FFMPegLog;

void idCinematicFFMpeg::LogCallback(void* avcl, int level, const char *fmt, va_list vl)
{
	Sys_EnterCriticalSection(CRITICAL_SECTION_THREE);

	char buf[5000];

	vsprintf(buf, fmt, vl);

	FFMPegLog.Append(buf);

	Sys_LeaveCriticalSection(CRITICAL_SECTION_THREE);
}
#endif

int idCinematicFFMpeg::FindBestStreamByType(AVMediaType type)
{
	int streamIndex = av_find_best_stream(_formatContext, type, -1, -1, NULL, 0);

	if (streamIndex < 0)
	{
		common->Warning("Could not find %s stream in input.\n", av_get_media_type_string(type));
		return -1;
	}

	AVStream* st = _formatContext->streams[streamIndex];

	// find decoder for the stream
	AVCodec* dec = avcodec_find_decoder(st->codec->codec_id);

	if (!dec)
	{
		common->Warning("Failed to find %s codec\n", av_get_media_type_string(type));
		return AVERROR(EINVAL);
	}

	AVDictionary *opts = NULL;

	// Use API_MODE_NEW_API_REF_COUNT
	av_dict_set(&opts, "refcounted_frames", "1", 0);

	if (avcodec_open2(st->codec, dec, &opts) < 0)
	{
		common->Warning("Failed to open %s codec\n", av_get_media_type_string(type));
		return -1;
	}

	// success
	return streamIndex;
}

int idCinematicFFMpeg::DecodePacket(AVPacket& avpkt, byte* targetRGBA, bool& frameDecoded)
{
	int decoded = avpkt.size;

	frameDecoded = false;

	// Decode the packet
	int got_frame = 0;

	int ret = avcodec_decode_video2(_videoDecoderContext, _tempFrame, &got_frame, &avpkt);

	if (ret < 0)
	{
		common->Warning("Error decoding video frame (%d)\n", ret);
		return ret;
	}

	if (got_frame)
	{
		frameDecoded = true;

		int lineWidths[4] = { _videoDecoderContext->width * 4, _videoDecoderContext->width * 4,
			_videoDecoderContext->width * 4, _videoDecoderContext->width * 4 };

		// We pass only one RGBA plane to this method even though it's expecting a maximum of 4 (I think)
		// If this is causing crashes, this might be a reason for it
		sws_scale(_swsContext, _tempFrame->data, _tempFrame->linesize,
			0, _videoDecoderContext->height, static_cast<uint8_t* const*>(&targetRGBA), lineWidths);
	}

	return decoded;
}

cinData_t idCinematicFFMpeg::ImageForTime(int milliseconds)
{
	if (milliseconds < 0) milliseconds = 0;

	cinData_t data;
	memset(&data, 0, sizeof(data));

	int requestedVideoTime = milliseconds - _startTime;

	//common->Printf("Requested %d, start time: %d\n", milliseconds, _startTime);

	// Ensure we have at least the first buffer decoded
	if (_buffer.timeStamp == -1)
	{
		if (!ReadFrame(_buffer))
		{
			common->Printf("No more frames available.\n");
			return data; // out of frames
		}
	}

	// Requests for frames before the buffered ones are not served right now
	if (requestedVideoTime < _buffer.timeStamp)
	{
		//common->Printf("Waiting to get in sync, requested time is too small.\n");
		return data;
	}

	// Requested video time >= _buffer.timeStamp past this point

	// Also load the second buffer, we need it to check whether it is closer
	if (_bufferNext.timeStamp == -1)
	{
		ReadFrame(_bufferNext);
	}

	//common->Printf("BEFORE: Reqested Time: %d ms, buf: %d ms, delta: %d, next: %d\n",
	//               requestedVideoTime, _buffer.timeStamp, requestedVideoTime - _buffer.timeStamp, _bufferNext.timeStamp);

	// Keep shifting buffers to the front if they are more suitable
	while (_bufferNext.timeStamp != -1 && requestedVideoTime >= _bufferNext.timeStamp)
	{
		// buffernext will take the place of buffer, which can be discarded
		_buffer = _bufferNext;
		_bufferNext.timeStamp = -1;

		// Attempt to load the next buffer
		if (!ReadFrame(_bufferNext)) break;
	}

	// At this point the first buffer should be aligned to be <= requestedTime
	assert(requestedVideoTime >= _buffer.timeStamp);

	if (_buffer.timeStamp != -1)
	{
		data.imageWidth = _videoDecoderContext->width;
		data.imageHeight = _videoDecoderContext->height;
		data.status = FMV_PLAY;

		// Check which frame is closest to the requested time
		if (_bufferNext.timeStamp != -1)
		{
			int delta = idMath::Abs(requestedVideoTime - _buffer.timeStamp);
			int deltaNext = idMath::Abs(requestedVideoTime - _bufferNext.timeStamp);

			if (deltaNext < delta)
			{
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

	if (_status == FMV_EOF)
	{
		if (!_looping)
		{
			return false;
		}

		// Add one full video duration as offset
		if (_highestNextPacketTime > 0)
		{
			_packetTimeOffset += _highestNextPacketTime;
		}

		// EOF, so let's rewind the whole thing
		CloseAVDecoder();
		OpenAVDecoder();
	}

	bool frameDecoded = false;

	while (av_read_frame(_formatContext, &_packet) >= 0)
	{
		//common->Printf("Read a packet: %d\n", CalculatePacketTime());

		if (_packet.stream_index != _videoStreamIndex)
		{
			av_free_packet(&_packet);
			continue;
		}

		do
		{
			// Ensure that the target buffer has an RGBA plane allocated
			if (!targetBuffer.rgbaImage)
			{
				targetBuffer.rgbaImage = std::shared_ptr<byte>(static_cast<byte*>(Mem_Alloc(_bufferSize)), Mem_Free);
			}

			int ret = DecodePacket(_packet, targetBuffer.rgbaImage.get(), frameDecoded);

			if (ret < 0)
				break;

			_packet.data += ret;
			_packet.size -= ret;

			if (frameDecoded)
			{
				//common->Printf("FRAME: %d\n", CalculatePacketTime());

				// Save the time stamp into the buffer
				targetBuffer.timeStamp = CalculatePacketTime();
				targetBuffer.duration = _packet.duration * av_q2d(_formatContext->streams[_videoStreamIndex]->time_base) * 1000;

				av_free_packet(&_packet);
				return true;
			}
		} while (_packet.size > 0);
	}

	// flush cached frames
	_packet.data = NULL;
	_packet.size = 0;

	DecodePacket(_packet, targetBuffer.rgbaImage.get(), frameDecoded);

	if (frameDecoded)
	{
		// Save the time stamp, we might re-use this buffer
		targetBuffer.timeStamp = CalculatePacketTime();
		targetBuffer.duration = _packet.duration * av_q2d(_formatContext->streams[_videoStreamIndex]->time_base) * 1000;

		av_free_packet(&_packet);
		return true;
	}

	av_free_packet(&_packet);

	// We seem to be out of frames
	_status = FMV_EOF;

	return false;
}

int idCinematicFFMpeg::CalculatePacketTime()
{
	double timeBase = av_q2d(_formatContext->streams[_videoStreamIndex]->time_base);
	double frameTime = _packet.pts * timeBase;

	// Calculate the time in msecs
	int packetTime = static_cast<int>(frameTime * 1000);

	// Estimate the time of the next packet, even if there would be none
	int estimatedNextPacketTime = packetTime + (_packet.duration * timeBase * 1000);

	// Keep track of the highest packet time so far
	if (estimatedNextPacketTime > _highestNextPacketTime)
	{
		_highestNextPacketTime = estimatedNextPacketTime;
	}

	// For looping, we just add an offset to packets such that their time is ever-ascending
	return packetTime + _packetTimeOffset;
}

int idCinematicFFMpeg::AnimationLength()
{
	return _duration;
}

void idCinematicFFMpeg::Close()
{
	CloseAVDecoder();

	_packetTimeOffset = 0;
	_highestNextPacketTime = -1;
}

void idCinematicFFMpeg::ResetTime(int time)
{
	// Even though a time is passed to this function, it seems we're just ditching it
	// and revert the start time to the reference time in the render backend, since
	// this backend time is the one that is passed to ImageForTime()
	_startTime = (backEnd.viewDef) ? 1000 * backEnd.viewDef->floatTime : -1;

	// Reset the loop time offset
	_packetTimeOffset = 0;
	_highestNextPacketTime = -1;

	//common->Printf("Resetting time to %d, startTime is now %d\n", time, _startTime);

#if ENABLE_AV_SEEKING
	if (av_seek_frame(_formatContext, _videoStreamIndex, 0, AVSEEK_FLAG_BYTE) < 0)
	{
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
}
