/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "D3ProcessChecker.h"

#include "TraceLog.h"
#include <boost/filesystem.hpp>

// Initialise the static member
bool D3ProcessChecker::processFound = false;

namespace fs = boost::filesystem;

#ifdef WIN32

#include <string>
#include <windows.h>
#include "Psapi.h"

bool D3ProcessChecker::D3IsRunning(const std::string& processName, const std::string& moduleName)
{
	// Clear the flag
	processFound = false;
	
	DWORD processes[1024];
	DWORD num;

	if (!EnumProcesses(processes, sizeof(processes), &num)) {
		return false;
	}

	// Iterate over the processes
	for (int i = 0; i < int(num/sizeof(DWORD)); i++) {
		char szProcessName[MAX_PATH] = "unknown";

		// Get the handle for this process
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processes[i]);

		if (hProcess) {
			HMODULE hMod;
			DWORD countBytes;

			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &countBytes)) {
				
				GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName));

				std::string processName(szProcessName);

				if (processName == processName) {
					//globalOutputStream() << "Doom3 found!\n";

					HMODULE hModules[1024];
					DWORD cbNeeded;

					if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
						for (unsigned int m = 0; m < (cbNeeded / sizeof(HMODULE)); m++)	{
							TCHAR szModName[MAX_PATH];

							// Get the full path to the module's file.
							if (GetModuleBaseName(hProcess, hModules[m], szModName, sizeof(szModName)/sizeof(TCHAR))) {
								// Print the module name and handle value.
								if (std::string(szModName) == moduleName) {
									//globalOutputStream() << "Module found: " << szModName << "\n";

									CloseHandle(hProcess); // close the handle, we're terminating
									processFound = true;
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

#elif defined(__linux__)
// Linux implementation

#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>

namespace {
	const std::string PROC_FOLDER("/proc/");
}

void CheckProcFile(const std::string& path, const std::string& processName)
{
	// Get the filename from the path 
	fs::path fullPath(path);
	std::string name = fullPath.leaf();
	
	// Try to cast the filename to an integer number (=PID)
	try {
		unsigned long pid = boost::lexical_cast<unsigned long>(name);
	
		//TraceLog::WriteLine("PID file found: " + name);
		
		// Was the PID read correctly?
		if (pid == 0) {
			return;
		}
		
		const std::string cmdLineFileName = PROC_FOLDER + name + "/cmdline";
		
		//TraceLog::WriteLine("Reading commandline file " + cmdLineFileName);
		
		std::ifstream cmdLineFile(cmdLineFileName.c_str());
		if (cmdLineFile.is_open()) {
			// Read the command line from the process file
			std::string cmdLine("");
			getline(cmdLineFile, cmdLine);
			
			// The cmdLine file contains a NULL-separated argument vector
			// Only use the executable part of the command line
			cmdLine = cmdLine.substr(0, cmdLine.find('\0'));
			
			//TraceLog::WriteLine("Command line is " + cmdLine);
			
			if (cmdLine.find(processName) != std::string::npos) {
				// Process found, return success
				D3ProcessChecker::processFound = true;
				TraceLog::WriteLine("FOUND! PID is " + name);
			}
			else {
				//TraceLog::WriteLine("negative");
			}
		}
		else {
			//globalOutputStream() << "could not open cmdline file";
		}
		
		// Close the file
		cmdLineFile.close();
	}
	catch (const boost::bad_lexical_cast&) {
		// Cast to int failed, no PID
	}
}

bool D3ProcessChecker::D3IsRunning(const std::string& processName, const std::string& moduleName)
{
	// Clear the flag before searching
	processFound = false;
	
	// Search the /proc/ folder for PID files
	TraceLog::WriteLine("Searching the /proc/ folder for PID files");
	
	fs::path start(PROC_FOLDER);
	
	for (fs::recursive_directory_iterator it(start); it != fs::recursive_directory_iterator(); ++it)
	{
		// Get the candidate
		const fs::path& candidate = *it;
		
		//TraceLog::WriteLine("Candidate in /proc/: " + candidate.string());
		
		// Only search directories
		if (!fs::is_directory(candidate)) continue;
		
		// It's a file, pass this to our checker method
		CheckProcFile(candidate.string(), processName);
		
		if (processFound) break; // no need to continue if already found
		
		// Don't go any deeper than one level
		it.no_push();
	}
	
	return processFound;
}
#elif defined(MACOS_X)

// Mac OS X implementation
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysctl.h>

namespace
{

// greebo: Checks for a named process, modeled loosely after
// http://developer.apple.com/library/mac/#qa/qa2001/qa1123.html
bool FindProcessByName(const char* processName)
{
	int name[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t length = 0;
	
	// Call sysctl with a NULL buffer.
	int err = sysctl(name, 4, NULL, &length, NULL, 0);
	
	if (err == -1)
	{
		TraceLog::WriteLine("Error: Failed to receive buffer size for process list.");
		return false;
	}
	
	kinfo_proc* procList = static_cast<kinfo_proc*>(malloc(length));
	
	if (procList == NULL)
	{
		TraceLog::WriteLine("Error: Out of Memory trying to allocate process buffer");
		return false;
	}
	
	// Load process info
	sysctl(name, 4, procList, &length, NULL, 0);
	
	size_t procCount = length / sizeof(kinfo_proc);
	bool result = false;
	
	for (size_t i = 0; i < procCount; ++i)
	{
		//TraceLog::WriteLine(procList[i].kp_proc.p_comm);
		
		if (strcmp(procList[i].kp_proc.p_comm, processName) == 0)
		{
			result = true;
			break;
		}
	}
	
	free(procList);
	
	return result;
}

} // namespace

bool D3ProcessChecker::D3IsRunning(const std::string& processName, const std::string& moduleName)
{
	return FindProcessByName(processName.c_str());
}

#else
#error Unsupported Platform
#endif

