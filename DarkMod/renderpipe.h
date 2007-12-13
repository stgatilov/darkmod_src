/**
 * A pipe that is used to read the snapshot images from. This is neccessary, because we have to store
 * the render snapshots somewhere and the only way to do this is via a pipe if we want to
 * avoid writing it constantly to disk.
 */
#ifndef RENDERPIPE_HEADER
#define RENDERPIPE_HEADER
 
#define DARKMOD_LG_RENDERPIPE_BUFSIZE		500*1024		// Buffersize for the renderpipe
#define DARKMOD_LG_RENDERPIPE_NAME_WINDOWS	"\\\\.\\pipe\\dm_renderpipe" // The filename of the render pipe, on Windows ONLY.

class CRenderPipe {
public:
	/** Call this to initialize the render pipe after constructing it.
	    @return Zero upon success; a system-specific error code upon failure.
	*/
	virtual int Initialize()=0;
	
	/** Return the filename, relative to fs_savepath, of the pipe. */
	virtual const char* FileName()=0;
	
	/** Retrieve up to size bytes from the pipe, and store them into the given buffer.
	    The actual number of bytes read is stored in size.
	    
	    @param size Value-result parameter. Must hold the maximum size of buf when passed it.
	                If this function returns successfully, size will hold the number of bytes actually read.
	    @return Zero upon success; a system-specific error code upon failure.
	*/
	virtual int Read(char *buf, int *size)=0;
	
	virtual ~CRenderPipe()=0;
};

#endif //RENDERPIPE_HEADER
