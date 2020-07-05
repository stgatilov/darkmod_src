#include "GuiPageInstall.h"
#include "GuiFluidAutoGen.h"
#include "LogUtils.h"


void cb_Install_ButtonClose(Fl_Widget *self) {
	g_logger->infof("Closing installer after successfull install");
	exit(0);
}
