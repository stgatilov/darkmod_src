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
#ifndef _TRACELOG_H_
#define _TRACELOG_H_

#include <fstream>
#include <string>

/** 
 * greebo: Public service class to write to logfile and/or console.
 */
class TraceLog
{
	// The file stream which will be filled with bytes
	std::ofstream _logStream;

public:
	TraceLog() :
		_logStream("tdmlauncher.log")
	{}

	~TraceLog();

	// Write to the logfile
	static void Write(const std::string& str);
	static void WriteLine(const std::string& str);

	// Contains the static singleton
	static TraceLog& Instance();
};

#endif /* TRACELOG */ 
