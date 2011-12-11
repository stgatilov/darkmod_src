/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
#include "../../idlib/precompiled.h"

/*
==================
Sys_ShutdownSymbols
==================
*/
void Sys_ShutdownSymbols( void ) {
}

#ifdef ID_BT_STUB

/*
==================
Sys_GetCallStack
==================
*/
void Sys_GetCallStack( address_t *callStack, const int callStackSize ) {
	for ( int i = 0; i < callStackSize; i++ ) {
		callStack[i] = 0;
	}
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char * Sys_GetCallStackStr( const address_t *callStack, const int callStackSize ) {
	return "";
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char * Sys_GetCallStackCurStr( int depth ) {
	return "";
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char *	Sys_GetCallStackCurAddressStr( int depth ) {
	return "";
}

#else

#include <execinfo.h>

/*
==================
Sys_GetCallStack
==================
*/
void Sys_GetCallStack( address_t *callStack, const int callStackSize ) {
	int i;
	i = backtrace( (void **)callStack, callStackSize );	
	while( i < callStackSize ) {
		callStack[i++] = 0;
	}
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *	Sys_GetCallStackStr( const address_t *callStack, int callStackSize ) {
	static char string[MAX_STRING_CHARS*2];
	char **strings;
	int i;
	
	strings = backtrace_symbols( (void **)callStack, callStackSize );
	string[ 0 ] = '\0';
	for ( i = 0; i < callStackSize; i++ ) {
		idStr::snPrintf( string + strlen( string ), MAX_STRING_CHARS*2 - strlen( string ) - 1, "%s\n", strings[ i ] );
	}
	free( strings );
	return string;	
}


/*
==================
Sys_GetCallStackStr
==================
*/
const char * Sys_GetCallStackCurStr( int depth ) {
	address_t array[ 32 ];
	size_t size;
	
	size = backtrace( (void **)array, Min( 32, depth ) );
	return Sys_GetCallStackStr( array, (int)size );
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char * Sys_GetCallStackCurAddressStr( int depth ) {
	return Sys_GetCallStackCurStr( depth );
}

#endif
