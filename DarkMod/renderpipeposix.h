#ifndef RENDERPIPEPOSIX_HEADER
#define RENDERPIPEPOSIX_HEADER

class CRenderPipe
{
public:
	CRenderPipe();
	~CRenderPipe();
	
	/** Call this to prepare the render pipe before *each* snapshot is taken.
	    @return Zero upon success; a system-specific error code upon failure.
	*/
	int Prepare()
	{
		// Nothing to do
		return 0;
	}
	
	/** Call this to clean up the render pipe after receiving each snapshot. */
	void CleanUp()
	{
		// Nothing to do
	}
	
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
	char m_filename[512]; // filename to read from
	int m_fd; // file descriptor to read with
};

#endif // RENDERPIPEPOSIX_HEADER
