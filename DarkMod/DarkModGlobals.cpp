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
 * Revision 1.5  2004/11/18 22:48:34  sparhawk
 * Changed the default frob highlight to 100
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
 * DESCRIPTION: This file contains all global identifiers, variables and
 * structures. Please note that global variables should be kept to a minimum
 * and only what is really neccessary should go in here.
 *
 *****************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4996)

#include "DarkModGlobals.h"
#include "Misc.h"
#include "Profile.h"
#include "direct.h"

static char *LTString[LT_COUNT+1] = {
	"FRC",
	"ERR",
	"BEG",
	"END",
	"WAR",
	"INF",
	"DEB",
	"---"
};

static char *LCString[LT_COUNT+1] = {
	"FORCE",
	"SYSTEM",
	"FROBBING",
	"AI",
	"SOUND",
	"FUNCTION",
	"ENTITY",
	"(empty)"
};

CGlobal::CGlobal(void)
{
	char ProfilePath[1024];
	char cwd[1024];
	PROFILE_HANDLE *pfh = NULL;
	FILE *logfile = NULL;

	getcwd(cwd, sizeof(cwd)-1);
	memset(m_LogArray, 0, sizeof(m_LogArray));
	memset(m_ClassArray, 0, sizeof(m_ClassArray));

	m_LogArray[LT_FORCE] = true;			// This is always on
	m_LogArray[LT_ERROR] = false;
	m_LogArray[LT_BEGIN] = false;
	m_LogArray[LT_END] = false;
	m_LogArray[LT_DEBUG] = false;

	m_ClassArray[LC_FORCE] = true;			// This is always on
	m_ClassArray[LC_SYSTEM] = false;
	m_ClassArray[LC_FROBBING] = false;
	m_ClassArray[LC_AI] = false;
	m_ClassArray[LC_SOUND] = false;
	m_ClassArray[LC_FUNCTION] = false;

	m_DefaultFrobDistance = 100.0f;
	m_LogClass = LC_SYSTEM;
	m_LogType = LT_DEBUG;
	m_Filename = "undefined";
	m_Linenumber = 0;

	if((logfile = fopen("c:\\darkmod.log", "w+b")) != NULL)
		fprintf(logfile, "Initialzing: %s\r", cwd);

	m_LogFile = NULL;

#ifdef _WINDOWS_
	strcpy(ProfilePath, cwd);
	strcat(ProfilePath, "\\Darkmod\\darkmod.ini");
#else   // LINUX
	char *home = getenv("HOME");

	ProfilePath[0] = 0;
	if(home)
		 strcpy(ProfilePath, home);

	strcat(ProfilePath, "/.darkmod.ini");
#endif

	fprintf(logfile, "Trying to open %s\r", ProfilePath);
	if((pfh = OpenProfile(ProfilePath, TRUE, FALSE)) == NULL)
	{
		fprintf(logfile, "darkmod.ini not found at %s\r", ProfilePath);
#ifdef _WINDOWS_
	strcpy(ProfilePath, cwd);
	strcat(ProfilePath, "\\darkmod.ini");
#endif
		fprintf(logfile, "retrying at %s\r", ProfilePath);
		pfh = OpenProfile(ProfilePath, TRUE, FALSE);
	}

	if(pfh == NULL)
		fprintf(logfile, "Unable to open darkmod.ini\r");
	else
		LoadINISettings(pfh);

	CloseProfile(pfh);

	if(logfile)
		fclose(logfile);
}

CGlobal::~CGlobal(void)
{
	if(m_LogFile != NULL)
		fclose(m_LogFile);
}

void CGlobal::LogString(char *fmt, ...)
{
	if(m_LogFile == NULL)
		return;

	LC_LogClass lc = m_LogClass;
	LT_LogType lt = m_LogType;

	if(m_ClassArray[lc] == false)
		return;

	if(m_LogArray[lt] == false)
		return;

	va_list arg;
	va_start(arg, fmt);

	fprintf(m_LogFile, "[%s:%s (%s) - %4u] ", m_Filename, LTString[lt], LCString[lc], m_Linenumber);
	vfprintf(m_LogFile, fmt, arg);
	fprintf(m_LogFile, "\n");
	fflush(m_LogFile);

	va_end(arg);
}

void CGlobal::LoadINISettings(void *p)
{
	PROFILE_HANDLE *pfh = (PROFILE_HANDLE *)p;
	PROFILE_SECTION *ps;
	PROFILE_MAP *pm;

	if(FindSection(pfh, "Debug", &ps) != -1)
	{
		if(FindMap(ps, "LogFile", TRUE, &pm) != -1)
		{
			struct tm *t;
			time_t timer;

			timer = time(NULL);
			t = localtime(&timer);

			if((m_LogFile = fopen(pm->Value, "w+b")) != NULL)
			{
				fprintf(m_LogFile, "LogFile created at %04u.%02u.%02u %02u:%02u:%02u\r",
							t->tm_year+1900, t->tm_mon, t->tm_mday, 
							t->tm_hour, t->tm_min, t->tm_sec);
				fprintf(m_LogFile, "DLL compiled on " __DATE__ " " __TIME__ "\r\r");
			}
		}

		if(FindMap(ps, "LogError", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_ERROR] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogError: %c\r", pm->Value[0]);
		}

		if(FindMap(ps, "LogBegin", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_BEGIN] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogBegin: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogEnd", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_END] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogEnd: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogDebug", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_DEBUG] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogDebug: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogWarning", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_WARNING] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogWarning: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogInfo", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_INFO] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogInfo: %c\r", pm->Value[0]);
		}

		if(FindMap(ps, "LogClass_SYSTEM", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_SYSTEM] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_SYSTEM: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_FROBBING", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_FROBBING] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_FROBBING: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_AI", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_AI] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_AI: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_SOUND", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_SOUND] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_SOUND: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_FUNCTION", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_FUNCTION] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_FUNCTION: %c\r", pm->Value[0]);
		}
	}

	if(FindSection(pfh, "GlobalParams", &ps) != -1)
	{
		if(FindMap(ps, "DefaultFrobDistance", TRUE, &pm) != -1)
			m_DefaultFrobDistance = abs(atof(pm->Value));
	}

	DM_LOG(LC_SYSTEM, LT_INFO).LogString("FrobDistance: %f\r", m_DefaultFrobDistance);
}
