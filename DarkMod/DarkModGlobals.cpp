/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2004            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4996 4800)

static bool init_version = FileVersionList("$Id$", init_version);

#ifdef _WINDOWS_
//#include "c:\compiled.h"
#endif

#include "DarkModGlobals.h"
#include "MissionData.h"
#include "Misc.h"
#include "Profile.h"
#include "sndPropLoader.h"
#include "sndProp.h"
#include "Relations.h"
#include "shop.h"
#include "DifficultyMenu.h"
#include "ModMenu.h"
#include "../game/ai/ai.h"
#include "sourcehook/sourcehook.h"
#include "sourcehook/sourcehook_impl.h"
#include "renderpipe.h"
#include "RevisionTracker.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class idAI;

VersionCheckResult CompareVersion(int major, int minor, int toMajor, int toMinor)
{
	// Equality check
	if (major == toMajor && minor == toMinor) return EQUAL;

	// Not equal, find the difference
	if (major < toMajor)
	{
		return OLDER; // major version is older
	}
	else if (major > toMajor)
	{
		return NEWER;
	}
	else // Major version equal, check minor versions
	{
		return (minor < toMinor) ? OLDER : NEWER;
	}
}

// Name of the logfile to use for Dark Mod logging
#if defined(__linux__)
const char* DARKMOD_LOGFILE = "/tmp/DarkMod.log";
#elif MACOS_X
const char* DARKMOD_LOGFILE = "~/Library/Logs/DarkMod.log";
#else // Windows
const char* DARKMOD_LOGFILE = "c:\\d3modlogger.log";
#endif

static const char *LTString[LT_COUNT+1] = {
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

static const char *LCString[LC_COUNT+1] = {
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
	"MOVEMENT",
	"LOCKPICK",
	"FRAME",
	"STIMRESP",
	"OBJECTIVES",
	"DIFFICULTY",
	"CONVERSATION",
	"MAINMENU",
	"(empty)"
};



SourceHook::CSourceHookImpl g_SourceHook;
SourceHook::ISourceHook *g_SHPtr = NULL;
int g_PLID = 0;
const char *DM_OSPathToRelativePath(const char *OSPath);
const char *DM_RelativePathToOSPath(const char *relativePath, const char *basePath = "fs_devpath");
const char *DM_BuildOSPath(const char *base, const char *game, const char *relativePath);
void DM_Frame();
//void DM_Printf(const char* fmt, ...);

// Intercept declarations
//SH_DECL_HOOK1(idFileSystem, OSPathToRelativePath, SH_NOATTRIB, 0, const char *, const char *);
//SH_DECL_HOOK2(idFileSystem, RelativePathToOSPath, SH_NOATTRIB, 0, const char *, const char *, const char *);
SH_DECL_HOOK3(idFileSystem, BuildOSPath, SH_NOATTRIB, 0, const char *, const char *, const char *, const char *);
SH_DECL_HOOK0_void(idCommon, Frame, SH_NOATTRIB, 0);

// greebo: Intercept declaration for idCommon::VPrintf 
//SH_DECL_HOOK0_void_vafmt(idCommon, Printf, SH_NOATTRIB, 0);

// declare various global objects
CsndPropLoader	g_SoundPropLoader;
CsndProp		g_SoundProp;

static idList<const char *> *s_FileVersion = NULL;

bool FileVersionList(const char *str, bool state)
{
	if (s_FileVersion == NULL)
	{
		s_FileVersion = new idList<const char *>;
	}

	if (state == false)
	{
		s_FileVersion->AddUnique(str);

		// greebo: Add the revision to the RevisionTracker class
		RevisionTracker::ParseSVNIdString(str);
	}

	return true;
}

void FileVersionDump(void)
{
	int i, n;

	n = s_FileVersion->Num();
	for(i = 0; i < n; i++)
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("%s\r", (*s_FileVersion)[i]);
}


CGlobal::CGlobal()
{
	memset(m_LogArray, 0, sizeof(m_LogArray));
	memset(m_ClassArray, 0, sizeof(m_ClassArray));

	// Initialise all logtypes to false
	for (int i = 0; i < LT_COUNT; ++i)
	{
		m_LogArray[i] = false;
	}

	// Except for these two
	m_LogArray[LT_INIT] = true;			// This is always on
	m_LogArray[LT_FORCE] = true;			// This is always on
	
	// Initialise all logging values to false
	for (int i = 0; i < LC_COUNT; ++i)
	{
		m_ClassArray[i] = false;
	}

	// Except for these two, these are always on
	m_ClassArray[LC_INIT] = true;
	m_ClassArray[LC_FORCE] = true;

	m_Frame = 0;
	m_MaxFrobDistance = 0;
	m_LogClass = LC_SYSTEM;
	m_LogType = LT_DEBUG;
	m_Filename = "undefined";
	m_Linenumber = 0;
	
	m_LogFile = fopen(DARKMOD_LOGFILE, "w+b");

	if (m_LogFile != NULL)
	{
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Initializing mod logging\r");
	}

	// initialize the AI Acuities hash

	/**
	* Define AI Acuities Here:
	* NOTE: If you add an acuity, your total number of acuities
	* must be below s_MAXACUITIES defined in AI.h, unless you
	* want to chagne that and recompile everything.
	**/
	m_AcuityNames.Append("vis");
	m_AcuityNames.Append("aud");
	m_AcuityNames.Append("tact");
	m_AcuityNames.Append("env");
	m_AcuityNames.Append("other");

	m_AcuityNames.Condense();

	for( int i=0; i < m_AcuityNames.Num(); i++ )
	{
		m_AcuityHash.Add( m_AcuityHash.GenerateKey( m_AcuityNames[i].c_str(), false ), i );
	}

	/* initialize Sourcehook required global */
	g_SHPtr = static_cast<SourceHook::ISourceHook*>(&g_SourceHook); 
}

CGlobal::~CGlobal()
{
	if(m_LogFile != NULL)
		fclose(m_LogFile);
}

void CGlobal::Init()
{
	PROFILE_HANDLE *pfh = NULL;

#ifdef _WINDOWS_

	SH_ADD_HOOK_STATICFUNC(idFileSystem, BuildOSPath, fileSystem, DM_BuildOSPath, 0);
//	SH_ADD_HOOK_STATICFUNC(idFileSystem, OSPathToRelativePath, fileSystem, DM_OSPathToRelativePath, 0);
//	SH_ADD_HOOK_STATICFUNC(idFileSystem, RelativePathToOSPath, fileSystem, DM_RelativePathToOSPath, 0);

//	SH_ADD_HOOK_STATICFUNC(idCommon, Printf, common, DM_Printf, 0);

#endif

	// Report the darkmod path for diagnostic purposes
	LogString("Darkmod path is %s\r", GetDarkmodPath().c_str());

#ifdef _WINDOWS_

	std::string iniPath = GetDarkmodPath();
	iniPath += "\\darkmod.ini";

#elif MACOS_X

	std::string iniPath = GetDarkmodPath();
	iniPath += "/darkmod.ini";

#else   // LINUX
	
	std::string iniPath = 
		std::string(getenv("HOME")) + "/.doom3/darkmod/darkmod.ini";
	
#endif

	const char* profilePath = iniPath.c_str();
	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Trying to open %s\r", profilePath);
	if((pfh = OpenProfile(profilePath, TRUE, FALSE)) == NULL)
	{
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("darkmod.ini not found at %s\r", profilePath);
	}

	if(pfh != NULL)
		LoadINISettings(pfh);
	else
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Unable to open darkmod.ini\r");

	CloseProfile(pfh);
	FileVersionDump();

	// Map the surface types to strings
	InitSurfaceHardness();
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

void CGlobal::LogMat3(idStr const &Name, idMat3 const &Mat)
{
	idVec3 a, b, c;

	Mat.GetMat3Params(a, b, c);
	LogString("Matrix %s:\r\t%f  %f  %f\r\t%f  %f  %f\r\t%f  %f  %f\r", Name.c_str(), 
		a.x, a.y, a.z,
		b.x, b.y, b.z,
		c.x, c.y, c.z
		);
}

void CGlobal::LogString(const char *fmt, ...)
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

	fprintf(m_LogFile, "[%s (%4u):%s (%s) FR: %4lu] ", m_Filename, m_Linenumber, LTString[lt], LCString[lc], m_Frame);
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

	DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Loading INI settings\r");

	// All logclasses are loaded from the INI file. Frame can be switched 
	// on explicitly. If any of the other classes are enabled, then Frame 
	// will also be enabled as a marker in the logfile.
	if(FindSection(pfh, "Debug", &ps) != static_cast<ULONG>(-1))
	{
		if(FindMap(ps, "LogFile", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if (idStr::Icmp(pm->Value, "") == 0)
			{
				DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Logging disabled by darkmod.ini, closing logfile.\r");

				// No logfile defined, quit logging
				fclose(m_LogFile);
				m_LogFile = NULL;
			}
			else
			{
				struct tm *t;
				time_t timer;

				timer = time(NULL);
				t = localtime(&timer);

				if((logfile = fopen(pm->Value, "w+b")) != NULL)
				{
					DM_LOG(LC_INIT, LT_INIT)LOGSTRING("Switching logfile to [%s].\r", pm->Value);
					if(m_LogFile != NULL)
					{
						fclose(m_LogFile);
						m_LogFile = logfile;
					}

					DM_LOG(LC_INIT, LT_INIT)LOGSTRING("LogFile created at %04u.%02u.%02u %02u:%02u:%02u\r",
								t->tm_year+1900, t->tm_mon, t->tm_mday, 
								t->tm_hour, t->tm_min, t->tm_sec);
					DM_LOG(LC_INIT, LT_INIT)LOGSTRING("DLL compiled on " __DATE__ " " __TIME__ "\r\r");
				}
			}
		}

		DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("Found Debug section \r");

		if(FindMap(ps, "LogError", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_ERROR] = true;

			DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("LogError: %c\r", pm->Value[0]);
		}

		if(FindMap(ps, "LogBegin", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_BEGIN] = true;

			DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("LogBegin: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogEnd", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_END] = true;

			DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("LogEnd: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogDebug", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_DEBUG] = true;

			DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("LogDebug: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogWarning", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_WARNING] = true;

			DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("LogWarning: %c\r", pm->Value[0]);
		}
		if(FindMap(ps, "LogInfo", TRUE, &pm) != static_cast<ULONG>(-1))
		{
			if(pm->Value[0] == '1')
				m_LogArray[LT_INFO] = true;

			DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("LogInfo: %c\r", pm->Value[0]);
		}

		CheckLogClass(ps, "LogClass_FRAME", LC_FRAME);
		CheckLogClass(ps, "LogClass_SYSTEM", LC_SYSTEM);
		CheckLogClass(ps, "LogClass_MISC", LC_MISC);
		CheckLogClass(ps, "LogClass_FROBBING", LC_FROBBING);
		CheckLogClass(ps, "LogClass_AI", LC_AI);
		CheckLogClass(ps, "LogClass_SOUND", LC_SOUND);
		CheckLogClass(ps, "LogClass_FUNCTION", LC_FUNCTION);
		CheckLogClass(ps, "LogClass_ENTITY", LC_ENTITY);
		CheckLogClass(ps, "LogClass_INVENTORY", LC_INVENTORY);
		CheckLogClass(ps, "LogClass_LIGHT", LC_LIGHT);
		CheckLogClass(ps, "LogClass_WEAPON", LC_WEAPON);
		CheckLogClass(ps, "LogClass_MATH", LC_MATH);
		CheckLogClass(ps, "LogClass_MOVEMENT", LC_MOVEMENT);
		CheckLogClass(ps, "LogClass_STIM_RESPONSE", LC_STIM_RESPONSE);
		CheckLogClass(ps, "LogClass_OBJECTIVES", LC_OBJECTIVES);
		CheckLogClass(ps, "LogClass_DIFFICULTY", LC_DIFFICULTY);
		CheckLogClass(ps, "LogClass_CONVERSATION", LC_CONVERSATION);
		CheckLogClass(ps, "LogClass_MAINMENU", LC_MAINMENU);
		CheckLogClass(ps, "LogClass_LOCKPICK", LC_LOCKPICK);
	}

	if(FindSection(pfh, "GlobalParams", &ps) != static_cast<ULONG>(-1))
	{
		DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("Found GlobalParams section \r");
	}
}

void CGlobal::CheckLogClass(PROFILE_SECTION* ps, const char* key, LC_LogClass logClass)
{
	PROFILE_MAP* pm = NULL;

	if (FindMap(ps, key, TRUE, &pm) != static_cast<ULONG>(-1))
	{
		if (pm->Value[0] == '1')
		{
			m_ClassArray[logClass] = true;
		}

		DM_LOG(LC_FORCE, LT_FORCE)LOGSTRING("%s: %c\r", key, pm->Value[0]);
	}
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

	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("GetFallOffTexture returns: [%s] for [%s]\r", (rc == NULL) ? "(null)" : rc->m_MaterialName.c_str(), mn.c_str());
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


void CLightMaterial::Save( idSaveGame *savefile ) const
{
	savefile->WriteString(m_MaterialName);
	savefile->WriteBool(m_AmbientLight);
	savefile->WriteString(m_FallOffTexture);
	savefile->WriteInt(m_FallOffIndex);
	savefile->WriteString(m_Map);
	savefile->WriteInt(m_MapIndex);
}

void CLightMaterial::Restore( idRestoreGame *savefile )
{
	savefile->ReadString(m_MaterialName);
	savefile->ReadBool(m_AmbientLight);
	savefile->ReadString(m_FallOffTexture);
	savefile->ReadInt(m_FallOffIndex);
	savefile->ReadString(m_Map);
	savefile->ReadInt(m_MapIndex);
}

unsigned char *CLightMaterial::GetFallOffTexture(int &Width, int &Height, int &Bpp)
{
	unsigned char *rc = NULL;
	CImage *im;

	if(m_FallOffIndex != -1)
	{
		if((im = g_Global.GetImage(m_FallOffIndex)) != NULL)
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Falloff [%s]\r", im->m_Name.c_str());
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
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Image [%s]\r", im->m_Name.c_str());
			rc = im->GetImage();
			Width = im->m_Width;
			Height = im->m_Height;
			Bpp = im->m_Bpp;
		}
	}

	return(rc);
}

void DM_Printf(const char* fmt, ...)
{
	va_list arg;
	char text[1024];

	va_start( arg, fmt );
	vsprintf( text, fmt, arg );
	va_end( arg );

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Console output %s!\r", text);
}

const char *DM_BuildOSPath(const char *basePath, const char *game, const char *relativePath)
{
	char *pRet = NULL;
	META_RES Ret = MRES_IGNORED;

#ifdef _WINDOWS
	static char p[1024];
	if(idStr::Cmpn("\\\\.\\", relativePath, 4) == 0)
	{
		strcpy(p, DARKMOD_LG_RENDERPIPE_NAME);
		Ret = MRES_SUPERCEDE;
		pRet = p;
	}
#endif

	RETURN_META_VALUE(Ret, pRet);
}

void CGlobal::GetSurfName(const idMaterial *material, idStr &strIn )
{
	int end;
	int surftype;
	
	if (material == NULL) {
		goto Quit;
	}
	
	end = -1;
	surftype = material->GetSurfaceType();

	if( surftype != SURFTYPE_15 )
	{
		strIn = gameLocal.sufaceTypeNames[ surftype ];
		goto Quit;
	}

	// return the first word of the description if it has surftype_15
	strIn = material->GetDescription();
	end = strIn.Find(' ');

	if ( end == -1 )
	{
		goto Quit;
	}

	strIn = strIn.Left( end );

Quit:
	if( strIn.IsEmpty() )
		strIn = "none";

	//DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("GetSurfName: Found surface type name %s\r", strIn.c_str());

	return;
}

idStr CGlobal::GetSurfName(const idMaterial* material)
{
	if (material == NULL)
	{
		return idStr();
	}

	const surfTypes_t surftype = material->GetSurfaceType();

	if (surftype != SURFTYPE_15)
	{
		return gameLocal.sufaceTypeNames[ surftype ];
	}

	// return the first word of the description if it has surftype_15
	idStr desc = material->GetDescription();
	int end = desc.Find(' ');

	return ( end == -1 ) ? desc : desc.Left(end);
}

const idStr& CGlobal::GetSurfaceHardness(const char* surfName)
{
	// Generate the hash from the string and look up the index into the list
	int index = m_SurfaceHardnessHash.First( m_SurfaceHardnessHash.GenerateKey(surfName) );	

	return (index != -1) ? m_SurfaceHardness[index] : m_SurfaceHardness[0]; // fall back to "soft"
}

void CGlobal::InitSurfaceHardness()
{
	m_SurfaceHardness.Clear();

	int soft = m_SurfaceHardness.Append("soft");
	int hard = m_SurfaceHardness.Append("hard");

	// The hard ones
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("none"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("metal"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("stone"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("wood"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("glass"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("plastic"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("ricochet"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype10"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype11"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype12"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype13"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype14"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("surftype15"), hard ); // shouldn't occur in normal operation
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("tile"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("gravel"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("rock"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("twigs"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("brokeglass"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("ice"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("squeakboard"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("puddle"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("ceramic"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("slate"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("armor_chain"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("armor_plate"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("climbable"), hard );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("hardwood"), hard ); // grayman #1421/1422

	// The soft ones
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("flesh"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("cardboard"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("liquid"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("carpet"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("dirt"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("grass"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("foliage"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("sand"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("mud"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("snow"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("moss"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("cloth"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("straw"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("armor_leath"), soft );
	m_SurfaceHardnessHash.Add( m_SurfaceHardnessHash.GenerateKey("paper"), soft );
}

std::string CGlobal::GetDarkmodPath()
{
	// Path to the parent directory
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Parent path is %s\r", parentPath.string().c_str());

	idStr modBaseName = cvarSystem->GetCVarString("fs_game_base");

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("fs_game_base is %s\r", modBaseName.c_str());

	if (modBaseName.IsEmpty())
	{
		// Fall back to fs_game if no game_base is set
		modBaseName = cvarSystem->GetCVarString("fs_game");

		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("fs_game is %s\r", modBaseName.c_str());

		if (modBaseName.IsEmpty())
		{
			modBaseName = "darkmod"; // last resort: hardcoded

			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Falling back to 'darkmod'\r");
		}
	}

	// Path to the darkmod directory
	fs::path darkmodPath(parentPath / modBaseName.c_str());

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Resulting darkmod path is %s\r", darkmodPath.string().c_str());

	return darkmodPath.file_string();
}

LC_LogClass CGlobal::GetLogClassForString(const char* str)
{
	for (int i = 0; i < LC_COUNT; ++i)
	{
		if (idStr::Icmp(str, LCString[i]) == 0)
		{
			return static_cast<LC_LogClass>(i);
		}
	}

	return LC_COUNT;
}

// Auto-completion function for log-classes
void CGlobal::ArgCompletion_LogClasses( const idCmdArgs &args, void(*callback)( const char *s ) )
{
	for (int i = 0; i < LC_COUNT; ++i)
	{
		callback( va( "%s %s", args.Argv( 0 ), LCString[i] ) );
	}
}
