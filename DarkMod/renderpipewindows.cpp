/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1869 $
 * $Date: 2007-12-13 12:45:27 +0100 (Do, 13 Dez 2007) $
 * $Author: crispy $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "renderpipewindows.h"

int CRenderPipe::Prepare()
{
	memset(&m_saPipeSecurity, 0, sizeof(m_saPipeSecurity));
	m_pPipeSD = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	InitializeSecurityDescriptor(m_pPipeSD, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(m_pPipeSD, TRUE, (PACL)NULL, FALSE);
	m_saPipeSecurity.nLength = sizeof(SECURITY_ATTRIBUTES);
	m_saPipeSecurity.bInheritHandle = FALSE;
	m_saPipeSecurity.lpSecurityDescriptor = m_pPipeSD;

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

	return 0;
}

void CRenderPipe::CleanUp()
{
	if(m_hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
	}
}

const char* CRenderPipe::FileName()
{
	return DARKMOD_LG_RENDERPIPE_NAME_WINDOWS;
}

int CRenderPipe::Read(void *buf, int *size)
{
	DWORD cbBytesRead, dwBufSize, BufLen, dwLastError;

	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Reading from renderpipe [%08lX]\r", m_hPipe);

	dwBufSize = *size;
	BufLen = 0;
	while(1)
	{
		ReadFile(m_hPipe, // handle to pipe
			&buf[BufLen],							// buffer to receive data
			dwBufSize,								// size of buffer
			&cbBytesRead,							// number of bytes read
			NULL);									// not overlapped I/O

		dwLastError = GetLastError();
		
		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("%lu bytes read from renderpipe [%08lX]   %lu %lu\r",
			cbBytesRead, m_hPipe, BufLen, dwLastError);
		
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

	return BufLen;
}
