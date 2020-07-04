#include "ProgressIndicatorGui.h"
#include "FL/Fl.H"
#include "FL/Fl_Progress.H"

ProgressIndicatorGui::ProgressIndicatorGui(Fl_Progress *widget) : _widget(widget) {}

void ProgressIndicatorGui::Update(const char *line) {
	_widget->label(line);
	Fl::flush();
}

void ProgressIndicatorGui::Update(double globalRatio, std::string globalComment, double localRatio, std::string localComment) {
	_widget->value(100.0 * globalRatio);
	_widget->label(globalComment.c_str());
	Fl::flush();
}
