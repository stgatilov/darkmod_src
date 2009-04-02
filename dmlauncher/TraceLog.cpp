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
