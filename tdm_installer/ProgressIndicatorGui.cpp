#include "ProgressIndicatorGui.h"
#include "FL/Fl.H"
#include "FL/Fl_Progress.H"
#include "FL/Fl_Group.H"
#include "FL/Fl_Output.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Text_Display.H"
#include "LogUtils.h"
#include "time.h"


ProgressIndicatorGui::ProgressIndicatorGui(Fl_Progress *widget) : _progressWidget(widget) {
	_progressWidget->value(0.0);
	_progressWidget->label("starting...");
}

void ProgressIndicatorGui::AttachRemainsLabel(Fl_Widget *label) {
	_labelWidget = label;
}

void ProgressIndicatorGui::Update(const char *line) {
	//TODO: is it ever called?...
	_lastProgressText = line;
	_progressWidget->label(_lastProgressText.c_str());
}

void ProgressIndicatorGui::Update(double globalRatio, std::string globalComment, double localRatio, std::string localComment) {
	_progressWidget->value(100.0 * globalRatio);

	if (_lastProgressText != globalComment) {
		g_logger->infof("PROGRESS %0.2lf%% : %s", globalRatio * 100.0, globalComment.c_str());
		_lastProgressText = globalComment;
		_progressWidget->label(_lastProgressText.c_str());
	}

	if (_labelWidget) {
		double nowTime = clock();
		if (_startTime == DBL_MAX)
			_startTime = _lastUpdateTime = nowTime;
		if (nowTime - _lastUpdateTime > 3.0 * CLOCKS_PER_SEC) {	//refresh once in 3 seconds
			_lastUpdateTime = nowTime;
			double elapsedSeconds = (nowTime - _startTime) / CLOCKS_PER_SEC;
			if (globalRatio > 1e-3 && elapsedSeconds > 10.0) {
				_labelWidget->show();
				unsigned remainSeconds = unsigned((1.0 - globalRatio) / globalRatio * elapsedSeconds);
				char buff[256];
				if (remainSeconds < 60)
					sprintf(buff, "Remaining ~ %u s", remainSeconds);
				else if (remainSeconds < 60 * 60)
					sprintf(buff, "Remaining ~ %u:%02u", remainSeconds/60, remainSeconds%60);
				else
					sprintf(buff, "Remaining ~ %u:%02u:%02u", remainSeconds/3600, remainSeconds%3600/60, remainSeconds%60);
				_lastLabelText = buff;
				_labelWidget->label(_lastLabelText.c_str());
			}
		}
		if (globalRatio == 1.0)
			_labelWidget->hide();
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
