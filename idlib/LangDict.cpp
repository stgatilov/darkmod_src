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



/*
============
idLangDict::idLangDict
============
*/
idLangDict::idLangDict( void ) {
	args.SetGranularity( 256 );
	hash.SetGranularity( 256 );
	hash.ClearFree( 4096, 8192 );
}

/*
============
idLangDict::~idLangDict
============
*/
idLangDict::~idLangDict( void ) {
	Clear();
}

/*
============
idLangDict::Clear
============
*/
void idLangDict::Clear( void ) {
	args.Clear();
	hash.Clear();
}

/*
============
idLangDict::LoadLegacy
============
*/
bool idLangDict::LoadLegacy( const char *fileName, const idStr &remap ) {
	Clear();

	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	int len = idLib::fileSystem->ReadFile( fileName, (void**)&buffer );
	if ( len <= 0 ) {
		// let whoever called us deal with the failure (so sys_lang can be reset)
		return false;
	}
    src.LoadMemory(buffer, static_cast<int>(strlen(buffer)), fileName);
	if ( !src.IsLoaded() ) {
		return false;
	}

	idToken tok, tok2;
	src.ExpectTokenString( "{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == "}" ) {
			break;
		}
		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == "}" ) {
				break;
			}
			idLangKeyValue kv;
			kv.key = tok;
			kv.value = tok2;
			{
				// Tels: fix #2812, some characters like 0xFF ("я" in russian) are not rendered
				// in the GUI, so replace them (as the font contains the characters elsewhere).
				// If we were given a replacement table, use it to exchange the characters:
				kv.value.RemapI18nLegacy( remap.Length() / 2, remap.c_str() );
			}
			assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
			hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
		}
	}
	idLib::common->Printf( "I18N: %i strings read from %s\n", args.Num(), fileName );
	idLib::fileSystem->FreeFile( (void*)buffer );
	
	return true;
}

/*
============
idLangDict::Merge
============
*/
void idLangDict::Merge( const idLangDict &dict, bool override ) {
	for ( int i = 0; i < dict.args.Num(); i++ ) {
		const idLangKeyValue &kv = dict.args[ i ];
		int found = 0;

		int hashKey = GetHashKey( kv.key );
		for ( int i = hash.First( hashKey ); i != -1; i = hash.Next( i ) ) {
			if ( args[i].key.Cmp( kv.key ) == 0 ) {
				found++;

				if ( override ) {
					args[i].value = kv.value;	// override
				} else {
					// skip
				}
			}
		}

		if ( !found ) {
			// new key
			hash.Add( hashKey, args.Append( kv ) );
		}
	}
}

/*
============
idLangDict::GetString
============
*/
const char *idLangDict::GetString( const char *str, const bool dowarn ) const {

	if ( str == NULL || str[0] == '\0' ) {
		return "";
	}

	if ( idStr::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
		return str;
	}

	int hashKey = GetHashKey( str );
	for ( int i = hash.First( hashKey ); i != -1; i = hash.Next( i ) ) {
		if ( args[i].key.Cmp( str ) == 0 ) {
			return args[i].value;
		}
	}

	if (dowarn)
	{
		idLib::common->Warning( "Unknown string id %s", str );
	}
	return str;
}

/*
============
idLangDict::AddKeyVal
============
*/
void idLangDict::AddKeyVal( const char *key, const char *val ) {
	idLangKeyValue kv;
	kv.key = key;
	kv.value = val;
	assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
	hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
}

/*
============
idLangDict::GetHashKey
============
*/
int idLangDict::GetHashKey( const char *str ) const {
	//stgatilov #5261: generic string hashing algorithm (djb2), no assert
	int hashKey = 5381;
	for ( str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++ ) {
		hashKey = (hashKey << 5) + hashKey + str[0];
	}
	return hashKey;
}
