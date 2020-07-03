#include "GuiPageSettings.h"
#include "GuiFluidAutoGen.h"
#include <FL/Fl_File_Chooser.H>
#include "StdFilesystem.h"
#include "StdString.h"
#include "OsUtils.h"
#include "Actions.h"
#include "InstallerConfig.h"

//TODO: move to better place
#include "CommandLine.h"
class ProgressIndicatorGui : public ZipSync::ProgressIndicator {
	Fl_Progress *_widget;
public:
	ProgressIndicatorGui(Fl_Progress *widget) : _widget(widget) {}
    virtual void Update(const char *line) {
		_widget->label(line);
		Fl::flush();
	}
    virtual void Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") {
		_widget->value(100.0 * globalRatio);
		_widget->label(globalComment.c_str());
		Fl::flush();
	}
};



void cb_Settings_InputInstallDirectory(Fl_Widget *self) {
	std::string installDir = g_Settings_InputInstallDirectory->value();
	bool invalid = false;

	//normalize current path
	std::string normalizedDir = installDir;
	if (normalizedDir.find("..") != -1) {
		try {
			normalizedDir = stdext::canonical(normalizedDir).string();	//collapse parents
		} catch(stdext::filesystem_error &e) {
			invalid = true;	//happens on Linux sometimes
		}
	}
	normalizedDir = stdext::path(normalizedDir).string();	//normalize slashes
	if (normalizedDir != installDir) {
		g_Settings_InputInstallDirectory->value(normalizedDir.c_str());
		installDir = normalizedDir;
	}

	//color red if path is bad
	static int DefaultColor = g_Settings_InputInstallDirectory->color();
	if (installDir.find('.') != -1)
		invalid = true;
	if (!stdext::path(installDir).is_absolute())
		invalid = true;
	bool pathExists = stdext::exists(installDir);
	bool pathIsDir = stdext::is_directory(installDir);
	if (pathExists && !pathIsDir)
		invalid = true;
	if (invalid) {
		g_Settings_InputInstallDirectory->color(FL_RED);
		g_Settings_ButtonRestartNewDir->deactivate();
	}
	else {
		g_Settings_InputInstallDirectory->color(DefaultColor);
		g_Settings_ButtonRestartNewDir->activate();
	}
	g_Settings_InputInstallDirectory->redraw();

	//update Next and Restart buttons
	std::string defaultDir = OsUtils::GetCwd();
	if (defaultDir != installDir) {
		if (pathIsDir)
			g_Settings_ButtonRestartNewDir->label("Restart");
		else
			g_Settings_ButtonRestartNewDir->label("Create and Restart");
		g_Settings_ButtonRestartNewDir->show();
		g_Settings_ButtonNext->deactivate();
	}
	else {
		g_Settings_ButtonRestartNewDir->hide();
		g_Settings_ButtonNext->activate();
	}
}

void cb_Settings_ButtonBrowseInstallDirectory(Fl_Widget *self) {
	//note: modal
	const char *chosenPath = fl_dir_chooser("Choose where to install TheDarkMod", NULL);
	if (chosenPath) {
		g_Settings_InputInstallDirectory->value(chosenPath);
		g_Settings_InputInstallDirectory->do_callback();
	}
}

void cb_Settings_ButtonRestartNewDir(Fl_Widget *self) {
	try {
		Actions::RestartWithInstallDir(g_Settings_InputInstallDirectory->value());
		//note: this line is never executed
	} catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
	}
}

void cb_Settings_CheckAdvancedSettings(Fl_Widget *self) {
	if (g_Settings_CheckAdvancedSettings->value()) {
		g_Settings_CheckSkipSelfUpdate->activate();
		g_Settings_CheckSkipMirrorsUpdate->activate();
		g_Settings_CheckForceScan->activate();
	}
	else {
		g_Settings_CheckSkipSelfUpdate->deactivate();
		g_Settings_CheckSkipMirrorsUpdate->deactivate();
		g_Settings_CheckForceScan->deactivate();
		g_Settings_CheckSkipSelfUpdate->value(false);
		g_Settings_CheckSkipMirrorsUpdate->value(false);
		g_Settings_CheckForceScan->value(false);
	}
}

void cb_Settings_ButtonNext(Fl_Widget *self) {
	try {
		Actions::StartLogFile();
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		return;
	}

	try {
		bool skipUpdate = g_Settings_CheckSkipMirrorsUpdate->value();
		Actions::ReadConfigFile(!skipUpdate);
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		return;
	}

	try {
		g_Settings_ProgressScanning->show();
		ProgressIndicatorGui progress(g_Settings_ProgressScanning);
		Actions::ScanInstallDirectoryIfNecessary(g_Settings_CheckForceScan->value(), &progress);
		g_Settings_ProgressScanning->hide();
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		g_Settings_ProgressScanning->hide();
		return;
	}

	//update versions tree on page "Version"
	g_Version_TreeVersions->clear();
	std::vector<std::string> allVersions = g_config->GetAllVersions();
	std::string defaultVersion = g_config->GetDefaultVersion();
	for (const std::string &version : allVersions) {
		std::vector<std::string> guiPath = g_config->GetFolderPath(version);
		guiPath.push_back(version);
		std::string wholePath = stdext::join(guiPath, "/");
		Fl_Tree_Item *item = g_Version_TreeVersions->add(wholePath.c_str());
		if (defaultVersion == version)
			g_Version_TreeVersions->select(item);
	}

	bool customVersion = g_Settings_CheckCustomVersion->value();
	if (customVersion) {
		g_Wizard->value(g_PageVersion);
	}
	else {
		//TODO: select default version
		g_Wizard->value(g_PageConfirm);
	}
}
