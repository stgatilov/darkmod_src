/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

$Revision: 6376 $ (Revision of last commit)
$Date: 2014-12-08 15:45:09 +0100 (Mo, 08 Dez 2014) $ (Date of last commit)
$Author: greebo $ (Author of last commit)

******************************************************************************/

#include "precompiled_engine.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id: CinematicFFMpeg.cpp 6376 2014-12-08 14:45:09Z greebo $");

#include "CinematicFFMpeg.h"
#include "Image.h"

extern "C"
{

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

idCinematicFFMpeg::idCinematicFFMpeg()
{
    av_register_all();
}

idCinematicFFMpeg::~idCinematicFFMpeg()
{}

bool idCinematicFFMpeg::InitFromFile(const char *qpath, bool looping)
{
    _path = fileSystem->RelativePathToOSPath(qpath);

    return true;
}

static void ffmpeg_log_callback(void* avcl, int level, const char *fmt, va_list vl)
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

static int decode_write_frame(AVCodecContext *avctx, AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
    int got_frame;
    //char buf[1024];

    int len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);

    if (len < 0)
    {
        common->Warning("Error while decoding frame %d\n", *frame_count);
        return len;
    }

    if (got_frame)
    {
        common->Printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
        fflush(stdout);

        /* the picture is allocated by the decoder, no need to free it */
        //snprintf(buf, sizeof(buf), outfilename, *frame_count);
        //pgm_save(frame->data[0], frame->linesize[0], avctx->width, avctx->height, buf);
        (*frame_count)++;
    }

    if (pkt->data)
    {
        pkt->size -= len;
        pkt->data += len;
    }

    return 0;
}

static int open_codec_context(int *stream_idx, AVFormatContext* fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);

    if (ret < 0)
    {
        common->Warning("Could not find %s stream in input.\n", av_get_media_type_string(type));
        return ret;
    }
    else 
    {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);

        if (!dec)
        {
            common->Warning("Failed to find %s codec\n", av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        // API_MODE_NEW_API_REF_COUNT
        av_dict_set(&opts, "refcounted_frames", "1", 0);

        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0)
        {
            common->Warning("Failed to open %s codec\n", av_get_media_type_string(type));
            return ret;
        }
    }

    return 0;
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

        // Upload to GL
        //R_WriteTGA("testmpeg.tga", targetRGBA, video_dec_ctx->width, video_dec_ctx->height);
    }

    av_frame_free(&tempFrame);

    return decoded;
}

cinData_t idCinematicFFMpeg::ImageForTime(int milliseconds)
{
    av_log_set_callback(ffmpeg_log_callback);

    cinData_t data;
    memset(&data, 0, sizeof(data));

    AVPacket avpkt;
    av_init_packet(&avpkt);
    avpkt.data = NULL;
    avpkt.size = 0;

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
    uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    // printf("Decode video file %s to %s\n", filename, outfilename);
#if 0
    /* find the mpeg1 video decoder */
    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        common->Warning("Codec not found\n");
        return data;
    }

    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        common->Warning("Could not allocate video codec context");
        return data;
    }

    if (codec->capabilities&CODEC_CAP_TRUNCATED)
        c->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
    MUST be initialized there because this information is not
    available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        common->Warning("Could not open codec\n");
        return data;
    }
#endif

    //std::shared_ptr<AVFormatContext> avFormat(avformat_alloc_context(), &avformat_free_context);

    // TESTING
    _path = "C:\\Games\\Doom3\\darkmod\\video\\trailer.mp4";

    AVFormatContext* avFormat = avformat_alloc_context();

    if (avformat_open_input(&avFormat, _path.c_str(), NULL, NULL) < 0)
    {
        common->Warning("Could not open %s\n", _path.c_str());
        return data;
    }

    if (avformat_find_stream_info(avFormat, NULL) < 0)
    {
        common->Warning("Could not find stream info %s\n", _path.c_str());
        return data;
    }

    for (unsigned int i = 0; i < avFormat->nb_streams; ++i)
    {
        AVStream* stream = avFormat->streams[i];		// pointer to a structure describing the stream
        AVMediaType codecType = stream->codec->codec_type;	// the type of data in this stream, notable values are AVMEDIA_TYPE_VIDEO and AVMEDIA_TYPE_AUDIO
        AVCodecID codecID = stream->codec->codec_id;		// identifier for the codec
    }

    int video_stream_idx = -1;
    AVCodecContext* video_dec_ctx = NULL;

    if (open_codec_context(&video_stream_idx, avFormat, AVMEDIA_TYPE_VIDEO) >= 0)
    {
        AVStream* video_stream = avFormat->streams[video_stream_idx];
        video_dec_ctx = video_stream->codec;
#if 0
        int ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             video_dec_ctx->width, video_dec_ctx->height,
                             video_dec_ctx->pix_fmt, 1);
        if (ret < 0) {
            common->Error("Could not allocate raw video buffer\n");
        }
        int video_dst_bufsize = ret;
#endif
    }

    // Allocate target buffer for RGBA data
    _rgbaBuffer = std::shared_ptr<byte>(static_cast<byte*>(Mem_Alloc(video_dec_ctx->width * video_dec_ctx->height * 4)), Mem_Free);

#if 0
    AVFrame* rgbaFrame = av_frame_alloc();

    rgbaFrame->format = AV_PIX_FMT_RGBA;
    rgbaFrame->width = video_dec_ctx->width;
    rgbaFrame->height = video_dec_ctx->height;

    // allocate the buffers for the RGBA frame data
    if (av_frame_get_buffer(rgbaFrame, 32) < 0)
    {
        common->Warning("Could not allocate frame data.\n");
        return data;
    }
#endif

    int got_frame = 0;
    while (av_read_frame(avFormat, &avpkt) >= 0)
    {
        AVPacket orig_pkt = avpkt;

        do
        {
            int ret = decode_packet(avpkt, _rgbaBuffer.get(), &got_frame, 0, video_dec_ctx);

            if (ret < 0)
                break;

            avpkt.data += ret;
            avpkt.size -= ret;

            if (got_frame)
            {
                data.image = _rgbaBuffer.get();
                data.imageWidth = video_dec_ctx->width;
                data.imageHeight = video_dec_ctx->height;
                data.status = FMV_PLAY;
                return data;
            }
        } 
        while (avpkt.size > 0);

        av_free_packet(&orig_pkt);
    }

    /* flush cached frames */
    avpkt.data = NULL;
    avpkt.size = 0;
    do 
    {
        decode_packet(avpkt, _rgbaBuffer.get(), &got_frame, 1, video_dec_ctx);
    } 
    while (got_frame);

#if 0
    int frame_count = 0;
    for (;;) {
        avpkt.size = static_cast<int>(fread(inbuf, 1, INBUF_SIZE, f));
        if (avpkt.size == 0)
            break;

        /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
        and this is the only method to use them because you cannot
        know the compressed data size before analysing it.

        BUT some other codecs (msmpeg4, mpeg4) are inherently frame
        based, so you must call them with all the data for one
        frame exactly. You must also initialize 'width' and
        'height' before initializing them. */

        /* NOTE2: some codecs allow the raw parameters (frame size,
        sample rate) to be changed at any frame. We handle this, so
        you should also take care of it */

        /* here, we use a stream based decoder (mpeg1video), so we
        feed decoder and see if it could decode a frame */
        avpkt.data = inbuf;

        while (avpkt.size > 0)
            if (decode_write_frame(c, frame, &frame_count, &avpkt, 0) < 0)
                return data;
    }

    /* some codecs, such as MPEG, transmit the I and P frame with a
    latency of one frame. You must do the following to have a
    chance to get the last frame of the video */
    avpkt.data = NULL;
    avpkt.size = 0;
    decode_write_frame(c, frame, &frame_count, &avpkt, 1);

    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_frame_free(&frame);
#endif

#if 0
    av_frame_free(&rgbaFrame);
#endif

    return data;
}

int idCinematicFFMpeg::AnimationLength()
{
    return 0;
}

void idCinematicFFMpeg::Close()
{
}

void idCinematicFFMpeg::ResetTime(int time)
{
}
