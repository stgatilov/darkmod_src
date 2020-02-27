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

#include "tr_local.h"
#include "Profiling.h"

typedef struct {
	GLenum			target;
	GLuint			ident;
	const char		*name;
	GLuint          genId; // glsl program id
} progDef_t;

#define MAX_GLPROGS			512

// a single file can have both a vertex program and a fragment program
static progDef_t	progs[MAX_GLPROGS] = {
	{ GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION, "interaction.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION, "interaction.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_ENVIRONMENT, "environment.vfp" },

	// SteveL #3878: Particle softening applied by the engine
	{ GL_VERTEX_PROGRAM_ARB, VPROG_SOFT_PARTICLE, "soft_particle.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_SOFT_PARTICLE, "soft_particle.vfp" },

	// additional programs can be dynamically specified in materials
};

/*
=================
R_LoadARBProgram
=================
*/
void R_LoadARBProgram( int progIndex ) {
	idStr	fullPath = "glprogs/";
	fullPath += progs[progIndex].name;
	char	*fileBuffer;
	char	*buffer;
	char	*start = NULL, *end;

	// load the program even if we don't support it
	fileSystem->ReadFile( fullPath.c_str(), ( void ** )&fileBuffer, NULL );

	if ( !fileBuffer ) {
		common->Warning( "LoadARBProgram: \'%s\' not found", fullPath.c_str() );
		return;
	}
	common->Printf( "%s", fullPath.c_str() );

	// copy to stack memory and free
	buffer = ( char * )_alloca( strlen( fileBuffer ) + 1 );
	strcpy( buffer, fileBuffer );
	fileSystem->FreeFile( fileBuffer );

	if ( !glConfig.isInitialized ) {
		return;
	}

	//
	// submit the program string at start to GL
	//
	if ( progs[progIndex].ident == 0 ) {
		// allocate a new identifier for this program
		progs[progIndex].ident = PROG_USER + progIndex;
	}
	common->Printf( " %d", progs[progIndex].ident );

	// vertex and fragment programs can both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end
	start = NULL;
	if ( progs[progIndex].target == GL_VERTEX_PROGRAM_ARB ) {
		start = strstr( ( char * )buffer, "!!ARBvp" );
	}
	if ( progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB ) {
		start = strstr( ( char * )buffer, "!!ARBfp" );
	}
	if ( !start ) {
		common->Printf( S_COLOR_RED ": !!ARB not found\n"  S_COLOR_DEFAULT );
		return;
	}
	end = strstr( start, "END" );

	if ( !end ) {
		common->Printf( S_COLOR_RED ": END not found\n"  S_COLOR_DEFAULT );
		return;
	}
	end[3] = 0;

	qglBindProgramARB( progs[progIndex].target, progs[progIndex].ident );
	qglProgramStringARB( progs[progIndex].target, GL_PROGRAM_FORMAT_ASCII_ARB,
	                     static_cast<GLsizei>( strlen( start ) ), ( unsigned char * )start );

	// this is pretty important for quick shader debugging, better have it in always
	int err = qglGetError();
	int	ofs;
	qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, ( GLint * )&ofs );
	if ( err == GL_INVALID_OPERATION ) {
		const GLubyte *str = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
		common->Printf( "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str );
		if ( ofs < 0 ) {
			common->Printf( "GL_PROGRAM_ERROR_POSITION_ARB < 0 with error\n" );
		} else if ( ofs >= ( int )strlen( ( char * )start ) ) {
			common->Printf( "error at end of program\n" );
		} else {
			common->Printf( "error at %i:\n%s", ofs, start + ofs );
		}
		return;
	}

	if ( ofs != -1 ) {
		common->Printf( "\nGL_PROGRAM_ERROR_POSITION_ARB != -1 without error\n" );
		return;
	}
	common->Printf( "\n" );
}

/*
==================
R_FindARBProgram

Returns a GL identifier that can be bound to the given target, parsing
a text file if it hasn't already been loaded.
==================
*/
int R_FindARBProgram( GLenum target, const char *program ) {
	int		i;
	idStr	stripped = program;

	if (!glConfig.arbAssemblyShadersAvailable)
		return 0;

	stripped.StripFileExtension();

	// see if it is already loaded
	for ( i = 0 ; progs[i].name ; i++ ) {
		if ( progs[i].target != target ) {
			continue;
		}
		idStr	compare = progs[i].name;

		compare.StripFileExtension();

		if ( !idStr::Icmp( stripped.c_str(), compare.c_str() ) ) {
			return progs[i].ident;
		}
	}

	if ( i == MAX_GLPROGS ) {
		common->Error( "R_FindARBProgram: MAX_GLPROGS" );
	}

	// add it to the list and load it
	progs[i].ident = ( program_t )0;	// will be gen'd by R_LoadARBProgram
	progs[i].target = target;
	char *progName = new char[strlen( program ) + 1];
	progs[i].name = progName;
	strcpy( progName, program );
	R_LoadARBProgram( i );

	return progs[i].ident;
}

/*
==================
R_UseProgramARB

One-liner for qglBindProgramARB+qglEnable frag+vert
Important: fprog needs to go straight after vprog in program_t
==================
*/
void R_UseProgramARB( int vProg ) {
	GL_CheckErrors();

	if ( vProg == PROG_INVALID ) {
		qglDisable( GL_VERTEX_PROGRAM_ARB );
		qglDisable( GL_FRAGMENT_PROGRAM_ARB );
	} else {
		RB_LogComment( "R_UseProgram %d\n", vProg );
		qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, vProg );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, vProg + 1 ); // as defined by program_t
		qglEnable( GL_VERTEX_PROGRAM_ARB );
		qglEnable( GL_FRAGMENT_PROGRAM_ARB );
		GL_CheckErrors();
	}
}

/*
==================
R_ReloadARBPrograms_f
==================
*/
void R_ReloadARBPrograms_f( const idCmdArgs &args ) {
	if (!glConfig.arbAssemblyShadersAvailable) {
		common->Printf("No ARB programs loaded\n");
		return;
	}

	common->Printf( "----- R_ReloadARBPrograms -----\n" );

	for ( int i = 0; progs[i].name && progs[i].name[0]; i++ ) {
		R_LoadARBProgram( i );
	}
	common->Printf( "-------------------------------\n" );
}

