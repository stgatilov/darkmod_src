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
#include "precompiled.h"
#define DOCTEST_THREAD_LOCAL	//stgatilov: initialization of threadlocal g_oss crashes because it happens before CRT starts
#define DOCTEST_CONFIG_IMPLEMENT
#include "testing.h"
#include "TestRun.h"

/*
 * To run tests, start TDM like this:
 * TheDarkModx64.exe +set com_skipRenderer 1 +set com_runTests 2
 */
idCVar com_runTests( "com_runTests", "0", CVAR_SYSTEM|CVAR_INTEGER|CVAR_INIT, "If set, will run unit tests instead of main game. A value of 2 will spawn a console for output on Windows." );
idCVar com_testParams( "com_testParams", "", CVAR_SYSTEM|CVAR_INIT, "Specify command line args for the test runner" );

int RunTests() {
	idCmdArgs args;
	args.TokenizeString( idStr("tdm_tests ") + com_testParams.GetString(), true );
	int argc = 0;
	const char **argv = args.GetArgs( &argc );
	doctest::Context context;
	context.applyCommandLine( argc, argv );
	return context.run();
}
