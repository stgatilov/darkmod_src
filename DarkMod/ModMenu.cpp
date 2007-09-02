#include "../idlib/precompiled.h"
#pragma hdrstop
#include "ModMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"
#include <process.h>

CModMenu::CModMenu()
{

}

CModMenu::~CModMenu()
{
}

// Handle mainmenu commands
void CModMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	if (idStr::Icmp(menuCommand, "showMods") == 0)
	{
		// list all FMs
		modsAvailable = idLib::fileSystem->ListFiles( "fms", ".pk4", true);
		modTop = 0;
		gui->SetStateBool("isModsMoreVisible", modsAvailable->GetNumFiles() > MODS_PER_PAGE); 
		gui->SetStateBool("isModsMenuVisible", true); 
		UpdateGUI(gui);
	}
	if (idStr::Icmp(menuCommand, "modMore") == 0)
	{
		// Scroll down a page
		modTop += MODS_PER_PAGE;
		if (modTop > modsAvailable->GetNumFiles()) {
			modTop = 0;
		}
		UpdateGUI(gui);
	}
	if (idStr::Icmp(menuCommand, "darkmodLoad") == 0)
	{
		// Get the path to the darkmod directory
		const char * darkmodPath = idLib::fileSystem->RelativePathToOSPath("", "fs_basepath");

		// copy the mod file selected by the user
		const char * modName = va("%s%s", gui->GetStateString("modSelected", ""), ".pk4");
		const char * sourceFile = va("%sfms/%s", darkmodPath, modName);
		const char * destFile = va("%s%s", darkmodPath, modName);
		idLib::fileSystem->CopyFile(sourceFile, destFile);

		// Get the current mod
		char * current = NULL;
		idLib::fileSystem->ReadFile("currentfm.txt", ( void ** )&current);

		// Make the new mod the current mod
		idLib::fileSystem->WriteFile("currentfm.txt", modName, strlen(modName), "fs_basepath");
		
		// Set DOOM3.EXE path
		darkmodPath = idLib::fileSystem->RelativePathToOSPath("", "fs_basepath");
		char * doomExe = va("%s../doom3.exe", darkmodPath);

		// Shutdown file system and delete existing mod file
		char * pk4ToDelete = NULL;
		if (current != NULL) {
			pk4ToDelete = va("%s%s", darkmodPath, current);
			fileSystem->FreeFile( current );
		}
		idLib::fileSystem->Shutdown(false);
		if (pk4ToDelete != NULL) {
			remove(pk4ToDelete);
		}

		// start up doom again and exit
		intptr_t x = _spawnl(_P_NOWAIT, doomExe, doomExe, "+set fs_game darkmod", NULL);
		_exit(EXIT_SUCCESS); 
	}
}
void CModMenu::UpdateGUI(idUserInterface* gui) {
	// Display the name of each FM
	for (int i = 0; i < MODS_PER_PAGE; i++) {
		idStr guiName = idStr("mod") + i + "_name";
		idStr guiDesc = idStr("mod") + i + "_desc";
		idStr guiAvailable = idStr("modAvail") + i;
		idStr name = idStr("");
		idStr desc = idStr("");
		int available = 0;
		if (modTop + i < modsAvailable->GetNumFiles()) {
			name = idStr(modsAvailable->GetFile(modTop + i));
			name.Replace(".pk4", "");
			desc = "";
			available = 1;
		}
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
	}
}