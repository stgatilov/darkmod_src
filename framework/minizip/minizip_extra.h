#pragma once

#include "unzip.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unzFile unzReOpen( const char* path, unzFile file );
/* Re-Open a Zip file, i.e. clone an existing one and give it a new file descriptor. */

extern int ZEXPORT unzseek OF((unzFile file, z_off_t offset, int origin));
extern int ZEXPORT unzseek64 OF((unzFile file, ZPOS64_T offset, int origin));
/* Seek within the uncompressed data if compression method is storage. */

#ifdef __cplusplus
}
#endif
