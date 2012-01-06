/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
#include "precompiled.h"
#pragma hdrstop

#include "snd_local.h"

/*
===============
idEFXFile::idEFXFile
===============
*/
idEFXFile::idEFXFile( void ) { }

/*
===============
idEFXFile::Clear
===============
*/
void idEFXFile::Clear( void ) {
	effects.DeleteContents( true );
}

/*
===============
idEFXFile::~idEFXFile
===============
*/
idEFXFile::~idEFXFile( void ) {
	Clear();
}

/*
===============
idEFXFile::FindEffect
===============
*/
bool idEFXFile::FindEffect( idStr &name, idSoundEffect **effect, int *index ) {
	int i;

	for ( i = 0; i < effects.Num(); i++ ) {
		if ( ( effects[i] ) && ( effects[i]->name == name ) ) {
			*effect = effects[i];
			*index = i;
			return true;
		}
	}
	return false;
}

/*
===============
idEFXFile::ReadEffect
===============
*/
bool idEFXFile::ReadEffect( idLexer &src, idSoundEffect *effect ) {
	idToken name, token;
	
	if ( !src.ReadToken( &token ) )
		return false;

	// reverb effect
	if ( token == "reverb" ) {
		EAXREVERBPROPERTIES *reverb = ( EAXREVERBPROPERTIES * )Mem_Alloc( sizeof( EAXREVERBPROPERTIES ) );
		if ( reverb ) {
			src.ReadTokenOnLine( &token );
			name = token;
				
			if ( !src.ReadToken( &token ) ) {
				Mem_Free( reverb );
				return false;
			}
			
			if ( token != "{" ) {
				src.Error( "idEFXFile::ReadEffect: { not found, found %s", token.c_str() );
				Mem_Free( reverb );
				return false;
			}
			
			do {
				if ( !src.ReadToken( &token ) ) {
					src.Error( "idEFXFile::ReadEffect: EOF without closing brace" );
					Mem_Free( reverb );
					return false;
				}

				if ( token == "}" ) {
					effect->name = name;
					effect->data = ( void * )reverb;
					effect->datasize = sizeof( EAXREVERBPROPERTIES );
					break;
				}

				if ( token == "environment" ) {
					src.ReadTokenOnLine( &token );
					reverb->ulEnvironment = token.GetUnsignedLongValue();
				} else if ( token == "environment size" ) {
					reverb->flEnvironmentSize = src.ParseFloat();
				} else if ( token == "environment diffusion" ) {
					reverb->flEnvironmentDiffusion = src.ParseFloat();
				} else if ( token == "room" ) {
					reverb->lRoom = src.ParseInt();
				} else if ( token == "room hf" ) {
					reverb->lRoomHF = src.ParseInt();
				} else if ( token == "room lf" ) {
					reverb->lRoomLF = src.ParseInt();
				} else if ( token == "decay time" ) {
					reverb->flDecayTime = src.ParseFloat();
				} else if ( token == "decay hf ratio" ) {
					reverb->flDecayHFRatio = src.ParseFloat();
				} else if ( token == "decay lf ratio" ) {
					reverb->flDecayLFRatio = src.ParseFloat();
				} else if ( token == "reflections" ) {
					reverb->lReflections = src.ParseInt();
				} else if ( token == "reflections delay" ) {
					reverb->flReflectionsDelay = src.ParseFloat();
				} else if ( token == "reflections pan" ) {
					reverb->vReflectionsPan.x = src.ParseFloat();
					reverb->vReflectionsPan.y = src.ParseFloat();
					reverb->vReflectionsPan.z = src.ParseFloat();
				} else if ( token == "reverb" ) {
					reverb->lReverb = src.ParseInt();
				} else if ( token == "reverb delay" ) {
					reverb->flReverbDelay = src.ParseFloat();
				} else if ( token == "reverb pan" ) {
					reverb->vReverbPan.x = src.ParseFloat();
					reverb->vReverbPan.y = src.ParseFloat();
					reverb->vReverbPan.z = src.ParseFloat();
				} else if ( token == "echo time" ) {
					reverb->flEchoTime = src.ParseFloat();
				} else if ( token == "echo depth" ) {
					reverb->flEchoDepth = src.ParseFloat();
				} else if ( token == "modulation time" ) {
					reverb->flModulationTime = src.ParseFloat();
				} else if ( token == "modulation depth" ) {
					reverb->flModulationDepth = src.ParseFloat();
				} else if ( token == "air absorption hf" ) {
					reverb->flAirAbsorptionHF = src.ParseFloat();
				} else if ( token == "hf reference" ) {
					reverb->flHFReference = src.ParseFloat();
				} else if ( token == "lf reference" ) {
					reverb->flLFReference = src.ParseFloat();
				} else if ( token == "room rolloff factor" ) {
					reverb->flRoomRolloffFactor = src.ParseFloat();
				} else if ( token == "flags" ) {
					src.ReadTokenOnLine( &token );
					reverb->ulFlags = token.GetUnsignedLongValue();
				} else {
					src.ReadTokenOnLine( &token );
					src.Error( "idEFXFile::ReadEffect: Invalid parameter in reverb definition" );
					Mem_Free( reverb );
				}
			} while ( 1 );

			return true;
		}
	} else {
		// other effect (not supported at the moment)
		src.Error( "idEFXFile::ReadEffect: Unknown effect definition" );
	}

	return false;
}


/*
===============
idEFXFile::LoadFile
===============
*/
bool idEFXFile::LoadFile( const char *filename, bool OSPath ) {
	idLexer src( LEXFL_NOSTRINGCONCAT );
	idToken token;

	src.LoadFile( filename, OSPath );
	if ( !src.IsLoaded() ) {
		return false;
	}

	if ( !src.ExpectTokenString( "Version" ) ) {
		return NULL;
	}

	if ( src.ParseInt() != 1 ) {
		src.Error( "idEFXFile::LoadFile: Unknown file version" );
		return false;
	}
	
	while ( !src.EndOfFile() ) {
		idSoundEffect *effect = new idSoundEffect;
		if ( ReadEffect( src, effect ) ) {
			effects.Append( effect );
		}
	};

	return true;
}


/*
===============
idEFXFile::UnloadFile
===============
*/
void idEFXFile::UnloadFile( void ) {
	Clear();
}
