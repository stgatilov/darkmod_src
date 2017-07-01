#include "stdafx.h"
#include "vorbis.h"
#include <stdlib.h>
#include <string.h>

extern "C" {
	void *_decoder_malloc( size_t size );
	void *_decoder_calloc( size_t num, size_t size );
	void *_decoder_realloc( void *memblock, size_t size );
	void _decoder_free( void *memblock );
}

void *_decoder_malloc( size_t size ) {
	void *ptr = malloc(size);
	//assert( size == 0 || ptr != NULL );
	return ptr;
}

void *_decoder_calloc( size_t num, size_t size ) {
	void *ptr = malloc(size*num);
	//assert( (num * size) == 0 || ptr != NULL );
	memset( ptr, 0, num * size );
	return ptr;
}

void *_decoder_realloc( void *memblock, size_t size ) {
	void *ptr = realloc( memblock, static_cast<int>(size) );
	//assert( size == 0 || ptr != NULL );
	return ptr;
}

void _decoder_free( void *memblock ) {
	free(memblock);
}

namespace ExtLibs {
	EXTLIB int ov_clear( OggVorbis_File *vf ) {
		return ::ov_clear( vf );
	}

	EXTLIB int ov_open_callbacks( void *datasource, OggVorbis_File *vf,
		char *initial, long ibytes, ov_callbacks callbacks ) {
		return ::ov_open_callbacks( datasource, vf,
			initial, ibytes, callbacks );
	}

	EXTLIB vorbis_info *ov_info( OggVorbis_File *vf, int link ) {
		return ::ov_info( vf, link );
	}

	EXTLIB ogg_int64_t ov_pcm_total( OggVorbis_File *vf, int i ) {
		return ::ov_pcm_total( vf, i );
	}

	EXTLIB long ov_read( OggVorbis_File *vf, char *buffer, int length,
		int bigendianp, int word, int sgned, int *bitstream ) {
		return ::ov_read( vf, buffer, length,
			bigendianp, word, sgned, bitstream );
	}

	EXTLIB int ov_pcm_seek( OggVorbis_File *vf, ogg_int64_t pos ) {
		return ::ov_pcm_seek( vf, pos );
	}

	EXTLIB long ov_read_float( OggVorbis_File *vf, float ***pcm_channels, int samples,
		int *bitstream ) {
		return ::ov_read_float( vf, pcm_channels, samples,
			bitstream );
	}
}