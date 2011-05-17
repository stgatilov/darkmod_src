/*************************************************************************
 *
 * PROJECT: The Dark Mod - Launcher
 * $Source$
 * $Revision: 4852 $
 * $Date: 2011-05-17 08:04:03 +0200 (Di, 17 Mai 2011) $
 * $Author: greebo $
 *
 *************************************************************************/

#include "TraceLog.h"

#include <iostream>

TraceLog::~TraceLog()
{
	_logStream << "Closing logfile." << std::endl;

	_logStream.flush();
	_logStream.close();
}

void TraceLog::Write(const std::string& str)
{
	Instance()._logStream << str;
	std::cout << str;
}

void TraceLog::WriteLine(const std::string& str)
{
	Instance()._logStream << str << std::endl;
	std::cout << str << std::endl;
}

TraceLog& TraceLog::Instance() 
{
	static TraceLog _instance;
	return _instance;
}
