/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1869 $
 * $Date: 2007-12-13 12:45:27 +0100 (Do, 13 Dez 2007) $
 * $Author: crispy $
 *
 ***************************************************************************/

/**
 * A pipe that is used to read the snapshot images from. This is neccessary, because we have to store
 * the render snapshots somewhere and the only way to do this is via a pipe if we want to
 * avoid writing it constantly to disk.
 */

// Each platform has a different implementation, though with compatible (source-wise) interfaces.

#define DARKMOD_LG_RENDERPIPE_BUFSIZE		500*1024		// Buffersize for the renderpipe
#define DARKMOD_LG_RENDERPIPE_TIMEOUT		1000
#define DARKMOD_LG_RENDERPIPE_NAME	"\\\\.\\pipe\\dm_renderpipe" // The filename of the render pipe, only needed for Windows

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

/*#ifndef HANDLE
#define HANDLE int
#endif*/

class CRenderPipe
{
public:
	CRenderPipe();
	~CRenderPipe();
	
	/** Call this to prepare the render pipe before *each* snapshot is taken.
	    @return Zero upon success; a system-specific error code upon failure.
	*/
	int Prepare();
	
	/** Call this to clean up the render pipe after receiving each snapshot. */
	void CleanUp();
	
	/** Return the filename, relative to fs_savepath, of the pipe. */
	const char* FileName();
	
	/** Retrieve up to size bytes from the pipe, and store them into the given buffer.
	    The actual number of bytes read is stored in size.
	    
	    @param size Value-result parameter. Must hold the maximum size of buf when passed it.
	                If this function returns successfully, size will hold the number of bytes actually read.
	    @return Zero upon success; a system-specific error code upon failure.
	*/
	int Read(char *buf, int *size);
	
private:
	char m_filename[512]; // filename to read from
	HANDLE m_fd; // file descriptor to read with
};

