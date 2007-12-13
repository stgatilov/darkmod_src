#include "renderpipewindows.h"

#define DARKMOD_LG_RENDERPIPE_NAME			DARKMOD_LG_RENDERPIPE_NAME_WINDOWS
#define DARKMOD_LG_RENDERPIPE_TIMEOUT		1000

int CRenderPipeWindows::Initialize()
{
	m_hPipe = CreateNamedPipe (DARKMOD_LG_RENDERPIPE_NAME,
		PIPE_ACCESS_DUPLEX,				// read/write access
		PIPE_TYPE_MESSAGE |				// message type pipe
		PIPE_READMODE_MESSAGE |			// message-read mode
		PIPE_WAIT,						// blocking mode
		PIPE_UNLIMITED_INSTANCES,		// max. instances
		DARKMOD_LG_RENDERPIPE_BUFSIZE,		// output buffer size
		DARKMOD_LG_RENDERPIPE_BUFSIZE,		// input buffer size
		DARKMOD_LG_RENDERPIPE_TIMEOUT,		// client time-out
		&m_saPipeSecurity);				// no security attribute
	if (m_hPipe == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}
}

const char* CRenderPipeWindows::FileName()
{
	return DARKMOD_LG_RENDERPIPE_NAME;
}

int CRenderPipeWindows::Read(void *buf, int *size)
{
	DWORD cbBytesRead, dwBufSize, BufLen, dwLastError;

	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Reading from renderpipe [%08lX]\r", Handle);

	dwBufSize = *size;
	BufLen = 0;
	while(1)

	{

		ReadFile(Handle, // handle to pipe
			&buf[BufLen],						// buffer to receive data
			dwBufSize,								// size of buffer
			&cbBytesRead,							// number of bytes read
			NULL);									// not overlapped I/O

		dwLastError = GetLastError();
		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("%lu bytes read from renderpipe [%08lX]   %lu (%08lX) %lu\r",
			cbBytesRead, Handle, BufLen, m_Image, dwLastError);

		BufLen += cbBytesRead;
		dwBufSize -= cbBytesRead;

		if(cbBytesRead == 0 || dwLastError == ERROR_BROKEN_PIPE) break;
		
		if(dwBufSize <= 0)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Bufferoverflow when reading from renderpipe\r");
			break;
		}
	}
	
	*size = BufLen;
}

void CRenderPipeWindows::~CRenderPipeWindows()
{
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
	}
}

