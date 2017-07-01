extern "C" {
#include <jpeglib.h>
}

#include "Export.h"

namespace ExtLibs {
	EXTLIB EXTERN( struct jpeg_error_mgr * ) jpeg_std_error
		JPP( (struct jpeg_error_mgr * err) );
	EXTLIB EXTERN( void ) jpeg_CreateDecompress JPP( (j_decompress_ptr cinfo,
		int version, size_t structsize) );
	EXTLIB EXTERN( void ) jpeg_destroy_decompress JPP( (j_decompress_ptr cinfo) );
	EXTLIB EXTERN( int ) jpeg_read_header JPP( (j_decompress_ptr cinfo,
		boolean require_image) );
	EXTLIB EXTERN( boolean ) jpeg_start_decompress JPP( (j_decompress_ptr cinfo) );
	EXTLIB EXTERN( JDIMENSION ) jpeg_read_scanlines JPP( (j_decompress_ptr cinfo,
		JSAMPARRAY scanlines,
		JDIMENSION max_lines) );
	EXTLIB EXTERN( boolean ) jpeg_finish_decompress JPP( (j_decompress_ptr cinfo) );
	EXTLIB EXTERN( boolean ) jpeg_resync_to_restart JPP( (j_decompress_ptr cinfo,
		int desired) );
	EXTLIB EXTERN( void ) jpeg_mem_src JPP( (j_decompress_ptr cinfo,
		unsigned char * inbuffer,
		unsigned long insize) );
	EXTLIB EXTERN( void ) jpeg_CreateCompress JPP( (j_compress_ptr cinfo,
		int version, size_t structsize) );
	EXTLIB EXTERN( void ) jpeg_destroy_compress JPP( (j_compress_ptr cinfo) );
	EXTLIB EXTERN( void ) jpeg_set_defaults JPP( (j_compress_ptr cinfo) );
	EXTLIB EXTERN( void ) jpeg_set_quality JPP( (j_compress_ptr cinfo, int quality,
		boolean force_baseline) );
	EXTLIB EXTERN( void ) jpeg_suppress_tables JPP( (j_compress_ptr cinfo,
		boolean suppress) );
	EXTLIB EXTERN( void ) jpeg_finish_compress JPP( (j_compress_ptr cinfo) );
	EXTLIB EXTERN( void ) jinit_compress_master JPP( (j_compress_ptr cinfo) );
}