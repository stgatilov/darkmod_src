//#include "libvorbis-1.3.5/include/vorbis/vorbisfile.h"
#include "../sound/OggVorbis/vorbis/vorbisfile.h"

#ifdef MAKEDLL
#  define EXTLIB __declspec(dllexport)
#else
#  define EXTLIB __declspec(dllimport)
#endif

namespace ExtLibs {
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