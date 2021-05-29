/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#pragma once

#include <string>
#include <stdexcept>
#include <memory>
#include <mutex>

namespace tdm
{

class FileOpenException : 
    public std::runtime_error 
{
public:
    FileOpenException() : std::runtime_error("File open failure") { }
};

class FileLogWriter :
	public tdm::ILogWriter
{
private:
	FILE* _logFile;

	std::mutex _mutex;

public:
	FileLogWriter(const std::string& path) :
		_logFile(NULL)
	{
		_logFile = fopen(path.c_str(), "w");
		if (_logFile == NULL) {
			throw FileOpenException();
		}

		time_t rawtime;
		time(&rawtime);

		fputs("TDM Update Logfile created: ", _logFile);
		fputs(asctime(localtime(&rawtime)), _logFile);
		fputs("\n", _logFile);
	}

	~FileLogWriter()
	{
		time_t rawtime;
		time(&rawtime);

		fputs("TDM Update Logfile closed: ", _logFile);
		fputs(asctime(localtime(&rawtime)), _logFile);
		fputs("\n", _logFile);

		fclose(_logFile);
		fflush(_logFile);
	}

	void WriteLog(LogClass lc, const std::string& str)
	{
		// Don't write progress stuff into the logfile
		if (lc != LOG_PROGRESS)
		{
			// Make sure only one thread is writing to the file at a time
			std::lock_guard<std::mutex> lock(_mutex);

			fputs(str.c_str(), _logFile);
			fflush(_logFile);
		}
	}
};

class ConsoleLogWriter :
	public tdm::ILogWriter
{
public:
	void WriteLog(LogClass lc, const std::string& str)
	{
		if (lc >= LOG_STANDARD)
		{
			std::cout << str;
		}
	}
};

void RegisterLogWriters()
{
	std::shared_ptr<tdm::FileLogWriter> logWriter(new tdm::FileLogWriter("tdm_update.log"));
	std::shared_ptr<tdm::ConsoleLogWriter> consoleWriter(new tdm::ConsoleLogWriter);

	tdm::TraceLog::Instance().Register(logWriter);
	tdm::TraceLog::Instance().Register(consoleWriter);
}

} // namespace
