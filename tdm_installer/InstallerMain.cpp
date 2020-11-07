#include "GuiFluidAutoGen.h"
#include "GuiGlobal.h"
#include "OsUtils.h"

//entry point on Linux (and on Windows with Console subsystem)
int main(int argc, char **argv) {
	bool unattended = false;
	bool help = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--unattended") == 0)
			unattended = true;
		if (strcmp(argv[i], "--help") == 0)
			help = true;
	}

	int ret = 0;
	if (help) {
		unattended = false;
		//create GUI designed in FLUID
		FluidGuiHelp();
		//additional initialization (adjust style)
		GuiInitHelp();
		//show the window
		g_HelpWindow->show();
		//enter event loop of FLTK
		ret = Fl::run();
	}
	else {
		//ensure that e.g. log file is written to directory with installer
		OsUtils::InitArgs(argv[0]);
		OsUtils::SetCwd(OsUtils::GetExecutableDir());

		//create all GUI designed in FLUID
		FluidAllGui();

		//additional GUI initialization (out of FLUID)
		GuiInitAll();

		//display the window
		g_Window->show();

		if (unattended) {
			//run everything with default options without human interaction
			GuiUnattended(argc, argv);
		}
		else {
			//run some checks immediately after start
			Fl::add_timeout(0.3, GuiLoaded);
			//enter event loop of FLTK
			ret = Fl::run();
		}
	}

	//window closed -> exit
	return ret;
}

#if defined(_WIN32) && !defined(_CONSOLE)
//entry point on Windows with GUI subsystem
int WINAPI aWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif
