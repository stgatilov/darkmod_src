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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "DarkModGlobals.h"
#include "Misc.h"
#include "Profile.h"
#include "direct.h"

static char *LTString[LT_COUNT] = {
	"ERR",
	"BEG",
	"END",
	"WAR",
	"INF",
	"DEB"
};

CGlobal::CGlobal(void)
{
	char ProfilePath[1024];
	char cwd[1024];
	PROFILE_HANDLE *pfh = NULL;
	FILE *logfile = NULL;
	PROFILE_SECTION *ps;
	PROFILE_MAP *pm;

	getcwd(cwd, sizeof(cwd)-1);
	memset(m_LogArray, 0, sizeof(m_LogArray));
	m_LogArray[LT_ERROR] = true;
	m_LogArray[LT_BEGIN] = true;
	m_LogArray[LT_END] = true;
	m_LogArray[LT_DEBUG] = true;

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
	{
		if(FindSection(pfh, "Debug", &ps) != -1)
		{
			if(FindMap(ps, "LogFile", TRUE, &pm) != -1)
			{
				struct tm *t;
				time_t timer;

				timer = time(NULL);
				t = localtime(&timer);

				if((m_LogFile = fopen(pm->Value, "w+b")) != NULL)
					fprintf(m_LogFile, "LogFile created at %04u.%02u.%02u %02u:%02u:%02u\r",
								t->tm_year+1900, t->tm_mon, t->tm_mday, 
								t->tm_hour, t->tm_min, t->tm_sec);
			}
			else
				fprintf(logfile, "LogFile entry not found\r");
		}
		else
			fprintf(logfile, "Debugsection not found\r");
	}

	CloseProfile(pfh);

	if(logfile)
		fclose(logfile);
}

CGlobal::~CGlobal(void)
{
	if(m_LogFile != NULL)
		fclose(m_LogFile);
}

void CGlobal::LogString(char *fn, char *fkt, int ln, LT_LogType lt, char *fmt, ...)
{
	if(m_LogFile == NULL)
		return;

	if(m_LogArray[lt] == false)
		return;

	va_list arg;
	va_start(arg, fmt);

	fprintf(m_LogFile, "[%s:%s (%s) - %u] ", LTString[lt], fn, fkt, ln);
	vfprintf(m_LogFile, fmt, arg);
	fprintf(m_LogFile, "\n");
	fflush(m_LogFile);

	va_end(arg);
}

