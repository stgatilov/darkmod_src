#pragma once

//reset all GUI widgets to initial state
void GuiInitAll();

//reset GUI widgets to initial state (Help window)
void GuiInitHelp();

//shows some warnings/alerts immediately on start
void GuiLoaded(void*);

//go through installation process in unattended mode (no human interaction)
void GuiUnattended(int argc, char **argv);
