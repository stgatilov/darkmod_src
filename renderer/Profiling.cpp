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
#include "precompiled.h"
#pragma hdrstop

#include "Profiling.h"

idCVar r_useDebugGroups( "r_useDebugGroups", "0", CVAR_RENDERER | CVAR_BOOL, "Emit GL debug groups during rendering. Useful for frame debugging and analysis with e.g. nSight, which will group render calls accordingly." );
idCVar r_glProfiling( "r_glProfiling", "0", CVAR_RENDERER | CVAR_BOOL, "Collect profiling information about GPU and CPU time spent in the rendering backend." );

/**
 * Tracks CPU and GPU time spent in profiled sections.
 * Sections are structured in hierarchical fashion, you can push and pop sections onto a stack.
 * Each section has a name; sections with the same name at the same place in the hierarchy will
 * be accumulated to a single total time in the end.
 */
class GlProfiler {
public:
	GlProfiler() : profilingActive(false), frameMarker(0) {}

	void BeginFrame() {
		sectionStack.clear();
		frame[ frameMarker ] = section();
		frame[ frameMarker ].name = "Frame";
		AddProfilingQuery( &frame[ frameMarker ] );
		sectionStack.push_back( &frame[ frameMarker ] );
		profilingActive = true;
	}

	void EndFrame() {
		LeaveSection();
		assert( sectionStack.empty() );
		frameMarker = ( frameMarker + 1 ) % 2;
		profilingActive = false;
	}

	void EnterSection(const idStr& name) {
		if( !profilingActive )
			return;
		assert( !sectionStack.empty() );
		section *s = FindOrInsertSection( name );
		AddProfilingQuery( s );
		sectionStack.push_back( s );
	}

	void LeaveSection() {
		if( !profilingActive )
			return;
		assert( !sectionStack.empty() );
		CompleteProfilingQuery( sectionStack.back() );
		sectionStack.pop_back();
	}

	typedef struct query {
		uint64_t cpuStartTime;
		uint64_t cpuStopTime;
		GLuint glQueries[ 2 ];
	} query;

	typedef struct section {
		idStr name;
		int count;
		double totalCpuTimeMillis;
		double totalGpuTimeMillis;
		std::vector<section> children;
		std::vector<query> queries;
	} section;

	section * CollectTimings() {
		AccumulateTotalTimes( frame[ frameMarker ] );
		return &frame[ frameMarker ];
	}
	
private:
	bool profilingActive;
	std::vector<section*> sectionStack;

	// double-buffering, since GL query information is only available after the frame has rendered
	section frame[ 2 ];
	int frameMarker;

	section *FindOrInsertSection(const idStr& name) {
		for ( auto& s : sectionStack.back()->children ) {
			if( s.name == name ) {
				return &s;
			}
		}
		sectionStack.back()->children.push_back( section() );
		section *s = &sectionStack.back()->children.back();
		s->name = name;
		return s;
	}

	void AddProfilingQuery( section *s ) {
		s->queries.push_back( query() );
		query &q = s->queries.back();
		q.cpuStartTime = Sys_GetClockTicks();
		qglGenQueries( 2, q.glQueries );
		qglQueryCounter( q.glQueries[ 0 ], GL_TIMESTAMP );
	}

	void CompleteProfilingQuery( section *s ) {
		query &q = s->queries.back();
		q.cpuStopTime = Sys_GetClockTicks();
		qglQueryCounter( q.glQueries[ 1 ], GL_TIMESTAMP );
	}

	void AccumulateTotalTimes( section &s) {
		s.totalCpuTimeMillis = s.totalGpuTimeMillis = 0;
		s.count = s.queries.size();
		for ( auto& q : s.queries ) {
			s.totalCpuTimeMillis += ( q.cpuStopTime - q.cpuStartTime ) * 1000 / Sys_ClockTicksPerSecond();
			uint64_t gpuStartNanos, gpuStopNanos;
			qglGetQueryObjectui64v( q.glQueries[ 0 ], GL_QUERY_RESULT, &gpuStartNanos );
			qglGetQueryObjectui64v( q.glQueries[ 1 ], GL_QUERY_RESULT, &gpuStopNanos );
			s.totalGpuTimeMillis += ( gpuStopNanos - gpuStartNanos ) / 1000000.0;
			qglDeleteQueries( 2, q.glQueries );
		}

		for ( auto& c : s.children ) {
			AccumulateTotalTimes( c );
		}
	}
};

GlProfiler glProfiler;

void EnterProfilingSection( const char *section ) {
	glProfiler.EnterSection( section );
}

void LeaveProfilingSection() {
	glProfiler.LeaveSection();
}

void ProfilingBeginFrame() {
	if( r_glProfiling.GetBool() ) {
		glProfiler.BeginFrame();
	}
}

void ProfilingEndFrame() {
	if( r_glProfiling.GetBool() ) {
		glProfiler.EndFrame();
	}
}

void PrintSectionTimings( GlProfiler::section &s, idStr indent ) {
	idStr level = indent + s.name;
	common->Printf( "%-35s  %6d  %6.3f ms  %6.3f ms\n", level.c_str(), s.count, s.totalCpuTimeMillis, s.totalGpuTimeMillis );
	for( auto& c : s.children ) {
		PrintSectionTimings( c, indent + "  " );
	}
}

void DisplayProfilingInfo() {
	if( !r_glProfiling.GetBool() ) {
		return;
	}

	GlProfiler::section *s = glProfiler.CollectTimings();

	// TODO: display info overlay like FPS counter
	static int limiter = 0;
	if( ++limiter % 10 != 0 )
		return;  // only print timing info every n-th frame to avoid excessive spam
	common->Printf( "%-35s  %6s  %9s  %9s\n", "# Section", "Count", "CPU", "GPU" );
	PrintSectionTimings( *s, "" );
}
