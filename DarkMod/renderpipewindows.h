#ifndef RENDERPIPEWINDOWS_HEADER
#define RENDERPIPEWINDOWS_HEADER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DARKMOD_LG_RENDERPIPE_TIMEOUT		1000
#define DARKMOD_LG_RENDERPIPE_NAME_WINDOWS	"\\\\.\\pipe\\dm_renderpipe" // The filename of the render pipe, on Windows

class CRenderPipe
{
public:
	CRenderPipe() : m_hPipe(INVALID_HANDLE_VALUE)
	{
	}

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
	int Read(void *buf, int *size);

private:
	HANDLE m_hPipe;
	SECURITY_ATTRIBUTES		m_saPipeSecurity;
	PSECURITY_DESCRIPTOR	m_pPipeSD;
};

#endif // RENDERPIPEWINDOWS_HEADER

