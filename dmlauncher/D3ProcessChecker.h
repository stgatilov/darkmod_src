#ifndef D3PROCESSCHECKER_H_
#define D3PROCESSCHECKER_H_

#include <string>

class D3ProcessChecker {
public:
	// Success flag
	static bool processFound;
		
	// Platform-dependent process check routine (searches for a process DOOM3.EXE with a module named gamex86.dll
	static bool D3IsRunning(const std::string& processName, const std::string& moduleName);
};

#endif /*D3PROCESSCHECKER_H_*/
