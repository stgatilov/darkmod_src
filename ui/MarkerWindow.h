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
#ifndef __MARKERWINDOW_H
#define __MARKERWINDOW_H

class idUserInterfaceLocal;

typedef struct {
	int time;
	const idMaterial *mat;
	idRectangle rect;
} markerData_t;

class idMarkerWindow : public idWindow {
public:
	idMarkerWindow(idUserInterfaceLocal *gui);
	idMarkerWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	virtual ~idMarkerWindow();
	virtual size_t Allocated(){return idWindow::Allocated();};
	virtual idWinVar *GetWinVarByName(const char *_name, bool winLookup = false);

	virtual const char *HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void PostParse();
	virtual void Draw(int time, float x, float y);
	virtual const char *RouteMouseCoords(float xd, float yd);
	virtual void		Activate(bool activate, idStr &act);
	virtual void MouseExit();
	virtual void MouseEnter();

	
private:
	virtual bool ParseInternalVar(const char *name, idParser *src);
	void CommonInit();
	void Line(int x1, int y1, int x2, int y2, dword* out, dword color);
	void Point(int x, int y, dword *out, dword color);
	logStats_t loggedStats[MAX_LOGGED_STATS];
	idList<markerData_t> markerTimes;
	idStr statData;
	int numStats;
	dword *imageBuff;
	const idMaterial *markerMat;
	const idMaterial *markerStop;
	idVec4 markerColor;
	int currentMarker;
	int currentTime;
	int stopTime;
};

#endif // __MARKERWINDOW_H
