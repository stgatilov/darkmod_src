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

#else
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

#endif

