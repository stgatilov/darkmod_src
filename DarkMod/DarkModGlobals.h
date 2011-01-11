/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2004            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * DESCRIPTION: This file contains all global identifiers, variables and
 * structures. Please note that global variables should be kept to a minimum
 * and only what is really neccessary should go in here.
 *
 *****************************************************************************/

#ifndef DARKMODGLOBALS_H
#define DARKMODGLOBALS_H

#include <stdio.h>
#include "../game/game_local.h"

// greebo: Defines the darkmod release version
#define TDM_VERSION_MAJOR 1
#define TDM_VERSION_MINOR 4

enum VersionCheckResult
{
	EQUAL,
	OLDER,
	NEWER,
};

// Compares the version pair <major, minor> to <toMajor, toMinor> and returns the result
// @returns: OLDER when <major, minor> is OLDER than <toMajor, toMinor>
VersionCheckResult CompareVersion(int major, int minor, int toMajor, int toMinor);

/*!
Darkmod LAS
*/
#include "../DarkMod/darkModLAS.h"
#include "Profile.h"
#include <boost/filesystem.hpp>

class CRenderPipe;

#ifndef ILuint
typedef unsigned int ILuint;
#endif

#ifndef ILenum
typedef unsigned int ILenum;
#endif

typedef enum {
	LT_INIT,
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

// greebo: NOTE: Keep these in accordance to the ones in scripts/tdm_defs.script!
typedef enum {
	LC_INIT,
	LC_FORCE,			// Never use this
	LC_MISC,
	LC_SYSTEM,			// Initialization, INI file and such stuff
	LC_FROBBING,		// Everything that has to do with frobbing
	LC_AI,				// same for AI
	LC_SOUND,			// same for sound
	LC_FUNCTION,		// general logging for functions (being, end, etc).
	LC_ENTITY,
	LC_INVENTORY,		// Everything that has to do with inventory
	LC_LIGHT,
	LC_WEAPON,
	LC_MATH,
	LC_MOVEMENT,		// mantling, leaning, ledge hanging, etc...
	LC_LOCKPICK,
	LC_FRAME,			// This is intended only as a framemarker and will always switched on if at least one other option is on.
	LC_STIM_RESPONSE,
	LC_OBJECTIVES,
	LC_DIFFICULTY,		// anything difficulty-related
	LC_CONVERSATION,	// conversation/dialogue stuff
	LC_MAINMENU,		// main menu logging
	LC_COUNT
} LC_LogClass;

class idCmdArgs;
class CDarkModPlayer;

class CImage
{
public:
	enum Type
	{
		AUTO_DETECT = 0,
		TGA,
		PNG,
		JPG,
		GIF,
	};

	CImage();
	CImage(const idStr& name);
	
	~CImage();

	/**
	 * Call this to let the image library assume a certain filetype when loading images
	 * from files or buffers.
	 */
	void SetDefaultImageType(Type type);

	/**
	 * Load the image from the given file (loaded via D3's filesystem) into memory 
	 * and allow access to it. If the filename is not NULL, it is assumed that a new 
	 * image is to be loaded from disk and the any previous one is unloaded.
	 */
	bool LoadImageFromVfs(const char* filename = NULL);

	/**
	 * Load the image from the given file (absolute OS paths) into memory 
	 * and allow access to it. If the filename is not NULL, it is assumed that a new 
	 * image is to be loaded from disk and the any previous one is unloaded.
	 */
	bool LoadImageFromFile(const boost::filesystem::path& path);

	/**
	 * Load the image into memory and allow access to it, reading the image
	 * data from a renderpipe.
	 */
	bool LoadImage(CRenderPipe *FileHandle);

	/**
	 * greebo: Saves the loaded image to the given path. Existing target files
	 * will be overwritten without further confirmation.
	 */
	bool SaveToFile(const boost::filesystem::path& path, Type type = TGA);

	/**
	 * Initialize Imageinfo like bitmap width, height and other stuff.
	 */
	void InitImageInfo();

	/**
	 * GetImage returns the pointer to the actual image data. the image has to be already
	 * loaded, otherwise NULL is returned.
	 */
	unsigned char* GetImage();

	/**
	 * Returns the buffer length of the loaded image data in bytes. Each pixel will have
	 * m_Bpp bytes (1 or 3 or 4, depending on format), and there are m_Width * m_Height pixel.
	 * The data will not be padded per-line (as f.i. with BMP) if you loaded a TGA file.
	 */
	unsigned long GetBufferLen();

	/**
	 * Unload will set the image to not loaded. If FreeMemory == false then the memory is not
	 * discarded and when you next load another image and it fits in the previous memory
	 * it will be loaded there. If it does not fit, then the memory is reallocated. This means that
	 * you can grow the memory usage by subsequently calling this with every groing imagesizes
	 * but on the other hand it will not constantly allocate and deallocate in case like the
	 * renderimages.
	 */
	void Unload(bool FreeMemory);

protected:
	unsigned long	m_BufferLength;
	unsigned char*	m_Image;
	ILuint			m_ImageId;
	bool			m_Loaded;

	// The image type used when opening files (defaults to IL_TGA)
	ILenum			m_defaultImageType;

	// Convert CImage::Type to ILenum
	ILenum GetILTypeForImageType(Type type);

public:
	idStr			m_Name;
	int				m_Width;	//!< Image width in pixel
	int				m_Height;	//!< Image height in pixel
	int				m_Bpp;		//!< Bytes per pixel (not bits!)
};

class CLightMaterial {
public:
	CLightMaterial(idStr const &MaterialName, idStr const &TextureName, idStr const &MapName);

	unsigned char *GetFallOffTexture(int &Width, int &Height, int &Bpp);
	unsigned char *GetImage(int &Width, int &Height, int &Bpp);

   	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );


public:
	idStr		m_MaterialName;
	bool		m_AmbientLight;		// Set to true if the ambientLight parameter is set.

protected:
	idStr		m_FallOffTexture;
	int			m_FallOffIndex;
	idStr		m_Map;
	int			m_MapIndex;
};

class CGlobal {
public:
	CGlobal();
	~CGlobal();

	void Init(void);
	void LogPlane(idStr const &Name, idPlane const &Plane);
	void LogVector(idStr const &Name, idVec3 const &Vector);
	void LogMat3(idStr const &Name, idMat3 const &Matrix);
	void LogString(const char *Format, ...);
	CLightMaterial *GetMaterial(idStr const &MaterialName);

	/**
	 * AddImageTexture will add the given image structure to the 
	 * texture list and returns the index with <Added> = true. If the
	 * image already exists in the list, based on it's name, the
	 * stucture will not be returned and the <Added> is set to false. 
	 * The index is still returned. If the image couldn't be added, -1
	 * is returned.
	 */
	int AddImage(idStr const &Name, bool &Added);

	/**
	 * Find an image by it's name or the index, if it is known.
	 * The second version will also return the index.
	 * NULL is returned if the image couldn't be found.
	 */
	CImage *GetImage(int Index);
	CImage *GetImage(idStr const &Name, int &Index);

	/**
	* Lookup the name of a the surface for a given material
	* Needed to incorporate new surface types
	* Stores the result in the strIn argument.
	* If the surface is not found or invalid, stores "none"
	**/
	void GetSurfName(const idMaterial *material, idStr &strIn);

	/**
	 * greebo: Returns the surface name for the given material
	 * or an empty string if not found.
	 **/
	idStr GetSurfName(const idMaterial* material);

	/** 
	 * greebo: Returns the surface hardness string ("soft", "hard")
	 * for the given material type.
	 */
	const idStr& GetSurfaceHardness(const char* surfName);

	// Returns the darkmod path
	static std::string GetDarkmodPath();

	// Converts a string to a logclass (LC_COUNT) if nothing found.
	static LC_LogClass GetLogClassForString(const char* str);

	static void ArgCompletion_LogClasses( const idCmdArgs &args, void(*callback)( const char *s ) );

private:
	void LoadINISettings(void *);

	void CheckLogClass(PROFILE_SECTION* ps, const char* key, LC_LogClass logClass);

	// Sets up the surface hardness mapping
	void InitSurfaceHardness();

	// A table for retrieving indices out of input strings
	idHashIndex m_SurfaceHardnessHash;
	
	// A list of hardness strings ("hard", "soft")
	idStringList m_SurfaceHardness;

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
	long			m_Frame;
	const char		*m_Filename;
	char			m_DriveLetter;		// Remember the last driveletter
	int				m_Linenumber;

	idList<CLightMaterial *>		m_LightMaterial;
	idList<CImage *>				m_Image;

	CImage			m_RenderImage;

public:

	/*!
	* Global game settings, default values
	*/

	/**
	* Maximum distance of reach for frobbing (updated based on map objects)
	**/
	float m_MaxFrobDistance;

	/**
	* List of AI Acuities
	**/
	idStrList m_AcuityNames;
	idHashIndex m_AcuityHash;
};

extern CGlobal g_Global;
extern const char *g_LCString[];

#define LOGBUILD

#ifdef LOGBUILD
#define DM_LOG(lc, lt)				if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global
#define LOGSTRING					.LogString
#define LOGVECTOR					.LogVector
#define DM_LOGVECTOR3(lc, lt, s, v)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogVector(s, v)
#define DM_LOGPLANE(lc, lt, s, p)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogPlane(s, p)
#define DM_LOGMAT3(lc, lt, s, m)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogMat3(s, m)
#else
#define DM_LOG(lc, lt)
#define LOGSTRING 
#define LOGVECTOR
#define DM_LOGVECTOR3(lc, lt, s, v)
#define DM_LOGPLANE(lc, lt, s, p)
#define DM_LOGMAT3(lc, lt, s, m)
#endif

/**
*	Message pragma so we can show file and line info in comments easily
*	Same principle as the one below but simpler to implement and use.
*   Been using it for about 8 or 9 years not sure where I found it
*	but I did have a subscription to windows developer journal so maybe thats where.
*	Usage: #pragma Message( "your message goes here")
*	
*	Submitted by Thelvyn
*/
#ifndef MacroStr2
#define MacroStr(x)   #x
#define MacroStr2(x)  MacroStr(x)
#define Message(desc) message(__FILE__ "(" MacroStr2(__LINE__) ") :" #desc)
#endif

/**
* The DARKMOD_NOTE macro makes it easy to add reminders which are shown when code is compiled. 
* You can double click on a reminder in the Output Window and jump to the line when using VC. 
* Adapted from highprogrammer.com (Originally from Windows Developer Journal).
*
* Usage: #pragma message(DARKMOD_NOTE "your reminder goes here")
*
* Submitted by Zaccheus
*/

#define DARKMOD_NOTE_AUX_STR( _S_ )             #_S_ 
#define DARKMOD_NOTE_AUX_MAKESTR( _M_, _L_ )    _M_(_L_) 
#define DARKMOD_NOTE_AUX_LINE                   DARKMOD_NOTE_AUX_MAKESTR(DARKMOD_NOTE_AUX_STR,__LINE__) 
#define DARKMOD_NOTE                            __FILE__ "(" DARKMOD_NOTE_AUX_LINE ") : DARKMOD_NOTE: " 

// A generic function to handle linear interpolation. J.C.Denton
template<class T> ID_INLINE T Lerp( const T &v1, const T &v2, const float l ) {
	
	T tRetVal;
	if ( l <= 0.0f ) {
		tRetVal = v1;
	} else if ( l >= 1.0f ) {
		tRetVal = v2;
	} else {
		tRetVal = v1 + l * ( v2 - v1 );
	}

	return tRetVal;
}


#endif
