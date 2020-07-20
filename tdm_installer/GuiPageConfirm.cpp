#include "GuiPageConfirm.h"
#include "GuiFluidAutoGen.h"
#include "GuiPageInstall.h"


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
	Install_MetaPerformInstall();
}
