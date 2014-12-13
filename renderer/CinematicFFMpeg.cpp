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
    _formatContext(NULL),
    _videoDecoderContext(NULL),
    _startTime(0),
    _status(FMV_EOF)
{
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

    AVStream* video_stream = _formatContext->streams[_videoStreamIndex];
    _videoDecoderContext = video_stream->codec;
    
    // Allocate target buffer for RGBA data
    int bufferSize = _videoDecoderContext->width * _videoDecoderContext->height * 4;

    _rgbaBuffer = std::shared_ptr<byte>(static_cast<byte*>(Mem_Alloc(bufferSize)), Mem_Free);

    av_init_packet(&_packet);
    _packet.data = NULL;
    _packet.size = 0;

    _status = FMV_PLAY;

    return true;
}

void idCinematicFFMpeg::LogCallback(void* avcl, int level, const char *fmt, va_list vl)
{
    if (level == AV_LOG_INFO)
    {
        common->Printf(fmt, vl);
    }

    if (level == AV_LOG_WARNING)
    {
        common->Warning(fmt, vl);
    }

    if (level == AV_LOG_ERROR)
    {
        common->Error(fmt, vl);
    }
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

static int decode_packet(AVPacket& avpkt, byte* targetRGBA, int *got_frame, int cached, AVCodecContext* video_dec_ctx)
{
    static int video_frame_count = 0;
    int ret = 0;
    int decoded = avpkt.size;

    static SwsContext* swsContext = NULL;

    AVFrame* tempFrame = av_frame_alloc();
    if (!tempFrame)
    {
        common->Warning("Could not allocate video frame\n");
        return -1;
    }

    *got_frame = 0;

    /* decode video frame */
    ret = avcodec_decode_video2(video_dec_ctx, tempFrame, got_frame, &avpkt);

    if (ret < 0) 
    {
        common->Warning("Error decoding video frame (%d)\n", ret);
        av_frame_free(&tempFrame);
        return ret;
    }

    if (*got_frame)
    {
        common->Printf("video_frame %s n:%d coded_n:%d\n",
                       cached ? "(cached)" : "",
                       video_frame_count++, tempFrame->coded_picture_number);
#if 0        
        /* copy decoded frame to destination buffer:
        * this is required since rawvideo expects non aligned data */
        av_image_copy(video_dst_data, video_dst_linesize,
                      (const uint8_t **)(tempFrame->data), tempFrame->linesize,
                        video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
#endif
        if (swsContext == NULL)
        {
            swsContext = sws_getContext(video_dec_ctx->width, video_dec_ctx->height,
                                        video_dec_ctx->pix_fmt,
                                        video_dec_ctx->width, video_dec_ctx->height,
                                        AV_PIX_FMT_RGBA,
                                        SWS_BICUBIC, NULL, NULL, NULL);
        }

        int lineWidths[4] = { video_dec_ctx->width * 4, video_dec_ctx->width * 4, video_dec_ctx->width * 4, video_dec_ctx->width * 4 };

        sws_scale(swsContext, (const uint8_t * const *)tempFrame->data, tempFrame->linesize,
                  0, video_dec_ctx->height, static_cast<uint8_t* const*>(&targetRGBA), lineWidths);
    }

    av_frame_free(&tempFrame);

    return decoded;
}

cinData_t idCinematicFFMpeg::ImageForTime(int milliseconds)
{
    if (milliseconds < 0)
    {
        milliseconds = 0;
    }

    cinData_t data;
    memset(&data, 0, sizeof(data));

#if 0
    for (unsigned int i = 0; i < avFormat->nb_streams; ++i)
    {
        AVStream* stream = avFormat->streams[i];		// pointer to a structure describing the stream
        AVMediaType codecType = stream->codec->codec_type;	// the type of data in this stream, notable values are AVMEDIA_TYPE_VIDEO and AVMEDIA_TYPE_AUDIO
        AVCodecID codecID = stream->codec->codec_id;		// identifier for the codec
    }
#endif

    // We need to get the actual time relative to video start
    // _startTime will have been set by a previous ResetTime() call.
    int timeRelativeToStart = milliseconds - _startTime;

    int seekTimeSeconds = timeRelativeToStart / 1000;
    int64_t seekDest = seekTimeSeconds * AV_TIME_BASE;

    // Flush buffers before seeking
    avcodec_flush_buffers(_videoDecoderContext);

    if (av_seek_frame(_formatContext, _videoStreamIndex, seekDest, AVSEEK_FLAG_FRAME) < 0)
    {
        common->Warning("Cannot seek in video stream.");
        return data;
    }

    if (av_read_frame(_formatContext, &_packet) < 0)
    {
        common->Warning("Failed to read frame.");
        return data;
    }

    int got_frame = 0;

    do
    {
        int ret = decode_packet(_packet, _rgbaBuffer.get(), &got_frame, 0, _videoDecoderContext);

        if (ret < 0)
            break;

        _packet.data += ret;
        _packet.size -= ret;

        if (got_frame)
        {
            data.image = _rgbaBuffer.get();
            data.imageWidth = _videoDecoderContext->width;
            data.imageHeight = _videoDecoderContext->height;
            data.status = FMV_PLAY;

            av_free_packet(&_packet);
            return data;
        }
    } 
    while (_packet.size > 0);

    /* flush cached frames */
    _packet.data = NULL;
    _packet.size = 0;

    do 
    {
        decode_packet(_packet, _rgbaBuffer.get(), &got_frame, 1, _videoDecoderContext);

        if (got_frame)
        {
            data.image = _rgbaBuffer.get();
            data.imageWidth = _videoDecoderContext->width;
            data.imageHeight = _videoDecoderContext->height;
            data.status = FMV_PLAY;

            break;
        }
    } 
    while (got_frame);

    av_free_packet(&_packet);

    return data;
}

int idCinematicFFMpeg::AnimationLength()
{
    if (_formatContext)
    {
        return (_formatContext->duration / AV_TIME_BASE) * 1000;
    }

    return 0;
}

void idCinematicFFMpeg::Close()
{
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
}
