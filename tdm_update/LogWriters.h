#pragma once

#include <string>
#include <boost/shared_ptr.hpp>

namespace tdm
{

class FileLogWriter :
	public tdm::ILogWriter
{
private:
	FILE* _logFile;
public:
	FileLogWriter(const std::string& path) :
		_logFile(fopen(path.c_str(), "w"))
	{
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
	boost::shared_ptr<tdm::FileLogWriter> logWriter(new tdm::FileLogWriter("tdm_update.log"));
	boost::shared_ptr<tdm::ConsoleLogWriter> consoleWriter(new tdm::ConsoleLogWriter);

	tdm::TraceLog::Instance().Register(logWriter);
	tdm::TraceLog::Instance().Register(consoleWriter);
}

} // namespace
