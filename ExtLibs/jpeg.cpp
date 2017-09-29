#include <stdio.h>
#include "jpeg.h"
extern "C" {
#include <jpegint.h>
}

namespace ExtLibs {
	EXTLIB EXTERN( struct jpeg_error_mgr * ) jpeg_std_error
		JPP( (struct jpeg_error_mgr * err) ) {
		return ::jpeg_std_error( err );
	}
	EXTLIB EXTERN( void ) jpeg_CreateDecompress JPP( (j_decompress_ptr cinfo,
		int version, size_t structsize) ) {
		return ::jpeg_CreateDecompress( cinfo, version, structsize );
	}
	EXTLIB EXTERN( void ) jpeg_destroy_decompress JPP( (j_decompress_ptr cinfo) ) {
		return ::jpeg_destroy_decompress( cinfo );
	}
	EXTLIB EXTERN( int ) jpeg_read_header JPP( (j_decompress_ptr cinfo,
		boolean require_image) ) {
		return ::jpeg_read_header( cinfo, require_image );
	}
	EXTLIB EXTERN( boolean ) jpeg_start_decompress JPP( (j_decompress_ptr cinfo) ) {
		return ::jpeg_start_decompress( cinfo );
	}
	EXTLIB EXTERN( JDIMENSION ) jpeg_read_scanlines JPP( (j_decompress_ptr cinfo,
		JSAMPARRAY scanlines,
		JDIMENSION max_lines) ) {
		return ::jpeg_read_scanlines( cinfo, scanlines, max_lines );
	}
	EXTLIB EXTERN( boolean ) jpeg_finish_decompress JPP( (j_decompress_ptr cinfo) ) {
		return ::jpeg_finish_decompress( cinfo );
	}
	EXTLIB EXTERN( boolean ) jpeg_resync_to_restart JPP( (j_decompress_ptr cinfo,
		int desired) ) {
		return ::jpeg_resync_to_restart( cinfo, desired );
	}
	EXTLIB EXTERN( void ) jpeg_mem_src JPP( (j_decompress_ptr cinfo,
		unsigned char * inbuffer,
		unsigned long insize) ) {
		return ::jpeg_mem_src( cinfo, inbuffer, insize );
	}
	EXTLIB EXTERN( void ) jpeg_CreateCompress JPP( (j_compress_ptr cinfo,
		int version, size_t structsize) ) {
		return ::jpeg_CreateCompress(cinfo, version, structsize);
	}
	EXTLIB EXTERN( void ) jpeg_destroy_compress JPP( (j_compress_ptr cinfo) ) {
		return ::jpeg_destroy_compress( cinfo );
	}
	EXTLIB EXTERN( void ) jpeg_set_defaults JPP( (j_compress_ptr cinfo) ) {
		return ::jpeg_set_defaults( cinfo );
	}
	EXTLIB EXTERN( void ) jpeg_set_quality JPP( (j_compress_ptr cinfo, int quality,
		boolean force_baseline) ) {
		return ::jpeg_set_quality( cinfo, quality, force_baseline );
	}
	EXTLIB EXTERN( void ) jpeg_suppress_tables JPP( (j_compress_ptr cinfo,
		boolean suppress) ) {
		return ::jpeg_suppress_tables( cinfo, suppress );
	}
	EXTLIB EXTERN( void ) jpeg_finish_compress JPP( (j_compress_ptr cinfo) ) {
		return ::jpeg_finish_compress( cinfo );
	}
	EXTLIB EXTERN( void ) jinit_compress_master JPP( (j_compress_ptr cinfo) ) {
		return ::jinit_compress_master( cinfo );
	}

}