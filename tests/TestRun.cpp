#include "precompiled.h"
#define DOCTEST_CONFIG_IMPLEMENT
#undef min
#undef max
#include "doctest.h"
#include "TestRun.h"

idCVar com_runTests( "com_runTests", "0", CVAR_SYSTEM|CVAR_BOOL|CVAR_INIT, "If set, will run unit tests instead of main game" );
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
