#include "GuiGlobal.h"
#include "GuiFluidAutoGen.h"
#include "OsUtils.h"
#include "State.h"
#include "Constants.h"
#include "GuiPageSettings.h"
#include "GuiPageVersion.h"
#include "GuiPageConfirm.h"
#include "GuiPageInstall.h"
#include "ProgressIndicatorGui.h"
#include <FL/fl_ask.H>


void cb_Settings_ButtonReset(Fl_Widget *self) {
	g_state->Reset();
	g_Settings_InputInstallDirectory->value(OsUtils::GetCwd().c_str());
	g_Settings_InputInstallDirectory->do_callback();
	g_Settings_CheckCustomVersion->value(false);
	g_Settings_CheckAdvancedSettings->value(false);
	g_Settings_CheckAdvancedSettings->do_callback();

	g_Version_InputCustomManifestUrl->value("");
	g_Version_TreeVersions->do_callback();
	g_Version_OutputCurrentSize->value("");
	g_Version_OutputFinalSize->value("");
	g_Version_OutputAddedSize->value("");
	g_Version_OutputRemovedSize->value("");
	g_Version_OutputDownloadSize->value("");
}

void cb_RaiseInterruptFlag(Fl_Widget *self) {
	ProgressIndicatorGui::Interrupt();
}

//============================================================

static void GuiToInitialState() {
	{
		static char buff[256];
		sprintf(buff, "TheDarkMod installer v%s (built on %s)", TDM_INSTALLER_VERSION, __DATE__);
		g_Window->label(buff);
	}

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
	g_Install_OutputRemainDownload->hide();
	g_Install_ProgressRepack->hide();
	g_Install_ProgressFinalize->hide();

	g_Wizard->value(g_PageSettings);
}

static void GuiInstallCallbacks() {
	g_Settings_InputInstallDirectory->when(FL_WHEN_CHANGED);
	g_Settings_InputInstallDirectory->callback(cb_Settings_InputInstallDirectory);
	g_Settings_ButtonBrowseInstallDirectory->callback(cb_Settings_ButtonBrowseInstallDirectory);
	g_Settings_ButtonRestartNewDir->callback(cb_Settings_ButtonRestartNewDir);
	g_Settings_ButtonReset->callback(cb_Settings_ButtonReset);
	g_Settings_CheckAdvancedSettings->callback(cb_Settings_CheckAdvancedSettings);
	g_Settings_ButtonNext->callback(cb_Settings_ButtonNext);

	g_Settings_ButtonReset->do_callback();

	g_Version_TreeVersions->callback(cb_Version_TreeVersions);
	g_Version_InputCustomManifestUrl->when(FL_WHEN_CHANGED);
	g_Version_InputCustomManifestUrl->callback(cb_Version_InputCustomManifestUrl);
	g_Version_ButtonRefreshInfo->callback(cb_Version_ButtonRefreshInfo);
	g_Version_ButtonNext->callback(cb_Version_ButtonNext);

	g_Confirm_ButtonBack->callback(cb_Confirm_ButtonBack);
	g_Confirm_ButtonStart->callback(cb_Confirm_ButtonStart);

	g_Install_ButtonCancel->callback(cb_RaiseInterruptFlag);
	g_Install_ButtonClose->callback(cb_Install_ButtonClose);
}

static void SetStyleRecursive(Fl_Widget *widget) {
	static const Fl_Color ProgressFillColor = fl_rgb_color(0, 255, 0);
	static const Fl_Color ButtonNormalColor = fl_rgb_color(225);
	static const Fl_Color ButtonDownColor = fl_rgb_color(204, 228, 247);

	if (Fl_Group *w = dynamic_cast<Fl_Group*>(widget)) {
		int k = w->children();
		for (int i = 0; i < k; i++)
			SetStyleRecursive(w->child(i));
	}
	else if (Fl_Progress *w = dynamic_cast<Fl_Progress*>(widget)) {
		w->selection_color(ProgressFillColor);
	}
	else if (Fl_Check_Button *w = dynamic_cast<Fl_Check_Button*>(widget)) {
	}
	else if (Fl_Button *w = dynamic_cast<Fl_Button*>(widget)) {
		w->box(FL_BORDER_BOX);
		w->down_box(FL_BORDER_BOX);
		w->color(ButtonNormalColor);
		w->down_color(ButtonDownColor);
	}
}
static void GuiSetStyles() {
	Fl::set_color(FL_BACKGROUND_COLOR, fl_rgb_color(240));
	SetStyleRecursive(g_Window);
}

void GuiInitAll() {
	GuiToInitialState();
	GuiInstallCallbacks();
	GuiSetStyles();
}

void GuiLoaded(void*) {
	if (OsUtils::HasElevatedPrivilegesWindows()) {
		int idx = fl_choice("%s",
			"I know what I'm doing", "Exit", nullptr,
			"The installer was run \"as admin\". This is strongly discouraged!\n"
			"If you continue, admin rights will most likely be necessary to play the game."
		);
		if (idx == 1)
			exit(0);
	}
}
