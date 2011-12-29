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
