#include "GuiFluidAutoGen.h"
#include "GuiGlobal.h"
#include "OsUtils.h"

//entry point on Linux (and on Windows with Console subsystem)
int main(int argc, char **argv) {
	//ensure that e.g. log file is written to directory with installer
	OsUtils::InitArgs(argv[0]);
	OsUtils::SetCwd(OsUtils::GetExecutableDir());

	//create all GUI designed in FLUID
	FluidAllGui();

	//additional GUI initialization (out of FLUID)
	GuiInitAll();

	//display the window
	g_Window->show();
	//run some checks immediately after start
	Fl::add_timeout(0.3, GuiLoaded);
	//enter event loop of FLTK
	int ret = Fl::run();

	//Window closed -> exit
	return ret;
}

#if defined(_WIN32) && !defined(_CONSOLE)
//entry point on Windows with GUI subsystem
int WINAPI aWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif
