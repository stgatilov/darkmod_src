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
	_modTop(0)
{}

void CModMenu::Init()
{
	LoadModList();
	InitCurrentMod();
	InitStartingMap();
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
	else if (idStr::Icmp(menuCommand, "modsNextPage") == 0)
	{
		// Scroll down a page
		_modTop += gui->GetStateInt("modsPerPage", "10");

		if (_modTop > _modsAvailable.Num())
		{
			_modTop = 0;
		}

		UpdateGUI(gui);
	}

	if (idStr::Icmp(menuCommand, "darkmodLoad") == 0)
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("modSelected", "0") + _modTop;

		InstallMod(modIndex, gui);
	}

	if (idStr::Icmp(menuCommand, "darkmodRestart") == 0)
	{
		// Get selected mod
		RestartGame();
	}

	if (idStr::Icmp(menuCommand, "briefing_show") == 0)
	{
		// Display the briefing text
		_briefingPage = 1;
		DisplayBriefingPage(gui);
	}

	if (idStr::Icmp(menuCommand, "briefing_scroll_down_request") == 0)
	{
		// Display the next page of briefing text
		_briefingPage++;
		DisplayBriefingPage(gui);
	}

	if (idStr::Icmp(menuCommand, "briefing_scroll_up_request") == 0)
	{
		// Display the previous page of briefing text
		_briefingPage--;
		DisplayBriefingPage(gui);
	}
}

// Displays the current page of briefing text
void CModMenu::DisplayBriefingPage(idUserInterface *gui)
{
	// look up the briefing xdata, which is in "maps/<map name>/mission_briefing"
	idStr briefingData = idStr("maps/") + cv_tdm_mapName.GetString() + "/mission_briefing";

	gameLocal.Printf("DisplayBriefingPage: briefingData is " + briefingData + "\n");

	// Load the XData declaration
	const tdmDeclXData* xd = static_cast<const tdmDeclXData*>(
		declManager->FindType(DECL_XDATA, briefingData, false)
	);

	const char* briefing = "";
	bool scrollDown = false;
	bool scrollUp = false;

	if (xd != NULL)
	{
		gameLocal.Printf("DisplayBriefingPage: xdata found.\n");

		// get page count from xdata
		int numPages = xd->m_data.GetInt("num_pages");

		gameLocal.Printf("DisplayBriefingPage: numPages is %d\n", numPages);

		// ensure current page is between 1 and page count, inclusive
		_briefingPage = idMath::ClampInt(1, numPages, _briefingPage);
		
		// load up page text
		idStr page = va("page%d_body", _briefingPage);

		gameLocal.Warning("DisplayBriefingPage: current page is " + page);

		briefing = xd->m_data.GetString(page);
		
		// set scroll button visibility
		scrollDown = numPages > _briefingPage;
		scrollUp = _briefingPage > 1;
	}
	else
	{
		gameLocal.Warning("DisplayBriefingPage: Could not find briefing xdata: %s", briefingData.c_str());
	}

	// update GUI
	gui->SetStateString("BriefingText", briefing);
	gui->SetStateBool("ScrollDownVisible", scrollDown);
	gui->SetStateBool("ScrollUpVisible", scrollUp);
}

void CModMenu::UpdateGUI(idUserInterface* gui)
{
	int modsPerPage = gui->GetStateInt("modsPerPage", "10");

	// Display the name of each FM
	for (int modIndex = 0; modIndex < modsPerPage; ++modIndex)
	{
		idStr guiName = idStr("mod") + modIndex + "_name";
		idStr guiDesc = idStr("mod") + modIndex + "_desc";
		idStr guiAuthor = idStr("mod") + modIndex + "_author";
		idStr guiImage = idStr("mod") + modIndex + "_image";
		idStr guiAvailable = idStr("modAvail") + modIndex;

		int available = 0;

		// Empty mod info structure for default values
		ModInfo info;
		
		if (_modTop + modIndex < _modsAvailable.Num())
		{
			// Load the mod info structure
			info = GetModInfo(_modTop + modIndex);

			available = 1;
		}

		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, info.title);
		gui->SetStateString(guiDesc, info.desc);
		gui->SetStateString(guiAuthor, info.author);
		gui->SetStateString(guiImage, info.image);
	}

	gui->SetStateBool("isModsMoreVisible", _modsAvailable.Num() > modsPerPage); 

	// Update the currently installed mod
	ModInfo curModInfo;
	
	if (!_curModName.IsEmpty())
	{
		curModInfo = GetModInfo(_curModName);
		gui->SetStateBool("hasCurrentMod", true); 
	}
	else
	{
		curModInfo.title = "<No Mission Installed>";
		gui->SetStateBool("hasCurrentMod", false); 
	}

	gui->SetStateString("currentModName", curModInfo.title);
	gui->SetStateString("currentModDesc", curModInfo.desc);
}

CModMenu::ModInfo CModMenu::GetModInfo(int modIndex) 
{
	// Sanity check
	if (modIndex < 0 || modIndex >= _modsAvailable.Num())
	{
		return ModInfo();
	}

	const idStr& modDirName = _modsAvailable[modIndex];

	idStr fmPath = cv_tdm_fm_path.GetString() + modDirName + "/";

	// Check for the darkmod.txt file
	idStr descFileName = fmPath + cv_tdm_fm_desc_file.GetString();

	char* buffer = NULL;
	if (fileSystem->ReadFile(descFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not readable?
		return ModInfo();
	}

	// Buffer is now holding the contents, copy to idStr
	idStr modFileContent(buffer);
	fileSystem->FreeFile(buffer);

	/*fs::path doomPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	doomPath /= "..";

	fs::path modNameFile(doomPath / modDirName / "darkmod.txt");

	idStr modFileContent = readFile(modNameFile);*/

	ModInfo info;
	info.pathToFMPackage = fmPath;
	
	if (!modFileContent.IsEmpty())
	{
		int titlePos = modFileContent.Find("Title:");
		int descPos = modFileContent.Find("Description:");
		int authorPos = modFileContent.Find("Author:");

		int len = modFileContent.Length();

		if (titlePos >= 0)
		{
			info.title = idStr(modFileContent, titlePos, (descPos != -1) ? descPos : len);
			info.title.StripLeadingOnce("Title:");
			info.title.StripLeading(" ");
			info.title.StripLeading("\t");
			info.title.StripTrailingWhitespace();
		}

		if (descPos >= 0)
		{
			info.desc = idStr(modFileContent, descPos, (authorPos != -1) ? authorPos : len);
			info.desc.StripLeadingOnce("Description:");
			info.desc.StripLeading(" ");
			info.desc.StripLeading("\t");
			info.desc.StripTrailingWhitespace();
		}

		if (authorPos >= 0)
		{
			info.author = idStr(modFileContent, authorPos, len);
			info.author.StripLeadingOnce("Author:");
			info.author.StripLeading(" ");
			info.author.StripLeading("\t");
			info.author.StripTrailingWhitespace();
		}

		// Check for mod image
		if (fileSystem->ReadFile(info.pathToFMPackage + "install_splash.tga", NULL) != -1)
		{
			info.image = info.pathToFMPackage + "install_splash";
		}
	}

	return info;
}

CModMenu::ModInfo CModMenu::GetModInfo(const idStr& modDirName)
{
	// Read the text file that contains the name and description
	/*idStr fmPath = cv_tdm_fm_path.GetString();

	// Check for the darkmod.txt file
	idStr descFileName = fmPath + modDirName + "/" + cv_tdm_fm_desc_file.GetString();

	char* buffer = NULL;
	if (fileSystem->ReadFile(descFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not readable?
		return ModInfo();
	}*/

	int index = _modsAvailable.FindIndex(modDirName);

	return (index != -1) ? GetModInfo(index) : ModInfo();
}

void CModMenu::LoadModList()
{
	// Clear the list first
	_modsAvailable.Clear();

	// List all folders in the fms/ directory
	idStr fmPath = cv_tdm_fm_path.GetString();
	idFileList*	files = fileSystem->ListFiles(fmPath, "/", false);

	for (int i = 0; i < files->GetNumFiles(); ++i)
	{
		// Check for the darkmod.txt file
		idStr descFileName = fmPath + files->GetFile(i) + "/" + cv_tdm_fm_desc_file.GetString();
	
		if (fileSystem->ReadFile(descFileName, NULL) != -1)
		{
			// File exists, add this as available mod
			_modsAvailable.Alloc() = files->GetFile(i);
		}
	}

	fileSystem->FreeFileList(files);

	gameLocal.Printf("Found %d mods in the FM folder.\n", _modsAvailable.Num());
}

void CModMenu::InitCurrentMod()
{
	idStr gameBase = cvarSystem->GetCVarString("fs_game_base");

	// We only have a mod if game_base is set correctly, otherwise we're in "darkmod".
	_curModName = (!gameBase.IsEmpty()) ? cvarSystem->GetCVarString("fs_game") : "";
}

void CModMenu::InitStartingMap()
{
	_startingMap.Empty();

	if (_curModName.IsEmpty()) 
	{
		return;
	}

	// Find out which is the starting map
	// list all FMs
	fs::path doomPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	doomPath /= "..";

	fs::path startingMapPath(doomPath / _curModName / "startingmap.txt");

	char* buffer = NULL;

	if (fileSystem->ReadFile("startingmap.txt", reinterpret_cast<void**>(&buffer)) != -1)
	{
		// We have a startingmap
		_startingMap = buffer;
		fileSystem->FreeFile(reinterpret_cast<void*>(buffer));

		cv_tdm_mapName.SetString(_startingMap);
	}
	else
	{
		gameLocal.Warning("No 'startingmap.txt' file for the current mod: %s", _curModName.c_str());
	}
}

std::string CModMenu::GetDarkmodPath()
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	idStr modBaseName = cvarSystem->GetCVarString("fs_game_base");

	if (modBaseName.IsEmpty())
	{
		// Fall back to fs_game if no game_base is set
		modBaseName = cvarSystem->GetCVarString("fs_game");

		if (modBaseName.IsEmpty())
		{
			modBaseName = "darkmod"; // last resort: hardcoded
		}
	}

	// Path to the darkmod directory
	fs::path darkmodPath(parentPath / modBaseName);

	return darkmodPath.string();
}

void CModMenu::InstallMod(int modIndex, idUserInterface* gui)
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	// Sanity-check
	if (modIndex < 0 || modIndex >= _modsAvailable.Num())
	{
		return;
	}

	const idStr& modDirName = _modsAvailable[modIndex];

	ModInfo info = GetModInfo(modIndex);

	// Issue the named command to the GUI
	gui->SetStateString("modInstallProgressText", "Installing FM Package\n\n" + info.title);
	gui->HandleNamedEvent("OnModInstallationStart");
	
	// Ensure that the target folder exists (idFileSystem::CopyFile requires this)
	fs::path targetFolder = parentPath / modDirName;

	if (!fs::create_directory(targetFolder))
	{
		// Directory exists, not a problem, but log this
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("FM targetFolder already exists: %s\r", targetFolder.string().c_str());
	}

	// Copy all PK4s from the FM folder
	idFileList*	pk4Files = fileSystem->ListFiles(info.pathToFMPackage, "PK4", false);
	
	for (int i = 0; i < pk4Files->GetNumFiles(); ++i)
	{
		// Check for the darkmod.txt file
		idStr pk4file = info.pathToFMPackage + pk4Files->GetFile(i);
	
		// Source file (full OS path)
		fs::path pk4fileOsPath = fileSystem->RelativePathToOSPath(pk4file);

		// Target location
		fs::path targetFile = targetFolder / pk4Files->GetFile(i);
		
		// Use boost::filesystem instead of id's (comments state that copying large files can be problematic)
		//fileSystem->CopyFile(pk4fileOsPath, targetFile.string().c_str());

		// Make sure any target file with the same name is removed beforehand
		try
		{
			fs::remove(targetFile);
		}
		catch (fs::basic_filesystem_error<fs::path> e)
		{
			// Don't care about removal error
			DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Caught exception while removing target file %s: %s\r", targetFile.string().c_str(), e.what());
		}

		// Copy the PK4 to the target folder
		try
		{
			fs::copy_file(pk4fileOsPath, targetFile);
		}
		catch (fs::basic_filesystem_error<fs::path> e)
		{
			DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Exception when coyping target file %s: %s\r", targetFile.string().c_str(), e.what());
		}
	}

	fileSystem->FreeFileList(pk4Files);

	// Path to the darkmod directory
	fs::path darkmodPath(GetDarkmodPath());

	// Path to file that holds the current FM name
	fs::path currentFMPath(darkmodPath / "currentfm.txt");

	// Save the name of the new mod
	FILE* currentFM = fopen(currentFMPath.file_string().c_str(), "w+");
	fputs(modDirName, currentFM);
	fclose(currentFM);

	gui->HandleNamedEvent("OnModInstallationFinished");
}

void CModMenu::RestartGame()
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	// Path to the darkmod directory
	fs::path darkmodPath(GetDarkmodPath());

	// path to tdmlauncher
#ifdef _WINDOWS
	fs::path launcherExe(darkmodPath / "tdmlauncher.exe");
#else
	// ???
	fs::path launcherExe(darkmodPath / "tdmlauncher");
#endif

	// FIXME: Check if tdmlauncher exists

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
#else
	// start tdmlauncher
	if (execlp(commandLine.c_str(), commandLine.c_str(), NULL)==-1) {
		int errnum = errno;
		gameLocal.Error("execlp failed with error code %d: %s", errnum, strerror(errnum));
	}
	_exit(EXIT_FAILURE);
#endif

	// Issue the quit command to the game
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "quit" );
}
