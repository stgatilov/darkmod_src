
#ifndef RENDERPIPEPOSIX_HEADER
#define RENDERPIPEPOSIX_HEADER

#include "renderpipe.h"

class CRenderPipePosix: public CRenderPipe {
public:
	CRenderPipePosix();
	~CRenderPipePosix();
	
	int Initialize() { return 0; }
	
	const char* FileName();
	
	int Read(void *buf, int *size);
	
private:
	char m_filename[512]; // filename to read from
	int m_fd; // file descriptor to read with
};

#endif // RENDERPIPEPOSIX_HEADER
