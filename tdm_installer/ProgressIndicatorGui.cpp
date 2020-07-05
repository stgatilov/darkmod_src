#include "ProgressIndicatorGui.h"
#include "FL/Fl.H"
#include "FL/Fl_Progress.H"
#include "FL/Fl_Group.H"
#include "FL/Fl_Output.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Text_Display.H"
#include "LogUtils.h"


ProgressIndicatorGui::ProgressIndicatorGui(Fl_Progress *widget) : _widget(widget) {
	_widget->value(0.0);
	_widget->label("starting...");
}

void ProgressIndicatorGui::Update(const char *line) {
	_widget->label(line);
	Fl::check();
}

void ProgressIndicatorGui::Update(double globalRatio, std::string globalComment, double localRatio, std::string localComment) {
	_widget->value(100.0 * globalRatio);
	_widget->label(globalComment.c_str());
	if (_lastComment != globalComment) {
		g_logger->infof("PROGRESS %0.2lf%% : %s", globalRatio * 100.0, globalComment.c_str());
		_lastComment = globalComment;
	}
	Fl::check();
}


GuiDeactivateGuard::~GuiDeactivateGuard() {
	Rollback();
}
GuiDeactivateGuard::GuiDeactivateGuard(Fl_Widget *blockedPage, std::initializer_list<Fl_Widget*> exceptThese) {
	Fl_Group *group = blockedPage->as_group();
	ZipSyncAssert(group);

	std::vector<Fl_Widget*> affectedWidgets;
	int num = group->children();
	for (int i = 0; i < num; i++) {
		Fl_Widget *widget = group->child(i);
		if (std::find(exceptThese.begin(), exceptThese.end(), widget) != exceptThese.end())
			continue;
		if (dynamic_cast<Fl_Output*>(widget) ||
			dynamic_cast<Fl_Text_Display*>(widget) ||
			dynamic_cast<Fl_Box*>(widget) ||
			dynamic_cast<Fl_Progress*>(widget) ||
		0) {
			continue;
		}
		affectedWidgets.push_back(widget);
	}

	for (int i = 0; i < affectedWidgets.size(); i++) {
		Fl_Widget *widget = affectedWidgets[i];
		_widgetToOldActive[widget] = widget->active();
		widget->deactivate();
	}
}
void GuiDeactivateGuard::Rollback() {
	for (const auto &pWA : _widgetToOldActive) {
		Fl_Widget *widget = pWA.first;
		bool oldActive = pWA.second;
		if (oldActive)
			widget->activate();
		else
			widget->deactivate();
	}
	_widgetToOldActive.clear();
}
