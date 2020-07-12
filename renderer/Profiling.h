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

extern idCVar r_useDebugGroups;
extern idCVar r_glProfiling;
extern idCVar r_frontendProfiling;

class Profiler;
extern Profiler *glProfiler;
extern Profiler *frontendProfiler;

void ProfilingEnterSection( Profiler *profiler, const char* section );
void ProfilingLeaveSection( Profiler *profiler );

void ProfilingBeginFrame();
void ProfilingEndFrame();

void ProfilingDrawCurrentTimings();

void GL_SetDebugLabel(GLenum identifier, GLuint name, const idStr &label );
void GL_SetDebugLabel(void *ptr, const idStr &label );

class GlProfileScope {
public:
	GlProfileScope(const char* section) {
		if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() )
			qglPushDebugGroupKHR( GL_DEBUG_SOURCE_APPLICATION, 1, -1, section );
		if( r_glProfiling.GetBool() )
			ProfilingEnterSection( glProfiler, section );
	}

	~GlProfileScope() {
		if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() )
			qglPopDebugGroupKHR();
		if( r_glProfiling.GetBool() )
			ProfilingLeaveSection( glProfiler );
	}
};

class FrontendProfileScope {
public:
	FrontendProfileScope(const char* section) {
		if( r_frontendProfiling.GetBool() )
			ProfilingEnterSection( frontendProfiler, section );
	}

	~FrontendProfileScope() {
		if( r_frontendProfiling.GetBool() )
			ProfilingLeaveSection( frontendProfiler );
	}
};

#define GL_PROFILE(section) GlProfileScope __glProfileCurrentScope(section);
#define FRONTEND_PROFILE(section) FrontendProfileScope __frontendProfileCurrentScope(section);

#endif