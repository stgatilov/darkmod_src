#pragma once

#include <unzip.h>
#include <zip.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unzFile unzReOpen( const char* path, unzFile file );
/* Re-Open a Zip file, i.e. clone an existing one and give it a new file descriptor. */

extern int ZEXPORT unzseek OF((unzFile file, z_off_t offset, int origin));
extern int ZEXPORT unzseek64 OF((unzFile file, ZPOS64_T offset, int origin));
/* Seek within the uncompressed data if compression method is storage. */

extern int ZEXPORT unzIsZip64(unzFile file);
/* Returns 1 iff specified file is zip64 */

extern int ZEXPORT minizipCopyDataRaw(unzFile srcHandle, zipFile dstHandle, voidp buffer, unsigned bufSize);
/* Directly copies current file data from unz file to zip file.
   Both unz and zip file must be opened in raw mode, without any bytes read/written to them. */

#ifdef __cplusplus
}
#endif
