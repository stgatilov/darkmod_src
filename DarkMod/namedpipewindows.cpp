// (C) Copyright 2007 Richard Day - richardvday@yahoo.com
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/*****************************************************************/
/*															     */
/*                 Windows version of Named Pipe class			 */
/*																 */
/*****************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop
#include "namedpipewindows.hpp"

CNamedPipeWindows::CNamedPipeWindows(void):CNamedPipe(), pHandle(NULL)
{
}

CNamedPipeWindows::~CNamedPipeWindows(void)
{
}

void CNamedPipeWindows::Construct( const char* Name )
{
	name = "\\\\.\\pipe\\";
	name += Name;
	pHandle = CreateNamedPipe ( name.c_str(),
		PIPE_ACCESS_DUPLEX,				// read/write access
		PIPE_TYPE_MESSAGE |				// message type pipe
		PIPE_READMODE_MESSAGE |			// message-read mode
		PIPE_WAIT,						// blocking mode
		PIPE_UNLIMITED_INSTANCES,		// max. instances
		DARKMOD_LG_RENDERPIPE_BUFSIZE,	// output buffer size
		DARKMOD_LG_RENDERPIPE_BUFSIZE,	// input buffer size
		DARKMOD_LG_RENDERPIPE_TIMEOUT,	// client time-out
		NULL );				// no security attribute
	assert( INVALID_HANDLE_VALUE != pHandle );
	if( INVALID_HANDLE_VALUE != pHandle )
	{
		status = true;
	}
}
