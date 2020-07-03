#include "GuiGlobal.h"
#include "GuiFluidAutoGen.h"
#include "OsUtils.h"
#include "GuiPageSettings.h"
#include "GuiPageVersion.h"

void cb_Settings_ButtonReset(Fl_Widget *self) {
	g_Settings_InputInstallDirectory->value(OsUtils::GetCwd().c_str());
	g_Settings_InputInstallDirectory->do_callback();
	g_Settings_CheckCustomVersion->value(false);
	g_Settings_CheckAdvancedSettings->value(false);
	g_Settings_CheckAdvancedSettings->do_callback();
	//TODO: reset all other pages too?
}

//============================================================

void GuiInitAll() {
	//----- "settings" page -----
	static Fl_Text_Buffer g_Settings_StringGreetings;
	g_Settings_StringGreetings.text(
		"This application will install TheDarkMod, or update existing installation \nto the most recent version available. "
		"An active internet connection will be required. \n"
		"Review the settings below, then click Next to start. "
	);
	g_Settings_TextGreetings->buffer(g_Settings_StringGreetings);
	g_Settings_ButtonRestartNewDir->hide();
	g_Settings_ProgressScanning->hide();

	//----- "version" page -----
	g_Version_TextCustomManifestMessage->hide();
	g_Version_ProgressDownloadManifests->hide();

	//----- "confirm" page -----
	static Fl_Text_Buffer g_Confirm_StringReadyToInstall;
	g_Confirm_StringReadyToInstall.text(
		"TheDarkMod is ready to install or update. \n"
		"Check the information below and click START. \n"
	);
	g_Confirm_TextReadyToInstall->buffer(g_Confirm_StringReadyToInstall);

	//----- "install" page -----
	static Fl_Text_Buffer g_Install_StringInstalling;
	g_Install_StringInstalling.text(
		"TheDarkMod is being installed right now. \n"
		"Please wait... \n"
	);
	g_Install_TextInstalling->buffer(g_Install_StringInstalling);
	static Fl_Text_Buffer g_Install_StringFinishedInstall;
	g_Install_StringFinishedInstall.text(
		"Installation finished successfully! \n"
		"Click Close to exit. \n"
	);
	g_Install_TextFinishedInstall->buffer(g_Install_StringFinishedInstall);
	g_Install_TextFinishedInstall->hide();
	g_Install_ProgressDownload->hide();
	g_Install_ProgressRepack->hide();
	g_Install_ProgressFinalize->hide();

	g_Wizard->value(g_PageSettings);

	//----- callbacks -----
	g_Settings_InputInstallDirectory->when(FL_WHEN_CHANGED);
	g_Settings_InputInstallDirectory->callback(cb_Settings_InputInstallDirectory);
	g_Settings_ButtonBrowseInstallDirectory->callback(cb_Settings_ButtonBrowseInstallDirectory);
	g_Settings_ButtonRestartNewDir->callback(cb_Settings_ButtonRestartNewDir);
	g_Settings_ButtonReset->callback(cb_Settings_ButtonReset);
	g_Settings_CheckAdvancedSettings->callback(cb_Settings_CheckAdvancedSettings);
	g_Settings_ButtonNext->callback(cb_Settings_ButtonNext);

	g_Settings_ButtonReset->do_callback();

	g_Version_TreeVersions->callback(cb_Version_TreeVersions);
}

void GuiUpdateAll(void*) {
}
