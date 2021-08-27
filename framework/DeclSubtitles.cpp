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
#include "DeclSubtitles.h"


bool idDeclSubtitles::Parse( const char *text, const int textLength ) {
	idLexer src;
	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );
	src.SkipUntilString( "{" );

	SubtitleLevel verbosity = SUBL_MISSING;

	idToken	token, tokenSound, tokenValue;
	while ( src.ReadToken( &token ) ) {
		if ( !token.Icmp("}") ) {
			break;

		} else if ( !token.Icmp("verbosity") ) {
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Missing subtitle verbosity level" );
				return false;
			}

			if ( !token.Icmp("story") ) {
				verbosity = SUBL_STORY;
			} else if ( !token.Icmp("speech") ) {
				verbosity = SUBL_SPEECH;
			} else if ( !token.Icmp("effect") ) {
				verbosity = SUBL_EFFECT;
			} else {
				src.Warning( "Subtitle verbosity must be one of: effect, speech, story" );
				return false;
			}

		} else if ( !token.Icmp( "inline" ) || !token.Icmp( "srt" ) ) {
			if ( !src.ReadToken( &tokenSound ) ) {
				src.Warning( "Missing sound sample name" );
				return false;
			}
			if ( verbosity == SUBL_MISSING ) {
				src.Warning( "Verbosity level not set for subtitle" );
				return false;
			}
			if ( !src.ReadToken( &tokenValue ) ) {
				src.Warning( "Missing subtitle value" );
				return false;
			}

			subtitleMapping_t mapping;
			mapping.owner = this;
			mapping.soundSampleName = tokenSound;
			mapping.verbosityLevel = verbosity;
			if ( !token.Icmp( "inline" ) ) {
				mapping.inlineText = tokenValue;
			} else if ( !token.Icmp( "srt" ) ) {
				mapping.srtFileName = tokenValue;
			}
			defs.Append( mapping );

		} else if ( !token.Icmp( "include" ) ) {
			if ( !src.ReadToken( &tokenValue ) ) {
				src.Warning( "Missing name of included subtitle decl" );
				return false;
			}

			const idDecl *decl = declManager->FindType( DECL_SUBTITLES, tokenValue.c_str() );
			includes.Append( (idDeclSubtitles*)decl );

		} else {
			src.Warning( "Unexpected token %s", token.c_str() );
			return false;
		}
	}

	return true;
}

static size_t SizeOfMapping( const subtitleMapping_t &mapping ) {
	return (
		sizeof(subtitleMapping_t) + 
		mapping.soundSampleName.Allocated() +
		mapping.inlineText.Allocated() + 
		mapping.srtFileName.Allocated()
	);
}

size_t idDeclSubtitles::Size( void ) const {
	size_t total = 0;
	for (int i = 0; i < defs.Num(); i++)
		total += SizeOfMapping(defs[i]);
	total += includes.MemoryUsed();
	return total;
}

void idDeclSubtitles::FreeData( void ) {
	defs.ClearFree();
	includes.ClearFree();
}

const subtitleMapping_t *idDeclSubtitles::FindSubtitleForSound( const char *soundName ) const {
	for (int i = 0; i < defs.Num(); i++)
		if (defs[i].soundSampleName.Icmp(soundName) == 0)
			return &defs[i];
	for (int i = 0; i < includes.Num(); i++)
		if (const subtitleMapping_t *res = includes[i]->FindSubtitleForSound(soundName))
			return res;
	return nullptr;
}
