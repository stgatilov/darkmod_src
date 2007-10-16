// (C) Copyright 2007 Richard Day - richardvday@yahoo.com
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/*********************************************************************************/
/*																	             */
/*      Pipe Abstraction class's												 */
/*		Named and nameless(Eventually)								             */
/*		For windows and Linux													 */
/*																				 */
/*********************************************************************************/

#ifndef _ABSTRACT_PIPE_HPP_
#define _ABSTRACT_PIPE_HPP_
#pragma once
/*
	Named pipes : class CNamedPipe 
	Unfortunately some behaviour is OS defined still.
	The name is the issue. Under windows "\\\\.\\pipe\\dm_renderpipe" is a valid name.
	Under linux any valid directory/filename combination is usable as long as it is writable to current process.
*/
#include <string>

class CNamedPipe
{
public:
	~CNamedPipe(void);
	virtual const char* GetName() const; // return fully qualified name
	virtual bool operator!() const; // Is it open would return true if closed false if open
	// Use this to get an actual NEW instance. Undefined behaviour if it already exist.
	static CNamedPipe* MakePipe( const char* Name );// Under windows just the pipe name the extra will be prepended for you
	// under linux whatever path/filename suits you and is valid of course. Must be writable to current user!
protected:
	std::string name;
	bool status;
	CNamedPipe(void);
	virtual void Construct( const char* Name ) = 0;
private:
	CNamedPipe& operator=( const CNamedPipe& );// not defined
	CNamedPipe& operator=( const CNamedPipe* );// not defined
	CNamedPipe( const CNamedPipe& );// not defined
	CNamedPipe( const CNamedPipe* );// not defined
};

#endif // #ifndef _ABSTRACT_PIPE_HPP_