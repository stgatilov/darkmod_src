/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "Util.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace fs = boost::filesystem;

#ifdef WIN32

#include <string>
#include <windows.h>
#include "Psapi.h"

namespace tdm
{

bool Util::D3IsRunning()
{
	DWORD processes[1024];
	DWORD num;

	if (!EnumProcesses(processes, sizeof(processes), &num))
	{
		return false;
	}

	// Iterate over the processes
	for (int i = 0; i < int(num/sizeof(DWORD)); i++)
	{
		char szProcessName[MAX_PATH] = "unknown";

		// Get the handle for this process
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processes[i]);

		if (hProcess)
		{
			HMODULE hMod;
			DWORD countBytes;

			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &countBytes))
			{
				GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName));

				std::string processName(szProcessName);

				if (processName == "DOOM3.exe")
				{
					HMODULE hModules[1024];
					DWORD cbNeeded;

					if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded))
					{
						for (unsigned int m = 0; m < (cbNeeded / sizeof(HMODULE)); m++)
						{
							TCHAR szModName[MAX_PATH];

							// Get the full path to the module's file.
							if (GetModuleBaseName(hProcess, hModules[m], szModName, sizeof(szModName)/sizeof(TCHAR)))
							{
								// Print the module name and handle value.
								if (std::string(szModName) == "gamex86.dll")
								{
									CloseHandle(hProcess); // close the handle, we're terminating

									return true;
								}
							}
						}
					}
				}
			}
		}

		CloseHandle(hProcess);
	}

	return false;
}

bool Util::DarkRadiantIsRunning()
{
	DWORD processes[1024];
	DWORD num;

	if (!EnumProcesses(processes, sizeof(processes), &num))
	{
		return false;
	}

	// Iterate over the processes
	for (int i = 0; i < int(num/sizeof(DWORD)); i++)
	{
		char szProcessName[MAX_PATH] = "unknown";

		// Get the handle for this process
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processes[i]);

		if (hProcess)
		{
			HMODULE hMod;
			DWORD countBytes;

			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &countBytes))
			{
				GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName));

				std::string processName(szProcessName);
				boost::algorithm::to_lower(processName);

				if (processName == "darkradiant.exe")
				{
					CloseHandle(hProcess); // close the handle, we're terminating
					return true;
				}
			}
		}

		CloseHandle(hProcess);
	}

	return false;
}

} // namespace

#elif defined(__linux__)
// Linux implementation

#include <iostream>
#include <fstream>
#include <string>
#include <boost/lexical_cast.hpp>

namespace tdm
{

namespace
{
	const std::string PROC_FOLDER("/proc/");
	const std::string DOOM_PROCESS_NAME("doom.x86"); 

	bool CheckProcessFile(const std::string& name, const std::string& processName)
	{
		// Try to cast the filename to an integer number (=PID)
		try
		{
			unsigned long pid = boost::lexical_cast<unsigned long>(name);
		
			// Was the PID read correctly?
			if (pid == 0)
			{
				return false;
			}
			
			const std::string cmdLineFileName = PROC_FOLDER + name + "/cmdline";
			
			std::ifstream cmdLineFile(cmdLineFileName.c_str());

			if (cmdLineFile.is_open())
			{
				// Read the command line from the process file
				std::string cmdLine;
				getline(cmdLineFile, cmdLine);
				
				if (cmdLine.find(processName) != std::string::npos)
				{
					// Process found, return success
					return true;
				}
			}
			
			// Close the file
			cmdLineFile.close();
		}
		catch (const boost::bad_lexical_cast&)
		{
			// Cast to int failed, no PID
		}

		return false;
	}

} // namespace

bool Util::D3IsRunning()
{
	// Traverse the /proc folder, this sets the flag to TRUE if the process was found
	for (fs::directory_iterator i = fs::directory_iterator(PROC_FOLDER); i != fs::directory_iterator(); ++i)
	{
		if (CheckProcessFile(i->leaf(), DOOM_PROCESS_NAME))
		{
			return true;
		}
	}
	
	return false;
}

bool Util::DarkRadiantIsRunning()
{
	// Traverse the /proc folder, this sets the flag to TRUE if the process was found
	for (fs::directory_iterator i = fs::directory_iterator(PROC_FOLDER); i != fs::directory_iterator(); ++i)
	{
		if (CheckProcessFile(i->leaf(), "darkradiant"))
		{
			return true;
		}
	}
	
	return false;
}

} // namespace

#elif defined(MACOS_X)
// Mac OS X

namespace tdm
{

bool Util::D3IsRunning()
{
	// Not implemented for Mac
	return false;
}

bool Util::DarkRadiantIsRunning()
{
	// Not implemented for Mac
	return false;
}

} // namespace

#else
#error Unsupported Platform
#endif
