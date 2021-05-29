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

#include "UpdateController.h"

#include "../Util.h"

namespace tdm
{

namespace updater
{

UpdateController::UpdateController(IUpdateView& view, const fs::path& executableName, UpdaterOptions& options) :
	_view(view),
	_curStep(Init),
	_updater(options, executableName.filename()),
	_abortFlag(false),
	_progress(new ProgressHandler(_view)),
	_differentialUpdatePerformed(false)
{
	_updater.SetDownloadProgressCallback(_progress);
	_updater.SetFileOperationProgressCallback(_progress);
}

UpdateController::~UpdateController()
{
	_updater.ClearFileOperationProgressCallback();
	_updater.ClearDownloadProgressCallback();
}

void UpdateController::PauseAt(UpdateStep step)
{
	_interruptionPoints.insert(step);
}

void UpdateController::DontPauseAt(UpdateStep step)
{
	_interruptionPoints.erase(step);
}

void UpdateController::StartOrContinue()
{
	assert(_workerThread == NULL); // debug builds take this seriously

	if (_workerThread != NULL) return;

	// Launch the thread and setup the callbacks
	StartStepThread(_curStep);
}

void UpdateController::PerformPostUpdateCleanup()
{
	_updater.PostUpdateCleanup();
}

void UpdateController::Abort()
{
	_abortFlag = true;

	if (_workerThread != NULL)
	{
		try
		{
			_workerThread->interrupt();
		}
		catch (std::runtime_error& ex)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Controller thread aborted." + std::string(ex.what()));
		}

		_updater.CancelDownloads();
	}
}

bool UpdateController::AllThreadsDone()
{
	return _workerThread == NULL;
}

std::size_t UpdateController::GetNumMirrors()
{
	return _updater.GetNumMirrors();
}

bool UpdateController::NewUpdaterAvailable()
{
	return _updater.NewUpdaterAvailable();
}

bool UpdateController::LocalFilesNeedUpdate()
{
	return _updater.LocalFilesNeedUpdate();
}

std::size_t UpdateController::GetTotalDownloadSize()
{
	return _updater.GetTotalDownloadSize();
}

std::size_t UpdateController::GetTotalBytesDownloaded()
{
	return _updater.GetTotalBytesDownloaded();
}

std::size_t UpdateController::GetNumFilesToBeUpdated()
{
	return _updater.GetNumFilesToBeUpdated();
}

bool UpdateController::RestartRequired()
{
	return _updater.RestartRequired();
}

bool UpdateController::DifferentialUpdateAvailable()
{
	return _updater.DifferentialUpdateAvailable();
}

std::string UpdateController::GetLocalVersion()
{
	return _updater.GetDeterminedLocalVersion();
}

std::string UpdateController::GetNewestVersion()
{
	return _updater.GetNewestVersion();
}

std::size_t UpdateController::GetTotalDifferentialUpdateSize()
{
	return _updater.GetTotalDifferentialUpdateSize();
}

DifferentialUpdateInfo UpdateController::GetDifferentialUpdateInfo()
{
	return _updater.GetDifferentialUpdateInfo();
}

void UpdateController::StartStepThread(UpdateStep step)
{
	if (_abortFlag) return;

	// Construct a new thread and start working
	_workerThread.reset(new ExceptionSafeThread(
		std::bind(&UpdateController::PerformStep, this, step),
		std::bind(&UpdateController::OnFinishStep, this, step)));
}

void UpdateController::PerformStep(UpdateStep step)
{
	TraceLog::WriteLine(LOG_VERBOSE, "Step thread started: " + std::to_string(step));

	_view.OnStartStep(step);

	// Dispatch the calls to the updater, any exceptions will be caught by the ExceptionSafeThread class

	switch (step)
	{
	case Init:
		// Check if TDM is active
		if (Util::TDMIsRunning())
		{
			// grayman - change "Doom3" to "The Dark Mod"
			_view.OnWarning("The Dark Mod was found to be active.\nThe updater will not be able to update any Dark Mod PK4s.\nPlease exit The Dark Mod before continuing.");
		}

		if (Util::DarkRadiantIsRunning())
		{
			_view.OnWarning("DarkRadiant was found to be active.\nThe updater will not be able to update any Dark Mod PK4s.\nPlease exit DarkRadiant before continuing.");
		}

		if (Util::HasElevatedPrivilegesWindows())
		{
			_view.OnWarning(
				"The updater was run \"as admin\". This is strongly discouraged!\n"
				"Please abort installation and restart updater without admin rights.\n"
				"Otherwise admin rights will most likely be necessary to play the game.\n"
				"\n"
				"Note: Better install TDM into a directory where you can normally save files without UAC prompt. "
				"Avoid \"Program Files\", install into something like C:\\games\\thedarkmod instead. "
			);
		}

		break;

	case CleanupPreviousSession:
		// Pass the call to the updater
		_updater.CleanupPreviousSession();
		break;

	case UpdateMirrors:
		// Pass the call to the updater
		if (_updater.MirrorsNeedUpdate())
		{
			_updater.UpdateMirrors();
		}
		else
		{
			// Load Mirrors
			_updater.LoadMirrors();
		}
		break;

	case DownloadCrcs:
		_updater.GetCrcFromServer();
		break;

	case CompareLocalFilesToNewest:
		_updater.CheckLocalFiles();
		break;

	case DownloadVersionInfo:
		_updater.GetVersionInfoFromServer();
		break;

	case DetermineLocalVersion:
		_updater.DetermineLocalVersion();
		break;

	case DownloadNewUpdater:
		// Prepare, Download, Apply
		_updater.PrepareUpdateStep();
		_updater.PerformUpdateStep();
		_updater.CleanupUpdateStep();
		break;
	
	case DownloadDifferentialUpdate:
		{
            DifferentialUpdateInfo info = _updater.GetDifferentialUpdateInfo();
			_view.OnStartDifferentialUpdate(info);

			// Download the update package, integrate it
			_updater.DownloadDifferentialUpdate();
		}
		break;

	case PerformDifferentialUpdate:
		{
			DifferentialUpdateInfo info = _updater.GetDifferentialUpdateInfo();

			_view.OnPerformDifferentialUpdate(info);

			_updater.PerformDifferentialUpdateStep();
		}
		break;

	case DownloadFullUpdate:
        _updater.PrepareUpdateStep();
		_updater.PerformUpdateStep();
		_updater.CleanupUpdateStep();
		break;

	case InstallVcRedist:
#if 0
		bool need32bit; need32bit = _updater.NeedsVCRedist(false);
		bool need64bit; need64bit = _updater.NeedsVCRedist(true);
		if (need32bit || need64bit) {
			_view.OnMessage(
				"Microsoft Visual C++ redistributable package will be installed now.\n"
				"You will probably be prompted for admin permissions (with UAC dialog).\n"
			);
			if (need64bit)
				_updater.InstallVCRedist(true);
			if (need32bit)
				_updater.InstallVCRedist(false);

			bool stillNeed32bit =_updater.NeedsVCRedist(false);
			bool stillNeed64bit =_updater.NeedsVCRedist(true);
			if (stillNeed32bit || stillNeed64bit) {
				std::string which = (stillNeed32bit && stillNeed64bit ? "vcredist_x86.exe and vcredist_x64.exe" : (stillNeed64bit ? "vcredist_x64.exe" : "vcredist_x86.exe"));
				_view.OnWarning(
					"Microsoft Visual C++ redistributable package failed to install.\n"
					"You have to install it yourself in order to run the game.\n"
					"Run " + which + " in game directory and make sure installation completes successfully."
				);
			}
		}
#endif
		break;

	case PostUpdateCleanup:
		//stgatilov #4864 and #5042: timestamps of files inside ZIP should not matter any more
		//_updater.FixPK4Dates();

		_updater.PostUpdateCleanup();
		break;

	case RestartUpdater:
		_updater.RestartUpdater();
		break;

	case Done:
		break;
	};
}

void UpdateController::OnFinishStep(UpdateStep step)
{
	assert(_workerThread != NULL); // Need an alive thread object

	TraceLog::WriteLine(LOG_VERBOSE, "Step thread finished: " + std::to_string(step));

	if (_workerThread->failed())
	{
		std::string errMsg = _workerThread->GetErrorMessage();

		TraceLog::WriteLine(LOG_VERBOSE, "Thread finished with error: " + errMsg);

		// Kill the thread object before notifying the frontend, it might react
		// with a StartOrContinue call, so we need to be prepared
		_workerThread.reset();

		// Notify the view
		_view.OnFailure(step, errMsg);

		return; // don't continue here, don't increase the _curStep pointer
	}
	else
	{
		// Kill the thread object before notifying the frontend, it might react
		// with a StartOrContinue call, so we need to be prepared
		_workerThread.reset();

		// Notify the view
		_view.OnFinishStep(step);
	}

	switch (_curStep)
	{
	case Init:
		// Startup, there's nothing to do, proceed to cleanup if allowed
		TryToProceedTo(CleanupPreviousSession);
		break;

	case CleanupPreviousSession:
		TryToProceedTo(UpdateMirrors);
		break;

	case UpdateMirrors:
		TryToProceedTo(DownloadCrcs);
		break;

	case DownloadCrcs:
		TryToProceedTo(DownloadVersionInfo);
		break;

	case DownloadVersionInfo:
		// Compare local version to see if it matches
		TryToProceedTo(DetermineLocalVersion);
		break;

	case DetermineLocalVersion:
		TryToProceedTo(CompareLocalFilesToNewest);
		break;

	case CompareLocalFilesToNewest:
		{
			if (_updater.NewUpdaterAvailable())
			{
				// Update necessary, new updater available
				TryToProceedTo(DownloadNewUpdater);
			}
			else if (_updater.LocalFilesNeedUpdate())
			{
				// Update necessary, updater is ok
				if (_updater.DifferentialUpdateAvailable())
				{
					TryToProceedTo(DownloadDifferentialUpdate);
				}
				else
				{
					// Did we already perform a differential update?
					if (_differentialUpdatePerformed)
					{
						_view.OnMessage("A differential update has been applied to your installation,\nthough some files still need to be updated.");
					}

					TryToProceedTo(DownloadFullUpdate);
				}
			}
			else
			{
				// No update necessary
				TryToProceedTo(InstallVcRedist);
			}
		}
		break;

	case DownloadNewUpdater:
		{
			if (_updater.RestartRequired())
			{
				TryToProceedTo(RestartUpdater);
			}
			else
			{
				TryToProceedTo(Done);
			}
		}
		break;

	case DownloadDifferentialUpdate:
		{
			TryToProceedTo(PerformDifferentialUpdate);
		}
		break;

	case PerformDifferentialUpdate:
		{
			_differentialUpdatePerformed = true;

			// After applying a differential update, check our progress
			DifferentialUpdateInfo info = _updater.GetDifferentialUpdateInfo();

			_view.OnFinishDifferentialUpdate(info);

			//#4529 stgatilov: Since differential update does not uncompress unchanged OGG/ROQ files,
			//The hashes of pk4 files are now different from the ones on the server.
			//That's why we exit right away to avoid redoing the same differential update and redownloading the unchanged data.
			if (info.fromVersion == "2.05" && info.toVersion == "2.06") {
				TryToProceedTo(InstallVcRedist);
				break;
			}

			// Go back to determine our local version
			TryToProceedTo(DetermineLocalVersion);
		}
		break;

	case DownloadFullUpdate:
		TryToProceedTo(InstallVcRedist);
		break;

	case InstallVcRedist:
		TryToProceedTo(PostUpdateCleanup);
		break;

	case PostUpdateCleanup:
		TryToProceedTo(Done);
		break;

	case RestartUpdater:
		TryToProceedTo(Done);
		break;

	case Done:
		// Nothing to do
		break;
	};
}

void UpdateController::TryToProceedTo(UpdateStep step)
{
	if (_abortFlag) return;

	_curStep = step;

	if (IsAllowedToContinueTo(step))
	{
		StartStepThread(step);
	}
}

// Returns true if we are allowed to proceed to the given step
bool UpdateController::IsAllowedToContinueTo(UpdateStep step)
{
	return _interruptionPoints.find(step) == _interruptionPoints.end();
}

bool UpdateController::IsDone()
{
	return _curStep == Done;
}

} // namespace

} // namespace
