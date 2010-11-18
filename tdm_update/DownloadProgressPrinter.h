#pragma once

#include "Updater/Updater.h"
#include "Util.h"

namespace tdm
{

const std::size_t PROGRESS_METER_WIDTH = 40;

class DownloadProgressPrinter :
	public updater::Updater::DownloadProgress
{
private:
	updater::CurDownloadInfo _info;

public:
	void OnDownloadProgress(const updater::CurDownloadInfo& info)
	{
		if (!_info.file.empty() && info.file != _info.file)
		{
			// Finish up the recent progress meter
			_info.progressFraction = 1.0f;
			PrintProgress();

			// Add a line break when a new file starts
			TraceLog::WriteLine(LOG_STANDARD, "");

			PrintFileInfo(info);
		}
		else if (_info.file.empty())
		{
			// First file
			PrintFileInfo(info);
		}

		_info = info;

		// Print the new info
		PrintProgress();
	}

	void OnDownloadFinish()
	{
		if (!_info.file.empty())
		{
			_info.progressFraction = 1.0f;
			PrintProgress();
		}

		// Add a line break when downloads are done
		TraceLog::WriteLine(LOG_STANDARD, "");
	}

private:
	void PrintFileInfo(const updater::CurDownloadInfo& info)
	{
		TraceLog::WriteLine(LOG_STANDARD, 
			(boost::format(" Downloading from Mirror %s: %s") % info.mirrorDisplayName % info.file.string()).str());
	}

	void PrintProgress()
	{
		TraceLog::Write(LOG_STANDARD, "\r");
		
		std::size_t numTicks = static_cast<std::size_t>(floor(_info.progressFraction * PROGRESS_METER_WIDTH));

		std::string progressBar(numTicks, '=');
		std::string progressSpace(PROGRESS_METER_WIDTH - numTicks, ' ');

		TraceLog::Write(LOG_STANDARD, " [" + progressBar + progressSpace + "]");

		TraceLog::Write(LOG_STANDARD, (boost::format(" %2.1f%%") % (_info.progressFraction*100)).str());

		TraceLog::Write(LOG_STANDARD, " at " + Util::GetHumanReadableBytes(static_cast<std::size_t>(_info.downloadSpeed)) + "/sec ");
	}
};

} // namespace
