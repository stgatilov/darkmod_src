/*************************************************************************
 *
 * PROJECT: The Dark Mod - Launcher
 * $Source$
 * $Revision: 4852 $
 * $Date: 2011-05-17 08:04:03 +0200 (Di, 17 Mai 2011) $
 * $Author: greebo $
 *
 *************************************************************************/

#ifndef D3PROCESSCHECKER_H_
#define D3PROCESSCHECKER_H_

#include <string>

class D3ProcessChecker
{
public:
	// Success flag
	static bool processFound;
		
	// Platform-dependent process check routine (searches for a process DOOM3.EXE with a module named gamex86.dll
	static bool D3IsRunning(const std::string& processName, const std::string& moduleName);
};

#endif /*D3PROCESSCHECKER_H_*/
