#include "../idlib/precompiled.h"
#pragma hdrstop
#include "ModMenu.h"
#include "../DarkMod/shop.h"
#include "../DarkMod/MissionData.h"
#include "../DarkMod/declxdata.h"
#include "boost/filesystem.hpp"
#include <string>
#ifdef _WINDOWS
#include <process.h>
#else
#include <unistd.h>
#endif

CModMenu::CModMenu() :
	modTop(0)
{}

void CModMenu::Init()
{
	LoadModList();
}

void CModMenu::Clear() 
{
	// Nothing to clear yet
}

void CModMenu::Save(idSaveGame* savefile) const
{
	// Nothing to save yet
}

void CModMenu::Restore(idRestoreGame* savefile)
{
	// Nothing to restore yet
}

extern int errorno;
idCVar tdm_mapName( "tdm_mapName", "", CVAR_GUI, "" );
namespace fs = boost::filesystem;

namespace {

	/**
	 * Private helper function reading the contents of the given file
	 * into a string.
	 */
	idStr readFile(const fs::path& fileName)
	{
		idStr returnValue;

		FILE* file = fopen(fileName.file_string().c_str(), "r");

		if (file != NULL)
		{
			// Get the filesize
			fseek(file, 0, SEEK_END);
			long len = ftell(file);
			fseek(file, 0, SEEK_SET);

			// Allocate a new buffer and read the contents
			char* buf = new char[len+1];
			fread(buf, len, 1, file);

			// NULL-terminate the string
			buf[len] = 0;

			// Copy the result into the idStr
			returnValue = buf;

			delete[] buf;
			
			fclose(file);
		}

		return returnValue;
	}

} // namespace

// Handle mainmenu commands
void CModMenu::HandleCommands(const char *menuCommand, idUserInterface *gui)
{
	if (idStr::Icmp(menuCommand, "updateModList") == 0)
	{
		// Reload the mod list and update the GUI
		LoadModList();

		// Update the GUI state
		UpdateGUI(gui);
	}

	if (idStr::Icmp(menuCommand, "showMods") == 0)
	{
		// LoadModList();

		gui->SetStateBool("isModsMoreVisible", modsAvailable.Num() > MODS_PER_PAGE); 
		gui->SetStateBool("isNewGameRootMenuVisible", true); 

		// Get the path to the darkmod directory
		fs::path doom3path(fileSystem->RelativePathToOSPath("", "fs_savepath"));
		doom3path /= "..";
		fs::path darkmodPath(doom3path / "darkmod");

		// Path to file that holds the current FM name
		fs::path currentFMPath(darkmodPath / "currentfm.txt");

		// Get the current mod
		//char * current = readFile(currentFMPath);

		char * current = NULL;
		fileSystem->ReadFile("currentfm.txt", (void**) &current);
		
		//const char* current = "saintlucia";

		idStr name = idStr("<No Mission Installed>");
		idStr desc = idStr("");
		gui->SetStateBool("hasCurrentMod", false); 
		if (current != NULL) {
			gui->SetStateBool("hasCurrentMod", true); 
			fs::path startingMapPath(doom3path / current / "startingmap.txt");
			idStr mapName = readFile(startingMapPath);
			tdm_mapName.SetString(mapName);
			fs::path modDescFile(doom3path / current / "darkmod.txt");
			idStr modFileContent = readFile(modDescFile);
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
		gui->SetStateInt("modSelected", -1); 

		UpdateGUI(gui);
	}

	if (idStr::Icmp(menuCommand, "modMore") == 0)
	{
		// Scroll down a page
		modTop += MODS_PER_PAGE;
		if (modTop > modsAvailable.Num())
		{
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
		fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
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

		// path to tdmlauncher
#ifdef _WINDOWS
		fs::path launcherExe(darkmodPath / "tdmlauncher.exe");
#else
		// ???
		fs::path launcherExe(darkmodPath / "tdmlauncher");
#endif

		// command line to spawn tdmlauncher
		idStr commandLine(launcherExe.file_string().c_str());

#ifdef _WINDOWS
		// Create a tdmlauncher process, setting the working directory to the doom directory
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
		// start tdmlauncher
		if (execlp(commandLine.c_str(), commandLine.c_str(), NULL)==-1) {
			int errnum = errno;
			gameLocal.Error("execlp failed with error code %d: %s", errnum, strerror(errnum));
		}
		_exit(EXIT_FAILURE);
#endif
	}
	if (idStr::Icmp(menuCommand, "briefing_show") == 0)
	{
		// Display the briefing text
		briefingPage = 1;
		DisplayBriefingPage(gui);
	}
	if (idStr::Icmp(menuCommand, "briefing_scroll_down_request") == 0)
	{
		// Display the next page of briefing text
		briefingPage++;
		DisplayBriefingPage(gui);
	}
	if (idStr::Icmp(menuCommand, "briefing_scroll_up_request") == 0)
	{
		// Display the previous page of briefing text
		briefingPage--;
		DisplayBriefingPage(gui);
	}
}

// Displays the current page of briefing text
void CModMenu::DisplayBriefingPage(idUserInterface *gui) {
	gameLocal.Warning("DisplayBriefingPage: start");
	// look up the briefing xdata, which is in "maps/<map name>/mission_briefing"
	idStr briefingData = idStr("maps/") + tdm_mapName.GetString() + "/mission_briefing";
	gameLocal.Warning("DisplayBriefingPage: briefingData is " + briefingData);
	const tdmDeclXData *xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, briefingData, false ) );

	const char * briefing = "";
	bool scrollDown = false;
	bool scrollUp = false;
	if (xd != NULL)
	{
		gameLocal.Warning("DisplayBriefingPage: xd is not null");
		// get page count from xdata
		int pages = xd->m_data.GetInt("num_pages");
		gameLocal.Warning("DisplayBriefingPage: pages is " + idStr(pages));

		// ensure current page is between 1 and page count, inclusive
		if (briefingPage < 1) briefingPage = 1;
		if (briefingPage > pages) briefingPage = pages;

		// load up page text
		idStr page = idStr("page") + idStr(briefingPage) + "_body";
		gameLocal.Warning("DisplayBriefingPage: page is " + page);
		briefing = xd->m_data.GetString(page);
		gameLocal.Warning("DisplayBriefingPage: briefing is " + idStr(briefing));

		// set scroll button visibility
		scrollDown = pages > briefingPage;
		scrollUp = briefingPage > 1;
	}
	else
	{
		gameLocal.Warning("DisplayBriefingPage: xd is null");
	}
	// update GUI
	gui->SetStateString("BriefingText", briefing);
	gui->SetStateBool("ScrollDownVisible", scrollDown);
	gui->SetStateBool("ScrollUpVisible", scrollUp);
}

void CModMenu::UpdateGUI(idUserInterface* gui)
{
	fs::path doomPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	doomPath /= "..";

	// Display the name of each FM
	for (int modIndex = 0; modIndex < MODS_PER_PAGE; ++modIndex)
	{
		idStr guiName = idStr("mod") + modIndex + "_name";
		idStr guiDesc = idStr("mod") + modIndex + "_desc";
		idStr guiAuthor = idStr("mod") + modIndex + "_author";
		idStr guiImage = idStr("mod") + modIndex + "_image";
		idStr guiAvailable = idStr("modAvail") + modIndex;

		idStr name = idStr("");
		idStr desc = idStr("");
		idStr author = idStr("");
		idStr image = idStr("");
	
		int available = 0;
		
		if (modTop + modIndex < modsAvailable.Num())
		{
			// Get the mod name (i.e. the folder name)
			const idStr& modDirName = modsAvailable[modTop + modIndex];

			// Read the text file that contains the name and description
			fs::path modNameFile(doomPath / modDirName / "darkmod.txt");

			idStr modFileContent = readFile(modNameFile);

			name = modDirName;
			desc = "";

			if (!modFileContent.IsEmpty())
			{
				idStr modInfo(modFileContent);
				int namePos = modFileContent.Find("Title:");
				int descPos = modFileContent.Find("Description:");
				int authorPos = modFileContent.Find("Author:");

				int len = modFileContent.Length();

				if (namePos >= 0)
				{
					name = idStr(modFileContent, namePos, (descPos != -1) ? descPos : len);
					name.StripLeadingOnce("Title:");
					name.StripLeading(" ");
					name.StripLeading("\t");
					name.StripTrailingWhitespace();
				}

				if (descPos >= 0)
				{
					desc = idStr(modFileContent, descPos, (authorPos != -1) ? authorPos : len);
					desc.StripLeadingOnce("Description:");
					desc.StripLeading(" ");
					desc.StripLeading("\t");
					desc.StripTrailingWhitespace();
				}

				if (authorPos >= 0)
				{
					author = idStr(modFileContent, authorPos, len);
					author.StripLeadingOnce("Author:");
					author.StripLeading(" ");
					author.StripLeading("\t");
					author.StripTrailingWhitespace();
				}
			}
			
			available = 1;
		}

		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
		gui->SetStateString(guiAuthor, author);
		gui->SetStateString(guiImage, image);
	}
}

void CModMenu::LoadModList()
{
	// Clear the list first
	modsAvailable.Clear();

	// list all FMs
	fs::path doomPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	doomPath /= "..";
	fs::directory_iterator end_iter;

	for (fs::directory_iterator dir_itr(doomPath); dir_itr != end_iter; ++dir_itr)
	{
		if (fs::is_directory(dir_itr->status()))
		{
			// look for darkmod.txt file
			fs::path descFile(dir_itr->path() / "darkmod.txt");

			if (fs::exists(descFile))
			{
				// Found a mod
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Found an available mod: %s\r", dir_itr->path().leaf().c_str());

				// Append to the list
				modsAvailable.Alloc() = dir_itr->path().leaf().c_str();
			}
		}
	}
}
