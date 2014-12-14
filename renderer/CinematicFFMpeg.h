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

#pragma once

#include "Cinematic.h"
#include <memory>

extern "C"
{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

#define ENABLE_AV_DEBUG_LOGGING 0

/**
 * Cinematic driven by the ffmpeg libavcodec library.
 */
class idCinematicFFMpeg : 
    public idCinematic
{
public:
    idCinematicFFMpeg();
    virtual					~idCinematicFFMpeg();

    virtual bool			InitFromFile(const char *qpath, bool looping);
    virtual cinData_t		ImageForTime(int milliseconds);
    virtual int				AnimationLength();
    virtual void			Close();
    virtual void			ResetTime(int time);

private:
    // A buffer with a timestamp for decoding and precaching 
    struct FrameBuffer
    {
        int                     timeStamp;     // video time in msecs
        int                     duration;      // frame duration in msecs
        std::shared_ptr<byte>   rgbaImage;     // byte buffer holding RGBA image

        FrameBuffer() :
            timeStamp(-1),
            duration(0)
        {}
    };

private: // methods

    // Creates the whole libavcodec contexts, also called when resetting the time
    bool                    OpenAVDecoder();

    // Frees the libavcodec structures, doesn't touch the FrameBuffers
    void                    CloseAVDecoder();

#if ENABLE_AV_DEBUG_LOGGING
    static void             LogCallback(void* avcl, int level, const char *fmt, va_list vl);
#endif

    // Returns the index of the best suitable stream type, requires an open format context
    int                     FindBestStreamByType(AVMediaType type);

    // Decodes a single stream packet into the RGBA buffer
    int                     DecodePacket(AVPacket& avpkt, byte* targetRGBA, bool& frameDecoded);

    // Load the next frame and save it to the given buffer. Buffer data will be overwritten.
    // Returns true if the buffer was filled, false on failure/EOF.
    bool                    ReadFrame(FrameBuffer& targetBuffer);

    // Returns the time in msecs of the current _packet
    int                     CalculatePacketTime();

private: // members

    // The path of the cinematic (VFS path)
    idStr                   _path;

    // Whether this cinematic is supposed to loop
    bool                    _looping;

    // The virtual file we're streaming from
    idFile*                 _file;

    class VFSIOContext;
    std::unique_ptr<VFSIOContext>   _customIOContext;

    // Duration of this cinematic in msecs
    int                     _duration;

    // The frame rate this cinematic was encoded in
    float                   _frameRate;

    // The backend time the video started
    int						_startTime;

    // The status of this cinematic
    cinStatus_t				_status;

    // The buffer holding the RGBA frame data
    FrameBuffer             _buffer;
    FrameBuffer             _bufferNext;

    // Size of the RGBA buffer in the FrameBuffer structs
    int                     _bufferSize;

    AVFormatContext*        _formatContext;
    AVCodecContext*         _videoDecoderContext;
    int                     _videoStreamIndex;

    // The packet used for decoding decoded 
    AVPacket                _packet;

    // The time offset that is added to packet timestamps, used for looping
    int                     _packetTimeOffset;

    // The highest calculated packet time (unlooped time)
    int                     _highestNextPacketTime;

    // Frame data in native pixel format
    AVFrame*                _tempFrame;

    // The scaling library context for this video
    SwsContext*             _swsContext;
};
