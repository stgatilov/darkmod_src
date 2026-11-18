/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "../sys_local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>

#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>

/*
==============
Sys_EXEPath
==============
*/
const char* Sys_EXEPath()
{
	static char path[1024];
	uint32_t size = sizeof( path );

	if ( _NSGetExecutablePath( path, &size ) != 0 )
	{
		Sys_Printf( "buffer too small to store exe path, need size %u\n", size );
		path[0] = '\0';
	}
	// resolve symlinks (e.g. /var -> /private/var)
	char resolved[PATH_MAX];
	if ( realpath( path, resolved ) != nullptr )
	{
		idStr::snPrintf( path, sizeof( path ), "%s", resolved );
	}
	return path;
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
uint64 Sys_ClockTicksPerSecond()
{
	static bool init = false;
	static uint64 ret;

	if ( init )
	{
		return ret;
	}

	// note: Apple Silicon has no fixed "CPU frequency" sysctl,
	// and clock ticks come from clock_gettime(CLOCK_MONOTONIC) in ns,
	// so the tick rate is nanoseconds per second: 1'000'000'000
	ret = 1000000000ull;
	init = true;
	common->Printf( "Sys_ClockTicksPerSecond: monotonic clock, %g ticks/ms\n", ret / 1000000.0 );
	return ret;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the total number of logical CPU cores
numPhysicalCores	- the total number of physical cores
numCPUPackages		- the total number of packages (physical processors)
========================
*/
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCores, int& numCPUPackages )
{
	static bool init = false;

	static int s_numLogicalCPUCores;
	static int s_numPhysicalCores;
	static int s_numCPUPackages;

	if ( init )
	{
		numLogicalCPUCores = s_numLogicalCPUCores;
		numPhysicalCores = s_numPhysicalCores;
		numCPUPackages = s_numCPUPackages;
		return;
	}

	s_numPhysicalCores = 1;
	s_numLogicalCPUCores = 1;
	s_numCPUPackages = 1;

	size_t len = sizeof( s_numPhysicalCores );
	sysctlbyname( "hw.physicalcpu", &s_numPhysicalCores, &len, NULL, 0 );
	len = sizeof( s_numLogicalCPUCores );
	sysctlbyname( "hw.logicalcpu", &s_numLogicalCPUCores, &len, NULL, 0 );

	common->Printf( "CPU processors: %d\n", s_numPhysicalCores );
	common->Printf( "CPU logical cores: %d\n", s_numLogicalCPUCores );

	numLogicalCPUCores = s_numLogicalCPUCores;
	numPhysicalCores = s_numPhysicalCores;
	numCPUPackages = s_numCPUPackages;
	init = true;
}

/*
========================
OSX_GetLocalizedString

The old sys/osx/macosx_misc.mm implementation looked up the string in the
application bundle's Localizable.strings. A plain executable has no such
bundle, so fall back to the key itself (English name of the key).
Declared in framework/KeyInput.cpp under #if MACOS_X.
========================
*/
const char* OSX_GetLocalizedString( const char* key )
{
	return key;
}

/*
==================
Sys_DoStartProcess

if we don't fork, this function never returns
the no-fork lets you keep the terminal when you're about to spawn an installer
==================
*/
void Sys_DoStartProcess( const char* exeName, bool dofork )
{
	bool use_system = false;
	if ( strchr( exeName, ' ' ) )
	{
		use_system = true;
	}
	else
	{
		// set exec rights when it's about a single file to execute
		struct stat buf;
		if ( stat( exeName, &buf ) == -1 )
		{
			printf( "stat %s failed: %s\n", exeName, strerror( errno ) );
		}
		else
		{
			if ( chmod( exeName, buf.st_mode | S_IXUSR ) == -1 )
			{
				printf( "chmod +x %s failed: %s\n", exeName, strerror( errno ) );
			}
		}
	}
	if ( dofork )
	{
		switch ( fork() )
		{
			case -1:
				// main thread
				break;
			case 0:
				if ( use_system )
				{
					printf( "system %s\n", exeName );
					system( exeName );
					_exit( 0 );
				}
				else
				{
					printf( "execl %s\n", exeName );
					execl( exeName, exeName, NULL );
					printf( "execl failed: %s\n", strerror( errno ) );
					_exit( -1 );
				}
				break;
		}
	}
	else
	{
		if ( use_system )
		{
			printf( "system %s\n", exeName );
			system( exeName );
			sleep( 1 );	// on some systems starting the new process and exiting this one should not be too close
		}
		else
		{
			printf( "execl %s\n", exeName );
			execl( exeName, exeName, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		// terminate
		_exit( 0 );
	}
}

/*
 ==================
 Sys_DoPreferences
 ==================
 */
void Sys_DoPreferences() { }

/*
===============
main
===============
*/
int main( int argc, const char** argv )
{
	Posix_EarlyInit( );

	if ( argc > 1 )
	{
		common->Init( argc - 1, &argv[1], NULL );
	}
	else
	{
		common->Init( 0, NULL, NULL );
	}

	Posix_LateInit( );

	while ( 1 )
	{
		common->Frame();
	}
}
