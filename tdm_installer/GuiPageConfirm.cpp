#include "GuiPageConfirm.h"
#include "GuiFluidAutoGen.h"
#include "FL/fl_ask.H"
#include "Actions.h"
#include "ProgressIndicatorGui.h"


void cb_Confirm_ButtonBack(Fl_Widget *self) {
	bool customVersion = g_Settings_CheckCustomVersion->value();
	if (customVersion) {
		g_Wizard->value(g_PageVersion);
	}
	else {
		g_Wizard->value(g_PageSettings);
	}
}

void cb_Confirm_ButtonStart(Fl_Widget *self) {
	g_Install_ProgressDownload->hide();
	g_Install_ProgressRepack->hide();
	g_Install_ProgressFinalize->hide();
	g_Wizard->next();
	g_Install_ButtonClose->deactivate();
	g_Install_ButtonCancel->activate();
	Fl::flush();

	g_Install_ProgressDownload->show();
	Fl::flush();
	try {
		ProgressIndicatorGui progress(g_Install_ProgressDownload);
		Actions::PerformInstallDownload(&progress);
	}
	catch(std::exception &e) {
		fl_alert("Error: %s", e.what());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	g_Install_ProgressRepack->show();
	Fl::flush();
	try {
		ProgressIndicatorGui progress(g_Install_ProgressRepack);
		Actions::PerformInstallRepack(&progress);
	}
	catch(std::exception &e) {
		fl_alert("Error: %s", e.what());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	//TODO: finalization?

	g_Install_ButtonClose->activate();
	g_Install_ButtonCancel->deactivate();
}
