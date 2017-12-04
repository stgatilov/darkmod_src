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



#include "snd_local.h"
#include "../idlib/math/Math.h"

#define mB_to_gain(millibels, property) \
	_mB_to_gain(millibels,AL_EAXREVERB_MIN_ ## property, AL_EAXREVERB_MAX_ ## property)

static inline ALfloat _mB_to_gain(ALfloat millibels, ALfloat min, ALfloat max) {
	return idMath::ClampFloat(min, max, idMath::Pow(10.0f, millibels / 2000.0f));
}

idSoundEffect::idSoundEffect() :
effect(0) {
}

idSoundEffect::~idSoundEffect() {
	if (soundSystemLocal.alIsEffect(effect))
		soundSystemLocal.alDeleteEffects(1, &effect);
}

bool idSoundEffect::alloc() {
	alGetError();

	ALenum e;

	soundSystemLocal.alGenEffects(1, &effect);
	e = alGetError();
	if (e != AL_NO_ERROR) {
		common->Warning("idSoundEffect::alloc: alGenEffects failed: 0x%x", e);
		return false;
	}

	soundSystemLocal.alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	e = alGetError();
	if (e != AL_NO_ERROR) {
		common->Warning("idSoundEffect::alloc: alEffecti failed: 0x%x", e);
		return false;
	}

	return true;
}

/*
===============
idEFXFile::idEFXFile
===============
*/
idEFXFile::idEFXFile( void ) {
	isAfterReload = false;
}

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
bool idEFXFile::FindEffect(idStr &name, ALuint *effect) {
	int i;

	for (i = 0; i < effects.Num(); i++) {
		if (effects[i]->name == name) {
			*effect = effects[i]->effect;
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
#define efxi(param, value)													\
	do {																	\
		ALint _v = value;													\
		EFXprintf("alEffecti(" #param ", %d)\n", _v);						\
		soundSystemLocal.alEffecti(effect->effect, param, _v);				\
		err = alGetError();													\
		if (err != AL_NO_ERROR)												\
			common->Warning("alEffecti(" # param ", %d) "					\
							"failed: 0x%x", _v, err);						\
		} while (false)

#define efxf(param, value)													\
	do {																	\
		ALfloat _v = value;													\
		EFXprintf("alEffectf(" #param ", %.3f)\n", _v);						\
		soundSystemLocal.alEffectf(effect->effect, param, _v);				\
		err = alGetError();													\
		if (err != AL_NO_ERROR)												\
			common->Warning("alEffectf(" # param ", %.3f) "					\
							"failed: 0x%x", _v, err);						\
		} while (false)

#define efxfv(param, value0, value1, value2)								\
	do {																	\
		ALfloat _v[3];														\
		_v[0] = value0;														\
		_v[1] = value1;														\
		_v[2] = value2;														\
		EFXprintf("alEffectfv(" #param ", %.3f, %.3f, %.3f)\n",				\
					_v[0], _v[1], _v[2]);									\
		soundSystemLocal.alEffectfv(effect->effect, param, _v);				\
		err = alGetError();													\
		if (err != AL_NO_ERROR)												\
			common->Warning("alEffectfv(" # param ", %.3f, %.3f, %.3f) "	\
							"failed: 0x%x",	_v[0], _v[1], _v[2], err);		\
		} while (false)

bool idEFXFile::ReadEffect(idLexer &src, idSoundEffect *effect) {
	idToken name, token;

	if ( !src.ReadToken( &token ) )
		return false;

	// reverb effect
	if ( token != "reverb" ) {
		// other effect (not supported at the moment)
		src.Error( "idEFXFile::ReadEffect: Unknown effect definition" );

		return false;
	}

	src.ReadTokenOnLine( &token );
	name = token;

	if ( !src.ReadToken( &token ) )
		return false;

	if ( token != "{" ) {
		src.Error( "idEFXFile::ReadEffect: { not found, found %s", token.c_str() );
		return false;
	}

	ALenum err;
	alGetError();
	common->Printf("Loading EFX effect '%s' (#%u)\n", name.c_str(), effect->effect);

	do {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "idEFXFile::ReadEffect: EOF without closing brace" );
			return false;
		}

		if ( token == "}" ) {
			effect->name = name;
			break;
		}

		if ( token == "environment" ) {
			// <+KittyCat> the "environment" token should be ignored (efx has nothing equatable to it)
			src.ParseInt();
		} else if ( token == "environment size" ) {
			float size = src.ParseFloat();
			efxf(AL_EAXREVERB_DENSITY, (size < 2.0f) ? (size - 1.0f) : 1.0f);
		} else if ( token == "environment diffusion" ) {
			efxf(AL_EAXREVERB_DIFFUSION, src.ParseFloat());
		} else if ( token == "room" ) {
			efxf(AL_EAXREVERB_GAIN, mB_to_gain(src.ParseInt(), GAIN));
		} else if ( token == "room hf" ) {
			efxf(AL_EAXREVERB_GAINHF, mB_to_gain(src.ParseInt(), GAINHF));
		} else if ( token == "room lf" ) {
			efxf(AL_EAXREVERB_GAINLF, mB_to_gain(src.ParseInt(), GAINLF));
		} else if ( token == "decay time" ) {
			efxf(AL_EAXREVERB_DECAY_TIME, src.ParseFloat());
		} else if ( token == "decay hf ratio" ) {
			efxf(AL_EAXREVERB_DECAY_HFRATIO, src.ParseFloat());
		} else if ( token == "decay lf ratio" ) {
			efxf(AL_EAXREVERB_DECAY_LFRATIO, src.ParseFloat());
		} else if ( token == "reflections" ) {
			efxf(AL_EAXREVERB_REFLECTIONS_GAIN, mB_to_gain(src.ParseInt(), REFLECTIONS_GAIN));
		} else if ( token == "reflections delay" ) {
			efxf(AL_EAXREVERB_REFLECTIONS_DELAY, src.ParseFloat());
		} else if ( token == "reflections pan" ) {
			efxfv(AL_EAXREVERB_REFLECTIONS_PAN, src.ParseFloat(), src.ParseFloat(), src.ParseFloat());
		} else if ( token == "reverb" ) {
			efxf(AL_EAXREVERB_LATE_REVERB_GAIN, mB_to_gain(src.ParseInt(), LATE_REVERB_GAIN));
		} else if ( token == "reverb delay" ) {
			efxf(AL_EAXREVERB_LATE_REVERB_DELAY, src.ParseFloat());
		} else if ( token == "reverb pan" ) {
			efxfv(AL_EAXREVERB_LATE_REVERB_PAN, src.ParseFloat(), src.ParseFloat(), src.ParseFloat());
		} else if ( token == "echo time" ) {
			efxf(AL_EAXREVERB_ECHO_TIME, src.ParseFloat());
		} else if ( token == "echo depth" ) {
			efxf(AL_EAXREVERB_ECHO_DEPTH, src.ParseFloat());
		} else if ( token == "modulation time" ) {
			efxf(AL_EAXREVERB_MODULATION_TIME, src.ParseFloat());
		} else if ( token == "modulation depth" ) {
			efxf(AL_EAXREVERB_MODULATION_DEPTH, src.ParseFloat());
		} else if ( token == "air absorption hf" ) {
			efxf(AL_EAXREVERB_AIR_ABSORPTION_GAINHF, mB_to_gain(src.ParseFloat(), AIR_ABSORPTION_GAINHF));
		} else if ( token == "hf reference" ) {
			efxf(AL_EAXREVERB_HFREFERENCE, src.ParseFloat());
		} else if ( token == "lf reference" ) {
			efxf(AL_EAXREVERB_LFREFERENCE, src.ParseFloat());
		} else if ( token == "room rolloff factor" ) {
			efxf(AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, src.ParseFloat());
		} else if ( token == "flags" ) {
			src.ReadTokenOnLine( &token );
			unsigned int flags = token.GetUnsignedIntValue();

			efxi(AL_EAXREVERB_DECAY_HFLIMIT, (flags & 0x20) ? AL_TRUE : AL_FALSE);
			// the other SCALE flags have no equivalent in efx
		} else {
			src.ReadTokenOnLine( &token );
			src.Error( "idEFXFile::ReadEffect: Invalid parameter in reverb definition" );
		}
	} while ( 1 );

	return true;
}

/*
===============
idEFXFile::LoadFile
===============
*/
bool idEFXFile::LoadFile( const char *filename/*, bool OSPath*/ ) {
	idLexer src( LEXFL_NOSTRINGCONCAT );
	idToken token;

	efxFilename = filename;
	src.LoadFile( filename/*, OSPath*/ );
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
	
	while (!src.EndOfFile()) {
		idSoundEffect *effect = new idSoundEffect;

		if (!effect->alloc()) {
			delete effect;
			Clear();
			return false;
		}

		if (ReadEffect(src, effect))
			effects.Append(effect);
		else
			delete effect;
	};

	return true;
}

/*
===============
idEFXFile::Reload
===============
*/
bool idEFXFile::Reload() {
	if (efxFilename.IsEmpty()) {
		common->Warning("EFX file was not loaded, skipping reload");
		return false;
	}
	//drop all idSoundEffect-s, delete all related OpenAL objects
	Clear();
	//mark that we reloaded the EFX file, all effects must be updated
	isAfterReload = true;
	//read all effects from file again
	return LoadFile(efxFilename);
}

/*
===============
idEFXFile::IsAfterReload
===============
*/
bool idEFXFile::IsAfterReload() {
	bool res = isAfterReload;
	isAfterReload = false;
	return res;
}
