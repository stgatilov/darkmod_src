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
