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
#ifndef __GUISCRIPT_H
#define __GUISCRIPT_H

#include "Window.h"
#include "Winvar.h"

struct idGSWinVar {
	idGSWinVar() {
		var = NULL;
		own = false;
	}
	idWinVar* var;
	bool own;
};

class idGuiScriptList;

class idGuiScript {
	friend class idGuiScriptList;
	friend class idWindow;

public:
	idGuiScript();
	~idGuiScript();

	void SetSourceLocation(const char *filename, int linenum) {
		srcFilename = filename;	//pointer must live as long as owner window lives!
		srcLineNum = linenum;
	}
	bool Parse(idParser *src);
	void Execute(idWindow *win) {
		if (handler) {
			handler(win, &parms);
		}
	}
	void FixupParms(idWindow *win);
	size_t Size() {
		size_t sz = sizeof(*this);
		for (int i = 0; i < parms.Num(); i++) {
			sz += parms[i].var->Size();
		}
		return sz;
	}

	void WriteToSaveGame( idFile *savefile );
	void ReadFromSaveGame( idFile *savefile );

protected:
	int conditionReg;
	idGuiScriptList *ifList;
	idGuiScriptList *elseList;
	idList<idGSWinVar> parms;
	void (*handler) (idWindow *window, idList<idGSWinVar> *src);
	//stgatilov: error reporting and debuggability
	const char *srcFilename;	//points into owner's idWindow::sourceFilenamePool
	int srcLineNum;
};


class idGuiScriptList {
	idList<idGuiScript*> list;
public:
	idGuiScriptList() { list.SetGranularity( 4 ); };
	~idGuiScriptList() { list.DeleteContents(true); };
	void Execute(idWindow *win);
	void Append(idGuiScript* gs) {
		list.Append(gs);
	}
	size_t Size() {
		size_t sz = sizeof(*this);
		for (int i = 0; i < list.Num(); i++) {
			sz += list[i]->Size();
		}
		return sz;
	}
	void FixupParms(idWindow *win);
	void ReadFromDemoFile( class idDemoFile *f ) {};
	void WriteToDemoFile( class idDemoFile *f ) {};

	void WriteToSaveGame( idFile *savefile );
	void ReadFromSaveGame( idFile *savefile );
};

#endif // __GUISCRIPT_H
