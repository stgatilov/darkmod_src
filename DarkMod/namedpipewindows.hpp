// (C) Copyright 2007 Richard Day - richardvday@yahoo.com
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/*****************************************************************/
/*															     */
/*                 Windows version of Named Pipe class			 */
/*																 */
/*****************************************************************/

#ifndef _CLASS_NAMED_PIPE_WINDOWS_HPP_
#define _CLASS_NAMED_PIPE_WINDOWS_HPP_
#pragma once
#include "pipe.hpp"

class CNamedPipeWindows :
	public CNamedPipe
{
	friend class CNamedPipe;
public:
	virtual ~CNamedPipeWindows(void);
protected:
	void Construct( const char* Name );
	CNamedPipeWindows(void);
	HANDLE pHandle;
};

#endif // #ifndef _CLASS_NAMED_PIPE_WINDOWS_HPP_