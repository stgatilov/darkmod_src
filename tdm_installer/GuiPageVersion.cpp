#include "GuiPageVersion.h"
#include "GuiFluidAutoGen.h"
#include "Actions.h"
#include "InstallerConfig.h"


void cb_Version_TreeVersions(Fl_Widget *self) {
	Fl_Tree_Reason reason = g_Version_TreeVersions->callback_reason();
	if (reason == FL_TREE_REASON_SELECTED || reason == FL_TREE_REASON_DESELECTED) {
		g_Version_OutputFinalSize->deactivate();
		g_Version_OutputAddedSize->deactivate();
		g_Version_OutputRemovedSize->deactivate();
		g_Version_OutputDownloadSize->deactivate();
	}

	Fl_Tree_Item *firstSel = g_Version_TreeVersions->first_selected_item();
	Fl_Tree_Item *lastSel = g_Version_TreeVersions->last_selected_item();
	//g_Version_TreeVersions->sele
	//bool oneSelected = 
}
