/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#pragma once

#include <algorithm>
#include <list>
#include "Download.h"
#include "MirrorList.h"
#include <random>

namespace tdm
{

/**
 * This extends the threaded Download class by adding
 * support for a weighted list of mirrors.
 */
class MirrorDownload :
	public Download
{
private:
	static std::mt19937 _rand;

	// The ordered list of mirrors, same order as the protected _url;
	std::vector<Mirror> _mirrors;

public:
	/**
	 * Construct a MirrorDownload by passing the connection to use,
	 * the list of available mirrors as well as the filename which is 
	 * appended to the respective mirror URL, e.g. "tdm_ai_animals01.pk4".
	 * 
	 * After successful download, the file will be moved to the given
	 * destination file.
	 * 
	 * If no more mirrors are availabe, the status will be FAILED.
	 */
	MirrorDownload(const HttpConnectionPtr& conn, 
				   const MirrorList& mirrors, 
				   const std::string& srcFilename,
				   const fs::path& destFilename);
	
	// Get the display name of the currently chosen mirror
	std::string GetCurrentMirrorName();

	// Called by the Updater
	static void InitRandomizer();
};
typedef std::shared_ptr<MirrorDownload> MirrorDownloadPtr;

} // namespace
