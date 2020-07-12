#include "GuiPageSettings.h"
#include "GuiFluidAutoGen.h"
#include <FL/Fl_File_Chooser.H>
#include "StdFilesystem.h"
#include "StdString.h"
#include "OsUtils.h"
#include "Actions.h"
#include "State.h"
#include "ProgressIndicatorGui.h"


void cb_Settings_InputInstallDirectory(Fl_Widget *self) {
	std::string installDir = g_Settings_InputInstallDirectory->value();
	bool invalid = false;

	//normalize current path
	std::string normalizedDir = installDir;
	if (normalizedDir.find("..") != -1) {
		try {
			normalizedDir = stdext::canonical(normalizedDir).string();	//collapse parents
		} catch(stdext::filesystem_error &) {
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
	std::string installDir = g_Settings_InputInstallDirectory->value();

	try {
		auto warnings = Actions::CheckSpaceAndPermissions(installDir);
		for (const std::string &message : warnings) {
			int idx = fl_choice(message.c_str(), "I know what I'm doing", "Stop", nullptr);
			if (idx == 1)
				return;
		}
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		return;
	}

	try {
		Actions::RestartWithInstallDir(installDir);
		//note: this line is never executed
	} catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
	}
}

void cb_Settings_CheckAdvancedSettings(Fl_Widget *self) {
	if (g_Settings_CheckAdvancedSettings->value()) {
		g_Settings_CheckSkipSelfUpdate->activate();
		g_Settings_CheckSkipConfigDownload->activate();
		g_Settings_CheckForceScan->activate();
		g_Settings_CheckBitwiseExact->activate();
	}
	else {
		g_Settings_CheckSkipSelfUpdate->deactivate();
		g_Settings_CheckSkipConfigDownload->deactivate();
		g_Settings_CheckForceScan->deactivate();
		g_Settings_CheckBitwiseExact->deactivate();
		g_Settings_CheckSkipSelfUpdate->value(false);
		g_Settings_CheckSkipConfigDownload->value(false);
		g_Settings_CheckForceScan->value(false);
		g_Settings_CheckBitwiseExact->value(false);
	}
}

void cb_Settings_ButtonNext(Fl_Widget *self) {
	try {
		auto warnings = Actions::CheckSpaceAndPermissions(OsUtils::GetCwd());
		for (const std::string &message : warnings) {
			int idx = fl_choice(message.c_str(), "I know what I'm doing", "Stop", nullptr);
			if (idx == 1)
				return;
		}
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		return;
	}

	try {
		Actions::StartLogFile();
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		return;
	}

	try {
		bool skipUpdate = g_Settings_CheckSkipSelfUpdate->value();
		GuiDeactivateGuard deactivator(g_PageSettings, {});
		g_Settings_ProgressScanning->show();
		ProgressIndicatorGui progress(g_Settings_ProgressScanning);
		if (!skipUpdate) {
			if (Actions::NeedsSelfUpdate(&progress)) {
				fl_alert("New version of installer has been downloaded. Installer will be restarted to finish update.");
				Actions::DoSelfUpdate();
			}
		}
		g_Settings_ProgressScanning->hide();
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		g_Settings_ProgressScanning->hide();
		return;
	}

	try {
		bool skipUpdate = g_Settings_CheckSkipConfigDownload->value();
		GuiDeactivateGuard deactivator(g_PageSettings, {});
		g_Settings_ProgressScanning->show();
		ProgressIndicatorGui progress(g_Settings_ProgressScanning);
		Actions::ReadConfigFile(!skipUpdate, &progress);
		g_Settings_ProgressScanning->hide();
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		g_Settings_ProgressScanning->hide();
		return;
	}

	try {
		GuiDeactivateGuard deactivator(g_PageSettings, {});
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
	g_Version_TreeVersions->sortorder(FL_TREE_SORT_DESCENDING);
	std::vector<std::string> allVersions = g_state->_config.GetAllVersions();
	std::string defaultVersion = g_state->_config.GetDefaultVersion();
	for (const std::string &version : allVersions) {
		std::vector<std::string> guiPath = g_state->_config.GetFolderPath(version);
		guiPath.push_back(version);
		std::string wholePath = stdext::join(guiPath, "/");
		Fl_Tree_Item *item = g_Version_TreeVersions->add(wholePath.c_str());
		if (defaultVersion == version) {
			g_Version_TreeVersions->select(item);
		}
	}
	g_Version_OutputLastInstalledVersion->value(g_state->_lastInstalledVersion.c_str());

	bool customVersion = g_Settings_CheckCustomVersion->value();
	g_state->_versionRefreshed.clear();
	g_Version_TreeVersions->do_callback();
	g_Wizard->next();

	if (!customVersion) {
		//user wants default version, so auto-click "Next" button for him
		g_Version_ButtonNext->do_callback();
	}
}
