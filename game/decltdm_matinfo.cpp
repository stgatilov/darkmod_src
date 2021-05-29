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



#include "decltdm_matinfo.h"

tdmDeclTDM_MatInfo::tdmDeclTDM_MatInfo()
{
}

tdmDeclTDM_MatInfo::~tdmDeclTDM_MatInfo()
{
	FreeData();
}

size_t tdmDeclTDM_MatInfo::Size() const
{
	return sizeof(tdmDeclTDM_MatInfo);
}

const char *tdmDeclTDM_MatInfo::DefaultDefinition() const
{
	return "tdm_matinfo {}";
}

void tdmDeclTDM_MatInfo::FreeData()
{
	surfaceType = "";
}

// Note Our coding standards require using gotos in this sort of code.
bool tdmDeclTDM_MatInfo::Parse( const char *text, const int textLength )
{
	// Only set to true if we have successfully parsed the decl.
	bool		successfulParse = false;

	idLexer		src;
	idToken		token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );

	// Only consider explicitly defined tdm_matinfo declarations...
	// All others are actual materials.
	if ( !src.ReadToken( &token ) ||
		 token.type != TT_NAME ||
		 token.Icmp( "tdm_matinfo" ) != 0 ) {
		goto Quit;
	}

	// Skip until the opening brace. I don't trust using the
	// skipUntilString function, since it could be fooled by
	// a string containing a brace.
	do {
		if ( !src.ReadToken( &token ) )
		{
			src.Warning( "Unable to find tdm_matinfo decl." );
			goto Quit;
		}
	} while ( token.type != TT_PUNCTUATION || token.subtype != P_BRACEOPEN );
	//src.SkipUntilString( "{" );

	while (1)
	{
		// If there's an EOF, fail to load.
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Unclosed tdm_matinfo decl." );
			goto Quit;
		}

		// Quit upon encountering the closing brace.
		if ( token.type == TT_PUNCTUATION && token.subtype == P_BRACECLOSE)
			break;

		if ( token.type == TT_NAME ) {
			if ( token.Icmp( "surfacetype" ) == 0 ) {

				// surfacetype keyword
				// Syntax:  surfacetype <typename>
				// Example: surfacetype wood

				if ( !src.ReadToken( &token ) ) {
					src.Warning( "Unexpected EOF encountered." );
					goto Quit;
				} else if ( token.type != TT_NAME ) {
					src.Warning( "Invalid surface type: %s", token.c_str() );
					goto Quit;
				}

				if ( surfaceType.Length() > 0 ) {
					src.Warning( "Redefinition of surfaceType." );
				}

				surfaceType = token;

			} else {
				src.Warning( "Unrecognized keyword: %s", token.c_str() );
				goto Quit;
			}
		} else {
			src.Warning( "Invalid keyword: %s", token.c_str() );
			goto Quit;
		}
	}

	successfulParse = true;

	Quit:
	if (!successfulParse) {
		MakeDefault();
	}
	return successfulParse;
}

/// Used to cache the TDM_MatInfos for all the materials applied to surfaces of a map.
void tdmDeclTDM_MatInfo::precacheMap( idMapFile *map ) {
	int numEntities = map->GetNumEntities();
	int e;
	for ( e = 0 ; e < numEntities ; e++ ) {
		idMapEntity *ent = map->GetEntity(e);
		int numPrimitives = ent->GetNumPrimitives();
		int p;
		for ( p = 0 ; p < numPrimitives ; p++ ) {
			idMapPrimitive *prim = ent->GetPrimitive(p);
			if ( prim->GetType() == idMapPrimitive::TYPE_BRUSH ) {
				idMapBrush *brush = dynamic_cast<idMapBrush*>(prim);
				int numSides = brush->GetNumSides();
				int s;
				for ( s = 0 ; s < numSides ; s++ ) {
					idMapBrushSide *side = brush->GetSide(s);
					declManager->MediaPrint( "Precaching brush TDM_MatInfo %s\n", side->GetMaterial() );
					declManager->FindType( DECL_TDM_MATINFO, side->GetMaterial() );
				}
			} else if ( prim->GetType() == idMapPrimitive::TYPE_PATCH ) {
				idMapPatch *patch = dynamic_cast<idMapPatch*>(prim);
				declManager->MediaPrint( "Precaching patch TDM_MatInfo %s\n", patch->GetMaterial() );
				declManager->FindType( DECL_TDM_MATINFO, patch->GetMaterial() );
			} else {
				gameLocal.Warning( "tdmDeclTDM_MatInfo(): unknown primitive type: %d", prim->GetType() );
			}
		}
	}
}

void tdmDeclTDM_MatInfo::precacheModel( idRenderModel *model ) {
	int numSurfaces = model->NumSurfaces();
	int s;
	for ( s = 0 ; s < numSurfaces ; s++ ) {
		const modelSurface_t *surface = model->Surface(s);
		declManager->MediaPrint( "Precaching TDM_MatInfo %s\n", surface->material->GetName() );
		declManager->FindType( DECL_TDM_MATINFO, surface->material->GetName() );
	}
}
