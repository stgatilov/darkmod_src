#pragma once

#include "Constants.h"
#include "Updater/Updater.h"
#include "Updater/UpdateController.h"
#include "Updater/UpdateView.h"
#include "TraceLog.h"
#include "ExceptionSafeThread.h"
#include <signal.h>

#include "DownloadProgressPrinter.h"

namespace tdm
{

namespace updater
{

class ConsoleUpdater :
	public IUpdateView
{
public:
	enum Outcome
	{
		None,
		Failed,
		Ok,
		OkNeedRestart
	};

private:
	// The status
	Outcome _outcome;

	UpdaterOptions _options;

	// The update controller manages the logic
	UpdateController _controller;

	// Exit flag
	volatile bool _done;

	static boost::function<void(int)> _abortSignalHandler;

	// Most recent progress info
	ProgressInfo _info;

public:
	ConsoleUpdater(int argc, char* argv[]);

	~ConsoleUpdater();

	Outcome GetOutcome() 
	{
		return _outcome;
	}

	// Main entry point, algorithm starts here, does not leak exceptions
	void Run();

	// IUpdateView implementation
	void OnStartStep(UpdateStep step);
	void OnFinishStep(UpdateStep step);
	void OnFailure(UpdateStep step, const std::string& errorMessage);
	void OnProgressChange(const ProgressInfo& info);
	void OnMessage(const std::string& message);
	void OnStartDifferentialUpdate(const DifferentialUpdateInfo& info);
	void OnPerformDifferentialUpdate(const DifferentialUpdateInfo& info);
	void OnFinishDifferentialUpdate(const DifferentialUpdateInfo& info);

private:
	static void AbortSignalHandler(int signal);

	void OnAbort(int signal);

	void PrintProgress();
};

} // namespace

} // namespace
