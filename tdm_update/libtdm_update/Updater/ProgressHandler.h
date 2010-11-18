#pragma once

#include "Updater.h"
#include "UpdateView.h"

namespace tdm
{

namespace updater
{

class ProgressHandler :
	public updater::Updater::DownloadProgress,
	public updater::Updater::FileOperationProgress
{
private:
	updater::CurDownloadInfo _info;

	IUpdateView& _view;

	double _recentDownloadSpeed;

public:
	ProgressHandler(IUpdateView& view) :
		_view(view)
	{}

	void OnDownloadProgress(const CurDownloadInfo& info)
	{
		ProgressInfo progress;

		progress.type = ProgressInfo::Download;
		progress.file = info.file;
		progress.overallProgressFraction = info.progressFraction > 1.0 ? 1.0 : info.progressFraction;
		progress.mirrorDisplayName = info.mirrorDisplayName;
		progress.downloadSpeed = info.downloadSpeed;
		progress.downloadedBytes = info.downloadedBytes;

		_view.OnProgressChange(progress);

		_recentDownloadSpeed = progress.downloadSpeed;
	}

	void OnDownloadFinish()
	{
		ProgressInfo progress;

		progress.type = ProgressInfo::Download;
		progress.overallProgressFraction = 1.0;
		progress.downloadSpeed = _recentDownloadSpeed;

		_view.OnProgressChange(progress);
	}

	void OnFileOperationProgress(const CurFileInfo& info)
	{
		ProgressInfo progress;

		progress.type = ProgressInfo::FileOperation;
		progress.file = info.file;
		progress.overallProgressFraction = info.overallProgressFraction > 1.0 ? 1.0 : info.overallProgressFraction;
		progress.operationType = GetOperationTypeForFileInfo(info);

		_view.OnProgressChange(progress);
	}

	void OnFileOperationFinish()
	{
		ProgressInfo progress;

		progress.type = ProgressInfo::FileOperation;
		progress.overallProgressFraction = 1.0;
		progress.operationType = ProgressInfo::Unspecified;

		_view.OnProgressChange(progress);
	}

private:
	ProgressInfo::FileOperationType GetOperationTypeForFileInfo(const CurFileInfo& info)
	{
		switch (info.operation)
		{
			case CurFileInfo::Check: return ProgressInfo::Check;
			case CurFileInfo::Delete: return ProgressInfo::Remove;
			case CurFileInfo::Replace: return ProgressInfo::Replace;
			case CurFileInfo::Add: return ProgressInfo::Add;
			case CurFileInfo::RemoveFilesFromPK4: return ProgressInfo::RemoveFilesFromPK4;
			default: return ProgressInfo::Unspecified;
		};
	}
};
typedef boost::shared_ptr<ProgressHandler> ProgressHandlerPtr;

} // namespace

} // namespace
