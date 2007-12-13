#ifndef RENDERPIPEWINDOWS_HEADER
#define RENDERPIPEWINDOWS_HEADER

#include "renderpipe.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CRenderPipeWindows: public CRenderPipe {
public:
	CRenderPipeWindows() : m_hPipe(INVALID_HANDLE_VALUE)
	{
	}
	
	int Initialize();
	
	const char* FileName();
	
	int Read(void *buf, int *size);
	
	~CRenderPipeWindows();
private:
	HANDLE m_hPipe;
};

#endif // RENDERPIPEWINDOWS_HEADER

