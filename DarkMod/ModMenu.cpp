#include "../idlib/precompiled.h"
#pragma hdrstop
#include "ModMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"
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

		// Get the path to the darkmod directory
		const char * darkmodPath = idLib::fileSystem->RelativePathToOSPath("", "fs_basepath");

		// copy the mod file selected by the user
		int modNum = gui->GetStateInt("modSelected", "0");
		modNum += modTop;
		const char * modDir = modsAvailable[modNum];
		const char * modName = va("%s%s", modDir, ".pk4");
		const char * sourceFile = va("%sfms/%s/%s", darkmodPath, modDir, modName);
		const char * destFile = va("%s%s", darkmodPath, modName);
		idLib::fileSystem->CopyFile(sourceFile, destFile);

		// Get the current mod
		char * current = NULL;
		idLib::fileSystem->ReadFile("currentfm.txt", ( void ** )&current);

		// Make the new mod the current mod
		idLib::fileSystem->WriteFile("currentfm.txt", modDir, strlen(modDir), "fs_basepath");
		
		// Set DOOM3.EXE path
		darkmodPath = idLib::fileSystem->RelativePathToOSPath("", "fs_basepath");
#ifdef _WINDOWS
		char * doomExe = va("%s../doom3.exe", darkmodPath);
		char * launcherExe = va("%sdmlauncher.exe", darkmodPath);
#else
		char * doomExe = va("%s../doom.x86", darkmodPath);
#endif

		// Shutdown file system and delete existing mod file
		char * pk4ToDelete = NULL;
		if (current != NULL) {
			pk4ToDelete = va("%s%s.pk4", darkmodPath, current);
			fileSystem->FreeFile( current );
		}

#ifdef _WINDOWS
		// start up dmlauncher and exit
		STARTUPINFO siStartupInfo;
		PROCESS_INFORMATION piProcessInfo;
		memset(&siStartupInfo, 0, sizeof(siStartupInfo));
		memset(&piProcessInfo, 0, sizeof(piProcessInfo));
		siStartupInfo.cb = sizeof(siStartupInfo);
		char * lpCmdLine = va("%s %s +set fs_game darkmod %s", launcherExe, doomExe, pk4ToDelete);
		CreateProcess(NULL, lpCmdLine, NULL, NULL,  false, 0, NULL, NULL, &siStartupInfo, &piProcessInfo);

		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "quit" );
#else
		// delete the current pk4
		idLib::fileSystem->Shutdown(false);
		if (pk4ToDelete != NULL) {
			remove(pk4ToDelete);
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

	// Is first directory the .svn directory? If so, be sure to skip it
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
