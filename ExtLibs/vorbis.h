//#include "libvorbis-1.3.5/include/vorbis/vorbisfile.h"
#include "../sound/OggVorbis/vorbis/vorbisfile.h"

#include "Export.h"

struct ov_alloc_callbacks {
	void* (*_decoder_malloc)( size_t size );
	void* (*_decoder_calloc)( size_t num, size_t size );
	void* (*_decoder_realloc)( void *memblock, size_t size );
	void (*_decoder_free)( void *memblock );
};

namespace ExtLibs {
	EXTLIB void ov_use_custom_alloc(ov_alloc_callbacks callbacks);

	EXTLIB int ov_clear( OggVorbis_File *vf );
	EXTLIB extern int ov_open_callbacks( void *datasource, OggVorbis_File *vf,
		char *initial, long ibytes, ov_callbacks callbacks );
	EXTLIB vorbis_info *ov_info( OggVorbis_File *vf, int link );
	EXTLIB ogg_int64_t ov_pcm_total( OggVorbis_File *vf, int i );
	EXTLIB long ov_read( OggVorbis_File *vf, char *buffer, int length,
		int bigendianp, int word, int sgned, int *bitstream );
	EXTLIB int ov_pcm_seek( OggVorbis_File *vf, ogg_int64_t pos );
	EXTLIB long ov_read_float( OggVorbis_File *vf, float ***pcm_channels, int samples,
		int *bitstream );

}