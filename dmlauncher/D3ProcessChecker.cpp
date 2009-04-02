#include "D3ProcessChecker.h"

bool D3ProcessChecker::processFound = false;

#ifdef WIN32

#include <string>
#include <windows.h>
#include "Psapi.h"

bool D3ProcessChecker::D3IsRunning(const std::string& processName, const std::string& moduleName) {
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
#include "os/dir.h"
#include <boost/lexical_cast.hpp>
#include "stream/textstream.h" 

namespace {
	const std::string PROC_FOLDER("/proc/");
	const std::string DOOM_PROCESS_NAME("doom.x86"); 
}

void ForeachFileFunctor(const std::string& name) {
	// Try to cast the filename to an integer number (=PID)
	try {
		unsigned long pid = boost::lexical_cast<unsigned long>(name);
	
		//globalOutputStream() << "PID file found: " << name << " - ";
		
		// Was the PID read correctly?
		if (pid == 0) {
			return;
		}
		
		const std::string cmdLineFileName = PROC_FOLDER + name + "/cmdline";
		
		//globalOutputStream() << "reading commandline file " << cmdLineFileName.c_str() << " - ";
		
		std::ifstream cmdLineFile(cmdLineFileName.c_str());
		if (cmdLineFile.is_open()) {
			// Read the command line from the process file
			std::string cmdLine("");
			getline(cmdLineFile, cmdLine);
			
			//globalOutputStream() << "'" << cmdLine.c_str() << "' - ";
			
			if (cmdLine.find(DOOM_PROCESS_NAME) != std::string::npos) {
				// Process found, return success
				D3ProcessChecker::processFound = true;
				//globalOutputStream() << "GOTCHA!";
			}
			else {
				//globalOutputStream() << "negative";
			}
		}
		else {
			//globalOutputStream() << "could not open cmdline file";
		}
		
		// Close the file
		cmdLineFile.close();
		
		//globalOutputStream() << "\n";
	}
	catch (const boost::bad_lexical_cast&) {
		// Cast to int failed, no PID
	}
}

bool D3ProcessChecker::D3IsRunning() {
	// Clear the flag before searching
	processFound = false;
	
	// Traverse the /proc folder, this sets the flag to TRUE if the process was found
	Directory_forEach(PROC_FOLDER, ForeachFileFunctor);
	
	return processFound;
}

#endif

