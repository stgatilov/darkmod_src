#include "GuiPageInstall.h"
#include "GuiFluidAutoGen.h"
#include "LogUtils.h"
#include "Actions.h"
#include "OsUtils.h"
#include "ProgressIndicatorGui.h"
#include "GuiUtils.h"


static void Install_UpdateAdditional() {
	if (Actions::CanRestoreOldConfig())
		g_Install_ButtonRestoreCfg->activate();
	else
		g_Install_ButtonRestoreCfg->deactivate();
	if (Actions::IfShortcutExists())
		g_Install_ButtonCreateShortcut->label("Recreate shortcut");
	else
		g_Install_ButtonCreateShortcut->label("Create shortcut");
}

void Install_MetaPerformInstall() {
	g_Install_ProgressDownload->hide();
	g_Install_ProgressVerify->hide();
	g_Install_OutputRemainDownload->hide();
	g_Install_ProgressRepack->hide();
	g_Install_ProgressFinalize->hide();

	g_Install_TextFinishedInstall->hide();
	g_Install_TextAdditional->hide();
	g_Install_ButtonRestoreCfg->hide();
	g_Install_ButtonCreateShortcut->hide();

	g_Wizard->next();
	g_Install_ButtonClose->deactivate();
	g_Install_ButtonCancel->activate();

	try {
		GuiDeactivateGuard deactivator(g_PageInstall, {g_Install_ButtonCancel});
		g_Install_ProgressDownload->show();
		Fl::flush();
		ProgressIndicatorGui progress1(g_Install_ProgressDownload);
		ProgressIndicatorGui progress2(g_Install_ProgressVerify);
		progress1.AttachRemainsLabel(g_Install_OutputRemainDownload);
		Actions::PerformInstallDownload(&progress1, &progress2);
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		ZipSync::DoClean(OsUtils::GetCwd());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	try {
		GuiDeactivateGuard deactivator(g_PageInstall, {});
		ProgressIndicatorGui progress(g_Install_ProgressRepack);
		Actions::PerformInstallRepack(&progress);
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		ZipSync::DoClean(OsUtils::GetCwd());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	try {
		GuiDeactivateGuard deactivator(g_PageInstall, {});
		ProgressIndicatorGui progress(g_Install_ProgressFinalize);
		Actions::PerformInstallFinalize(&progress);
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
		ZipSync::DoClean(OsUtils::GetCwd());
		g_Wizard->value(g_PageConfirm);
		return;
	}

	g_Install_TextInstalling->hide();
	g_Install_TextFinishedInstall->show();
	g_Install_TextAdditional->show();
	g_Install_ButtonRestoreCfg->show();
	g_Install_ButtonCreateShortcut->show();

	g_Install_ButtonClose->activate();
	g_Install_ButtonCancel->deactivate();

	Install_UpdateAdditional();
}

void cb_Install_ButtonClose(Fl_Widget *self) {
	g_logger->infof("Closing installer after successful install");
	exit(0);
}

void cb_Install_ButtonRestoreCfg(Fl_Widget *self) {
	try {
		Actions::DoRestoreOldConfig();
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
	}
	Install_UpdateAdditional();
}

void cb_Install_ButtonCreateShortcut(Fl_Widget *self) {
	try {
		Actions::CreateShortcut();
		g_Install_ButtonCreateShortcut->deactivate();
	}
	catch(std::exception &e) {
		GuiMessageBox(mbfError, e.what());
	}
	Install_UpdateAdditional();
}
