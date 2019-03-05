#include "precompiled.h"
#define DOCTEST_CONFIG_IMPLEMENT
#undef min
#undef max
#include "doctest.h"
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
