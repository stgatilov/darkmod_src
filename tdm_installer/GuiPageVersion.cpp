#include "GuiPageVersion.h"
#include "GuiFluidAutoGen.h"
#include "Actions.h"
#include "State.h"
#include "LogUtils.h"
#include "FL/fl_ask.H"
#include "ProgressIndicatorGui.h"


void cb_Version_TreeVersions(Fl_Widget *self) {
	Fl_Tree_Reason reason = g_Version_TreeVersions->callback_reason();
	if (reason == FL_TREE_REASON_SELECTED || reason == FL_TREE_REASON_DESELECTED) {
		g_Version_OutputCurrentSize->deactivate();
		g_Version_OutputFinalSize->deactivate();
		g_Version_OutputAddedSize->deactivate();
		g_Version_OutputRemovedSize->deactivate();
		g_Version_OutputDownloadSize->deactivate();
	}

	Fl_Tree_Item *firstSel = g_Version_TreeVersions->first_selected_item();
	Fl_Tree_Item *lastSel = g_Version_TreeVersions->last_selected_item();
	bool oneSelected = (firstSel && firstSel == lastSel);
	bool isVersionSelected = (oneSelected && firstSel->children() == 0);
	if (isVersionSelected) {
		g_Version_ButtonNext->activate();
		g_Version_ButtonRefreshInfo->activate();
	}
	else {
		g_Version_ButtonNext->deactivate();
		g_Version_ButtonRefreshInfo->deactivate();
	}

	bool correctStats = false;
	if (isVersionSelected) {
		std::string selVersion = firstSel->label();
		if (selVersion == g_state->_versionRefreshed)
			correctStats = true;
	}
	if (correctStats) {
		g_Version_OutputCurrentSize->activate();
		g_Version_OutputFinalSize->activate();
		g_Version_OutputAddedSize->activate();
		g_Version_OutputRemovedSize->activate();
		g_Version_OutputDownloadSize->activate();
		g_Version_ButtonRefreshInfo->hide();
	}
	else {
		g_Version_OutputCurrentSize->deactivate();
		g_Version_OutputFinalSize->deactivate();
		g_Version_OutputAddedSize->deactivate();
		g_Version_OutputRemovedSize->deactivate();
		g_Version_OutputDownloadSize->deactivate();
		g_Version_ButtonRefreshInfo->show();
	}
}

void cb_Version_ButtonRefreshInfo(Fl_Widget *self) {
	Fl_Tree_Item *firstSel = g_Version_TreeVersions->first_selected_item();
	ZipSyncAssert(firstSel);	//never happens
	std::string version = firstSel->label();

	//find information for the new version
	Actions::VersionInfo info;
	try {
		GuiDeactivateGuard deactivator(g_PageVersion, {});
		g_Version_ProgressDownloadManifests->show();
		ProgressIndicatorGui progress(g_Version_ProgressDownloadManifests);
		info = Actions::RefreshVersionInfo(version, g_Settings_CheckBitwiseExact->value(), &progress);
		g_Version_ProgressDownloadManifests->hide();
	}
	catch(const std::exception &e) {
		fl_alert("Error: %s", e.what());
		g_Version_ProgressDownloadManifests->hide();
		return;
	}

	//update GUI items
	auto BytesToString = [](uint64_t bytes) -> std::string {
		return std::to_string((bytes + 999999) / 1000000) + " MB";
	};
	g_Version_OutputCurrentSize->value(BytesToString(info.currentSize).c_str());
	g_Version_OutputFinalSize->value(BytesToString(info.finalSize).c_str());
	g_Version_OutputAddedSize->value(BytesToString(info.addedSize).c_str());
	g_Version_OutputRemovedSize->value(BytesToString(info.removedSize).c_str());
	g_Version_OutputDownloadSize->value(BytesToString(info.downloadSize).c_str());

	//remember that we display info for this version
	g_state->_versionRefreshed = version;
	//will activate outputs and hide refresh button
	g_Version_TreeVersions->do_callback();
}

void cb_Version_ButtonNext(Fl_Widget *self) {
	//make sure selected version has been evaluated/refreshed
	if (g_Version_ButtonRefreshInfo->visible() && g_Version_ButtonRefreshInfo->active()) {
		g_Version_ButtonRefreshInfo->do_callback();
		if (g_Version_ButtonRefreshInfo->visible()) {
			//could not load manifest -> stop
			return;
		}
	}

	g_Confirm_OutputInstallDirectory->value(g_Settings_InputInstallDirectory->value());
	g_Confirm_OutputLastInstalledVersion->value(g_Version_OutputLastInstalledVersion->value());
	g_Confirm_OutputVersionToInstall->value(g_state->_versionRefreshed.c_str());

	g_Confirm_OutputCurrentSize->value(g_Version_OutputCurrentSize->value());
	g_Confirm_OutputFinalSize->value(g_Version_OutputFinalSize->value());
	g_Confirm_OutputAddedSize->value(g_Version_OutputAddedSize->value());
	g_Confirm_OutputRemovedSize->value(g_Version_OutputRemovedSize->value());
	g_Confirm_OutputDownloadSize->value(g_Version_OutputDownloadSize->value());

	g_Wizard->next();
}

void cb_Version_ButtonPrev(Fl_Widget *self) {
	bool customVersion = g_Settings_CheckCustomVersion->value();
	if (customVersion) {
		g_Wizard->value(g_PageVersion);
	}
	else {
		g_Wizard->value(g_PageSettings);
	}
}
