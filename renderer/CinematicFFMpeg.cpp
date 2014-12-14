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
#include <libswscale/swscale.h>

}

idCinematicFFMpeg::idCinematicFFMpeg() :
    _duration(0),
    _frameRate(0),
    _startTime(0),
    _status(FMV_EOF),
    _formatContext(NULL),
    _videoDecoderContext(NULL),
    _tempFrame(NULL),
    _swsContext(NULL)
{
    // Make sure all codecs are registered
    av_register_all();
}

idCinematicFFMpeg::~idCinematicFFMpeg()
{}

bool idCinematicFFMpeg::InitFromFile(const char *qpath, bool looping)
{
    av_log_set_callback(idCinematicFFMpeg::LogCallback);

    _path = fileSystem->RelativePathToOSPath(qpath);

    // TESTING
    _path = "C:\\Games\\Doom3\\darkmod\\video\\trailer.mp4";

    // Use libavformat to detect the video type and stream
    _formatContext = avformat_alloc_context();

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
    
    // Calculate duration in milliseconds and the framerate
    _duration = static_cast<int>(videoStream->duration * av_q2d(videoStream->time_base) * 1000);
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

static idStr FFMPegLog;

void idCinematicFFMpeg::LogCallback(void* avcl, int level, const char *fmt, va_list vl)
{
    Sys_EnterCriticalSection(CRITICAL_SECTION_THREE);

    char buf[5000];

    vsprintf(buf, fmt, vl);

    FFMPegLog.Append(buf);

    Sys_LeaveCriticalSection(CRITICAL_SECTION_THREE);
}

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

int idCinematicFFMpeg::DecodePacket(AVPacket& avpkt, byte* targetRGBA, int *got_frame, int cached)
{
    int decoded = avpkt.size;

    *got_frame = 0;

    /* decode video frame */
    int ret = avcodec_decode_video2(_videoDecoderContext, _tempFrame, got_frame, &avpkt);

    if (ret < 0) 
    {
        common->Warning("Error decoding video frame (%d)\n", ret);
        return ret;
    }

    if (*got_frame)
    {
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

    /*if (_status == FMV_EOF)
    {
        // out of data
        return data; 
    }*/

    int requestedVideoTime = milliseconds - _startTime;

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
        common->Printf("Waiting to get in sync, requested time is too small.\n");
        return data;
    }

    // Requested video time >= _buffer.timeStamp past this point

    // Also load the second buffer, we need it to check whether it is closer
    if (_bufferNext.timeStamp == -1)
    {
        ReadFrame(_bufferNext);
    }

    // Keep shifting buffers to the front if they are more suitable
    while (_bufferNext.timeStamp != -1 && requestedVideoTime >= _bufferNext.timeStamp)
    {
        // buffernext will take the place of buffer, which can be discarded
        _buffer = _bufferNext;
        _bufferNext.timeStamp = -1;

        // Attempt to load the next buffer
        if (!ReadFrame(_bufferNext)) break;
    }

    // At this point the first buffer should be the most suitable one
    assert(requestedVideoTime >= _buffer.timeStamp);

    if (_buffer.timeStamp != -1)
    {
        // Return this frame
        data.image = _buffer.rgbaImage.get();
        data.imageWidth = _videoDecoderContext->width;
        data.imageHeight = _videoDecoderContext->height;

        data.status = FMV_PLAY;

        common->Printf("Reqested Time: %d ms, served: %d ms\n", requestedVideoTime, _buffer.timeStamp);

        return data;
    }
     
    // We don't have a valid _buffer anymore, return an empty frame
    return data;
}

bool idCinematicFFMpeg::ReadFrame(FrameBuffer& targetBuffer)
{
    if (_status == FMV_EOF)
    {
        return false;
    }

    int got_frame = 0;

    while (av_read_frame(_formatContext, &_packet) >= 0)
    {
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

            int ret = DecodePacket(_packet, targetBuffer.rgbaImage.get(), &got_frame, 0);

            if (ret < 0)
                break;

            _packet.data += ret;
            _packet.size -= ret;

            if (got_frame)
            {
                // Save the time stamp into the buffer
                targetBuffer.timeStamp = GetPacketTime();

                av_free_packet(&_packet);
                return true;
            }
        } 
        while (_packet.size > 0);
    }

    /* flush cached frames */
    _packet.data = NULL;
    _packet.size = 0;

    DecodePacket(_packet, targetBuffer.rgbaImage.get(), &got_frame, 1);

    if (got_frame)
    {
        // Save the time stamp, we might re-use this buffer
        targetBuffer.timeStamp = GetPacketTime();
            
        av_free_packet(&_packet);
        return true;
    }

    targetBuffer.timeStamp = -1;
    av_free_packet(&_packet);

    // We seem to be out of frames
    _status = FMV_EOF;

    return false;
}

int idCinematicFFMpeg::GetPacketTime()
{
    double frameTime = _packet.pts * av_q2d(_formatContext->streams[_packet.stream_index]->time_base);
    return static_cast<int>(frameTime * 1000);
}

int idCinematicFFMpeg::AnimationLength()
{
    return _duration;
}

void idCinematicFFMpeg::Close()
{
    _buffer.rgbaImage.reset();
    _buffer.timeStamp = -1;

    _bufferNext.rgbaImage.reset();
    _bufferNext.timeStamp = -1;

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
}

void idCinematicFFMpeg::ResetTime(int time)
{
    _startTime = (backEnd.viewDef) ? 1000 * backEnd.viewDef->floatTime : -1;
    _status = FMV_PLAY;

#if 0
    // We need to get the actual time relative to video start
    // _startTime will have been set by a previous ResetTime() call.
    int timeRelativeToStart = milliseconds - _startTime;

    double seekTimeSeconds = timeRelativeToStart / 1000;
    int64_t seekPts = seekTimeSeconds / av_q2d(_formatContext->streams[_videoStreamIndex]->time_base);

    // Flush buffers before seeking
    avcodec_flush_buffers(_videoDecoderContext);

    common->Printf("Seeking to time: %g secs\n", seekTimeSeconds);

    if (av_seek_frame(_formatContext, _videoStreamIndex, seekPts, AVSEEK_FLAG_ANY) < 0)
    {
        common->Warning("Cannot seek in video stream.");
        return data;
    }
#endif
}
