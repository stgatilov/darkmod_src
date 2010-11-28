/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

#include <string>
#include <set>
#include <boost/shared_ptr.hpp>

namespace tdm
{

enum LogClass
{
	LOG_VERBOSE,		// print every little thing
	LOG_STANDARD,		// mirror info, file checks, etc.
	LOG_PROGRESS,		// download/file operation progress
	LOG_ERROR,			// program errors
};

/**
 * An abstract class that digests log messages.
 *
 * Client code can implement this class and register it in 
 * the library's TraceLog instance.
 */
class ILogWriter
{
public:
	virtual ~ILogWriter() {}

	/**
	 * A Log writer must implement this method such that it can receive
	 * text messages sent by the library.
	 */
	virtual void WriteLog(LogClass lc, const std::string& str) = 0;
};
typedef boost::shared_ptr<ILogWriter> ILogWriterPtr;

/**
 * The tracelog singleton class. Register any LogWriters here, to have
 * the library's log output being sent to them.
 */
class TraceLog
{
private:
	typedef std::set<ILogWriterPtr> LogWriters;
	LogWriters _writers;

public:
	// Add a new logwriter to this instance. All future logging output will be sent
	// to this log writer too.
	void Register(const ILogWriterPtr& logWriter);

	// Remove a logwriter, no more logging will be sent to it.
	void Unregister(const ILogWriterPtr& logWriter);

	// Write a string to the trace log, this is broadcast to all registered writers.
	static void Write(LogClass lc, const std::string& output);

	// Write a line to the trace log, this is broadcast to all registered writers.
	// A line break is appended automatically at the end of the given string.
	static void WriteLine(LogClass lc, const std::string& output);

	// Convenience method, wraps to WriteLine(LOG_ERROR, ...)
	static void Error(const std::string& output)
	{
		WriteLine(LOG_ERROR, output);
	}

	// Accessor to the singleton instance
	static TraceLog& Instance();
};

} // namespace
