/**
 * A pipe that is used to read the snapshot images from. This is neccessary, because we have to store
 * the render snapshots somewhere and the only way to do this is via a pipe if we want to
 * avoid writing it constantly to disk.
 */

// Each platform has a different implementation, though with compatible (source-wise) interfaces.

#define DARKMOD_LG_RENDERPIPE_BUFSIZE		500*1024		// Buffersize for the renderpipe

#ifdef _WINDOWS
#include "renderpipewindows.h"
#else
#include "renderpipeposix.h"
#endif
