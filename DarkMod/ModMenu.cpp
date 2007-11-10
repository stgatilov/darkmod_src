#include "../idlib/precompiled.h"
#pragma hdrstop
#include "ModMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"
#include "boost/filesystem.hpp"
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

// Handle mainmenu commands
void CModMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	if (idStr::Icmp(menuCommand, "showMods") == 0)
	{
		// list all FMs
		modsAvailable.Clear();
		idFileList * modDirs = idLib::fileSystem->ListFiles( "fms", "/", true);
		for (int i = 0; i < modDirs->GetNumFiles(); i++)
		{
			// filter out the .svn directory
			if (idStr::Icmp(modDirs->GetFile(i), ".svn") != 0) {
				modsAvailable.Append(modDirs->GetFile(i));
			}
		}
		gui->SetStateBool("isModsMoreVisible", modsAvailable.Num() > MODS_PER_PAGE); 
		gui->SetStateBool("isNewGameRootMenuVisible", true); 

		// Get the path to the darkmod directory
		const char * darkmodPath = idLib::fileSystem->RelativePathToOSPath("", "fs_basepath");

		// Get the current mod
		char * current = NULL;
		idLib::fileSystem->ReadFile("currentfm.txt", ( void ** )&current);

		idStr name = idStr("");
		idStr desc = idStr("");
		if (current != NULL) {
			char * modFileContent = NULL;
			char * mapName = NULL;
			idLib::fileSystem->ReadFile("startingMap.txt", (void**) &mapName);
			tdm_mapName.SetString(mapName);
			const char * modDescFile = va("fms/%s/%s.txt", current, current);
			idLib::fileSystem->ReadFile(modDescFile, ( void ** )&modFileContent);
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
				fileSystem->FreeFile( modFileContent );
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
		// TODO: get current mission, copy savefiles directory contents to this mission's directory
		// if no current mission, copy to savefiles.original directory (or throw away?)

		// Get selected mod
		int modNum = gui->GetStateInt("modSelected", "0");
		modNum += modTop;
		const char * modDirName = modsAvailable[modNum];

		// The name of the mod .pk4 file
		const char * modFileName = va("%s%s", modDirName, ".pk4");

		// Path to the darkmod directory
		fs::path darkmodPath(idLib::fileSystem->RelativePathToOSPath("", "fs_basepath"));

		// Path to the savegames directory
		fs::path savegamePath(darkmodPath / "savegames");

		// Path to mod directory in fms folder
		fs::path modDirPath(darkmodPath / "fms" / modDirName);

		// Path to file that holds the current FM name
		fs::path currentFMPath(darkmodPath / "currentfm.txt");

		// Path to the PK4 file in the mod directory
		fs::path modpk4(modDirPath / modFileName);

		// Path to copy the mod PK4 file to
		fs::path modpk4dest(darkmodPath / modFileName);

		// Copy the PK4
		if (!fs::exists(modpk4dest)) {
			fs::copy_file(modpk4, modpk4dest);
		}

		// Get the current mod name
		char * current = NULL;
		FILE* currentFM = fopen(currentFMPath.file_string().c_str(), "r");

		fs::path pk4ToDelete;
		if (currentFM) {
			// read the mod name
			current = new char[100];
			fgets(current, 100, currentFM);
			fclose(currentFM);

			// current pk4
			pk4ToDelete = darkmodPath / va("%s.pk4", current);

			// Path to the savegames of the current mod (create if necessary)
			fs::path modSavegamesPath(darkmodPath / "fms" / current / "savegames");
			fs::create_directory(modSavegamesPath);

			// Move all current savegames to mod directory in fms folder
			fs::directory_iterator end_iter;
		    for (fs::directory_iterator dir_itr(savegamePath); dir_itr != end_iter; ++dir_itr)
		    {
				if (!fs::is_directory(dir_itr->status()))
				{
					fs::path dest(modSavegamesPath / dir_itr->path().leaf());
					if (fs::exists(dest)) {
						fs::remove(dest);
					}
					fs::rename(dir_itr->path(), dest);
				}
			}
			delete current;
		}
		// Copy all existing savegames from mod directory in fms folder to current savegames folder
		fs::path newmodSavegamesPath(darkmodPath / "fms" / modDirName / "savegames");
		if (fs::exists(newmodSavegamesPath)) {
			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_itr(newmodSavegamesPath); dir_itr != end_iter; ++dir_itr)
			{
				if (!fs::is_directory(dir_itr->status()))
				{
					fs::copy_file(dir_itr->path(), savegamePath / dir_itr->path().leaf());
				}
			}
		}

		// Save the name of the new mod
		currentFM = fopen(currentFMPath.file_string().c_str(), "w+");
		fputs(modDirName, currentFM);
		fclose(currentFM);
		
		// Set DOOM3.EXE path
#ifdef _WINDOWS
		fs::path doomExe(darkmodPath);
		doomExe = doomExe.remove_leaf().remove_leaf();
		doomExe /= "doom3.exe";
		fs::path launcherExe(darkmodPath / "dmlauncher.exe");
#else
		char * doomExe = va("%s../doom.x86", darkmodPath.file_string().c_str());
#endif

#ifdef _WINDOWS
		// start up dmlauncher and exit
		STARTUPINFO siStartupInfo;
		PROCESS_INFORMATION piProcessInfo;
		memset(&siStartupInfo, 0, sizeof(siStartupInfo));
		memset(&piProcessInfo, 0, sizeof(piProcessInfo));
		siStartupInfo.cb = sizeof(siStartupInfo);
		char * lpCmdLine = va("%s %s +set fs_game darkmod %s",
			launcherExe.file_string().c_str(), doomExe.file_string().c_str(), pk4ToDelete.file_string().c_str());
		CreateProcess(NULL, lpCmdLine, NULL, NULL,  false, 0, NULL, NULL, &siStartupInfo, &piProcessInfo);

		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "quit" );
#else
		// delete the current pk4
		idLib::fileSystem->Shutdown(false);
		if (!pk4ToDelete.empty()) {
			remove(pk4ToDelete.file_string().c_str());
		}
		// start doom
		if (execlp(doomExe, doomExe, "+set", "fs_game", "darkmod", NULL)==-1) {
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
			const char * modNameFile = va("fms/%s/%s.txt", modDirName, modDirName);
			idLib::fileSystem->ReadFile(modNameFile, ( void ** )&modFileContent);
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
			}
			
			available = 1;
		}
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
		gui->SetStateString(guiImage, image);
		if (modFileContent != NULL) {
			fileSystem->FreeFile( modFileContent );
		}
		modPos++;
	}
}
