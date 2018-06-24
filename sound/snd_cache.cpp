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
#include <limits.h>

#define USE_SOUND_CACHE_ALLOCATOR

#ifdef USE_SOUND_CACHE_ALLOCATOR
static idDynamicBlockAlloc<byte, 1<<20, 1<<10>	soundCacheAllocator;
#else
static idDynamicAlloc<byte, 1<<20, 1<<10>		soundCacheAllocator;
#endif


/*
===================
idSoundCache::idSoundCache()
===================
*/
idSoundCache::idSoundCache() {
	soundCacheAllocator.Init();
	soundCacheAllocator.SetLockMemory( true );
	listCache.AssureSize( 1024, NULL );
	listCache.SetGranularity( 256 );
	insideLevelLoad = false;
}

/*
===================
idSoundCache::~idSoundCache()
===================
*/
idSoundCache::~idSoundCache() {
	listCache.DeleteContents( true );
	soundCacheAllocator.Shutdown();
}

/*
===================
idSoundCache::::GetObject

returns a single cached object pointer
===================
*/
const idSoundSample* idSoundCache::GetObject( const int index ) const {
	if (index<0 || index>listCache.Num()) {
		return NULL;
	}
	return listCache[index]; 
}

/*
===================
idSoundCache::FindSound

Adds a sound object to the cache and returns a handle for it.
===================
*/
idSoundSample *idSoundCache::FindSound( const idStr& filename, bool loadOnDemandOnly ) {
	idStr fname;

	fname = filename;
	fname.BackSlashesToSlashes();
	fname.ToLower();

	declManager->MediaPrint( "%s\n", fname.c_str() );

	// check to see if object is already in cache
	for( int i = 0; i < listCache.Num(); i++ ) {
		idSoundSample *def = listCache[i];
		if ( def && def->name == fname ) {
			def->levelLoadReferenced = true;
			if ( def->purged && !loadOnDemandOnly ) {
				def->Load();
			}
			return def;
		}
	}

	// create a new entry
	idSoundSample *def = new idSoundSample;

	int shandle = listCache.FindNull();
	if ( shandle != -1 ) {
		listCache[shandle] = def;
	} else {
		shandle = listCache.Append( def );
	}

	def->name = fname;
	def->levelLoadReferenced = true;
	def->onDemand = loadOnDemandOnly;
	def->purged = true;

	if ( !loadOnDemandOnly ) {
		// this may make it a default sound if it can't be loaded
		def->Load();
	}

	return def;
}

/*
===================
idSoundCache::ReloadSounds

Completely nukes the current cache
===================
*/
void idSoundCache::ReloadSounds( bool force ) {
	int i;

	for( i = 0; i < listCache.Num(); i++ ) {
		idSoundSample *def = listCache[i];
		if ( def ) {
			def->Reload( force );
		}
	}
}

/*
====================
BeginLevelLoad

Mark all file based images as currently unused,
but don't free anything.  Calls to ImageFromFile() will
either mark the image as used, or create a new image without
loading the actual data.
====================
*/
void idSoundCache::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0 ; i < listCache.Num() ; i++ ) {
		idSoundSample *sample = listCache[ i ];
		if ( !sample ) {
			continue;
		}

		if ( com_purgeAll.GetBool() ) {
			sample->PurgeSoundSample();
		}

		sample->levelLoadReferenced = false;
	}

	soundCacheAllocator.FreeEmptyBaseBlocks();
}

/*
====================
EndLevelLoad

Free all samples marked as unused
====================
*/
void idSoundCache::EndLevelLoad() {
	int	useCount, purgeCount;
	common->Printf( "----- idSoundCache::EndLevelLoad -----\n" );

	insideLevelLoad = false;

	// purge the ones we don't need
	useCount = 0;
	purgeCount = 0;
	for ( int i = 0 ; i < listCache.Num() ; i++ ) {
		idSoundSample	*sample = listCache[ i ];
		if ( !sample ) {
			continue;
		}
		if ( sample->purged ) {
			continue;
		}
		if ( !sample->levelLoadReferenced ) {
//			common->Printf( "Purging %s\n", sample->name.c_str() );
			purgeCount += sample->objectMemSize;
			sample->PurgeSoundSample();
		} else {
			useCount += sample->objectMemSize;
		}
	}

	soundCacheAllocator.FreeEmptyBaseBlocks();

	common->Printf( "%5ik referenced\n", useCount / 1024 );
	common->Printf( "%5ik purged\n", purgeCount / 1024 );
	common->Printf( "----------------------------------------\n" );
}

/*
===================
idSoundCache::PrintMemInfo
===================
*/
void idSoundCache::PrintMemInfo( MemInfo_t *mi ) {
	int i, j, num = 0, total = 0;
	int *sortIndex;
	idFile *f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_sounds.txt" );
	if ( !f ) {
		return;
	}

	// count
	for ( i = 0; i < listCache.Num(); i++, num++ ) {
		if ( !listCache[i] ) {
			break;
		}
	}

	// sort first
	sortIndex = new int[num];

	for ( i = 0; i < num; i++ ) {
		sortIndex[i] = i;
	}

	for ( i = 0; i < num - 1; i++ ) {
		for ( j = i + 1; j < num; j++ ) {
			if ( listCache[sortIndex[i]]->objectMemSize < listCache[sortIndex[j]]->objectMemSize ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( i = 0; i < num; i++ ) {
		idSoundSample *sample = listCache[sortIndex[i]];

		// this is strange
		if ( !sample ) {
			continue;
		}

		total += sample->objectMemSize;
		f->Printf( "%s %s\n", idStr::FormatNumber( sample->objectMemSize ).c_str(), sample->name.c_str() );
	}

	mi->soundAssetsTotal = total;

	f->Printf( "\nTotal sound bytes allocated: %s\n", idStr::FormatNumber( total ).c_str() );
	fileSystem->CloseFile( f );
	delete[] sortIndex;
}


/*
==========================================================================

idSoundSample

==========================================================================
*/

/*
===================
idSoundSample::idSoundSample
===================
*/
idSoundSample::idSoundSample() {
	memset( &objectInfo, 0, sizeof(waveformatex_t) );
	objectSize = 0;
	objectMemSize = 0;
	nonCacheData = NULL;
	amplitudeData = NULL;
	openalBuffer = NULL;
	hardwareBuffer = false;
	defaultSound = false;
	onDemand = false;
	purged = false;
	levelLoadReferenced = false;
	cinematic = NULL;
}

/*
===================
idSoundSample::~idSoundSample
===================
*/
idSoundSample::~idSoundSample() {
	PurgeSoundSample();
}

/*
===================
idSoundSample::LengthIn44kHzSamples
===================
*/
int idSoundSample::LengthIn44kHzSamples( void ) const {
	// objectSize is samples
	if ( objectInfo.nSamplesPerSec == 11025 ) {
		return objectSize << 2;
	} else if ( objectInfo.nSamplesPerSec == 22050 ) {
		return objectSize << 1;
	} else {
		return objectSize << 0;			
	}
}

/*
===================
idSoundSample::MakeDefault
===================
*/
void idSoundSample::MakeDefault( void ) {	
	int		i;
	float	v;
	int		sample;

	memset( &objectInfo, 0, sizeof( objectInfo ) );

	objectInfo.nChannels = 1;
	objectInfo.wBitsPerSample = 16;
	objectInfo.nSamplesPerSec = 44100;

	objectSize = MIXBUFFER_SAMPLES * 2;
	objectMemSize = objectSize * sizeof( short );

	nonCacheData = (byte *)soundCacheAllocator.Alloc( objectMemSize );

	short *ncd = (short *)nonCacheData;

	for ( i = 0; i < MIXBUFFER_SAMPLES; i ++ ) {
		v = sin( idMath::PI * 2 * i / 64 );
		sample = v * 0x4000;
		ncd[i*2+0] = sample;
		ncd[i*2+1] = sample;
	}

	alGetError();
	alGenBuffers(1, &openalBuffer);
	if (alGetError() != AL_NO_ERROR) {
		common->Error("idSoundCache: error generating OpenAL hardware buffer");
	}

	alGetError();
	alBufferData(openalBuffer, objectInfo.nChannels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, nonCacheData, objectMemSize, objectInfo.nSamplesPerSec);
	if (alGetError() != AL_NO_ERROR) {
		common->Error("idSoundCache: error loading data into OpenAL hardware buffer");
	}
	else {
		hardwareBuffer = true;
	}

	defaultSound = true;
}

/*
===================
idSoundSample::CheckForDownSample
===================
*/
void idSoundSample::CheckForDownSample( void ) {
	if ( !idSoundSystemLocal::s_force22kHz.GetBool() ) {
		return;
	}
	if ( objectInfo.wFormatTag != WAVE_FORMAT_TAG_PCM || objectInfo.nSamplesPerSec != 44100 ) {
		return;
	}
	int shortSamples = objectSize >> 1;
	short *converted = (short *)soundCacheAllocator.Alloc( shortSamples * sizeof( short ) );

	if ( objectInfo.nChannels == 1 ) {
		for ( int i = 0; i < shortSamples; i++ ) {
			converted[i] = ((short *)nonCacheData)[i*2];
		}
	} else {
		for ( int i = 0; i < shortSamples; i += 2 ) {
			converted[i+0] = ((short *)nonCacheData)[i*2+0];
			converted[i+1] = ((short *)nonCacheData)[i*2+1];
		}
	}
	soundCacheAllocator.Free( nonCacheData );
	nonCacheData = (byte *)converted;
	objectSize = shortSamples;
	objectMemSize = shortSamples * sizeof( short );
	objectInfo.nAvgBytesPerSec >>= 1;
	objectInfo.nSamplesPerSec >>= 1;
}

/*
===================
idSoundSample::GetNewTimeStamp
===================
*/
ID_TIME_T idSoundSample::GetNewTimeStamp( void ) const {
	ID_TIME_T timestamp;

	fileSystem->ReadFile( name, NULL, &timestamp );
	if ( timestamp == FILE_NOT_FOUND_TIMESTAMP ) {
		idStr oggName = name;
		oggName.SetFileExtension( ".ogg" );
		fileSystem->ReadFile( oggName, NULL, &timestamp );
	}
	return timestamp;
}

inline void IssueSoundSampleFailure(const char* message, ALenum errorCode, const char* soundName)
{
	common->Error("%s: file: %s, error code: %d", message, soundName, errorCode);
}

void idSoundSample::LoadFromCinematic(idCinematic *cin) {
	if (!cin) {
		MakeDefault();
		return;
	}

	cinematic = cin;

	objectInfo.wFormatTag = WAVE_FORMAT_TAG_STREAM_CINEMATICS;
	objectInfo.nChannels = 2;	//stereo
	objectInfo.nSamplesPerSec = PRIMARYFREQ;	//44100 hz
	objectInfo.nAvgBytesPerSec = 0;	//nobody cares
	objectInfo.nBlockAlign = sizeof(float) * objectInfo.nChannels;
	objectInfo.wBitsPerSample = sizeof(float) * 8;
	objectInfo.cbSize = 0;

	//cinematic decides when it ends: set infinite duration here
	objectSize = INT_MAX / 2;
}

/*
===================
idSoundSample::Load

Loads based on name, possibly doing a MakeDefault if necessary
===================
*/
void idSoundSample::Load( void ) {	
	defaultSound = false;
	purged = false;
	hardwareBuffer = false;

	//stgatilov #4847: check if this sound was created via testVideo command
	if (name.CmpPrefix("__testvideo") == 0) {
		idCinematic *cin = 0;
		sscanf(name.c_str(), "__testvideo:%p__", &cin);
		return LoadFromCinematic(cin);
	}
	//stgatilov #4534: material name may be specified instead of sound file
	//in such case this material must have cinematics, and sound is taken from there
	if (const idMaterial *material = declManager->FindMaterial(name, false)) {
		idCinematic *cin = material->GetCinematic();
		return LoadFromCinematic(cin);
	}

	timestamp = GetNewTimeStamp();

	if ( timestamp == FILE_NOT_FOUND_TIMESTAMP ) {
		common->Warning( "Couldn't load sound '%s' using default", name.c_str() );
		MakeDefault();
		return;
	}

	// load it
	idWaveFile	fh;
	waveformatex_t info;

	if ( fh.Open( name, &info ) == -1 ) {
		common->Warning( "Couldn't load sound '%s' using default", name.c_str() );
		MakeDefault();
		return;
	}

	if ( info.nChannels != 1 && info.nChannels != 2 ) {
		common->Warning( "idSoundSample: %s has %i channels, using default", name.c_str(), info.nChannels );
		fh.Close();
		MakeDefault();
		return;
	}

	if ( info.wBitsPerSample != 16 ) {
		common->Warning( "idSoundSample: %s is %dbits, expected 16bits using default", name.c_str(), info.wBitsPerSample );
		fh.Close();
		MakeDefault();
		return;
	}

	if ( info.nSamplesPerSec != 44100 && info.nSamplesPerSec != 22050 && info.nSamplesPerSec != 11025 ) {
		common->Warning( "idSoundCache: %s is %dHz, expected 11025, 22050 or 44100 Hz. Using default", name.c_str(), info.nSamplesPerSec );
		fh.Close();
		MakeDefault();
		return;
	}

	objectInfo = info;
	objectSize = fh.GetOutputSize();
	objectMemSize = fh.GetMemorySize();

	nonCacheData = (byte *)soundCacheAllocator.Alloc( objectMemSize );
	fh.Read( nonCacheData, objectMemSize, NULL );

	// optionally convert it to 22kHz to save memory
	CheckForDownSample();

	// create hardware audio buffers 
	// PCM loads directly
	if (objectInfo.wFormatTag == WAVE_FORMAT_TAG_PCM) {
		alGetError();
		alGenBuffers(1, &openalBuffer);

		ALenum errorCode = alGetError();

		if (errorCode != AL_NO_ERROR)
			IssueSoundSampleFailure("idSoundCache: WAV error generating OpenAL hardware buffer", errorCode, name.c_str());
		if (alIsBuffer(openalBuffer)) {
			alGetError();
			alBufferData(openalBuffer, objectInfo.nChannels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, nonCacheData, objectMemSize, objectInfo.nSamplesPerSec);

			errorCode = alGetError();
			if (errorCode != AL_NO_ERROR)
			{
				IssueSoundSampleFailure("idSoundCache: WAV error loading data into OpenAL hardware buffer", errorCode, name.c_str());
			}
			else {
				hardwareBuffer = true;
			}
		}
		
		// OGG decompressed at load time (when smaller than s_decompressionLimit seconds, 6 seconds by default)
		if ( objectInfo.wFormatTag == WAVE_FORMAT_TAG_OGG ) {
			if ( ( objectSize < ( ( int ) objectInfo.nSamplesPerSec * idSoundSystemLocal::s_decompressionLimit.GetInteger() ) ) ) {
				alGetError();
				alGenBuffers( 1, &openalBuffer );
				errorCode = alGetError();
				if (errorCode != AL_NO_ERROR)
				{
						IssueSoundSampleFailure("idSoundCache: OGG error generating OpenAL hardware buffer", errorCode, name.c_str());
				}
				if ( alIsBuffer( openalBuffer ) ) {
					idSampleDecoder *decoder = idSampleDecoder::Alloc();
					float *destData = (float *)soundCacheAllocator.Alloc( ( LengthIn44kHzSamples() + 1 ) * sizeof( float ) );
					
					// Decoder *always* outputs 44 kHz data
					decoder->Decode( this, 0, LengthIn44kHzSamples(), destData );

					// Downsample back to original frequency (save memory)
					if ( objectInfo.nSamplesPerSec == 11025 ) {
						for ( int i = 0; i < objectSize; i++ ) {
							if ( destData[i*4] < -32768.0f )
								((short *)destData)[i] = -32768;
							else if ( destData[i*4] > 32767.0f )
								((short *)destData)[i] = 32767;
							else
								((short *)destData)[i] = idMath::FtoiFast( destData[i*4] );
						}
					} else if ( objectInfo.nSamplesPerSec == 22050 ) {
						for ( int i = 0; i < objectSize; i++ ) {
							if ( destData[i*2] < -32768.0f )
								((short *)destData)[i] = -32768;
							else if ( destData[i*2] > 32767.0f )
								((short *)destData)[i] = 32767;
							else
								((short *)destData)[i] = idMath::FtoiFast( destData[i*2] );
						}
					} else {
						for ( int i = 0; i < objectSize; i++ ) {
							if ( destData[i] < -32768.0f )
								((short *)destData)[i] = -32768;
							else if ( destData[i] > 32767.0f )
								((short *)destData)[i] = 32767;
							else
								((short *)destData)[i] = idMath::FtoiFast( destData[i] );
						}
					}

					alGetError();
					alBufferData( openalBuffer, objectInfo.nChannels==1?AL_FORMAT_MONO16:AL_FORMAT_STEREO16, destData, objectSize * sizeof( short ), objectInfo.nSamplesPerSec );
					ALenum alError = alGetError();
					if ( alError != AL_NO_ERROR )
						IssueSoundSampleFailure("idSoundCache Load OGG: error loading data into OpenAL hardware buffer", alError, name.c_str());
					else {
						hardwareBuffer = true;
					}

					soundCacheAllocator.Free( (byte *)destData );
					idSampleDecoder::Free( decoder );
				}
			}
		}
	}

	fh.Close();
}

/*
===================
idSoundSample::PurgeSoundSample
===================
*/
void idSoundSample::PurgeSoundSample() {
	purged = true;

	alGetError();
	alDeleteBuffers(1, &openalBuffer);
	if (alGetError() != AL_NO_ERROR) {
		common->Error("idSoundCache: error unloading data from OpenAL hardware buffer");
	}
	else {
		openalBuffer = 0;
		hardwareBuffer = false;
	}

	if ( amplitudeData ) {
		soundCacheAllocator.Free( amplitudeData );
		amplitudeData = NULL;
	}

	if ( nonCacheData ) {
		soundCacheAllocator.Free( nonCacheData );
		nonCacheData = NULL;
	}
}

/*
===================
idSoundSample::Reload
===================
*/
void idSoundSample::Reload( bool force ) {
	if ( !force ) {
		ID_TIME_T newTimestamp;

		// check the timestamp
		newTimestamp = GetNewTimeStamp();

		if ( newTimestamp == FILE_NOT_FOUND_TIMESTAMP ) {
			if ( !defaultSound ) {
				common->Warning( "Couldn't load sound '%s' using default", name.c_str() );
				MakeDefault();
			}
			return;
		}
		if ( newTimestamp == timestamp ) {
			return;	// don't need to reload it
		}
	}

	common->Printf( "reloading %s\n", name.c_str() );
	PurgeSoundSample();
	Load();
}

/*
===================
idSoundSample::FetchFromCache

Returns true on success.
===================
*/
bool idSoundSample::FetchFromCache( int offset, const byte **output, int *position, int *size, const bool allowIO ) {
	offset &= 0xfffffffe;

	if ( objectSize == 0 || offset < 0 || offset > objectSize * (int)sizeof( short ) || !nonCacheData ) {
		return false;
	}

	if ( output ) {
		*output = nonCacheData + offset;
	}
	if ( position ) {
		*position = 0;
	}
	if ( size ) {
		*size = objectSize * sizeof( short ) - offset;
		if ( *size > SCACHE_SIZE ) {
			*size = SCACHE_SIZE;
		}
	}
	return true;
}

bool idSoundSample::FetchFromCinematic(int sampleOffset, int *sampleSize, float *output) {
	assert(cinematic);
	return cinematic->SoundForTimeInterval(sampleOffset, sampleSize, PRIMARYFREQ, output);
}
