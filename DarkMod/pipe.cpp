// (C) Copyright 2007 Richard Day - richardvday@yahoo.com
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/*********************************************************************************/
/*																	             */
/*         Named Pipe Abstraction class for windows and Linux	                 */
/*																				 */
/*********************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

#if defined(linux) || defined(__linux) || defined(__linux__)
// linux:
#include "namedpipelinux.hpp"
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// win32:
#include "namedpipewindows.hpp"
#endif

CNamedPipe::CNamedPipe(void):status(false)// only derived classes may instantiate
{
}

// nothing to do yet
CNamedPipe::~CNamedPipe(void)
{
}

bool CNamedPipe::operator!() const
{
	return status;
}

const char* CNamedPipe::GetName() const
{
	return name.c_str();
}

// static factory maker
CNamedPipe* CNamedPipe::MakePipe( const char* Name )
{
	assert( NULL != Name );// cant be NULL!
	assert( strlen( Name ) > 0 );// cant be empty either!
	// Perhaps we should change this to make a tmp name instead on NULL
	// Further validation required.
	CNamedPipe* pipe = NULL;
#if defined(linux) || defined(__linux) || defined(__linux__)
// linux:
	pipe = new CNamedPipeLinux( Name );
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// win32:
	pipe = new CNamedPipeWindows();
#else
#error OS Not defined or not implemented yet.
#endif
	assert( NULL != pipe );
	if( pipe )
	{
		pipe->Construct( Name );
	}
	return pipe;
}