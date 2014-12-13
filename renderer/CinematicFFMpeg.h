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

}

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
    static void             LogCallback(void* avcl, int level, const char *fmt, va_list vl);

    // Returns the index of the best suitable stream type, requires an open format context
    int                     FindBestStreamByType(AVMediaType type);

    idStr _path;

    // The backend time the video started
    int						_startTime;

    // The status of this cinematic
    cinStatus_t				_status;

    std::shared_ptr<byte>   _rgbaBuffer;

    AVFormatContext*        _formatContext;
    AVCodecContext*         _videoDecoderContext;
    int                     _videoStreamIndex;

    AVPacket                _packet;
};
