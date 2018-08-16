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

#ifndef __PROFILING_H__
#define __PROFILING_H__

#include "qgl.h"
#include "tr_local.h"

extern idCVar r_useDebugGroups;
extern idCVar r_glProfiling;

void ProfilingEnterSection( const char* section );
void ProfilingLeaveSection();

void ProfilingBeginFrame();
void ProfilingEndFrame();

void ProfilingDrawCurrentTimings();
void ProfilingPrintTimings_f( const idCmdArgs &args );

class GlProfileScope {
public:
	GlProfileScope(const char* section) {
		if( glConfig.debugGroupsAvailable && r_useDebugGroups.GetBool() )
			qglPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, 1, -1, section );
		if( glConfig.timerQueriesAvailable && r_glProfiling.GetBool() )
			ProfilingEnterSection( section );
	}

	~GlProfileScope() {
		if( glConfig.debugGroupsAvailable && r_useDebugGroups.GetBool() )
			qglPopDebugGroup();
		if( glConfig.timerQueriesAvailable && r_glProfiling.GetBool() )
			ProfilingLeaveSection();
	}
};

#define GL_PROFILE(section) GlProfileScope __glProfileCurrentScope(section);

#endif