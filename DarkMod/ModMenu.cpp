#include "../idlib/precompiled.h"
#pragma hdrstop
#include "ModMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"
#include "boost/filesystem.hpp"
#include <string>
#ifdef _WINDOWS
#include <process.h>
#else
#include <unistd.h>
#endif

CModMenu::CModMenu()
{

}

CModMenu::~CModMenu()
{
}

extern int errorno;
idCVar tdm_mapName( "tdm_mapName", "", CVAR_GUI, "" );
namespace fs = boost::filesystem;

char * readFile(fs::path fileName)
{
	FILE* file = fopen(fileName.file_string().c_str(), "r");
	char * buf = NULL;
	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		long len = ftell(file);
		fseek(file, 0, SEEK_SET);
		buf = (char *)malloc(len+1);
		fread(buf, len, 1, file);
		buf[len] = 0;
		fclose(file);
	}
	return buf;
}

// Handle mainmenu commands
void CModMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	if (idStr::Icmp(menuCommand, "showMods") == 0)
	{
		// list all FMs
		fs::path doomPath(idLib::fileSystem->RelativePathToOSPath("", "fs_savepath"));
		doomPath /= "..";
		fs::directory_iterator end_iter;
		modsAvailable.Clear();
		for (fs::directory_iterator dir_itr(doomPath); dir_itr != end_iter; ++dir_itr)
		{
			if (fs::is_directory(dir_itr->status()))
			{
				// look for darkmod.txt file
				fs::path descFile(dir_itr->path() / "darkmod.txt");
				if (fs::exists(descFile)) {
					idStr * modName = new idStr(dir_itr->path().leaf().c_str());
					modsAvailable.Append(modName->c_str());
				}
			}
		}
		gui->SetStateBool("isModsMoreVisible", modsAvailable.Num() > MODS_PER_PAGE); 
		gui->SetStateBool("isNewGameRootMenuVisible", true); 

		// Get the path to the darkmod directory
		fs::path doom3path(idLib::fileSystem->RelativePathToOSPath("", "fs_savepath"));
		doom3path /= "..";
		fs::path darkmodPath(doom3path / "darkmod");

		// Path to file that holds the current FM name
		fs::path currentFMPath(darkmodPath / "currentfm.txt");

		// Get the current mod
		char * current = readFile(currentFMPath);
		idStr name = idStr("<No Mission Installed>");
		idStr desc = idStr("");
		gui->SetStateBool("hasCurrentMod", false); 
		if (current != NULL) {
			gui->SetStateBool("hasCurrentMod", true); 
			fs::path startingMapPath(doom3path / current / "startingMap.txt");
			char * mapName = readFile(startingMapPath);
			tdm_mapName.SetString(mapName);
			fs::path modDescFile(doom3path / current / "darkmod.txt");
			char * modFileContent = readFile(modDescFile);
			name = current;
			desc = "";
			if (modFileContent != NULL) {
				idStr modInfo(modFileContent);
				int spos = modInfo.Find("Title:");
				int epos = modInfo.Find("Description:");
				int len = modInfo.Length();
				if (spos >= 0 && epos >= 0) {
					modInfo.Mid(spos+6, epos-(spos+6), name);
					modInfo.Right(len-(epos+12), desc);
					name.StripTrailingWhitespace();
					name.Strip(' ');
					desc.StripTrailingWhitespace();
					desc.Strip(' ');
				}
				delete modFileContent;
			}
		}
		gui->SetStateString("currentModName", name); 
		gui->SetStateString("currentModDesc", desc); 

		UpdateGUI(gui);
	}
	if (idStr::Icmp(menuCommand, "modMore") == 0)
	{
		// Scroll down a page
		modTop += MODS_PER_PAGE;
		if (modTop > (unsigned) modsAvailable.Num()) {
			modTop = 0;
		}
		UpdateGUI(gui);
	}
	if (idStr::Icmp(menuCommand, "darkmodLoad") == 0)
	{
		// Get selected mod
		int modNum = gui->GetStateInt("modSelected", "0");
		modNum += modTop;
		const char * modDirName = modsAvailable[modNum];

		// Path to the parent directory
		fs::path parentPath(idLib::fileSystem->RelativePathToOSPath("", "fs_savepath"));
		parentPath = parentPath.remove_leaf().remove_leaf();

		// Path to the darkmod directory
		fs::path darkmodPath(parentPath / "darkmod");

		// Path to mod directory in fms folder
		fs::path modDirPath(parentPath / modDirName);

		// Path to file that holds the current FM name
		fs::path currentFMPath(darkmodPath / "currentfm.txt");

		// Path to file that contains the command line arguments to DM
		fs::path dmArgs(darkmodPath / "dmargs.txt");

		// Save the name of the new mod
		FILE* currentFM = fopen(currentFMPath.file_string().c_str(), "w+");
		fputs(modDirName, currentFM);
		fclose(currentFM);

		// path to dmlauncher
#ifdef _WINDOWS
		fs::path launcherExe(darkmodPath / "dmlauncher.exe");
#else
		// ???
		fs::path launcherExe(darkmodPath / "dmlauncher");
#endif

		// command line to spawn dmlauncher
		idStr commandLine(launcherExe.file_string().c_str());

#ifdef _WINDOWS
		// Create a dmlauncher process, setting the working directory to the doom directory
		STARTUPINFO siStartupInfo;
		PROCESS_INFORMATION piProcessInfo;
		memset(&siStartupInfo, 0, sizeof(siStartupInfo));
		memset(&piProcessInfo, 0, sizeof(piProcessInfo));
		siStartupInfo.cb = sizeof(siStartupInfo);
		commandLine += " pause";
		CreateProcess(NULL, (LPSTR) commandLine.c_str(), NULL, NULL,  false, 0, NULL,
			parentPath.file_string().c_str(), &siStartupInfo, &piProcessInfo);
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "quit" );
#else
		// start dmlauncher
		if (execlp(commandLine.c_str(), commandLine.c_str(), NULL)==-1) {
			int errnum = errno;
			gameLocal.Error("execlp failed with error code %d: %s", errnum, strerror(errnum));
		}
		_exit(EXIT_FAILURE);
#endif
	}
}
void CModMenu::UpdateGUI(idUserInterface* gui) {
	// Display the name of each FM
	int modPos = 0;
	fs::path doomPath(idLib::fileSystem->RelativePathToOSPath("", "fs_savepath"));
	doomPath /= "..";
	while (modPos < MODS_PER_PAGE) {
		idStr guiName = idStr("mod") + modPos + "_name";
		idStr guiDesc = idStr("mod") + modPos + "_desc";
		idStr guiImage = idStr("mod") + modPos + "_image";
		idStr guiAvailable = idStr("modAvail") + modPos;
		idStr name = idStr("");
		idStr desc = idStr("");
		idStr image = idStr("");
		int available = 0;
		char * modFileContent = NULL;
		if (modTop + modPos < (unsigned) modsAvailable.Num()) {
			const char * modDirName = modsAvailable[modTop + modPos];
			// Read the text file that contains the name and description
			fs::path modNameFile(doomPath / modDirName / "darkmod.txt");
			modFileContent = readFile(modNameFile);
			name = modDirName;
			desc = "";
			if (modFileContent != NULL) {
				idStr modInfo(modFileContent);
				int spos = modInfo.Find("Title:");
				int epos = modInfo.Find("Description:");
				int len = modInfo.Length();
				if (spos >= 0 && epos >= 0) {
					modInfo.Mid(spos+6, epos-(spos+6), name);
					modInfo.Right(len-(epos+12), desc);
					name.StripTrailingWhitespace();
					name.Strip(' ');
					desc.StripTrailingWhitespace();
					desc.Strip(' ');
				}
				delete modFileContent;
			}
			
			available = 1;
		}
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
		gui->SetStateString(guiImage, image);
		modPos++;
	}
}
