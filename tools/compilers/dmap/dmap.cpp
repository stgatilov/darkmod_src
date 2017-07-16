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



#include "dmap.h"

dmapGlobals_t	dmapGlobals;

/*
============
PrintIfVerbosityAtLeast

Added #4123. Filter console output by verbosity level. Not used for errors and warnings.
============
*/
void PrintIfVerbosityAtLeast( verbosityLevel_t vl, const char* fmt, ... )
{
	if ( vl <= dmapGlobals.verbose )
	{
		va_list argptr;
		char text[MAX_STRING_CHARS];
		va_start( argptr, fmt );
		idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );
		common->Printf( "%s", text );
	}
}

/*
============
PrintEntityHeader

Added #4123. Print entity number, class, and name, for leaks and other messages.
============
*/
void PrintEntityHeader( verbosityLevel_t vl, const uEntity_t* e )
{
	PrintIfVerbosityAtLeast( vl, "############### entity %i ###############\n", dmapGlobals.entityNum );
	const idDict* entKeys = &e->mapEntity->epairs;
	PrintIfVerbosityAtLeast( vl, "-- ( %s: %s )\n", entKeys->GetString("classname"), entKeys->GetString("name") ); 
}

/*
============
ProcessModel
============
*/
bool ProcessModel( uEntity_t *e, bool floodFill ) {
	bspface_t	*faces;

	// build a bsp tree using all of the sides
	// of all of the structural brushes
	faces = MakeStructuralBspFaceList ( e->primitives );
	e->tree = FaceBSP( faces );

	// create portals at every leaf intersection
	// to allow flood filling
	MakeTreePortals( e->tree );

	// classify the leafs as opaque or areaportal
	FilterBrushesIntoTree( e );

	// see if the bsp is completely enclosed
	if ( floodFill && !dmapGlobals.noFlood ) {
		if ( FloodEntities( e->tree ) ) {
			// set the outside leafs to opaque
			FillOutside( e );
		} else {
			// We have a leak.
			if ( dmapGlobals.verbose < VL_ORIGDEFAULT ) // #4123
			{
				// We haven't printed which entity we're working on so do it now
				PrintEntityHeader( VL_CONCISE, e );
			}
			PrintIfVerbosityAtLeast( VL_CONCISE, "**********************\n" );
			common->Warning( "******* leaked *******" );
			PrintIfVerbosityAtLeast( VL_CONCISE, "**********************\n" );
			LeakFile( e->tree );
			// bail out here.  If someone really wants to
			// process a map that leaks, they should use
			// -noFlood
			return false;
		}
	}

	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree( e );

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas( e );

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas( e );

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	Prelight( e );

	// optimizing is a superset of fixing tjunctions
	if ( !dmapGlobals.noOptimize ) {
		OptimizeEntity( e );
	} else  if ( !dmapGlobals.noTJunc ) {
		FixEntityTjunctions( e );
	}

	// now fix t junctions across areas
	FixGlobalTjunctions( e );

	return true;
}

/*
============
ProcessModels
============
*/
bool ProcessModels( void ) {
	verbosityLevel_t	oldVerbose;
	uEntity_t			*entity;
	uint				counter = 0;  // 4123

	oldVerbose = dmapGlobals.verbose;

	for ( dmapGlobals.entityNum = 0 ; dmapGlobals.entityNum < dmapGlobals.num_entities ; dmapGlobals.entityNum++ ) {

		entity = &dmapGlobals.uEntities[dmapGlobals.entityNum];
		if ( !entity->primitives ) {
			continue;
		}

		if ( dmapGlobals.entityNum == 0 ) // worldspawn
		{
			PrintEntityHeader( VL_CONCISE, entity );
		} else {
			PrintEntityHeader( VL_ORIGDEFAULT, entity );
		}

		// if we leaked, stop without any more processing
		if ( !ProcessModel( entity, (bool)(dmapGlobals.entityNum == 0 ) ) ) {
			return false;
		}

		// we usually don't want to see output for submodels unless
		// something strange is going on
		// SteveL #4123: This (pre-existing) hack allows highly verbose output for 
		// worldspawn (entity 0) without getting it for func statics too.
		if ( !dmapGlobals.verboseentities ) {
			dmapGlobals.verbose = (verbosityLevel_t)idMath::Imin( dmapGlobals.verbose, VL_ORIGDEFAULT);
		}

		++counter;
	}

	dmapGlobals.verbose = oldVerbose;
	PrintIfVerbosityAtLeast( VL_CONCISE, "%d entities containing primitives processed.\n", counter);
	return true;
}

/*
============
DmapHelp
============
*/
void DmapHelp( void ) {
	common->Printf(
		
	"Usage: dmap [options] mapfile\n"
	"Options:\n"
	"noCurves          = don't process curves\n"
	"noCM              = don't create collision map\n"
	"noAAS             = don't create AAS files\n"
	"v                 = verbose mode (default pre TDM 2.04)"
	"v2                = very verbose mode"
	"verboseentities   = very verbose + submodel detail for entities. Requires v2"
	);
}

/*
============
ResetDmapGlobals
============
*/
void ResetDmapGlobals( void ) {
	dmapGlobals.mapFileBase[0] = '\0';
	dmapGlobals.dmapFile = NULL;
	dmapGlobals.mapPlanes.Clear();
	dmapGlobals.num_entities = 0;
	dmapGlobals.uEntities = NULL;
	dmapGlobals.entityNum = 0;
	dmapGlobals.mapLights.Clear();
	dmapGlobals.verbose = VL_CONCISE;
	dmapGlobals.glview = false;
	dmapGlobals.noOptimize = false;
	dmapGlobals.verboseentities = false;
	dmapGlobals.noCurves = false;
	dmapGlobals.fullCarve = false;
	dmapGlobals.noModelBrushes = false;
	dmapGlobals.noTJunc = false;
	dmapGlobals.nomerge = false;
	dmapGlobals.noFlood = false;
	dmapGlobals.noClipSides = false;
	dmapGlobals.noLightCarve = false;
	dmapGlobals.noShadow = false;
	dmapGlobals.shadowOptLevel = SO_NONE;
	dmapGlobals.drawBounds.Clear();
	dmapGlobals.drawflag = false;
	dmapGlobals.totalShadowTriangles = 0;
	dmapGlobals.totalShadowVerts = 0;
}

/*
============
Dmap
============
*/
void Dmap( const idCmdArgs &args ) {
	int			i;
	int			start, end;
	char		path[1024];
	idStr		passedName;
	bool		leaked = false;
	bool		noCM = false;
	bool		noAAS = false;

	ResetDmapGlobals();

	if ( args.Argc() < 2 ) {
		DmapHelp();
		return;
	}

	common->Printf("---- dmap ----\n");

	dmapGlobals.fullCarve = true;
	dmapGlobals.shadowOptLevel = SO_MERGE_SURFACES;		// create shadows by merging all surfaces, but no super optimization
//	dmapGlobals.shadowOptLevel = SO_CLIP_OCCLUDERS;		// remove occluders that are completely covered
//	dmapGlobals.shadowOptLevel = SO_SIL_OPTIMIZE;
//	dmapGlobals.shadowOptLevel = SO_CULL_OCCLUDED;

	dmapGlobals.noLightCarve = true;

	for ( i = 1 ; i < args.Argc() ; i++ ) {
		const char *s;

		s = args.Argv(i);
		if ( s[0] == '-' ) {
			s++;
			if ( s[0] == '\0' ) {
				continue;
			}
		}

		if ( !idStr::Icmp( s,"glview" ) ) {
			dmapGlobals.glview = true;
		} else if ( !idStr::Icmp( s, "v" ) ) {
			common->Printf( "verbose = true (original default)\n" );
			dmapGlobals.verbose = VL_ORIGDEFAULT;
		} else if ( !idStr::Icmp( s, "v2" ) ) {
			common->Printf( "verbose = very\n" );
			dmapGlobals.verbose = VL_VERBOSE;
		} else if ( !idStr::Icmp( s, "draw" ) ) {
			common->Printf( "drawflag = true\n" );
			dmapGlobals.drawflag = true;
		} else if ( !idStr::Icmp( s, "noFlood" ) ) {
			common->Printf( "noFlood = true\n" );
			dmapGlobals.noFlood = true;
		} else if ( !idStr::Icmp( s, "noLightCarve" ) ) {
			common->Printf( "noLightCarve = true\n" );
			dmapGlobals.noLightCarve = true;
		} else if ( !idStr::Icmp( s, "lightCarve" ) ) {
			common->Printf( "noLightCarve = false\n" );
			dmapGlobals.noLightCarve = false;
		} else if ( !idStr::Icmp( s, "noOpt" ) ) {
			common->Printf( "noOptimize = true\n" );
			dmapGlobals.noOptimize = true;
		} else if ( !idStr::Icmp( s, "verboseentities" ) ) {
			common->Printf( "verboseentities = true\n");
			dmapGlobals.verboseentities = true;
		} else if ( !idStr::Icmp( s, "noCurves" ) ) {
			common->Printf( "noCurves = true\n");
			dmapGlobals.noCurves = true;
		} else if ( !idStr::Icmp( s, "noModels" ) ) {
			common->Printf( "noModels = true\n" );
			dmapGlobals.noModelBrushes = true;
		} else if ( !idStr::Icmp( s, "noClipSides" ) ) {
			common->Printf( "noClipSides = true\n" );
			dmapGlobals.noClipSides = true;
		} else if ( !idStr::Icmp( s, "noCarve" ) ) {
			common->Printf( "noCarve = true\n" );
			dmapGlobals.fullCarve = false;
		} else if ( !idStr::Icmp( s, "shadowOpt" ) ) {
			dmapGlobals.shadowOptLevel = (shadowOptLevel_t)atoi( args.Argv( i+1 ) );
			common->Printf( "shadowOpt = %i\n",dmapGlobals.shadowOptLevel );
			i += 1;
		} else if ( !idStr::Icmp( s, "noTjunc" ) ) {
			// triangle optimization won't work properly without tjunction fixing
			common->Printf ("noTJunc = true\n" );
			dmapGlobals.noTJunc = true;
			dmapGlobals.noOptimize = true;
			common->Printf ("forcing noOptimize = true\n" );
		} else if ( !idStr::Icmp( s, "noCM" ) ) {
			noCM = true;
			common->Printf( "noCM = true\n" );
		} else if ( !idStr::Icmp( s, "noAAS" ) ) {
			noAAS = true;
			common->Printf( "noAAS = true\n" );
		} else if ( !idStr::Icmp( s, "editorOutput" ) ) {
#ifdef _WIN32
			com_outputMsg = true;
#endif
		} else {
			break;
		}
	}

	if ( i >= args.Argc() ) {
		common->Error( "usage: dmap [options] mapfile" );
	}

	passedName = args.Argv(i);		// may have an extension
	passedName.BackSlashesToSlashes();
	if ( passedName.Icmpn( "maps/", 4 ) != 0 ) {
		passedName = "maps/" + passedName;
	}

    // taaaki - support map files from darkmod/fms/<mission>/maps as well as darkmod/maps
    //          this is done by opening the file to get the true full path, then converting
    //          the path back to a RelativePath based off fs_devpath
    passedName.SetFileExtension( "map" );
    idFile *fp = idLib::fileSystem->OpenFileRead( passedName, "" );
    if ( fp ) {
        passedName = idLib::fileSystem->OSPathToRelativePath(fp->GetFullPath());
        idLib::fileSystem->CloseFile( fp );
    }

    idStr stripped = passedName;
	stripped.StripFileExtension();
	idStr::Copynz( dmapGlobals.mapFileBase, stripped, sizeof(dmapGlobals.mapFileBase) );

	bool region = false;
	// if this isn't a regioned map, delete the last saved region map
	if ( passedName.Right( 4 ) != ".reg" ) {
		sprintf( path, "%s.reg", dmapGlobals.mapFileBase );
		fileSystem->RemoveFile( path, "" );
	} else {
		region = true;
	}


	passedName = stripped;

	// delete any old line leak files
	sprintf( path, "%s.lin", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( path, "" );


	//
	// start from scratch
	//
	start = Sys_Milliseconds();

	if ( !LoadDMapFile( passedName ) ) {
		return;
	}

	if ( ProcessModels() ) {
		WriteOutputFile();
		PrintIfVerbosityAtLeast( VL_CONCISE, "Dmap complete, moving on to collision world and AAS...\n");
	} else {
		leaked = true;
	}

	FreeDMapFile();

	PrintIfVerbosityAtLeast( VL_CONCISE, "%i total shadow triangles\n", dmapGlobals.totalShadowTriangles );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%i total shadow verts\n", dmapGlobals.totalShadowVerts );

	end = Sys_Milliseconds();
	PrintIfVerbosityAtLeast( VL_CONCISE, "-----------------------\n" );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5.0f seconds for dmap\n", ( end - start ) * 0.001f );

	if ( !leaked ) {

		if ( !noCM ) {

			// make sure the collision model manager is not used by the game
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );

			// disconnect closes the console. reopen it. #4123
			console->Open(0.5);

			// create the collision map
			start = Sys_Milliseconds();

			collisionModelManager->LoadMap( dmapGlobals.dmapFile );
			collisionModelManager->FreeMap();

			end = Sys_Milliseconds();
			PrintIfVerbosityAtLeast( VL_CONCISE, "-------------------------------------\n" );
			PrintIfVerbosityAtLeast( VL_CONCISE, "%5.0f seconds to create collision map\n", ( end - start ) * 0.001f );
		}

		if ( !noAAS && !region ) {
			// create AAS files
			RunAAS_f( args );
		}
	}

	// free the common .map representation
	delete dmapGlobals.dmapFile;

	// clear the map plane list
	dmapGlobals.mapPlanes.Clear();

#ifdef _WIN32
	if ( com_outputMsg && com_hwndMsg != NULL ) {
		unsigned int msg = ::RegisterWindowMessage( DMAP_DONE );
		::PostMessage( com_hwndMsg, msg, 0, 0 );
	}
#endif
}

/*
============
Dmap_f
============
*/
void Dmap_f( const idCmdArgs &args ) {

	common->ClearWarnings( "running dmap" );

	// refresh the screen each time we print so it doesn't look
	// like it is hung
	common->SetRefreshOnPrint( true );
	Dmap( args );
	common->SetRefreshOnPrint( false );

	common->PrintWarnings();
}
