#pragma once

#include "CommandLine.h"
#include <functional>
#include <map>
#include <float.h>

class Fl_Progress;
class Fl_Widget;


class ProgressIndicatorGui : public ZipSync::ProgressIndicator {
	Fl_Progress *_progressWidget = nullptr;
	std::string _lastProgressText;

	Fl_Widget *_labelWidget = nullptr;
	std::string _lastLabelText;
	double _startTime = DBL_MAX;
	double _lastUpdateTime = DBL_MAX;

public:
	ProgressIndicatorGui(Fl_Progress *widget);
	void AttachRemainsLabel(Fl_Widget *label);
	void Update(const char *line) override;
	void Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") override;
};


class GuiDeactivateGuard {
	std::map<Fl_Widget*, bool> _widgetToOldActive;
public:
	~GuiDeactivateGuard();
	GuiDeactivateGuard(Fl_Widget *blockedPage, std::initializer_list<Fl_Widget*> exceptThese);
	void Rollback();

	GuiDeactivateGuard(const GuiDeactivateGuard&) = delete;
	GuiDeactivateGuard& operator= (const GuiDeactivateGuard&) = delete;
};
