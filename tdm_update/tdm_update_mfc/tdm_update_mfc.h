/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/


// tdm_update_mfc.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// UpdaterApplication:
// See tdm_update_mfc.cpp for the implementation of this class
//
class UpdaterApplication : public CWinAppEx
{
public:
	UpdaterApplication();

public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern UpdaterApplication theApp;