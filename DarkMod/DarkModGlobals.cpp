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
 * Revision 1.16  2005/03/29 07:38:42  ishtvan
 * Added declaration of global AI Relations object
 *
 * Revision 1.15  2005/03/26 20:59:52  sparhawk
 * Logging initialization added for automatic mod name detection.
 *
 * Revision 1.14  2005/03/21 22:57:36  sparhawk
 * Special plane and vectorlogs added.
 *
 * Revision 1.13  2005/02/07 21:28:11  sparhawk
 * Added MATH class and LogVector3 function.
 *
 * Revision 1.12  2005/01/28 22:56:52  sparhawk
 * WEAPON class added.
 *
 * Revision 1.11  2005/01/24 00:15:22  sparhawk
 * AmbientLight parameter added to material
 *
 * Revision 1.10  2005/01/20 19:36:00  sparhawk
 * CImage class implemented to load and store texture images.
 *
 * Revision 1.9  2005/01/07 02:01:10  sparhawk
 * Lightgem updates
 *
 * Revision 1.8  2004/12/04 22:50:45  sparhawk
 * Added LogClass LIGHT
 *
 * Revision 1.7  2004/11/22 23:51:34  sparhawk
 * Added MISC log class.
 *
 * Revision 1.6  2004/11/19 20:44:00  sparhawk
 * Added a trick to update compiletime automatically
 *
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

#ifdef _WINDOWS_
#include "c:\compiled.h"
#endif

#include "DarkModGlobals.h"
#include "PlayerData.h"
#include "Misc.h"
#include "Profile.h"
#include "direct.h"
#include "il/il.h"
#include "relations.h"

static char *LTString[LT_COUNT+1] = {
	"INI",
	"FRC",
	"ERR",
	"BEG",
	"END",
	"WAR",
	"INF",
	"DEB",
	"---"
};

static char *LCString[LC_COUNT+1] = {
	"INIT",
	"FORCE",
	"MISC",
	"SYSTEM",
	"FROBBING",
	"AI",
	"SOUND",
	"FUNCTION",
	"ENTITY",
	"INVENTORY",
	"LIGHT",
	"WEAPON",
	"MATH",
	"(empty)"
};

CRelations g_globalRelations;

CGlobal::CGlobal(void)
{
	m_DarkModPlayer = new CDarkModPlayer;

	memset(m_LogArray, 0, sizeof(m_LogArray));
	memset(m_ClassArray, 0, sizeof(m_ClassArray));

	memset(m_ModPath, 0, sizeof(m_ModPath));
	memset(m_ModName, 0, sizeof(m_ModName));

	m_LogArray[LT_INIT] = true;			// This is always on
	m_LogArray[LT_FORCE] = true;			// This is always on
	m_LogArray[LT_ERROR] = false;
	m_LogArray[LT_BEGIN] = false;
	m_LogArray[LT_END] = false;
	m_LogArray[LT_DEBUG] = false;

	m_ClassArray[LC_INIT] = true;
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

	m_LogFile = NULL;

	if((m_LogFile = fopen("c:\\d3modlogger.log", "w+b")) != NULL)
		DM_LOG(LC_INIT, LT_INIT).LogString("Initialzing mod logging\r");
}

CGlobal::~CGlobal(void)
{
	if(m_LogFile != NULL)
		fclose(m_LogFile);
}

void CGlobal::GetModName()
{
	int i, n;
	char PathSep, *p;
	const char *modpath = fileSystem->RelativePathToOSPath(".");
	char name[256];

	DM_LOG(LC_INIT, LT_INIT).LogString("Modpath: %08lx - [%s]\r", modpath, modpath);

#ifdef _WINDOWS_
		PathSep = '\\';
#else
		PathSep = '/';
#endif

	strcpy(m_ModPath, modpath);
	n = strlen(m_ModPath)-1;
	if(n <= 0)
		goto Quit;

	// First we cut of the path
	for(i = n; i >= 0; i--)
	{
		if(m_ModPath[i] == '.')
		{
			m_ModPath[i] = 0;
			continue;
		}

		if(m_ModPath[i] == PathSep)
		{
			m_ModPath[i] = 0;
			break;
		}
	}

	n = strlen(m_ModPath)-1;
	if(n <= 0)
		goto Quit;

	memset(name, 0, sizeof(name));
	p = name;
	for(i = n; i >= 0; i--)
	{
		if(m_ModPath[i] == PathSep)
			break;

		*p = m_ModPath[i];
		p++;
	}

	n = strlen(name)-1;
	memset(m_ModName, 0, sizeof(m_ModName));
	p = m_ModName;
	for(i = n; i >= 0; i--)
	{
		*p = name[i];
		p++;
	}

Quit:
	DM_LOG(LC_INIT, LT_INIT).LogString("Modpath: [%s]\r", m_ModPath);
	DM_LOG(LC_INIT, LT_INIT).LogString("Modname: [%s]\r", m_ModName);
	return;
}

void CGlobal::Init()
{
	char ProfilePath[1024];
	PROFILE_HANDLE *pfh = NULL;

	GetModName();

#ifdef _WINDOWS_
	strcpy(ProfilePath, m_ModPath);
	sprintf(ProfilePath, "%s\\%s.ini", m_ModPath, m_ModName);
//	strcat(ProfilePath, "\\Darkmod\\darkmod.ini");
#else   // LINUX
	char *home = getenv("HOME");

	ProfilePath[0] = 0;
	if(home)
		 strcpy(ProfilePath, home);

	strcat(ProfilePath, "/.darkmod.ini");
#endif

	DM_LOG(LC_INIT, LT_INIT).LogString("Trying to open %s\r", ProfilePath);
	if((pfh = OpenProfile(ProfilePath, TRUE, FALSE)) == NULL)
	{
		DM_LOG(LC_INIT, LT_INIT).LogString("%s.ini not found at %s\r", m_ModName, ProfilePath);
	}

	if(pfh != NULL)
		LoadINISettings(pfh);
	else
		DM_LOG(LC_INIT, LT_INIT).LogString("Unable to open %s.ini\r", m_ModName);

	CloseProfile(pfh);
}

void CGlobal::LogPlane(idStr const &Name, idPlane const &Plane)
{
	float a, b, c, d;

	Plane.GetPlaneParams(a, b, c, d);
	LogString("Plane %s:    a: %f   b: %f   c: %f   d: %f\r", Name.c_str(), a, b, c, d);
}

void CGlobal::LogVector(idStr const &Name, idVec3 const &Vector)
{
	LogString("Vector %s:    x: %f   y: %f   z: %f\r", Name.c_str(), Vector.x, Vector.y, Vector.z);
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
	FILE *logfile;

	DM_LOG(LC_INIT, LT_INIT).LogString("Loading INI settings\r");

	if(FindSection(pfh, "Debug", &ps) != -1)
	{
		if(FindMap(ps, "LogFile", TRUE, &pm) != -1)
		{
			struct tm *t;
			time_t timer;

			timer = time(NULL);
			t = localtime(&timer);

			if((logfile = fopen(pm->Value, "w+b")) != NULL)
			{
				DM_LOG(LC_INIT, LT_INIT).LogString("Switching logfile to [%s].\r", pm->Value);
				if(m_LogFile != NULL)
				{
					fclose(m_LogFile);
					m_LogFile = logfile;
				}

				DM_LOG(LC_INIT, LT_INIT).LogString("LogFile created at %04u.%02u.%02u %02u:%02u:%02u\r",
							t->tm_year+1900, t->tm_mon, t->tm_mday, 
							t->tm_hour, t->tm_min, t->tm_sec);
				DM_LOG(LC_INIT, LT_INIT).LogString("DLL compiled on " __DATE__ " " __TIME__ "\r\r");
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
		if(FindMap(ps, "LogClass_MISC", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_MISC] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_MISC: %c\r", pm->Value[0]);
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
		if(FindMap(ps, "LogClass_INVENTORY", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_INVENTORY] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_INVENTORY: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_LIGHT", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_LIGHT] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_LIGHT: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_WEAPON", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_WEAPON] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_WEAPON: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogClass_MATH", TRUE, &pm) != -1)
		{
			if(pm->Value[0] == '1')
				m_ClassArray[LC_MATH] = true;

			DM_LOG(LC_FORCE, LT_FORCE).LogString("LogClass_MATH: %c\r", pm->Value[0]);
		}
	}

	if(FindSection(pfh, "GlobalParams", &ps) != -1)
	{
		if(FindMap(ps, "DefaultFrobDistance", TRUE, &pm) != -1)
			m_DefaultFrobDistance = abs(atof(pm->Value));
	}

	DM_LOG(LC_SYSTEM, LT_INFO).LogString("FrobDistance: %f\r", m_DefaultFrobDistance);
}


CLightMaterial *CGlobal::GetMaterial(idStr const &mn)
{
	CLightMaterial *rc = NULL;
	int i, n;

	n = m_LightMaterial.Num();
	for(i = 0; i < n; i++)
	{
		if(m_LightMaterial[i]->m_MaterialName.Icmp(mn) == 0)
		{
			rc = m_LightMaterial[i];
			break;
		}
	}

	DM_LOG(LC_SYSTEM, LT_INFO).LogString("GetFallOffTexture returns: [%s] for [%s]\r", (rc == NULL) ? "(null)" : rc->m_MaterialName.c_str(), mn.c_str());
	return rc;
}


int CGlobal::AddImage(idStr const &Name, bool &Added)
{
	int rc = -1;
	CImage *im;
	Added = false;

	if(Name.Length() == 0)
		goto Quit;

	// If the image is already in the list, we have now the
	// index and can immediately return.
	if(GetImage(Name, rc) != NULL)
		goto Quit;

	im = new CImage(Name);

	m_Image.Append(im);
	rc = m_Image.Num()-1;
	Added = true;

Quit:
	return rc;
}

CImage *CGlobal::GetImage(int i)
{
	if(i > m_Image.Num())
		return NULL;
	else
		return m_Image[i];
}

CImage *CGlobal::GetImage(idStr const &Name, int &Index)
{
	int i, n;

	Index = -1;

	n = m_Image.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Image[i]->m_Name.Icmp(Name) == 0)
		{
			Index = i;
			return m_Image[i];
		}
	}

	return NULL;
}

CLightMaterial::CLightMaterial(idStr const &MaterialName, idStr const &TextureName, idStr const &MapName)
{
	bool added;

	m_MaterialName = MaterialName;
	m_FallOffTexture = TextureName;
	m_Map = MapName;
	m_AmbientLight = false;

	m_FallOffIndex = g_Global.AddImage(TextureName, added);
	m_MapIndex = g_Global.AddImage(MapName, added);
}

CLightMaterial::~CLightMaterial()
{
}

unsigned char *CLightMaterial::GetFallOffTexture(int &Width, int &Height, int &Bpp)
{
	unsigned char *rc = NULL;
	CImage *im;

	if(m_FallOffIndex != -1)
	{
		if((im = g_Global.GetImage(m_FallOffIndex)) != NULL)
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG).LogString("Falloff [%s]\r", im->m_Name.c_str());
			rc = im->GetImage();
			Width = im->m_Width;
			Height = im->m_Height;
			Bpp = im->m_Bpp;
		}
	}

	return(rc);
}

unsigned char *CLightMaterial::GetImage(int &Width, int &Height, int &Bpp)
{
	unsigned char *rc = NULL;
	CImage *im;

	if(m_MapIndex != -1)
	{
		if((im = g_Global.GetImage(m_MapIndex)) != NULL)
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG).LogString("Image [%s]\r", im->m_Name.c_str());
			rc = im->GetImage();
			Width = im->m_Width;
			Height = im->m_Height;
			Bpp = im->m_Bpp;
		}
	}

	return(rc);
}

CImage::CImage(idStr const &Name)
{
	m_Name = Name;
	m_Image = NULL;
	m_BufferLength = 0L;
	m_ImageId = -1;
	m_Width = 0;
	m_Height = 0;
	m_Loaded = false;
	m_Bpp = 0;
}

CImage::~CImage(void)
{
	if(m_ImageId != -1)
		ilDeleteImages(1, &m_ImageId);

	if(m_Image != NULL)
		delete [] m_Image;
}

unsigned char *CImage::GetImage(void)
{
	unsigned char *rc = NULL;
	idFile *fl = NULL;

	if(m_Loaded == false)
	{
		DM_LOG(LC_SYSTEM, LT_INFO).LogString("Loading Image [%s]\r", m_Name.c_str());

		if((fl = fileSystem->OpenFileRead(m_Name)) == NULL)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR).LogString("Unable to load LightFallOffImage [%s]\r", m_Name.c_str());
			goto Quit;
		}

		m_BufferLength = fl->Length();
		if((m_Image = new unsigned char[m_BufferLength]) == NULL)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR).LogString("Out of memory while allocating %lu bytes for [%s]\r", m_BufferLength, m_Name.c_str());
			goto Quit;
		}

		fl->Read(m_Image, m_BufferLength);
		ilGenImages(1, &m_ImageId);
		ilBindImage(m_ImageId);

		if(ilLoadL(IL_TYPE_UNKNOWN, m_Image, m_BufferLength) == IL_FALSE)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR).LogString("Error while loading image [%s]\r", m_Name.c_str());
			goto Quit;
		}

		m_Width = ilGetInteger(IL_IMAGE_WIDTH);
		m_Height = ilGetInteger(IL_IMAGE_HEIGHT);
		m_Bpp = ilGetInteger(IL_IMAGE_BPP);
		DM_LOG(LC_SYSTEM, LT_INFO).LogString("ImageWidth: %u   ImageHeight: %u   ImageDepth: %u   BPP: %u   Buffer: %u\r", m_Width, m_Height, ilGetInteger(IL_IMAGE_DEPTH), m_Bpp, m_BufferLength);
	}

	if(m_Image != NULL)
	{
		ilBindImage(m_ImageId);
		ilLoadL(IL_TYPE_UNKNOWN, m_Image, m_BufferLength);
		rc = (unsigned char *)ilGetData();
		m_Loaded = true;
	}

Quit:
	if(m_Loaded == false && m_Image != NULL)
	{
		delete [] m_Image;
		m_Image = NULL;
	}

	if(fl)
		fileSystem->CloseFile(fl);

	return rc;
}

