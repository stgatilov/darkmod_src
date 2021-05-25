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

#include "../renderer/qgl.h"
#include "TracyOpenGL.hpp"
#include "common/TracySystem.hpp"

extern idCVar r_useDebugGroups;

void GL_SetDebugLabel(GLenum identifier, GLuint name, const idStr &label );
void GL_SetDebugLabel(void *ptr, const idStr &label );

void InitTracing();
void InitOpenGLTracing();
void TracingEndFrame();

extern bool g_tracingEnabled;
extern bool g_glTraceInitialized;

#define TRACE_THREAD_NAME( name ) if ( g_tracingEnabled ) tracy::SetThreadName( name );
#define TRACE_PLOT_NUMBER( name, value ) if ( g_tracingEnabled ) { TracyPlot( name, value ); TracyPlotConfig( name, tracy::PlotFormatType::Number ); }
#define TRACE_PLOT_BYTES( name, value ) if ( g_tracingEnabled ) { TracyPlot( name, value ); TracyPlotConfig( name, tracy::PlotFormatType::Memory ); }
#define TRACE_PLOT_FRACTION( name, value ) if ( g_tracingEnabled ) { TracyPlot( name, value*100 ); TracyPlotConfig( name, tracy::PlotFormatType::Percentage ); }

#define TRACE_COLOR_IDLE 0x808080

#define TRACE_CPU_SCOPE( section ) ZoneNamedN( __tracy_scoped_zone, section, g_tracingEnabled )
#define TRACE_CPU_SCOPE_COLOR( section, color ) ZoneNamedNC( __tracy_scoped_zone, section, color, g_tracingEnabled )

class GlDebugGroupScope {
public:
	GlDebugGroupScope(const char* section) {
		if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() )
			qglPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, 1, -1, section );
	}

	~GlDebugGroupScope() {
		if( GLAD_GL_KHR_debug && r_useDebugGroups.GetBool() )
			qglPopDebugGroup();
	}
};

#define TRACE_GL_SCOPE( section ) GlDebugGroupScope __glDebugGroupCurentScope(section); TracyGpuNamedZone( __tracy_gpu_zone, section, g_tracingEnabled && g_glTraceInitialized );
#define TRACE_GL_SCOPE_COLOR( section, color ) GlDebugGroupScope __glDebugGroupCurentScope(section); TracyGpuNamedZoneC( __tracy_gpu_zone, section, color, g_tracingEnabled && g_glTraceInitialized );
