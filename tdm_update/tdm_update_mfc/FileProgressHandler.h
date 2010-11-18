#pragma once

#include "Updater/Updater.h"
#include "Util.h"

namespace tdm
{

class FileProgressHandler :
	public updater::Updater::FileOperationProgress
{
private:
	updater::CurFileInfo _info;

	UpdaterDialog& _dialog;

public:
	FileProgressHandler(UpdaterDialog& dialog) :
		_dialog(dialog)
	{}

	void OnProgress(const updater::CurFileInfo& info)
	{
		_dialog.SetProgress(info.overallProgressFraction);
		_dialog.SetProgressText(info.file.string());
	}

	void OnFinish()
	{
		_dialog.SetProgress(1.0);
	}
};

}
