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

#include "ConsoleUpdater.h"

#include <functional>
#include "Util.h"

namespace tdm
{

namespace updater
{

namespace
{
	const std::size_t PROGRESS_METER_WIDTH = 25;

	std::string GetShortenedString(const std::string& input, std::size_t maxLength)
	{
		if (input.length() > maxLength)
		{
			if (maxLength == 0)
			{
				return "";
			}
			else if (maxLength < 3)
			{
				return std::string(maxLength, '.');
			}

			std::size_t diff = input.length() - maxLength + 3; // 3 chars for the ellipsis
			std::size_t curLength = input.length();

			return input.substr(0, (curLength - diff) / 2) + "..." +
				input.substr((curLength + diff) / 2);
		}

		return input;
	}
}

ConsoleUpdater::ConsoleUpdater(int argc, char* argv[]) :
	_outcome(None),
	_options(argc, argv),
	_controller(*this, fs::path(argv[0]).filename(), _options),
	_done(false)
{
	_abortSignalHandler = std::bind(&ConsoleUpdater::OnAbort, this, std::placeholders::_1);

	signal(SIGABRT, AbortSignalHandler);
	signal(SIGTERM, AbortSignalHandler);
	signal(SIGINT, AbortSignalHandler);
}

ConsoleUpdater::~ConsoleUpdater()
{
	_abortSignalHandler = std::function<void(int)>();
}

void ConsoleUpdater::Run()
{
	// Parse the command line
	if (_options.IsSet("help"))
	{
		_options.PrintHelp();
		_outcome = Ok;
		return;
	}

	_controller.StartOrContinue();

	// Main loop, just keep the controller object going
	while (!_done)
	{
		Sleep(50);
	}

	if (!_controller.AllThreadsDone())
	{
		// Termination seems to be abnormal, attempt to exit gracefully 

		TraceLog::WriteLine(LOG_STANDARD, " Waiting 5 seconds for threads to finish their work...");
		std::size_t count = 0;

		while (count++ < 100 && !_controller.AllThreadsDone())
		{
			Sleep(50);
		}

		// Exit anyway after 5000 milliseconds

		_controller.PerformPostUpdateCleanup();
	}
}

void ConsoleUpdater::AbortSignalHandler(int signal)
{
	if (_abortSignalHandler)
	{
		_abortSignalHandler(signal);
	}
}

void ConsoleUpdater::OnAbort(int signal)
{
	TraceLog::Error("\nAbort signal received, trying to exit gracefully.");

	_controller.Abort();

	_done = true; // exit main loop
}

void ConsoleUpdater::OnStartStep(UpdateStep step)
{
	switch (step)
	{
	case Init:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Initialising...");
		break;

	case CleanupPreviousSession:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Cleaning up previous update session...");
		break;

	case UpdateMirrors:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Downloading mirror information...");
		break;

	case DownloadCrcs:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Downloading CRC file...");
		break;

	case DownloadVersionInfo:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Downloading version info file...");
		break;

	case DetermineLocalVersion:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Trying to match local files to version definitions...");
		break;

	case CompareLocalFilesToNewest:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Comparing local files to server definitions...");
		break;

	case DownloadNewUpdater:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Downloading TDM Update application...");
		break;

	case DownloadDifferentialUpdate:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Downloading differential update package...");
		break;

	case PerformDifferentialUpdate:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Applying differential update package...");
		break;

	case DownloadFullUpdate:
		TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " Downloading updates...");
		break;

    case PostUpdateCleanup:
        TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
        TraceLog::WriteLine(LOG_STANDARD, " Performing cleanup steps and correcting bad dates in PK4 files");
        break;
	};
}

void ConsoleUpdater::OnFinishStep(UpdateStep step)
{
    switch (step)
    {
    case Init:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done.");
    }
    break;

    case CleanupPreviousSession:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done.");
    }
    break;

    case UpdateMirrors:
    {
        // Mirrors
        bool keepMirrors = _options.IsSet("keep-mirrors");

        if (keepMirrors)
        {
            TraceLog::WriteLine(LOG_STANDARD, " Skipped downloading mirrors.");
        }
        else
        {
            TraceLog::WriteLine(LOG_STANDARD, " Done downloading mirrors.");
        }

        std::size_t numMirrors = _controller.GetNumMirrors();

        TraceLog::WriteLine(LOG_STANDARD, stdext::format("   Found %d mirror%s.", numMirrors, (numMirrors == 1 ? "" : "s")));

        if (numMirrors == 0)
        {
            TraceLog::WriteLine(LOG_STANDARD, " No mirror information available - cannot continue.");

            // Stop right here
            _controller.Abort();
        }
    }
    break;

    case DownloadCrcs:
    {
        //TraceLog::WriteLine(LOG_STANDARD, " Done downloading checksums.");
    }
    break;

    case DownloadVersionInfo:
    {
        TraceLog::Write(LOG_STANDARD, " Done downloading versions.");

        if (!_controller.GetNewestVersion().empty())
        {
            std::string newest = stdext::format(" Newest version is %s.", _controller.GetNewestVersion());
            TraceLog::WriteLine(LOG_STANDARD, newest);
        }
        else
        {
            TraceLog::WriteLine(LOG_STANDARD, "");
        }
    }
    break;

    case DetermineLocalVersion:
    {
        TraceLog::Write(LOG_STANDARD, " Done comparing local files: ");

        if (_controller.GetLocalVersion().empty())
        {
            TraceLog::WriteLine(LOG_STANDARD, "no luck, PK4 files do not match.");
        }
        else
        {
            std::string versionFound = stdext::format("local version is %s.", _controller.GetLocalVersion());
            TraceLog::WriteLine(LOG_STANDARD, versionFound);
        }
    }
    break;

    case CompareLocalFilesToNewest:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done comparing local files to server definitions.");

        std::string sizeStr = Util::GetHumanReadableBytes(_controller.GetTotalDownloadSize());
        std::size_t numFiles = _controller.GetNumFilesToBeUpdated();

        std::string totalSize = stdext::format("  %d %s to be downloaded (size: %s).", numFiles, (numFiles == 1 ? "file needs" : "files need"), sizeStr);

        // Print a summary
        if (_controller.NewUpdaterAvailable())
        {
            TraceLog::WriteLine(LOG_STANDARD, " A new updater is available: " + totalSize);
        }
        else if (_controller.LocalFilesNeedUpdate())
        {
            if (_controller.DifferentialUpdateAvailable())
            {
                TraceLog::Write(LOG_STANDARD, " A differential update is available.");

                sizeStr = Util::GetHumanReadableBytes(_controller.GetTotalDifferentialUpdateSize());
                totalSize = stdext::format(" Download size: %s", sizeStr);
            }
            else
            {
                TraceLog::Write(LOG_STANDARD, " Updates are available.");
            }

            TraceLog::WriteLine(LOG_STANDARD, totalSize);
        }
        else
        {
            TraceLog::WriteLine(LOG_STANDARD, " Your TDM installation is up to date");
        }
    }
    break;

    case DownloadNewUpdater:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done downloading updater - will restart the application.");
    }
    break;

    case DownloadDifferentialUpdate:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done downloading the differential update.");
    }
    break;

    case PerformDifferentialUpdate:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done applying the differential update.");
    }
    break;

    case DownloadFullUpdate:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done downloading updates.");

        std::string totalBytesStr = stdext::format(" Total bytes downloaded: %s", Util::GetHumanReadableBytes(_controller.GetTotalBytesDownloaded()));
        TraceLog::WriteLine(LOG_STANDARD, totalBytesStr);
    }
    break;

    case PostUpdateCleanup:
    {
        TraceLog::WriteLine(LOG_STANDARD, " Done performing cleanup steps.");
    }
    break;

    case Done:
    {
        if (!_controller.LocalFilesNeedUpdate())
        {
            TraceLog::WriteLine(LOG_STANDARD, "----------------------------------------------------------------------------");
            TraceLog::WriteLine(LOG_STANDARD, " Your TDM installation is up to date.");
        }

        _done = true; // break main loop
    }
    break;

    case RestartUpdater:
    {
        _done = true; // break main loop
    }
	break;

	};
}

void ConsoleUpdater::OnFailure(UpdateStep step, const std::string& errorMessage)
{
	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::Error(errorMessage);

	_done = true; // break main loop
}

void ConsoleUpdater::OnMessage(const std::string& message)
{
	TraceLog::WriteLine(LOG_STANDARD, "=======================================");
	TraceLog::WriteLine(LOG_STANDARD, message);
}

void ConsoleUpdater::OnWarning(const std::string& message)
{
	TraceLog::WriteLine(LOG_STANDARD, "============== WARNING ================");
	TraceLog::WriteLine(LOG_STANDARD, message);
	TraceLog::WriteLine(LOG_STANDARD, "=======================================");

	TraceLog::WriteLine(LOG_STANDARD, "Waiting 10 seconds before continuing automatically...");

	Util::Wait(10000);
}

void ConsoleUpdater::OnProgressChange(const ProgressInfo& info)
{
	switch (info.type)
	{
	case ProgressInfo::FileDownload:
		// Download progress
		if (!_info.file.empty() && !info.file.empty() && info.file != _info.file)
		{
			// New file, finish the current download
			_info.progressFraction = 1.0f;
			PrintProgress();

			// Add a line break when a new file starts
			TraceLog::WriteLine(LOG_PROGRESS, "");

			TraceLog::WriteLine(LOG_PROGRESS, 
				stdext::format(" Downloading from Mirror %s: %s", info.mirrorDisplayName, info.file.string()));
		}
		else if (_info.file.empty())
		{
			// First file
			TraceLog::WriteLine(LOG_PROGRESS, 
				stdext::format(" Downloading from Mirror %s: %s", info.mirrorDisplayName, info.file.string()));
		}

		_info = info;

		// Print the new info
		PrintProgress();

		// Add a new line if we're done here
		if (info.progressFraction >= 1)
		{
			TraceLog::WriteLine(LOG_PROGRESS, "");
		}
		break;

	case ProgressInfo::FileOperation:

		_info = info;

		// Print the new info
		PrintProgress();

		// Add a new line if we're done here
		if (info.progressFraction >= 1)
		{
			TraceLog::WriteLine(LOG_PROGRESS, "");
		}
		break;
	};
}

void ConsoleUpdater::PrintProgress()
{
	TraceLog::Write(LOG_PROGRESS, "\r");

	// Progress bar
	std::size_t numTicks = static_cast<std::size_t>(floor(_info.progressFraction * PROGRESS_METER_WIDTH));
	std::string progressBar(numTicks, '=');
	std::string progressSpace(PROGRESS_METER_WIDTH - numTicks, ' ');

	std::string line = " [" + progressBar + progressSpace + "]";
	
	// Percent
	line += stdext::format(" %2.1f%%", (_info.progressFraction*100));

	switch (_info.type)
	{
	case ProgressInfo::FileDownload:	
	{
		line += " at " + Util::GetHumanReadableBytes(static_cast<std::size_t>(_info.downloadSpeed)) + "/sec ";
	}
	break;

	case ProgressInfo::FileOperation:
	{
		std::string verb;

		switch (_info.operationType)
		{
		case ProgressInfo::Check: 
			verb = "Checking: "; 
			break;
		case ProgressInfo::Remove: 
			verb = "Removing: ";
			break;
		case ProgressInfo::Replace: 
			verb = "Replacing: ";
			break;
		case ProgressInfo::Add: 
			verb = "Adding: ";
			break;
		case ProgressInfo::RemoveFilesFromPK4: 
			verb = "Preparing PK4: ";
			break;
        case ProgressInfo::RegeneratePK4:
            verb = "Regenerating PK4: ";
            break;
		default: 
			verb = "File: ";
		};

		line += " " + verb;

		std::size_t remainingLength = line.length() > 79 ? 0 : 79 - line.length();
		line += GetShortenedString(_info.file.filename().string(), remainingLength);
	}
	break;
	};

	// Expand the line length to 79 characters
	if (line.length() < 79)
	{
		line += std::string(79 - line.length(), ' ');
	}
	else if (line.length() > 79)
	{
		line = line.substr(0, 79);
	}

	TraceLog::Write(LOG_PROGRESS, line);
}

void ConsoleUpdater::OnStartDifferentialUpdate(const DifferentialUpdateInfo& info)
{
}

void ConsoleUpdater::OnPerformDifferentialUpdate(const DifferentialUpdateInfo& info)
{
}

void ConsoleUpdater::OnFinishDifferentialUpdate(const DifferentialUpdateInfo& info)
{
}

std::function<void(int)> ConsoleUpdater::_abortSignalHandler;

} // namespace

} // namespace
