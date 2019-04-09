#include "minizip_extra.h"
#include <string.h>

//stgatilov: Note that these data structures must be kept in sync with stuff defined in unzip.c.
//Given that minizip's development is almost frozen, I guess they will rarely change.

#ifndef ALLOC
# define ALLOC(size) (malloc(size))
#endif
#ifndef TRYFREE
# define TRYFREE(p) {if (p) free(p);}
#endif

#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (16384)
#endif

/* unz_file_info_interntal contain internal info about a file in zipfile*/
typedef struct unz_file_info64_internal_s
{
	ZPOS64_T offset_curfile;/* relative offset of local header 8 bytes */
} unz_file_info64_internal;

/* file_in_zip_read_info_s contain internal information about a file in zipfile,
	when reading and decompress it */
typedef struct
{
	char  *read_buffer;         /* internal buffer for compressed data */
	z_stream stream;            /* zLib stream structure for inflate */

#ifdef HAVE_BZIP2
	bz_stream bstream;          /* bzLib stream structure for bziped */
#endif

	ZPOS64_T pos_in_zipfile;       /* position in byte on the zipfile, for fseek*/
	uLong stream_initialised;   /* flag set if stream structure is initialised*/

	ZPOS64_T offset_local_extrafield;/* offset of the local extra field */
	uInt  size_local_extrafield;/* size of the local extra field */
	ZPOS64_T pos_local_extrafield;   /* position in the local extra field in read*/
	ZPOS64_T total_out_64;

	uLong crc32;                /* crc32 of all data uncompressed */
	uLong crc32_wait;           /* crc32 we must obtain after decompress all */
	ZPOS64_T rest_read_compressed; /* number of byte to be decompressed */
	ZPOS64_T rest_read_uncompressed;/*number of byte to be obtained after decomp*/
	zlib_filefunc64_32_def z_filefunc;
	voidpf filestream;        /* io structore of the zipfile */
	uLong compression_method;   /* compression method (0==store) */
	ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
	int   raw;
} file_in_zip64_read_info_s;

typedef struct
{
	zlib_filefunc64_32_def z_filefunc;
	int is64bitOpenFunction;
	voidpf filestream;        /* io structore of the zipfile */
	unz_global_info64 gi;       /* public global information */
	ZPOS64_T byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
	ZPOS64_T num_file;             /* number of the current file in the zipfile*/
	ZPOS64_T pos_in_central_dir;   /* pos of the current file in the central dir*/
	ZPOS64_T current_file_ok;      /* flag about the usability of the current file*/
	ZPOS64_T central_pos;          /* position of the beginning of the central dir*/

	ZPOS64_T size_central_dir;     /* size of the central directory  */
	ZPOS64_T offset_central_dir;   /* offset of start of central directory with
								   respect to the starting disk number */

	unz_file_info64 cur_file_info; /* public info about the current file in zip*/
	unz_file_info64_internal cur_file_info_internal; /* private info about it*/
	file_in_zip64_read_info_s* pfile_in_zip_read; /* structure about the current
										file if we are decompressing it */
	int encrypted;

	int isZip64;
} unz64_s;

//=======================================================================================================

extern unzFile unzReOpen (const char* path, unzFile file)
{
	unz64_s* s;
	unz64_s* zFile = (unz64_s*)file;

	if(zFile == NULL)
		return NULL;

	// create unz64_s* "s" as clone of "file"
	s=(unz64_s*)ALLOC(sizeof(unz64_s));
	if(s == NULL)
		return NULL;

	memcpy(s, zFile, sizeof(unz64_s));

	// create new filestream for path
	voidp fin = ZOPEN64(s->z_filefunc,
						path,
						ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_EXISTING);

	if( fin == NULL ) {
		TRYFREE(s);
		return NULL;
	}

	// set that filestream in s
	s->filestream = fin;

	unzOpenCurrentFile( s );

	return (unzFile)s;
}

extern int ZEXPORT unzseek(unzFile file, z_off_t offset, int origin)
{
	return unzseek64(file, (ZPOS64_T)offset, origin);
}

extern int ZEXPORT unzseek64(unzFile file, ZPOS64_T offset, int origin)
{
	unz64_s* s;
	file_in_zip64_read_info_s* pfile_in_zip_read_info;
	ZPOS64_T stream_pos_begin;
	ZPOS64_T stream_pos_end;
	int isWithinBuffer;
	ZPOS64_T position;

	if (file == NULL)
		return UNZ_PARAMERROR;

	s = (unz64_s*)file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;

	if (pfile_in_zip_read_info == NULL)
		return UNZ_ERRNO;

	if (pfile_in_zip_read_info->compression_method != 0)
		return UNZ_ERRNO;

	if (origin == SEEK_SET)
		position = offset;
	else if (origin == SEEK_CUR)
		position = pfile_in_zip_read_info->total_out_64 + offset;
	else if (origin == SEEK_END)
		position = s->cur_file_info.compressed_size + offset;
	else
		return UNZ_PARAMERROR;

	if (position > s->cur_file_info.compressed_size)
		return UNZ_PARAMERROR;

	stream_pos_end = pfile_in_zip_read_info->pos_in_zipfile;
	stream_pos_begin = stream_pos_end;
	if (stream_pos_begin > UNZ_BUFSIZE)
		stream_pos_begin -= UNZ_BUFSIZE;
	else
		stream_pos_begin = 0;

	isWithinBuffer = pfile_in_zip_read_info->stream.avail_in != 0 &&
		(pfile_in_zip_read_info->rest_read_compressed != 0 || s->cur_file_info.compressed_size < UNZ_BUFSIZE) &&
		position >= stream_pos_begin && position < stream_pos_end;

	if (isWithinBuffer)
	{
		pfile_in_zip_read_info->stream.next_in += position - pfile_in_zip_read_info->total_out_64;
		pfile_in_zip_read_info->stream.avail_in = (uInt)(stream_pos_end - position);
	}
	else
	{
		pfile_in_zip_read_info->stream.avail_in = 0;
		pfile_in_zip_read_info->stream.next_in = 0;

		pfile_in_zip_read_info->pos_in_zipfile = pfile_in_zip_read_info->offset_local_extrafield + position;
		pfile_in_zip_read_info->rest_read_compressed = s->cur_file_info.compressed_size - position;
	}

	pfile_in_zip_read_info->rest_read_uncompressed -= (position - pfile_in_zip_read_info->total_out_64);
	pfile_in_zip_read_info->stream.total_out = (uLong)position;
	pfile_in_zip_read_info->total_out_64 = position;

	return UNZ_OK;
}
