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
 * Revision 1.43  2007/02/01 07:49:28  thelvyn
 * debugging macros
 *
 * Revision 1.42  2007/01/17 03:44:19  thelvyn
 * *** empty log message ***
 *
 * Revision 1.41  2006/12/09 17:30:20  sophisticatedzombie
 * Added  a configurable scale for computing maximum observation distance from
 * a lighting quotient given for a point by the LAS.  Visual acuity of the AI still needs
 * to be factored in afterword.
 *
 * Revision 1.40  2006/11/03 23:20:17  sparhawk
 * Lockpick Logclass added.
 *
 * Revision 1.39  2006/08/08 21:27:45  sparhawk
 * Added an accessor for the bufferlength in th CImage class.
 *
 * Revision 1.38  2006/07/20 21:07:25  sparhawk
 * Frame logging fixed.
 *
 * Revision 1.37  2006/07/15 02:14:52  ishtvan
 * surface name fix (for real this time)
 *
 * Revision 1.36  2006/05/26 04:42:01  sophisticatedzombie
 * Added variable which gives maximum number of hiding spot point tests per AI frame
 *
 * Revision 1.35  2006/05/17 05:39:16  sophisticatedzombie
 * Added new variables related to AIComm_Messsage and AIComm_StimResponse modules.
 *
 * Revision 1.34  2006/05/02 00:09:23  sophisticatedzombie
 * Added m_drawAIDebugGraphics. It defaults to 0.0 which is off. Values >= 1.0 draw the AI debug graphics for that number of milliseconds
 *
 * Revision 1.33  2006/01/31 22:34:44  sparhawk
 * StimReponse first working version
 *
 * Revision 1.32  2005/12/13 18:19:40  ishtvan
 * added m_MaxFrobDistance for frob distance cube around player
 *
 * Revision 1.31  2005/12/10 17:18:58  sophisticatedzombie
 * Added header references for LAS and hding spot detection
 *
 * Revision 1.30  2005/12/04 02:42:29  ishtvan
 * added GetSurfName function for surface names
 *
 * Revision 1.29  2005/11/26 17:42:45  sparhawk
 * Lightgem cleaned up
 *
 * Revision 1.28  2005/11/19 19:54:09  sparhawk
 * Logging disabler added.
 *
 * Revision 1.27  2005/11/19 19:41:52  sparhawk
 * Logging disabler added.
 *
 * Revision 1.26  2005/11/19 17:26:48  sparhawk
 * LogString with macro replaced
 *
 * Revision 1.25  2005/11/19 17:06:05  sparhawk
 * Frame marker added
 *
 * Revision 1.24  2005/11/17 22:40:13  sparhawk
 * Lightgem renderpipe fixed
 *
 * Revision 1.23  2005/10/30 22:15:26  sparhawk
 * Renderpipe creation removed because D3 can handle the pipename on it's own.
 *
 * Revision 1.22  2005/10/26 21:12:42  sparhawk
 * Lightgem renderpipe implemented
 *
 * Revision 1.21  2005/10/23 18:41:06  sparhawk
 * Lightgem cleanup
 *
 * Revision 1.20  2005/10/21 21:56:13  sparhawk
 * Ramdisk support added.
 *
 * Revision 1.19  2005/10/18 13:56:09  sparhawk
 * Lightgem updates
 *
 * Revision 1.18  2005/09/17 07:13:34  sophisticatedzombie
 * Added constants that control the scale by which damage can occur when mantling at a high relative velocity.
 *
 * Revision 1.17  2005/08/14 23:26:41  sophisticatedzombie
 * Added mantling and leaning constants to g_Global
 *
 * Revision 1.16  2005/04/07 08:35:42  ishtvan
 * Added AI acuities hash, moved soundprop flags to game_local.h
 *
 * Revision 1.15  2005/03/29 07:38:00  ishtvan
 * Added global typedefs for sound propagation flags and team alert flags
 *
 * Revision 1.14  2005/03/26 20:59:52  sparhawk
 * Logging initialization added for automatic mod name detection.
 *
 * Revision 1.13  2005/03/21 22:57:36  sparhawk
 * Special plane and vectorlogs added.
 *
 * Revision 1.12  2005/02/07 21:28:11  sparhawk
 * Added MATH class and LogVector3 function.
 *
 * Revision 1.11  2005/01/28 22:56:53  sparhawk
 * WEAPON class added.
 *
 * Revision 1.10  2005/01/24 00:15:22  sparhawk
 * AmbientLight parameter added to material
 *
 * Revision 1.9  2005/01/20 19:36:01  sparhawk
 * CImage class implemented to load and store texture images.
 *
 * Revision 1.8  2005/01/07 02:01:10  sparhawk
 * Lightgem updates
 *
 * Revision 1.7  2004/12/04 22:50:45  sparhawk
 * Added LogClass LIGHT
 *
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
#include "..\game\game_local.h"

/*!
Darkmod LAS
*/
#include "..\darkmod\DarkModLAS.h"

#ifndef ILuint
typedef unsigned int ILuint;
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
	LC_COUNT
} LC_LogClass;

class CDarkModPlayer;

class CImage {
public:
	CImage(idStr const &Name);
	CImage(void);
	~CImage(void);

	/**
	 * Load the image into memory and allow access to it. If the filename is not
	 * NULL, it is assumed that a new image is to be loaded and the old one is unloaded.
	 */
	bool LoadImage(const char *Filename = NULL);

	/**
	 * Load the image into memory and allow access to it. This method requires
	 * an already open filehandle.
	 */
	bool LoadImage(HANDLE &FileHandle);

	/**
	 * Initialize Imageinfo like bitmap width, height and other stuff.
	 */
	void InitImageInfo(void);

	/**
	 * GetImage returns the pointer to the actual image data. the image has to be already
	 * loaded, otherwise NULL is returned.
	 */
	unsigned char *GetImage(void);

	/**
	 * GetImage returns the pointer to the actual image data. the image has to be already
	 * loaded, otherwise NULL is returned.
	 */
	unsigned long GetBufferLen(void);

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
	unsigned char	*m_Image;
	ILuint			m_ImageId;
	bool			m_Loaded;

public:
	idStr			m_Name;
	int				m_Width;
	int				m_Height;
	int				m_Bpp;
};

class CLightMaterial {
public:
	CLightMaterial(idStr const &MaterialName, idStr const &TextureName, idStr const &MapName);
	~CLightMaterial(void);

	unsigned char *GetFallOffTexture(int &Width, int &Height, int &Bpp);
	unsigned char *GetImage(int &Width, int &Height, int &Bpp);

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
	CGlobal(void);
	~CGlobal(void);

	void Init(void);
	void GetModName(void);
	void LogPlane(idStr const &Name, idPlane const &Plane);
	void LogVector(idStr const &Name, idVec3 const &Vector);
	void LogMat3(idStr const &Name, idMat3 const &Matrix);
	void LogString(char *Format, ...);
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
	long			m_Frame;
	char			m_ModPath[1024];
	char			m_ModName[256];
	char			*m_Filename;
	char			m_DriveLetter;		// Remember the last driveletter
	int				m_Linenumber;
	CDarkModPlayer	*m_DarkModPlayer;

	idList<CLightMaterial *>		m_LightMaterial;
	idList<CImage *>				m_Image;

	CImage			m_RenderImage;

public:

	/*!
	* Global game settings, default values
	*/

	/*!
	* Default distance of reach for frobbing
	*/
	float m_DefaultFrobDistance;

	/**
	* Maximum distance of reach for frobbing (updated based on map objects)
	**/
	float m_MaxFrobDistance;

	/*!
	* Hiding spot search light quotient. This value probably needs tweaking.
	*/
	float m_hidingSpotMaxLightQuotient;


	/*!
	* Arm length for mantling
	* @author: sophisticatedZombie (DH)
	*/
	float m_armLengthAsFractionOfPlayerHeight;

	// Mantle trigger timer for holding jump key
	float m_jumpHoldMantleTrigger_Milliseconds;
	
	/*!
	* The meters per second of relative velocity beyond which the player takes damage 
	* when trying to mantle a target 
	*/
	float m_minimumVelocityForMantleDamage;

	/*!
	* The damage points per meter per second over the damage
	* velocity minimum
	*/
	float m_damagePointsPerMetersPerSecondOverMinimum;

	/*!
	* Milliseconds of time that player hangs if mantle begins
	* with the player's feet of the ground
	* @author: sophisticatedZombie (DH)
	*/
	float m_mantleHang_Milliseconds;

	/*!
	* Milliseconds of time it takes for the player to pull themselves
	* up to shoulder level with the mantle surface
	* @author: sophisticatedZombie (DH)
	*/
	float m_mantlePull_Milliseconds;

	/*!
	* Milliseconds of time it takes for the player to shift their
	* hands from pulling to pushing
	* @author: sophisticatedZombie (DH)
	*/
	float m_mantleShiftHands_Milliseconds;

	/*!
	* Milliseconds of time it takes for the player to push
	* themselves up onto the mantle surface
	* @author: sophisticatedZombie (DH)
	*/
	float m_mantlePush_Milliseconds;

	/*!
	* Milliseconds of time it takes for the player to enter
	* or exit a lean
	* @author: sophisticatedZombie (DH)
	*/
	float m_leanMove_Milliseconds;

	/*!
	* The angle to which a player rolls to the side during
	* a lean. The center of the circle is the player's feet,
	* so lean angles are smaller than they would be if the
	* player had a waist joint.
	* @author: sophisticatedZombie (DH)
	*/
	float m_leanMove_DegreesTilt;

	/*!
	* This is a variable that specifies a linear multiplier that is multiplied
	* by the LAS lighting quotient for a point to see how far away an AI can be 
	* from a given point and still think it is adequately searching it.
	*
	* A reasonable value seems to be in the 300-600 range.
	*
	*/
	float m_lightingQuotientObservationDistanceScale;

	/*!
	* This is a float that indicates for how many milliseconds the AI hiding spot 
	* detection routines should draw debug graphics on the screen. A value of
	* 0.0 indicates that they should not be drawn at all.
	*
	* The default value is 0.0
	*
	* A value of 1.0 or greater indicates a number of milliseconds that the
	* debug graphics for an AI operation should remain on the screen.
	*
	* @author: sophisticatedZobmie (DH)
	*/
	float m_drawAIDebugGraphics;

	/**
	* List of AI Acuities
	**/
	idStrList m_AcuityNames;
	idHashIndex m_AcuityHash;

	/**
	 * WeakLigthgem is set to true if the mathemmatical lightgem should be used.
	 * The prefered solution will be to use the rendersnapshot mechanism, but 
	 * this may not work for slower computers and the old solution is still available.
	 * This solution is less accurate, though. Default for this value is false.
	 */
	bool m_WeakLightgem;

	/**
	* The maximum distance a Responder can be from an Entity with an active 
	* AIComm stim (ST_COMMUNICATION) and have its responses potentially triggered.
	* Note that each Message has its own internal radius which if less than this
	* value, furthere restricts responding on message by message basis.
	*
	* However, this radius is the one that is used to find entities that will
	* check through the response list. Therefore, setting it higher may make
	* the processing of ST_COMMUNICATION stims take longer.
	*/
	float m_AICommStimRadius;

	/**
	* This is the maximum number of hiding spot point tests to do in a single AI frame.
	* Values in the 100 range seem to maintain decent frame rates
	*/
	int m_maxNumHidingSpotPointTestsPerAIFrame;


};

extern CGlobal g_Global;
extern char *g_LCString[];

#define LOGBUILD

#ifdef LOGBUILD
#define DM_LOG(lc, lt)				if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global
#define LOGSTRING					.LogString
#define DM_LOGVECTOR3(lc, lt, s, v)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogVector(s, v)
#define DM_LOGPLANE(lc, lt, s, p)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogPlane(s, p)
#define DM_LOGMAT3(lc, lt, s, m)	if(g_Global.m_ClassArray[lc] == true && g_Global.m_LogArray[lt] == true) g_Global.m_LogClass = lc, g_Global.m_LogType = lt, g_Global.m_Filename = __FILE__, g_Global.m_Linenumber = __LINE__, g_Global.LogMat3(s, m)
#else
#define DM_LOG(lc, lt)
#define LOGSTRING 
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

#ifdef _DEBUG
static char DebugStr[2048];
#define Debug(X)				{ strcpy (DebugStr, __FILE__ "(" MacroStr2(__LINE__) ") :" #X ); DebugStr[strlen(DebugStr)+1] = 0; DebugStr[strlen(DebugStr)] = '\n'; OutputDebugString(DebugStr); }
#define Debug1(X, Y)			{ sprintf(DebugStr, __FILE__ "(" MacroStr2(__LINE__) ") :" #X, Y); DebugStr[strlen(DebugStr)+1] = 0; DebugStr[strlen(DebugStr)] = '\n'; OutputDebugString(DebugStr); }
#define Debug2(X, Y, Z)			{ sprintf(DebugStr, __FILE__ "(" MacroStr2(__LINE__) ") :" #X, Y, Z); DebugStr[strlen(DebugStr)+1] = 0; DebugStr[strlen(DebugStr)] = '\n'; OutputDebugString(DebugStr); }
#define Debug3(X, Y, Z, A)		{ sprintf(DebugStr, __FILE__ "(" MacroStr2(__LINE__) ") :" #X, Y, Z, A); DebugStr[strlen(DebugStr)+1] = 0; DebugStr[strlen(DebugStr)] = '\n'; OutputDebugString(DebugStr); }
#define Debug4(X, Y, Z, A, B)	{ sprintf(DebugStr, __FILE__ "(" MacroStr2(__LINE__) ") :" #X, Y, Z, A, B); DebugStr[strlen(DebugStr)+1] = 0; DebugStr[strlen(DebugStr)] = '\n'; OutputDebugString(DebugStr); }
#else  // #ifdef _DEBUG
#define Debug(X)
#define Debug1(X, Y)
#define Debug2(X, Y, Z)
#define Debug3(X, Y, Z, A)
#define Debug4(X, Y, Z, A, B)
#endif // #ifdef _DEBUG

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

#endif
