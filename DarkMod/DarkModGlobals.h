/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2004            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.6  2004/11/22 23:51:34  sparhawk
 * Added MISC log class.
 *
 * Revision 1.5  2004/11/16 23:51:55  sparhawk
 * Fixed a bug that prevented the proper logclass being shown in the log.
 *
 * Revision 1.4  2004/11/06 17:16:53  sparhawk
 * Optimized the debug log function for ease of use and speed.
 *
 * Revision 1.3  2004/11/05 21:23:01  sparhawk
 * Added ENTITY class
 * Added compile time info to logfile header to check DLL version on client installation.
 *
 * Revision 1.2  2004/11/03 21:47:17  sparhawk
 * Changed debug LogString for better performance and group settings
 *
 * Revision 1.1  2004/10/30 17:06:36  sparhawk
 * DarkMod added to project.
 *
 *
 *
 * DESCRIPTION: This file contains all global identifiers, variables and
 * structures. Please note that global variables should be kept to a minimum
 * and only what is really neccessary should go in here.
 *
 *****************************************************************************/

#ifndef DARKMODGLOBALS_H
#define DARKMODGLOBALS_H

#include <stdio.h>

typedef enum {
	LT_FORCE,			// Never use this
	LT_ERROR,			// Errormessage
	LT_BEGIN,			// Begin function
	LT_END,				// Leave function
	LT_WARNING,
	LT_INFO,
	LT_DEBUG,
	LT_COUNT
} LT_LogType;

// The log class determines a class or group of
// actions belonging together. This does not neccessarily mean 
// that this class is a C++ type class. For example. We have a class
// Frobbing which contains all loginfo concerning frobbing an item
// independent of it's class (like AI, item, doors, switche, etc.).
typedef enum {
	LC_FORCE,			// Never use this
	LC_MISC,
	LC_SYSTEM,			// Initialization, INI file and such stuff
	LC_FROBBING,		// Everything that has to do with frobbing
	LC_AI,				// same for AI
	LC_SOUND,			// same for sound
	LC_FUNCTION,		// general logging for functions (being, end, etc).
	LC_ENTITY,
	LC_INVENTORY,		// Everything that has to do with inventory
	LC_COUNT
} LC_LogClass;

class CGlobal {
public:
	CGlobal(void);
	~CGlobal(void);

	void LogString(char *Format, ...);

private:
	void LoadINISettings(void *);

public:
	/**
	 * LogFile is initialized to NULL if no Logfile is in use. Otherwise it
	 * contains a filepointer which can be used to write debugging information
	 * to the logfile. The logsettings are switched on in the INI file.
	 */
	FILE *m_LogFile;
	bool m_LogArray[LT_COUNT];
	bool m_ClassArray[LC_COUNT];

	LC_LogClass		m_LogClass;
	LT_LogType		m_LogType;
	char			*m_Filename;
	int				m_Linenumber;

public:
	// Global game settings, default values
	float m_DefaultFrobDistance;
};

extern CGlobal g_Global;
extern char *g_LCString[];

#define DM_LOG(lc, lt)		if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global

#endif