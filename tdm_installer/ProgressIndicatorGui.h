#pragma once

#include "CommandLine.h"
#include <functional>

class Fl_Progress;

class ProgressIndicatorGui : public ZipSync::ProgressIndicator {
	Fl_Progress *_widget;
public:
	ProgressIndicatorGui(Fl_Progress *widget);
    void Update(const char *line) override;
    void Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") override;
};
