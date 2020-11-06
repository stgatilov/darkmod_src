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

	//set this to nonzero to generate interruption on next progress callback
	static int InterruptFlag;

public:
	~ProgressIndicatorGui();
	ProgressIndicatorGui(Fl_Progress *widget);

	void AttachRemainsLabel(Fl_Widget *label);
	int Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") override;

	static void Interrupt(int code = 1) { InterruptFlag = code; }
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
