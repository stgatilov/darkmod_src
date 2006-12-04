/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.37  2006/12/04 00:27:03  ishtvan
 * added logging of frame number in keyboard handler logs
 *
 * Revision 1.36  2006/11/20 05:34:19  ishtvan
 * added PauseGame function
 *
 * Revision 1.35  2006/11/01 11:57:38  sparhawk
 * Signals method added to entity.
 *
 * Revision 1.34  2006/08/21 05:06:49  ishtvan
 * added PlayerTraceEntity which returns the ent the player is looking at out to 512 units
 *
 * Revision 1.33  2006/08/01 21:13:20  sparhawk
 * Lightgem splitcode
 *
 * Revision 1.32  2006/06/21 10:12:45  sparhawk
 * Added version tracking per file
 *
 * Revision 1.31  2006/06/05 21:33:25  sparhawk
 * Stimtimer code updated/added
 *
 * Revision 1.30  2006/05/26 10:26:24  ishtvan
 * added mission data object, which gets updated in runframe
 *
 * Revision 1.29  2006/05/17 05:44:15  sophisticatedzombie
 * DoResponseAction now returns the number of CResponse objects triggered.
 * Added call to PostFired method of CStim after firing off a Stim.
 *
 * Revision 1.28  2006/04/26 21:29:16  sparhawk
 * Timed stim/response core added.
 *
 * Revision 1.27  2006/03/30 19:45:34  gildoran
 * I made three main changes:
 * 1. I moved the new decl headers out of game_local.h and into the few files
 * that actually use them.
 * 2. I added two new functions to idLinkList: next/prevNodeCircular().
 * 3. I added the first version of the tdmInventory objects. I've been working on
 * these on a vanilla 1.3 SDK, so I could test saving/loading. They appear to work
 * just fine.
 *
 * Revision 1.26  2006/03/25 08:13:58  gildoran
 * New update for declarations... Improved the documentation/etc for xdata decls, and added some basic code for tdm_matinfo decls.
 *
 * Revision 1.25  2006/03/23 06:24:53  gildoran
 * Added external data declarations for scripts to use. Readables can now have
 * their contents stored in a file.
 *
 * Revision 1.24  2006/02/06 22:14:28  sparhawk
 * Added ignore list for responses.
 *
 * Revision 1.23  2006/01/31 22:35:07  sparhawk
 * StimReponse first working version
 *
 * Revision 1.22  2006/01/29 04:28:00  ishtvan
 * *) Added GetLocationForArea, used by soundprop
 *
 * Revision 1.21  2006/01/24 22:03:46  sparhawk
 * Stim/Response implementation preliminary
 *
 * Revision 1.20  2005/12/04 02:45:02  ishtvan
 * fixed errors in surface variable names
 *
 * Revision 1.19  2005/12/02 19:45:07  sparhawk
 * Lightgem update. Particle and waterreflection fixed.
 *
 * Revision 1.18  2005/11/26 22:50:07  sparhawk
 * Keyboardhandler added.
 *
 * Revision 1.17  2005/11/26 17:44:44  sparhawk
 * Lightgem cleaned up
 *
 * Revision 1.16  2005/11/18 21:04:23  sparhawk
 * Particle effect fix
 *
 * Revision 1.15  2005/11/17 22:40:37  sparhawk
 * Lightgem renderpipe fixed
 *
 * Revision 1.14  2005/11/12 14:59:20  sparhawk
 * SDK 1.3 Merge
 *
 * Revision 1.13  2005/11/11 20:38:16  sparhawk
 * SDK 1.3 Merge
 *
 * Revision 1.12  2005/10/23 18:42:30  sparhawk
 * Lightgem cleanup
 *
 * Revision 1.11  2005/10/23 18:11:21  sparhawk
 * Lightgem entity spawn implemented
 *
 * Revision 1.10  2005/10/23 13:51:06  sparhawk
 * Top lightgem shot implemented. Image analyzing now assumes a
 * foursided triangulated rendershot instead of a single surface.
 *
 * Revision 1.9  2005/10/18 13:56:40  sparhawk
 * Lightgem updates
 *
 * Revision 1.8  2005/09/21 05:42:04  ishtvan
 * Modified sound prop propParms
 *
 * Revision 1.7  2005/08/22 04:55:24  ishtvan
 * minor changes in soundprop parms and function names
 *
 * Revision 1.6  2005/08/19 00:27:48  lloyd
 * *** empty log message ***
 *
 * Revision 1.5  2005/04/23 10:07:26  ishtvan
 * added fix for pm_walkspeed being reset to 140 by the engine on map load
 *
 * Revision 1.4  2005/04/07 09:38:10  ishtvan
 * *) Added members for global sound prop and sound prop loader objects
 *
 * *) Added global typedef for sound propagation flags which are needed by several different classes
 *
 * Revision 1.3  2005/03/29 07:43:42  ishtvan
 * Added forward declared pointer to global AI relations object: m_RelationsManager
 *
 * Revision 1.2  2005/01/07 02:10:35  sparhawk
 * Lightgem updates
 *
 * Revision 1.1.1.1  2004/10/30 15:52:30  sparhawk
 * Initial release
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

/** DarkMod AI Note:
* The following members in class idGameLocal:
* lastAIAlertEntity, lastAIAlertTime and idGameLocal::AlertAI
* ALL have nothing to do with DarkMod AI.
*
* DarkMod AI alerts are handled in class idAI
*
* AIAlertEntity alerts ALL AI to the entity in vanilla D3
*
* Unfortunately idGameLocal::AlertAI has the same name as
* our DarkMod alert function, idAI::AlertAI.  DarkMod
* alerts do not directly make use of idGameLocal::AlertAI.
**/

#ifndef __GAME_LOCAL_H__
#define	__GAME_LOCAL_H__

/**
 * Global function to keep track of the files and it's version.
 */
bool FileVersionList(const char *str, bool state);

class CStim;

// enables water physics
#define MOD_WATERPHYSICS

// Number of passes that we can do at most. This is 6 because it's simply a cube that is rendered 
// from all sides. This is not needed though, because a top and a bottom render with a pyramidic
// shape would be sufficient to cover all lighting situations. For silouhette detection we might
// consider more stages though.
#define DARKMOD_LG_MAX_RENDERPASSES			2
#define DARKMOD_LG_MAX_IMAGESPLIT			4
#define DARKMOD_LG_RENDER_MODEL				"models/props/misc/lightgem.lwo"
#define DARKMOD_LG_ENTITY_NAME				"lightgem_surface"
// The lightgem viewid defines the viewid that is to be used for the lightgem surfacetestmodel
#define DARKMOD_LG_VIEWID					-1
#define DARKMOD_LG_RENDERPIPE_NAME			"\\\\.\\pipe\\dm_renderpipe"
#define DARKMOD_LG_RENDERPIPE_BUFSIZE		50*1024		// Buffersize for the renderpipe
#define DARKMOD_LG_RENDERPIPE_TIMEOUT		1000
#define DARKMOD_LG_RENDER_WIDTH				50
// The colour is converted to a grayscale value which determines the state
// of the lightgem.
// LightGem = (0.29900*R+0.58700*G+0.11400*B) * 0.0625

#define DARKMOD_LG_MIN						1
#define DARKMOD_LG_MAX						32
#define DARKMOD_LG_FRACTION					(1.0f/32.0f)
#define DARKMOD_LG_RED						0.29900f
#define DARKMOD_LG_GREEN					0.58700f
#define DARKMOD_LG_BLUE						0.11400f
#define DARKMOD_LG_SCALE					(1.0/255.0)			// scaling factor for grayscale value


/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/

#define LAGO_IMG_WIDTH 64
#define LAGO_IMG_HEIGHT 64
#define LAGO_WIDTH	64
#define LAGO_HEIGHT	44
#define LAGO_MATERIAL	"textures/sfx/lagometer"
#define LAGO_IMAGE		"textures/sfx/lagometer.tga"

// if set to 1 the server sends the client PVS with snapshots and the client compares against what it sees
#ifndef ASYNC_WRITE_PVS
	#define ASYNC_WRITE_PVS 0
#endif

#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
// This is real evil but allows the code to inspect arbitrary class variables.
#define private		public
#define protected	public
#endif

extern idRenderWorld *				gameRenderWorld;
extern idSoundWorld *				gameSoundWorld;
/**
* place to store the sound world pointer when we temporarily set it to NULL
**/
extern idSoundWorld *			gameSoundWorldBuf;

// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"baseDOOM-1"

// classes used by idGameLocal
class idEntity;
class idActor;
class idPlayer;
class idCamera;
class idWorldspawn;
class idTestModel;
class idAAS;
class idAI;
class idSmokeParticles;
class idEntityFx;
class idTypeInfo;
class idProgram;
class idThread;
class idEditEntities;
class idLocationEntity;

#define	MAX_CLIENTS				32
#define	GENTITYNUM_BITS			12
#define	MAX_GENTITIES			(1<<GENTITYNUM_BITS)
#define	ENTITYNUM_NONE			(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD			(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)

//============================================================================

#include "gamesys/Event.h"
#include "gamesys/Class.h"
#include "gamesys/SysCvar.h"
#include "gamesys/SysCmds.h"
#include "gamesys/SaveGame.h"
#include "gamesys/DebugGraph.h"

#include "script/Script_Program.h"

#include "anim/Anim.h"

#include "ai/AAS.h"

#include "physics/Clip.h"
#include "physics/Push.h"

#include "Pvs.h"
#include "MultiplayerGame.h"

//============================================================================

class CLightMaterial;
class CsndPropLoader;
class CsndProp;
class CRelations;
class CMissionData;

const int MAX_GAME_MESSAGE_SIZE		= 8192;
const int MAX_ENTITY_STATE_SIZE		= 512;
const int ENTITY_PVS_SIZE			= ((MAX_GENTITIES+31)>>5);
const int NUM_RENDER_PORTAL_BITS	= idMath::BitsForInteger( PS_BLOCK_ALL );

typedef struct entityState_s {
	int						entityNumber;
	idBitMsg				state;
	byte					stateBuf[MAX_ENTITY_STATE_SIZE];
	struct entityState_s *	next;
} entityState_t;

typedef struct snapshot_s {
	int						sequence;
	entityState_t *			firstEntityState;
	int						pvs[ENTITY_PVS_SIZE];
	struct snapshot_s *		next;
} snapshot_t;

const int MAX_EVENT_PARAM_SIZE		= 128;

typedef struct entityNetEvent_s {
	int						spawnId;
	int						event;
	int						time;
	int						paramsSize;
	byte					paramsBuf[MAX_EVENT_PARAM_SIZE];
	struct entityNetEvent_s	*next;
	struct entityNetEvent_s *prev;
} entityNetEvent_t;

enum {
	GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP,
	GAME_RELIABLE_MESSAGE_REMAP_DECL,
	GAME_RELIABLE_MESSAGE_SPAWN_PLAYER,
	GAME_RELIABLE_MESSAGE_DELETE_ENT,
	GAME_RELIABLE_MESSAGE_CHAT,
	GAME_RELIABLE_MESSAGE_TCHAT,
	GAME_RELIABLE_MESSAGE_SOUND_EVENT,
	GAME_RELIABLE_MESSAGE_SOUND_INDEX,
	GAME_RELIABLE_MESSAGE_DB,
	GAME_RELIABLE_MESSAGE_KILL,
	GAME_RELIABLE_MESSAGE_DROPWEAPON,
	GAME_RELIABLE_MESSAGE_RESTART,
	GAME_RELIABLE_MESSAGE_SERVERINFO,
	GAME_RELIABLE_MESSAGE_TOURNEYLINE,
	GAME_RELIABLE_MESSAGE_CALLVOTE,
	GAME_RELIABLE_MESSAGE_CASTVOTE,
	GAME_RELIABLE_MESSAGE_STARTVOTE,
	GAME_RELIABLE_MESSAGE_UPDATEVOTE,
	GAME_RELIABLE_MESSAGE_PORTALSTATES,
	GAME_RELIABLE_MESSAGE_PORTAL,
	GAME_RELIABLE_MESSAGE_VCHAT,
	GAME_RELIABLE_MESSAGE_STARTSTATE,
	GAME_RELIABLE_MESSAGE_MENU,
	GAME_RELIABLE_MESSAGE_WARMUPTIME,
	GAME_RELIABLE_MESSAGE_EVENT
};

typedef enum {
	GAMESTATE_UNINITIALIZED,		// prior to Init being called
	GAMESTATE_NOMAP,				// no map loaded
	GAMESTATE_STARTUP,				// inside InitFromNewMap().  spawning map entities.
	GAMESTATE_ACTIVE,				// normal gameplay
	GAMESTATE_SHUTDOWN				// inside MapShutdown().  clearing memory.
} gameState_t;

typedef struct {
	idEntity	*ent;
	int			dist;
} spawnSpot_t;

//===========Dark Mod Global Typedefs===========

// Any key that is to be changed from an impulse to a button behaviour
// has to be listed here. The id gives the index in the array which slot
// is reserved for that function.
typedef enum {
	IR_FROB,
	IR_INVENTORY_NEXT,
	IR_INVENTORY_PREV,
	IR_LEAN_FORWARD,
	IR_LEAN_LEFT,
	IR_LEAN_RIGHT,
	IR_COUNT
} ImpulseFunction_t;

typedef enum {
	KS_UPDATED,			// Keyinfo has been updated by the hook
	KS_PROCESSED,		// Key has been processed by the gameengine
	KS_FREE,			// Keyslot is currently free.
	KS_COUNT
} KeyState_t;

/**
 * KeyCode is a structure that contains the information for a key which is related
 * to an IMPULSE.
 */
typedef struct KeyCode_s
{
	KeyState_t	KeyState;		// protocoll state for the interface with the gameengine
	int		Impulse;			// Impulsevalue this is associated with.
	int		VirtualKeyCode;
	int		RepeatCount;
	int		ScanCode;			// The value depends on the OEM.
	bool	Extended;			// Specifies whether the key is an extended key, such as a function key or
								// a key on the numeric keypad. The value is 1 if the key is an extended key,
								// otherwise, it is 0.
	int		Reserved;
	bool	Context;			// Specifies the context code. The value is 1 if the ALT key is down; otherwise, it is 0.
	bool	PreviousKeyState;	// The value is 1 if the key is down before the message is sent or 0 if the key is up.
	bool	TransitionState;	// The value is 0 if the key is being pressed and 1 if it is being released.
	int		FrameUpdated;		// Frame number in which this key state was last updated (used to update impulses only once per frame)
} KeyCode_t;

/**
* Sound prop. flags are used by many classes (Actor, soundprop, entity, etc)
* Therefore they are global.
* See sound prop doc file for definitions of these flags.
**/

typedef struct SSprFlagBits_s
{
	// team alert flags
	unsigned int friendly : 1;
	unsigned int neutral : 1;
	unsigned int enemy : 1;
	unsigned int same : 1;

	// propagation flags
	unsigned int omni_dir : 1; // omnidirectional
	unsigned int unique_loc : 1; // sound comes from a unique location
	unsigned int urgent : 1; // urgent (AI tries to respond ASAP)
	unsigned int global_vol : 1; // sound has same volume over whole map
	unsigned int check_touched : 1; // for non-AI, check who last touched the entity
} SSprFlagBits;

typedef union USprFlags_s
{
	unsigned int m_field;
	SSprFlagBits m_bits;
} USprFlags;

/**
* Sound propagation parameters: needed for function arguments
**/

typedef struct SSprParms_s
{
	USprFlags flags;

	const char	*name; // sound name
	float		propVol; // propagated volume

	// Apparent direction of the sound, determined by the path point on the portal
	idVec3		direction; 
	// actual origin of the sound, used for some localization simulation
	idVec3		origin; 
	float		duration; // duration
	int			frequency; // int representing the octave of the sound
	float		bandwidth; // sound bandwidth

	float		loudness; // this is set by AI hearing response

	bool		bSameArea; // true if the sound came from same portal area
	bool		bDetailedPath; // true if detailed path minimization was used to obtain the sound path
	int			floods; // number of portals the sound travelled thru before it hit the AI

	idEntity *maker; // it turns out the AI needs to know who made the sound to avoid bugs in some cases

} SSprParms;

//============================================================================

class idEventQueue {
public:
	typedef enum {
		OUTOFORDER_IGNORE,
		OUTOFORDER_DROP,
		OUTOFORDER_SORT
	} outOfOrderBehaviour_t;

							idEventQueue() : start( NULL ), end( NULL ) {}

	entityNetEvent_t *		Alloc();
	void					Free( entityNetEvent_t *event );
	void					Shutdown();

	void					Init();
	void					Enqueue( entityNetEvent_t* event, outOfOrderBehaviour_t oooBehaviour );
	entityNetEvent_t *		Dequeue( void );
	entityNetEvent_t *		RemoveLast( void );

	entityNetEvent_t *		Start( void ) { return start; }

private:
	entityNetEvent_t *					start;
	entityNetEvent_t *					end;
	idBlockAlloc<entityNetEvent_t,32>	eventAllocator;
};

//============================================================================

template< class type >
class idEntityPtr {
public:
							idEntityPtr();

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	idEntityPtr<type> &		operator=( type *ent );

	// synchronize entity pointers over the network
	int						GetSpawnId( void ) const { return spawnId; }
	bool					SetSpawnId( int id );
	bool					UpdateSpawnId( void );

	bool					IsValid( void ) const;
	type *					GetEntity( void ) const;
	int						GetEntityNum( void ) const;

private:
	int						spawnId;
};

template< class type >
ID_INLINE idEntityPtr<type>::idEntityPtr() {
	spawnId = 0;
}

template< class type >
ID_INLINE void idEntityPtr<type>::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( spawnId );
}

template< class type >
ID_INLINE void idEntityPtr<type>::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( spawnId );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( type *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS ) | ent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::SetSpawnId( int id ) {
	if ( id == spawnId ) {
		return false;
	}
	if ( ( id >> GENTITYNUM_BITS ) == gameLocal.spawnIds[ id & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsValid( void ) const {
	return ( gameLocal.spawnIds[ spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] == ( spawnId >> GENTITYNUM_BITS ) );
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetEntity( void ) const {
	int entityNum = spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( ( gameLocal.spawnIds[ entityNum ] == ( spawnId >> GENTITYNUM_BITS ) ) ) {
		return static_cast<type *>( gameLocal.entities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetEntityNum( void ) const {
	return ( spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) );
}

//============================================================================

class idGameLocal : public idGame {
public:
	idDict					serverInfo;				// all the tunable parameters, like numclients, etc
	int						numClients;				// pulled from serverInfo and verified
	idDict					userInfo[MAX_CLIENTS];	// client specific settings
	usercmd_t				usercmds[MAX_CLIENTS];	// client input commands
	idDict					persistentPlayerInfo[MAX_CLIENTS];
	idEntity *				entities[MAX_GENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES];// for use in idEntityPtr
	int						firstFreeIndex;			// first free index in the entities array
	int						num_entities;			// current number <= MAX_GENTITIES
	idHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn *			world;					// world entity
	idLinkList<idEntity>	spawnedEntities;		// all spawned entities
	idLinkList<idEntity>	activeEntities;			// all thinking entities (idEntity::thinkFlags != 0)
	int						numEntitiesToDeactivate;// number of entities that became inactive in current frame
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
	idDict					persistentLevelInfo;	// contains args that are kept around between levels

	// can be used to automatically effect every material in the world that references globalParms
	float					globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ];	

	idRandom				random;					// random number generator used throughout the game

	idProgram				program;				// currently loaded script and data space
	idThread *				frameCommandThread;

	idClip					clip;					// collision detection
	idPush					push;					// geometric pushing
	idPVS					pvs;					// potential visible set

	idTestModel *			testmodel;				// for development testing of models
	idEntityFx *			testFx;					// for development testing of fx

	idStr					sessionCommand;			// a target_sessionCommand can set this to return something to the session 

	idMultiplayerGame		mpGame;					// handles rules for standard dm

	idSmokeParticles *		smokeParticles;			// global smoke trails
	idEditEntities *		editEntities;			// in game editing
/**
* Pointer to global AI Relations object
**/
	CRelations *			m_RelationsManager;

/**
* Pointer to global Mission Data object (objectives & stats)
**/
	CMissionData *			m_MissionData;

/**
* Pointer to global sound prop loader object
**/
	CsndPropLoader *		m_sndPropLoader;

/**
* Pointer to global sound prop gameplay object
**/
	CsndProp *				m_sndProp;

/**
* Temporary storage of the walkspeed.  This is a workaround
*	because the walkspeed keeps getting reset.
**/
	float					m_walkSpeed;

	idList<CStim *>			m_StimTimer;			// All stims that have a timer associated. 
	idList<idEntity *>		m_StimEntity;			// all entities that currently have a stim regardless of it's state
	idList<idEntity *>		m_RespEntity;			// all entities that currently have a response regardless of it's state

	int						cinematicSkipTime;		// don't allow skipping cinemetics until this time has passed so player doesn't skip out accidently from a firefight
	int						cinematicStopTime;		// cinematics have several camera changes, so keep track of when we stop them so that we don't reset cinematicSkipTime unnecessarily
	int						cinematicMaxSkipTime;	// time to end cinematic when skipping.  there's a possibility of an infinite loop if the map isn't set up right.
	bool					inCinematic;			// game is playing cinematic (player controls frozen)
	bool					skipCinematic;

													// are kept up to date with changes to serverInfo
	int						framenum;
	int						previousTime;			// time in msec of last frame
	int						time;					// in msec
	int						m_Interleave;			// How often should the lightgem calculation be skipped?
	static const int		msec = USERCMD_MSEC;	// time since last update in milliseconds

	int						vacuumAreaNum;			// -1 if level doesn't have any outside areas

	gameType_t				gameType;
	bool					isMultiplayer;			// set if the game is run in multiplayer mode
	bool					isServer;				// set if the game is run for a dedicated or listen server
	bool					isClient;				// set if the game is run for a client
													// discriminates between the RunFrame path and the ClientPrediction path
													// NOTE: on a listen server, isClient is false
	int						localClientNum;			// number of the local client. MP: -1 on a dedicated
	idLinkList<idEntity>	snapshotEntities;		// entities from the last snapshot
	int						realClientTime;			// real client time
	bool					isNewFrame;				// true if this is a new game frame, not a rerun due to prediction
	float					clientSmoothing;		// smoothing of other clients in the view
	int						entityDefBits;			// bits required to store an entity def number

	static const char *		sufaceTypeNames[ MAX_SURFACE_TYPES ];	// text names for surface types
	/**
	* DarkMod: text names for new surface types
	**/
	static const char *		m_NewSurfaceTypes[ MAX_SURFACE_TYPES*2 ];

	idEntityPtr<idEntity>	lastGUIEnt;				// last entity with a GUI, used by Cmd_NextGUI_f
	int						lastGUI;				// last GUI on the lastGUIEnt

	idEntityPtr<idEntity>	portalSkyEnt;
	bool					portalSkyActive;

	HHOOK					m_KeyboardHook;
	KeyCode_t				m_KeyPress;				// Current keypress
	KeyCode_t				m_KeyData[IR_COUNT];	// Keypress associated with an IMPULSE

	void					SetPortalSkyEnt( idEntity *ent );
	bool					IsPortalSkyAcive();

	// ---------------------- Public idGame Interface -------------------

							idGameLocal();

	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			SetLocalClient( int clientNum );
	virtual void			ThrottleUserInfo( void );
	virtual const idDict *	SetUserInfo( int clientNum, const idDict &userInfo, bool isClient, bool canModify );
	virtual const idDict *	GetUserInfo( int clientNum );
	virtual void			SetServerInfo( const idDict &serverInfo );

	virtual const idDict &	GetPersistentPlayerInfo( int clientNum );
	virtual void			SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo );
	virtual void			InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randSeed );
	virtual bool			InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, idFile *saveGameFile );
	virtual void			SaveGame( idFile *saveGameFile );
	virtual void			MapShutdown( void );
	virtual void			CacheDictionaryMedia( const idDict *dict );
	virtual void			SpawnPlayer( int clientNum );
	virtual gameReturn_t	RunFrame( const usercmd_t *clientCmds );

	/**
	* TDM: Pause/Unpause game
	**/
	virtual void			PauseGame( bool bPauseState );
	virtual bool			Draw( int clientNum );
	virtual escReply_t		HandleESC( idUserInterface **gui );
	virtual idUserInterface	*StartMenu( void );
	virtual const char *	HandleGuiCommands( const char *menuCommand );
	virtual allowReply_t	ServerAllowClient( int numClients, const char *IP, const char *guid, const char *password, char reason[MAX_STRING_CHARS] );
	virtual void			ServerClientConnect( int clientNum );
	virtual void			ServerClientBegin( int clientNum );
	virtual void			ServerClientDisconnect( int clientNum );
	virtual void			ServerWriteInitialReliableMessages( int clientNum );
	virtual void			ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, byte *clientInPVS, int numPVSClients );
	virtual bool			ServerApplySnapshot( int clientNum, int sequence );
	virtual void			ServerProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual void			ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg );
	virtual bool			ClientApplySnapshot( int clientNum, int sequence );
	virtual void			ClientProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual gameReturn_t	ClientPrediction( int clientNum, const usercmd_t *clientCmds );

	virtual void			GetClientStats( int clientNum, char *data, const int len );
	virtual void			SwitchTeam( int clientNum, int team );

	virtual bool			DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] );

	// ---------------------- Public idGameLocal Interface -------------------

	void					Printf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DPrintf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Warning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DWarning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Error( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

							// Initializes all map variables common to both save games and spawned games
	void					LoadMap( const char *mapName, int randseed );

	void					LocalMapRestart( void );
	void					MapRestart( void );
	static void				MapRestart_f( const idCmdArgs &args );
	bool					NextMap( void );	// returns wether serverinfo settings have been modified
	static void				NextMap_f( const idCmdArgs &args );

	idMapFile *				GetLevelMap( void );
	const char *			GetMapName( void ) const;

	int						NumAAS( void ) const;
	idAAS *					GetAAS( int num ) const;
	idAAS *					GetAAS( const char *name ) const;
	void					SetAASAreaState( const idBounds &bounds, const int areaContents, bool closed );
	aasHandle_t				AddAASObstacle( const idBounds &bounds );
	void					RemoveAASObstacle( const aasHandle_t handle );
	void					RemoveAllAASObstacles( void );

	bool					CheatsOk( bool requirePlayer = true );
	void					SetSkill( int value );
	gameState_t				GameState( void ) const;
	idEntity *				SpawnEntityType( const idTypeInfo &classdef, const idDict *args = NULL, bool bIsClientReadSnapshot = false );
	bool					SpawnEntityDef( const idDict &args, idEntity **ent = NULL, bool setDefaults = true );
	int						GetSpawnId( const idEntity *ent ) const;

	const idDeclEntityDef *	FindEntityDef( const char *name, bool makeDefault = true ) const;
	const idDict *			FindEntityDefDict( const char *name, bool makeDefault = true ) const;

	void					RegisterEntity( idEntity *ent );
	void					UnregisterEntity( idEntity *ent );

	bool					RequirementMet( idEntity *activator, const idStr &requires, int removeItem );

/**
* The following are vanilla D3 functions that have nothing to do with TDM's AI alert system
**/
	void					AlertAI( idEntity *ent );
	idActor *				GetAlertEntity( void );

	bool					InPlayerPVS( idEntity *ent ) const;
	bool					InPlayerConnectedArea( idEntity *ent ) const;

	pvsHandle_t				GetPlayerPVS()			{ return playerPVS; };

	void					SetCamera( idCamera *cam );
	idCamera *				GetCamera( void ) const;
	bool					SkipCinematic( void );
	void					CalcFov( float base_fov, float &fov_x, float &fov_y ) const;

	void					AddEntityToHash( const char *name, idEntity *ent );
	bool					RemoveEntityFromHash( const char *name, idEntity *ent );
	int						GetTargets( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const;

							// returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
	idEntity *				GetTraceEntity( const trace_t &trace ) const;

	static void				ArgCompletion_EntityName( const idCmdArgs &args, void(*callback)( const char *s ) );
	idEntity *				FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo &c, const idEntity *skip ) const;
	idEntity *				FindEntity( const char *name ) const;
	idEntity *				FindEntityUsingDef( idEntity *from, const char *match ) const;
	int						EntitiesWithinRadius( const idVec3 org, float radius, idEntity **entityList, int maxCount ) const;

	/**
	* Get the entity that the player is looking at
	* Currently called by console commands, does a long trace so should not be
	* called every frame.
	**/
	idEntity *				PlayerTraceEntity( void );

	void					KillBox( idEntity *ent, bool catch_teleport = false );
	void					RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f );
	void					RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake );
	void					RadiusPushClipModel( const idVec3 &origin, const float push, const idClipModel *clipModel );

	void					ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0 );
	void					BloodSplat( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	void					CallFrameCommand( idEntity *ent, const function_t *frameCommand );
	void					CallObjectFrameCommand( idEntity *ent, const char *frameCommand );

	const idVec3 &			GetGravity( void ) const;

	// added the following to assist licensees with merge issues
	int						GetFrameNum() const { return framenum; };
	int						GetTime() const { return time; };
	int						GetMSec() const { return msec; };

	int						GetNextClientNum( int current ) const;
	idPlayer *				GetClientByNum( int current ) const;
	idPlayer *				GetClientByName( const char *name ) const;
	idPlayer *				GetClientByCmdArgs( const idCmdArgs &args ) const;

	idPlayer *				GetLocalPlayer() const;

	void					SpreadLocations();
	idLocationEntity *		LocationForPoint( const idVec3 &point );	// May return NULL
	/**
	* LocationForArea returns a pointer to the location entity for the given area number
	* Returns NULL if the area number is out of bounds, or if locations haven't sprad yet
	**/
	idLocationEntity *		LocationForArea( const int areaNum ); 

	idEntity *				SelectInitialSpawnPoint( idPlayer *player );

	void					SetPortalState( qhandle_t portal, int blockingBits );
	void					SaveEntityNetworkEvent( const idEntity *ent, int event, const idBitMsg *msg );
	void					ServerSendChatMessage( int to, const char *name, const char *text );
	int						ServerRemapDecl( int clientNum, declType_t type, int index );
	int						ClientRemapDecl( declType_t type, int index );

	void					SetGlobalMaterial( const idMaterial *mat );
	const idMaterial *		GetGlobalMaterial();

	void					SetGibTime( int _time ) { nextGibTime = _time; };
	int						GetGibTime() { return nextGibTime; };

	bool					NeedRestart();

	/**
	 * LoadLightMaterial loads the falloff textures from the light materials. The appropriate
	 * textures are only loaded when the light is spawned and requests the texture.
	 */
	void					LoadLightMaterial(const char *Filename, idList<CLightMaterial *> *);

	/**
	 * Createrenderpipe will create a pipe that is used to read the snapshot images from.
	 * Currently this works under Windows only. This is neccessary, because we have to store
	 * the rendersnapshots somehwere and the only way to do this is via a pipe if we want to
	 * avoid writing it constantly to disc.
	 */
	HANDLE					CreateRenderPipe(int timeout = DARKMOD_LG_RENDERPIPE_TIMEOUT);

	/**
	 * CloseRenderPipe will close the renderpipe. Who would have thought that. :)
	 */
	void					CloseRenderPipe(HANDLE &hPipe);

	/**
	 * SpawnlightgemEntity will create exactly one lightgem entity for the map and ensures
	 * that no multiple copies of it will exist.
	 */
	void					SpawnLightgemEntity(void);

	/**
	 * ProcessLightgem will trigger the actual lightgem processing.
	 */
	void					ProcessLightgem(idPlayer *pPlayer, bool bProcessing);

	/**
	 * CalcLightgem will do the rendersnapshot and analyze the snaphost image in order
	 * to determine the lightvalue for the lightgem.
	 */
	float					CalcLightgem(idPlayer *);

	/**
	 * AnalyzeRenderImage will analyze the given image and yields an averaged single value
	 * determining the lightvalue for the given image.
	 */
	void					AnalyzeRenderImage(HANDLE hPipe, float fColVal[DARKMOD_LG_MAX_IMAGESPLIT]);

	/**
	 * ImpulseInit will initialize a slot with the current keypress if it is empty.
	 * The function returns true if the slot has already been initialized for this 
	 * keypress. If false is returned the slot was free before and is now ready to use
	 * This should always be the first function to be called in order to determine wether
	 * an impulse has been triggered already for continous use.
	 */
	bool					ImpulseInit(ImpulseFunction_t Function, int Impulse);

	/**
	 * ImpulseIsUpdated checks wether the slot has been updated since the last time the impulse
	 * has been processed.
	 */
	bool					ImpulseIsUpdated(ImpulseFunction_t Function);

	/**
	 * ImpulseProcessed has to be called whenever the impulse function has processed it's
	 * keystate, but is not finished yet.
	 */
	void					ImpulseProcessed(ImpulseFunction_t Function);

	/**
	 * ImpulseFree is called when the processing of the impulse is finished and no further
	 * reporting should be done. This would usually be when the key is released.
	 */
	void					ImpulseFree(ImpulseFunction_t Function);

	/**
	 * ImpulseData returns the pointer to the keyinfo structure. The state should not be modified 
	 * via this pointer.
	 */
	KeyCode_t				*ImpulseData(ImpulseFunction_t Function) { return &m_KeyData[Function]; };

	bool					AddStim(idEntity *);
	void					RemoveStim(idEntity *);
	bool					AddResponse(idEntity *);
	void					RemoveResponse(idEntity *);
	int						CheckStimResponse(idList<idEntity *> &, idEntity *);

	/**
	* Fires off all the enabled responses to this stim of the entities in the given entites list.
	* If the trigger is coming from a timer, <Timer> is set to true.
	* 
	* @return The number of responses triggered
	*
	*/
	int						DoResponseAction(CStim *, idEntity *Ent[MAX_GENTITIES], int NumEntities, idEntity *Originator, bool Timer);

	/**
	 * ProcessStimResponse will check wether stims are in reach of a response and if so activate them.
	 */
	void					ProcessStimResponse(void);

	/**
	 * CheckSignal will call all entites registered for a signal actacvtion.
	 */
	void					CheckSDKSignal(void);
	void					AddSDKSignal(idEntity *oObject);

private:
	const static int		INITIAL_SPAWN_COUNT = 1;

	idStr					mapFileName;			// name of the map, empty string if no map loaded
	idMapFile *				mapFile;				// will be NULL during the game unless in-game editing is used
	bool					mapCycleLoaded;

	int						spawnCount;
	int						mapSpawnCount;			// it's handy to know which entities are part of the map

	idLocationEntity **		locationEntities;		// for location names, etc

	idCamera *				camera;
	const idMaterial *		globalMaterial;			// for overriding everything

	idList<idAAS *>			aasList;				// area system
	idStrList				aasNames;

	idEntityPtr<idActor>	lastAIAlertEntity;
	int						lastAIAlertTime;

	idDict					spawnArgs;				// spawn args used during entity spawning  FIXME: shouldn't be necessary anymore

	pvsHandle_t				playerPVS;				// merged pvs of all players
	pvsHandle_t				playerConnectedAreas;	// all areas connected to any player area

	idVec3					gravity;				// global gravity vector
	gameState_t				gamestate;				// keeps track of whether we're spawning, shutting down, or normal gameplay
	bool					influenceActive;		// true when a phantasm is happening
	int						nextGibTime;

	idList<int>				clientDeclRemap[MAX_CLIENTS][DECL_MAX_TYPES];

	entityState_t *			clientEntityStates[MAX_CLIENTS][MAX_GENTITIES];
	int						clientPVS[MAX_CLIENTS][ENTITY_PVS_SIZE];
	snapshot_t *			clientSnapshots[MAX_CLIENTS];
	idBlockAlloc<entityState_t,256>entityStateAllocator;
	idBlockAlloc<snapshot_t,64>snapshotAllocator;

	idEventQueue			eventQueue;
	idEventQueue			savedEventQueue;

	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots;
	idStaticList<idEntity *, MAX_GENTITIES> initialSpots;
	int						currentInitialSpot;

	idDict					newInfo;

	idStrList				shakeSounds;

	byte					lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ];

	/**
	 * Lightgemsurface contains a pointer to the lightgem surface entity. This
	 * is constantly required and therfore we store it permanently.
	 */
	idEntity				*m_LightgemSurface;
	bool					m_DoLightgem;		// Signal when the lightgem may be processed.
	int						m_LightgemShotSpot;
	float					m_LightgemShotValue[DARKMOD_LG_MAX_RENDERPASSES];
	SECURITY_ATTRIBUTES		m_saPipeSecurity;
	PSECURITY_DESCRIPTOR	m_pPipeSD;
	idList<idEntity *>		m_SignalList;

	void					Clear( void );
							// returns true if the entity shouldn't be spawned at all in this game type or difficulty level
	bool					InhibitEntitySpawn( idDict &spawnArgs );
							// spawn entities from the map file
	void					SpawnMapEntities( void );
							// commons used by init, shutdown, and restart
	void					MapPopulate( void );
	void					MapClear( bool clearClients );

	pvsHandle_t				GetClientPVS( idPlayer *player, pvsType_t type );
	void					SetupPlayerPVS( void );
	void					FreePlayerPVS( void );
	void					UpdateGravity( void );
	void					SortActiveEntityList( void );
	void					ShowTargets( void );
	void					RunDebugInfo( void );

	void					InitScriptForMap( void );

	void					InitConsoleCommands( void );
	void					ShutdownConsoleCommands( void );

	void					InitAsyncNetwork( void );
	void					ShutdownAsyncNetwork( void );
	void					InitLocalClient( int clientNum );
	void					InitClientDeclRemap( int clientNum );
	void					ServerSendDeclRemapToClient( int clientNum, declType_t type, int index );
	void					FreeSnapshotsOlderThanSequence( int clientNum, int sequence );
	bool					ApplySnapshot( int clientNum, int sequence );
	void					WriteGameStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGameStateFromSnapshot( const idBitMsgDelta &msg );
	void					NetworkEventWarning( const entityNetEvent_t *event, const char *fmt, ... ) id_attribute((format(printf,3,4)));
	void					ServerProcessEntityNetworkEventQueue( void );
	void					ClientProcessEntityNetworkEventQueue( void );
	void					ClientShowSnapshot( int clientNum ) const;
							// call after any change to serverInfo. Will update various quick-access flags
	void					UpdateServerInfoFlags( void );
	void					RandomizeInitialSpawns( void );
	static int				sortSpawnPoints( const void *ptr1, const void *ptr2 );

	void					DumpOggSounds( void );
	void					GetShakeSounds( const idDict *dict );
	void					SelectTimeGroup( int timeGroup );
	int						GetTimeGroupTime( int timeGroup );
	idStr					GetBestGameType( const char* map, const char* gametype );

	void					Tokenize( idStrList &out, const char *in );

	void					UpdateLagometer( int aheadOfServer, int dupeUsercmds );
};

//============================================================================

extern idGameLocal			gameLocal;
extern idAnimManager		animationLib;

//============================================================================

class idGameError : public idException {
public:
	idGameError( const char *text ) : idException( text ) {}
};

//============================================================================


//
// these defines work for all startsounds from all entity types
// make sure to change script/doom_defs.script if you add any channels, or change their order
//
typedef enum {
	SND_CHANNEL_ANY = SCHANNEL_ANY,
	SND_CHANNEL_VOICE = SCHANNEL_ONE,
	SND_CHANNEL_VOICE2,
	SND_CHANNEL_BODY,
	SND_CHANNEL_BODY2,
	SND_CHANNEL_BODY3,
	SND_CHANNEL_WEAPON,
	SND_CHANNEL_ITEM,
	SND_CHANNEL_HEART,
	SND_CHANNEL_PDA,
	SND_CHANNEL_DEMONIC,
	SND_CHANNEL_RADIO,

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE
} gameSoundChannel_t;

// content masks
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY)

const float DEFAULT_GRAVITY			= 1066.0f;
#define DEFAULT_GRAVITY_STRING		"1066"
const idVec3 DEFAULT_GRAVITY_VEC3( 0, 0, -DEFAULT_GRAVITY );

const int	CINEMATIC_SKIP_DELAY	= SEC2MS( 2.0f );

//============================================================================

#include "physics/Force.h"
#include "physics/Force_Constant.h"
#include "physics/Force_Drag.h"
#include "physics/Force_Field.h"
#include "physics/Force_Spring.h"
#include "physics/Physics.h"
#include "physics/Physics_Static.h"
#include "physics/Physics_StaticMulti.h"
#include "physics/Physics_Base.h"
#include "physics/Physics_Actor.h"
#include "physics/Physics_Monster.h"
#include "physics/Physics_Player.h"
#include "physics/Physics_Parametric.h"
#include "physics/Physics_RigidBody.h"
#include "physics/Physics_AF.h"
#include "physics/Physics_Liquid.h"

#include "SmokeParticles.h"

#include "Entity.h"
#include "GameEdit.h"
#include "AF.h"
#include "IK.h"
#include "AFEntity.h"
#include "Misc.h"
#include "Actor.h"
#include "Projectile.h"
#include "Weapon.h"
#include "Light.h"
#include "WorldSpawn.h"
#include "Item.h"
#include "PlayerView.h"
#include "PlayerIcon.h"
#include "Player.h"
#include "Mover.h"
#include "Camera.h"
#include "Moveable.h"
#include "Target.h"
#include "Trigger.h"
#include "Sound.h"
#include "Fx.h"
#include "SecurityCamera.h"
#include "BrittleFracture.h"
#include "Liquid.h"

#include "ai/AI.h"
#include "anim/Anim_Testmodel.h"

#include "script/Script_Compiler.h"
#include "script/Script_Interpreter.h"
#include "script/Script_Thread.h"

const float	RB_VELOCITY_MAX				= 16000;
const int	RB_VELOCITY_TOTAL_BITS		= 16;
const int	RB_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( RB_VELOCITY_MAX ) ) + 1;
const int	RB_VELOCITY_MANTISSA_BITS	= RB_VELOCITY_TOTAL_BITS - 1 - RB_VELOCITY_EXPONENT_BITS;
const float	RB_MOMENTUM_MAX				= 1e20f;
const int	RB_MOMENTUM_TOTAL_BITS		= 16;
const int	RB_MOMENTUM_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( RB_MOMENTUM_MAX ) ) + 1;
const int	RB_MOMENTUM_MANTISSA_BITS	= RB_MOMENTUM_TOTAL_BITS - 1 - RB_MOMENTUM_EXPONENT_BITS;
const float	RB_FORCE_MAX				= 1e20f;
const int	RB_FORCE_TOTAL_BITS			= 16;
const int	RB_FORCE_EXPONENT_BITS		= idMath::BitsForInteger( idMath::BitsForFloat( RB_FORCE_MAX ) ) + 1;
const int	RB_FORCE_MANTISSA_BITS		= RB_FORCE_TOTAL_BITS - 1 - RB_FORCE_EXPONENT_BITS;

#endif	/* !__GAME_LOCAL_H__ */
