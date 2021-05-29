/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#ifndef __CHOICEWINDOW_H
#define __CHOICEWINDOW_H

#include "Window.h"

class idUserInterfaceLocal;
class idChoiceWindow : public idWindow {
public:
						idChoiceWindow(idUserInterfaceLocal *gui);
						idChoiceWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	virtual				~idChoiceWindow();

	virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void 		PostParse();
	virtual void 		Draw(int time, float x, float y);
	virtual void		Activate( bool activate, idStr &act );
	virtual size_t		Allocated(){return idWindow::Allocated();}; 
  
	virtual idWinVar	*GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

	void				RunNamedEvent( const char* eventName );
	
private:
	virtual bool		ParseInternalVar(const char *name, idParser *src);
	void				CommonInit();
	void				UpdateChoice();
	void				ValidateChoice();
	
	void				InitVars();
						// true: read the updated cvar from cvar system, gui from dict
						// false: write to the cvar system, to the gui dict
						// force == true overrides liveUpdate 0
	void				UpdateVars( bool read, bool force = false );

	void				UpdateChoicesAndVals( void );
	
	int					currentChoice;
	int					choiceType;
	idStr				latchedChoices;
	idWinStr			choicesStr;
	idStr				latchedVals;
	idWinStr			choiceVals;
	idStrList			choices;
	idStrList			values;

	idWinStr			guiStr;
	idWinStr			cvarStr;
	idCVar *			cvar;
	idMultiWinVar		updateStr;

	idWinBool			liveUpdate;
	idWinStr			updateGroup;
};

#endif // __CHOICEWINDOW_H
