/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $HeadURL$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

#include "../DarkMod/overlaySys.h"

class CStimResponseCollection;
class CStim;
class CResponse;

class CInventory;
class CInventoryItem;
class CInventoryCursor;

/**
* SDK_SIGNALS are used to signal either from a script to the SDK code or
* inside the SDK code itself. It can not signal from SDK to a script. In order
* to do this, you can use the regular signal mechanism from id.
*/
#define SDK_SIGNAL			int
typedef enum {
	SIG_REMOVE,
	SIG_CONTINUE,
	SIG_COUNT
} E_SDK_SIGNAL_STATE;

typedef struct {
	SDK_SIGNAL	m_Id;
	bool		m_Signaled;
	idEntity	*m_Object;
	void		*m_Data;
	E_SDK_SIGNAL_STATE	(*m_Fkt)(idEntity *oObject, void *pData);
} SDKSignalInfo;

/*
===============================================================================

	Game entity base class.

===============================================================================
*/

static const int DELAY_DORMANT_TIME = 3000;
/**
* greebo: This is the value that has to be exceeded by the lightingQuotient
*		  returned by the LAS routines (range 0.0 ... 1.0).
*		  0.0 means no threshold - target entities are always visible
*		  This is used by the idEntity::canSeeEntity() member method.
*/
static const float VISIBILTIY_LIGHTING_THRESHOLD = 0.2f; 

extern const idEventDef EV_PostSpawn;
extern const idEventDef EV_FindTargets;
extern const idEventDef EV_Touch;
extern const idEventDef EV_Use;
extern const idEventDef EV_Activate;
extern const idEventDef EV_ActivateTargets;
extern const idEventDef EV_Hide;
extern const idEventDef EV_Show;
extern const idEventDef EV_GetShaderParm;
extern const idEventDef EV_SetShaderParm;
extern const idEventDef EV_SetOwner;
extern const idEventDef EV_GetAngles;
extern const idEventDef EV_SetAngles;
extern const idEventDef EV_SetLinearVelocity;
extern const idEventDef EV_SetAngularVelocity;
extern const idEventDef EV_SetSkin;
extern const idEventDef EV_StartSoundShader;
extern const idEventDef EV_StopSound;
extern const idEventDef EV_CacheSoundShader;

extern const idEventDef EV_IsType;

#ifdef MOD_WATERPHYSICS

extern const idEventDef EV_GetMass;				// MOD_WATERPHYSICS

extern const idEventDef EV_IsInLiquid;			// MOD_WATERPHYSICS

#endif		// MOD_WATERPHYSICS

extern const idEventDef EV_CopyBind;
extern const idEventDef EV_IsFrobable;
extern const idEventDef EV_SetFrobable;
extern const idEventDef EV_InPVS;
extern const idEventDef EV_CanSeeEntity;

// greebo: Script event definition for dealing damage
extern const idEventDef EV_Damage;
extern const idEventDef EV_Heal;

// Think flags
enum {
	TH_ALL					= -1,
	TH_THINK				= 1,		// run think function each frame
	TH_PHYSICS				= 2,		// run physics each frame
	TH_ANIMATE				= 4,		// update animation each frame
	TH_UPDATEVISUALS		= 8,		// update renderEntity
	TH_UPDATEPARTICLES		= 16
};

//
// Signals
// make sure to change script/doom_defs.script if you add any, or change their order
//
typedef enum {
	SIG_TOUCH,				// object was touched
	SIG_USE,				// object was used
	SIG_TRIGGER,			// object was activated
	SIG_REMOVED,			// object was removed from the game
	SIG_DAMAGE,				// object was damaged
	SIG_BLOCKED,			// object was blocked

	SIG_MOVER_POS1,			// mover at position 1 (door closed)
	SIG_MOVER_POS2,			// mover at position 2 (door open)
	SIG_MOVER_1TO2,			// mover changing from position 1 to 2
	SIG_MOVER_2TO1,			// mover changing from position 2 to 1

	NUM_SIGNALS
} signalNum_t;

// FIXME: At some point we may want to just limit it to one thread per signal, but
// for now, I'm allowing multiple threads.  We should reevaluate this later in the project
#define MAX_SIGNAL_THREADS 16		// probably overkill, but idList uses a granularity of 16

struct signal_t {
	int					threadnum;
	const function_t	*function;
};

class signalList_t {
public:
	idList<signal_t> signal[ NUM_SIGNALS ];
};

// Inventory-related flags.
enum {
	EINV_GROUP	= 1, // Jump to the next/previous group.
	EINV_PREV	= 2, // Iterate backwards.
	EINV_FAST	= 4, // Don't use group histories.
};

// Used by m_renderTrigger
#define EMPTY_MODEL "models/darkmod/props/misc/empty.lwo"

class idEntity : public idClass {
public:
	static const int		MAX_PVS_AREAS = 4;

	int						entityNumber;			// index into the entity list
	int						entityDefNumber;		// index into the entity def list

	idLinkList<idEntity>	spawnNode;				// for being linked into spawnedEntities list
	idLinkList<idEntity>	activeNode;				// for being linked into activeEntities list

	idLinkList<idEntity>	snapshotNode;			// for being linked into snapshotEntities list
	int						snapshotSequence;		// last snapshot this entity was in
	int						snapshotBits;			// number of bits this entity occupied in the last snapshot

	idStr					name;					// name of entity
	idDict					spawnArgs;				// key/value pairs used to spawn and initialize entity
	idScriptObject			scriptObject;			// contains all script defined data for this entity

	int						thinkFlags;				// TH_? flags
	int						dormantStart;			// time that the entity was first closed off from player
	bool					cinematic;				// during cinematics, entity will only think if cinematic is set

	renderView_t *			renderView;				// for camera views from this entity
	idEntity *				cameraTarget;			// any remoteRenderMap shaders will use this

	idList< idEntityPtr<idEntity> >	targets;		// when this entity is activated these entities entity are activated

	int						health;					// FIXME: do all objects really need health?
	int						maxHealth;				// greebo: Moved this from idInventory to here

	struct entityFlags_s {
		bool				notarget			:1;	// if true never attack or target this entity
		bool				noknockback			:1;	// if true no knockback from hits
		bool				takedamage			:1;	// if true this entity can be damaged
		bool				hidden				:1;	// if true this entity is not visible
		bool				bindOrientated		:1;	// if true both the master orientation is used for binding
		bool				solidForTeam		:1;	// if true this entity is considered solid when a physics team mate pushes entities
		bool				forcePhysicsUpdate	:1;	// if true always update from the physics whether the object moved or not
		bool				selected			:1;	// if true the entity is selected for editing
		bool				neverDormant		:1;	// if true the entity never goes dormant
		bool				isDormant			:1;	// if true the entity is dormant
		bool				hasAwakened			:1;	// before a monster has been awakened the first time, use full PVS for dormant instead of area-connected
		bool				networkSync			:1; // if true the entity is synchronized over the network
	} fl;

	/**
	 * UsedBy ist the list of entity names that this entity can be used by.
	 * i.e. A door can have a list of keys that can unlock it. A fountain can
	 * be used by a water arrow to make it holy, etc.
	 * The list is initialized by the key "used"_by" in the entity and contains
	 * the names of the entities seperated by ';'. If the first character of
	 * the name is a '*' the name donates a classname instead of an entity. In
	 * this case all entities of that class can be used by this entity.
	 * Example: If you have several holy fountains you would have to list all
	 * fountain names. Alternatively you can create a "holy_fountain" class and
	 * list it like this:
	 * "used_by" "*holy_fountain;*holy_bottle;holy_cross"
	 * In this example the entity can be used by any holy_fountain, any holy_bottle
	 * and addtionaly by the entity named holy_cross.
	 */
	idList<idStr>			m_UsedBy;

	/**
	* Set to true if objective locations should update the objectives system when
	* this entity the objective area.
	* May also determine inventory callbacks to objective system.
	**/
	bool					m_bIsObjective;

	/**
	* Set to true if the entity is frobable.  May be dynamically changed.
	**/
	bool					m_bFrobable;

	/**
	* Frobdistance determines the distance in renderunits. If set to 0
	* the entity is not frobable.
	**/
	int						m_FrobDistance;

	/**
	* Frob bias: Multiplies the angle deviation cosine test in idPlayer::FrobCheck
	* Effectively biases the frob selection toward this object.  Used for small objects
	* To make them easier to frob when placed next to large objects.
	**/
	float					m_FrobBias;

	/**
	* Set to true if this entity is a climbable rope.  This could be set on either
	* static ropes or AF entities, so it makes sense to let all entities have this var
	**/
	bool					m_bIsClimbableRope;

public:
	ABSTRACT_PROTOTYPE( idEntity );

							idEntity();
							~idEntity();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	const char *			GetEntityDefName( void ) const;
	void					SetName( const char *name );
	const char *			GetName( void ) const;

	virtual void			UpdateChangeableSpawnArgs( const idDict *source );

							// clients generate views based on all the player specific options,
							// cameras have custom code, and everything else just uses the axis orientation
	virtual renderView_t *	GetRenderView();

	// thinking
	virtual void			Think( void );
	bool					CheckDormant( void );	// dormant == on the active list, but out of PVS
	virtual	void			DormantBegin( void );	// called when entity becomes dormant
	virtual	void			DormantEnd( void );		// called when entity wakes from being dormant
	bool					IsActive( void ) const;
	void					BecomeActive( int flags );
	void					BecomeInactive( int flags );
	void					UpdatePVSAreas( const idVec3 &pos );

	// visuals
	virtual void			Present( void );
	virtual renderEntity_t *GetRenderEntity( void );
	virtual int				GetModelDefHandle( void );
	virtual void			SetModel( const char *modelname );
	void					SetSkin( const idDeclSkin *skin );
	const idDeclSkin *		GetSkin( void ) const;
	void					SetShaderParm( int parmnum, float value );
	virtual void			SetColor( float red, float green, float blue );
	virtual void			SetColor( const idVec3 &color );
	virtual void			GetColor( idVec3 &out ) const;
	virtual void			SetColor( const idVec4 &color );
	virtual void			GetColor( idVec4 &out ) const;
	virtual void			FreeModelDef( void );
	virtual void			FreeLightDef( void );
	virtual void			Hide( void );
	virtual void			Show( void );
	bool					IsHidden( void ) const;

	// TDM: SZ: January 9, 2006 Made virtual to handle unique behavior in descendents
	virtual void			UpdateVisuals( void );

	void					UpdateModel( void );
	void					UpdateModelTransform( void );
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );
	int						GetNumPVSAreas( void );
	const int *				GetPVSAreas( void );
	void					ClearPVSAreas( void );
	bool					PhysicsTeamInPVS( pvsHandle_t pvsHandle );

	// animation
	virtual bool			UpdateAnimationControllers( void );
	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	virtual idAnimator *	GetAnimator( void );	// returns animator object used by this entity

	// sound
	virtual bool			CanPlayChatterSounds( void ) const;
	bool					StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	bool					StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	void					StopSound( const s_channelType channel, bool broadcast );	// pass SND_CHANNEL_ANY to stop all sounds
	void					SetSoundVolume( float volume );
	void					UpdateSound( void );
	int						GetListenerId( void ) const;
	idSoundEmitter *		GetSoundEmitter( void ) const;
	void					FreeSoundEmitter( bool immediate );

	// entity binding
	virtual void			PreBind( void );
	virtual void			PostBind( void );
	virtual void			PreUnbind( void );
	virtual void			PostUnbind( void );
	void					JoinTeam( idEntity *teammember );
	void					Bind( idEntity *master, bool orientated );
	void					BindToJoint( idEntity *master, const char *jointname, bool orientated );
	void					BindToJoint( idEntity *master, jointHandle_t jointnum, bool orientated );
	void					BindToBody( idEntity *master, int bodyId, bool orientated );
	void					Unbind( void );
	bool					IsBound( void ) const;
	bool					IsBoundTo( idEntity *master ) const;
	idEntity *				GetBindMaster( void ) const;
	jointHandle_t			GetBindJoint( void ) const;
	int						GetBindBody( void ) const;
	idEntity *				GetTeamMaster( void ) const;
	idEntity *				GetNextTeamEntity( void ) const;
	/**
	* TDM: Get a list of bound team members lower down on the chain than this one
	* Populates the list in the argument argument with entity pointers
	**/
	void					GetTeamChildren( idList<idEntity *> *list );
	void					ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis );
	idVec3					GetLocalVector( const idVec3 &vec ) const;
	idVec3					GetLocalCoordinates( const idVec3 &vec ) const;
	idVec3					GetWorldVector( const idVec3 &vec ) const;
	idVec3					GetWorldCoordinates( const idVec3 &vec ) const;
	bool					GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
	void					GetWorldVelocities( idVec3 &linearVelocity, idVec3 &angularVelocity ) const;
	int						GetModelDefHandle(void) const  { return modelDefHandle; }; 
	// physics
							// set a new physics object to be used by this entity
	void					SetPhysics( idPhysics *phys );
							// get the physics object used by this entity
	idPhysics *				GetPhysics( void ) const;
							// restore physics pointer for save games
	void					RestorePhysics( idPhysics *phys );
							// run the physics for this entity
	bool					RunPhysics( void );
							// set the origin of the physics object (relative to bindMaster if not NULL)
	void					SetOrigin( const idVec3 &org );
							// set the axis of the physics object (relative to bindMaster if not NULL)
	void					SetAxis( const idMat3 &axis );
							// use angles to set the axis of the physics object (relative to bindMaster if not NULL)
	void					SetAngles( const idAngles &ang );
							// get the floor position underneath the physics object
	bool					GetFloorPos( float max_dist, idVec3 &floorpos ) const;
							// retrieves the transformation going from the physics origin/axis to the visual origin/axis
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
							// retrieves the transformation going from the physics origin/axis to the sound origin/axis
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
							// called from the physics object when colliding, should return true if the physics simulation should stop
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	/**
	* TDM: Process collision stims when collisions occur
	**/
	virtual void			ProcCollisionStims( idEntity *other );
							// retrieves impact information, 'ent' is the entity retrieving the info
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
							// apply an impulse to the physics object, 'ent' is the entity applying the impulse
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
							// add a force to the physics object, 'ent' is the entity adding the force
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
							// activate the physics object, 'ent' is the entity activating this entity
	virtual void			ActivatePhysics( idEntity *ent );
							// returns true if the physics object is at rest
	virtual bool			IsAtRest( void ) const;
							// returns the time the physics object came to rest
	virtual int				GetRestStartTime( void ) const;
							// add a contact entity
	virtual void			AddContactEntity( idEntity *ent );
							// remove a touching entity
	virtual void			RemoveContactEntity( idEntity *ent );

	// damage
							// returns true if this entity can be damaged from the given origin
	virtual bool			CanDamage( const idVec3 &origin, idVec3 &damagePoint ) const;
							// applies damage to this entity
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, 
									const idVec3 &dir, const char *damageDefName, 
									const float damageScale, const int location, trace_t *tr = NULL );
	// greebo: Heals this entity using the information found in the entitDef named healDefName
	// returns 1 if the entity was healed, or 0 if the entity was already at full health
	virtual int			heal(const char* healDefName, float healScale);
							// adds a damage effect like overlays, blood, sparks, debris etc.
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName );
							// callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
							// notifies this entity that it is in pain
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
							// notifies this entity that is has been killed
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	// scripting
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual idThread *		ConstructScriptObject( void );
	virtual void			DeconstructScriptObject( void );
	void					SetSignal( signalNum_t signalnum, idThread *thread, const function_t *function );
	void					ClearSignal( idThread *thread, signalNum_t signalnum );
	void					ClearSignalThread( signalNum_t signalnum, idThread *thread );
	bool					HasSignal( signalNum_t signalnum ) const;
	void					Signal( signalNum_t signalnum );
	void					SignalEvent( idThread *thread, signalNum_t signalnum );

	// gui
	void					TriggerGuis( void );
	bool					HandleGuiCommands( idEntity *entityGui, const char *cmds );
	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );

	// targets
	void					FindTargets( void );
	void					RemoveNullTargets( void );
	void					ActivateTargets( idEntity *activator ) const;

	// misc
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );
	bool					TouchTriggers( void ) const;
	idCurve_Spline<idVec3> *GetSpline( void ) const;
	virtual void			ShowEditingDialog( void );

	enum {
		EVENT_STARTSOUNDSHADER,
		EVENT_STOPSOUNDSHADER,
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	void					WriteBindToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadBindFromSnapshot( const idBitMsgDelta &msg );
	void					WriteColorToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadColorFromSnapshot( const idBitMsgDelta &msg );
	void					WriteGUIToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGUIFromSnapshot( const idBitMsgDelta &msg );

	void					ServerSendEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) const;
	void					ClientSendEvent( int eventId, const idBitMsg *msg ) const;

	// ---===<* Darkmod functions *>===---

	/**
	 * LoadTDMSettings will initialize the settings required for 
	 * darkmod to operate. It should be called form the respective
	 * Spawn function on each object as there is no common function
	 * that is called on Spawn for all entities. If a class needs
	 * to load additional settings, which are only of interest to this
	 * particular class it should override this virtual function but
	 * don't forget to call it for the base class.
	 */
	virtual void			LoadTDMSettings(void);

	/**
	 * Frobaction will determine what a particular item should do when an entity is highlighted.
	 * The actual action depends on the type of the entity.
	 * Loot is being picked up and counted to the loot.
	 * Special items are transfered to the inventory. If the item is also loot it will count 
	 *    to that as well.
	 * Doors are tested for their state (locked/unlocked) and opened if unlocked. if the 
	 *    lockpicks or an appropriate key is equipped the door will either unlock or the
	 *    lockpick HUD should appear.
	 * Windows are just smaller doors so they don't need special treatment.
	 * AI is tested for it's state. If it is an ally some defined script should start. If
	 *    it is unconscious or dead it is picked up and the player can carry it around.
	 * If it is a movable item the item is picked up and the player can carry it around.
	 * Switches are flipped and/or de-/activated and appropriate scripts are triggered.
	 * bMaster indicates wheter the entity should call it's master or not.
	 */
	virtual void FrobAction(bool bMaster, bool bPeer = false);

	/**
	 * AddToMasterList adds a string entry to a list and checks if a) the new entry
	 * is not the current entities name and b) if the name already exists in the list.
	 * If both conditions are met, the name is added to the list and true is returned,
	 * otherwise false is returned and the name is not added to the list.
	 */
	bool AddToMasterList(idList<idStr> &, idStr &name);

	/**
	 * UsedBy determines the behaviour when an entity is used against another one.
	 * The entity passed in as an argument is the one that uses this entity. If the 
	 * argument is NULL, then the called entity is the one being used.
	 * The return value indicates if the item could be used. If false the item is not
	 * appropriate for usage on that entity and the default frobaction will be executed
	 * instead.
	 * bInit should be set to true if the Use just started. In case of continuous uses
	 * like using the lockpíck), the feirst call when the USE button is pressed, this
	 * would be set to true and in subsequent calls it would be set to false.
	 */
	virtual bool UsedBy(bool bInit, idEntity *);

	/**
	 * Parses a used_by string. For a detailed information on how to use this feature
	 * refer to the m_UsedBy description.
	 */
	void ParseUsedByList(idList<idStr> &, idStr &);

	/**
	* Toggle whether the entity has been frobbed.  Should ONLY be called by idPlayer::CheckFrob
	**/
	virtual void SetFrobbed( bool val );

	/**
	* Return whether the entity is currently frobbed.
	* Should be false at the beginning of the frame
	**/
	virtual bool IsFrobbed( void );

	/**
	* DarkMod sound prop functions (called by StartSound and StopSound)
	**/

	/**
	* The following two functions propagate either a suspicious or
	* an environmental sound.  They are called by idEntity::StartSound.
	*
	* The parameters are the local sound name (ie, the name in the entity
	* def file, INCLUDING the "snd_" at the beginning), and the global sound
	* name (name in the soundprop def file, NOT including the prefix)
	*
	* (For now sounds can be suspicious or environmental, but not both.
	*  defining a sound as suspicious overrides any environmental def
	*  with the same sound name. )
	**/

	/**
	* Propagate a suspicious sound
	**/
	void PropSoundS( const char *localName, const char *globalName, float VolModIn = 0.0f );

	/**
	* Propagate an environmental sound (called by PropSound)
	**/
	void PropSoundE( const char *localName, const char *globalName, float VolModIn = 0.0f );

	/**
	* Propagate a sound directly, outside of StartSound
	* Direct calling should be done in cases where the gamecode calls
	* StartSoundShader directly, without going thru PlaySound.
	* 
	* PropSoundDirect first looks for a local definition of the sound
	* on the entity, to find volume/duration modifier, extra flags
	* and the "global" sound definition (in the sound def file)
	*
	* If the local definition is not found, it calls sound prop
	* with the unmodified global definition.
	*
	* VolModIn is a modifier in dB added to the volume, in addition to 
	* any modifier that might be present in the local sound def.
	**/
	void PropSoundDirect( const char *sndName, bool bForceLocal = false, 
						  bool bAssumeEnv = false, float VolModIn = 0.0f );

	CStimResponseCollection *GetStimResponseCollection(void) { return m_StimResponseColl; };

	/**
	 * Set the item this entity is currently pointing to. If the item is 
	 * destroyed, the pointer must be set to NULL.
	 */
	void					SetInventoryItem(CInventoryItem *item) { m_InventoryItem = item; }
	// Returns (and creates if necessary) this entity's inventory item.
	CInventoryItem*		InventoryItem() { return m_InventoryItem; };

	// Returns (and creates if necessary) this entity's inventory.
	CInventory*			Inventory();
	// Returns (and creates if necessary) this entity's inventory cursor.
	CInventoryCursor*	InventoryCursor();

	/**
	 * InitInventory reads the spawndata and puts this entity into the inventory
	 * of the target entity, defined by the def or map argument inv_owner.
	 * If the target entity does not yet exist, it will put itself into a list
	 * on game_local, and each entity will check wether there are inventory items for itself.
	 * if this is the case InitInventory will be called again, because at this point
	 * the target definitely exists.
	 */
	void InitInventory(void);

	/**
	 * CheckInventoryInit is the counterpart for InitInventory. It will check wether
	 * some entities exists that have to put in it's own inventory. This is only needed
	 * when entities are spawning out of order.
	 */
	void CheckInventoryInit(void);

	/**
	 * CreateOverlay will create an overaly for the given entity. This is a hud element
	 * that displays some objects on the screen. The default is for the playerscreen
	 * to show the inventory items and other stats. However, each entity can have his own
	 * hud which allows him to add a custom display for inventory items.
	 */
	void					Event_CreateOverlay( const char *guiFile, int layer );
	int						CreateOverlay( const char *guiFile, int layer );

	/**
	* greebo: DestroyOverlay removes the overlay from this entity. Pass the handle
	*		  as argument, it's the <int> returned by CreateOverlay.
	*/
	void					Event_DestroyOverlay(int handle);
	void					DestroyOverlay(int handle);

	/**
	 * Generic function for calling a scriptfunction with arbitrary arguments.
	 * The the thread is returned or NULL.
	 */
	idThread *CallScriptFunctionArgs(const char *ScriptFunction, bool ClearStack, int Delay, const char *Format, ...);

	/**
	* Attach another entity to this entity.  The base version assumes single-clipmodel
	* rigid body physics and simply calls idEntity::Bind.  
	* Will be overloaded in derived classes with joints to call BindToJoint.
	**/
	virtual void Attach( idEntity *ent );
	
	/**
	* Called when the given entity is about to attach (bind) to this entity.
	* Does not actually bind it.
	**/
	virtual void BindNotify( idEntity *ent );
	
	/**
	* Called when the given entity is about to detach (unbind) from this entity.
	* Does not actually unbind it.
	**/
	virtual void UnbindNotify( idEntity *ent );

	/**
	 * GetSignalId assignes a unique Id to be used in a signal function. To differentiate these
	 * from the script signals I named them SDKSignal. These signal mechanism can be used for both.
	 * So you can have a SDK function signaling, or a script. You can not signal a script, though, only
	 * from a signal to SDK. To signal a script use the regular signal functions.
	 */
	SDK_SIGNAL			GetSDKSignalId(void);

	/**
	 * Adds a new signal funktion that can be triggered. The object pointer must always be set. 
	 * The datapointer can be NULL and would contain a pointer to data that the signal function
	 * may need. If the SIGNAL is 0 an error occured and the signal was not added to the list.
	 */
	SDK_SIGNAL		AddSDKSignal(E_SDK_SIGNAL_STATE (*oFkt)(idEntity *oObject, void *pData), void *pData);
	void			CheckSDKSignal(void);

	/**
	 * SDKSignal will trigger a signal that a script has finished.
	 */
	void SDKSignal(SDK_SIGNAL SDKSignalId, int bState);

	/**
	 * Return nonzero if this entity could potentially attack the given (target) entity at range,
	 * or entities in general if target is NULL.
	 */
	virtual float			RangedThreatTo(idEntity* target);

	/**
	 * AddToInventory will add an entity to the inventory. The item is only
	 * added if the appropriate spawnargs are set, otherwise it will be rejected
	 * and NULL is returned.
	 *
	 * If the hud parameter is not null, then it will also call the updatefunction.
	 */
	virtual CInventoryItem *AddToInventory(idEntity *ent, idUserInterface *_hud = NULL);

	/**
	 * Script event: Changes the inventory count of the given item <name> in <category> by <amount>
	 */
	void ChangeInventoryItemCount(const char* invName, const char* invCategory, int amount);

	/**
	 * Return true if this entity can be mantled, false otherwise.
	 */
	virtual bool			IsMantleable();

protected:
	/**
	* Update frob highlighting and frob entity if frobbed.
	* Also stops highlighting and clears itself from the frob entity
	* if it's no longer frobbed.
	* Depends on frob variables set in idPlayer::Think()
	* Called in idEntity::Present
	**/
	void UpdateFrob( void );

	/**
	* Used to fade frob highlight in and out smoothly
	* Called in idEntity::Present().
	**/
	void UpdateFrobDisplay( void );

	/**
	* Activate/deactivate frob highlighting on an entity
	* Also calls this on any of the entity's peers
	**/
	void FrobHighlight( bool bVal );

	/**
	* Bind to the same object that the "other" argument is bound to
	**/
	void					Event_CopyBind( idEntity *other );

	/**
	* Returns the bindmaster
	**/
	void					Event_GetBindMaster( void );

	/**
	* Return the number of bind team members lower in the chain than this one
	**/
	void					Event_NumBindChildren( void );
	/**
	* Return the ind_th bind team member
	**/
	void					Event_GetBindChild( int ind );

	CStim					*AddStim(int Type, float Radius = 0.0f, bool Removable = true, bool Default = false);
	CResponse				*AddResponse(int Type, bool Removable = true, bool Default = false);

	/**
	 * RemoveStim/Response removes the given stim. If the entity has no 
	 * stims/responses left, it is also removed from the global list in gameLocal.
	 * 
	 */
	void					RemoveStim(int Type);
	void					RemoveResponse(int Type);

protected:
	renderEntity_t			renderEntity;						// used to present a model to the renderer
	int						modelDefHandle;						// handle to static renderer model
	refSound_t				refSound;							// used to present sound to the audio engine

	/**
	* Used to keep track of the GUIs used by this entity.
	**/
	COverlaySys				m_overlays;

	/**
	* This is set by idPlayer if the player is looking at the entity in a way
	* that will frob it.
	**/
	bool						m_bFrobbed;

	/**
	* This is separate from m_bFrobbed due to peer frob highlighting,
	* to let an entity display the highlight when not frobbed.
	**/
	bool						m_bFrobHighlightState;

	/**
	* Records the frame number of when the ent was frob peer flooded
	* Also counts an object hilighting itself as peer flooding
	**/
	int							m_FrobPeerFloodFrame;

	/**
	* Timestamp indicating when the frob highlight last changed
	* Used for continuous fade in and fade out, distinct from
	* m_FrobPeerFloodFrame which is used for flooding the frob to peers.
	**/
	int							m_FrobChangeTime;

	/**
	 * FrobActionScript will contain the name of the script that is to be
	 * exected whenever a frobaction occurs. The default should be set by
	 * the constructor of the respective derived class but can be overriden
	 * by the property "frob_action_script" in the entity defintion file.
	 */
	idStr						m_FrobActionScript;

	/**
	 * FrobPeers lists the names of other entites that will also be
	 * frobbed when this entity is frobbed.
	 */
	idStrList					m_FrobPeers;

	/**
	 * If AssociatedFrob is set then MasterFrob contains
	 * the parent of the entity  chain in order to be able to walk the chain in
	 * both directions. The user can frob any object in the chain but all of them
	 * have to be notified.
	 */
	idStr					m_MasterFrob;

	/**
	 * Froblist is the list of entities that should be notified when this entity
	 * is frobbed. Each entity in this list is called as if it were the one being
	 * frobbed.
	 */
	idList<idStr>				m_FrobList;

	CStimResponseCollection		*m_StimResponseColl;

	SDK_SIGNAL				m_Signal;
	idList<SDKSignalInfo *>	m_SignalList;

	bool					m_bIsMantleable;

	/** Used to implement waitForRender()...
	 *	This merely contains a bounding box and a callback.
	 */
	renderEntity_t			m_renderTrigger;
	/// The render world handle for m_renderTrigger.
	int						m_renderTriggerHandle;
	///	The thread that's waiting for this entity to render. (via waitForRender())
	int						m_renderWaitingThread;

	/**	Called when m_renderTrigger is rendered.
	 *	It will resume the m_renderWaitingThread.
	 */
	static bool				WaitForRenderTriggered( renderEntity_s* renderEntity, const renderView_s* view );
	/**	Called to update m_renderTrigger after the render entity is modified.
	 *	Only updates the render trigger if a thread is waiting for it.
	 */
	void					PresentRenderTrigger();

	/**
	* Set and get whether the entity is frobable
	* Note: IsFrobable is only used by scripting, since we can check public var m_bFrobable
	**/
	void					Event_IsFrobable( void );
	void					Event_SetFrobable( bool bVal );
	void					Event_IsHilighted( void );

private:
	idPhysics_Static		defaultPhysicsObj;					// default physics object
	idPhysics *				physics;							// physics used for this entity
	idEntity *				bindMaster;							// entity bound to if unequal NULL
	jointHandle_t			bindJoint;							// joint bound to if unequal INVALID_JOINT
	int						bindBody;							// body bound to if unequal -1
	idEntity *				teamMaster;							// master of the physics team
	idEntity *				teamChain;							// next entity in physics team

	int						numPVSAreas;						// number of renderer areas the entity covers
	int						PVSAreas[MAX_PVS_AREAS];			// numbers of the renderer areas the entity covers

	signalList_t *			signals;

	int						mpGUIState;							// local cache to avoid systematic SetStateInt

	// A pointer to our inventory.
	CInventory				*m_Inventory;

	// A pointer to our item, so that we can be added/removed to/from inventories.
	CInventoryItem			*m_InventoryItem;

	// A pointer to our cursor - the cursor is for arbitrary use, and may not point to our own inventory.
	CInventoryCursor		*m_InventoryCursor;

private:
	void					FixupLocalizedStrings();

	bool					DoDormantTests( void );				// dormant == on the active list, but out of PVS

	// physics
							// initialize the default physics
	void					InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis );
							// update visual position from the physics
	void					UpdateFromPhysics( bool moveBack );

	// entity binding
	bool					InitBind( idEntity *master );		// initialize an entity binding
	void					FinishBind( void );					// finish an entity binding
	void					RemoveBinds( void );				// deletes any entities bound to this object
	void					QuitTeam( void );					// leave the current team

	void					UpdatePVSAreas( void );

public:			// Events should be public, so they can be used from other places as well.
	// events
	void					Event_GetName( void );
	void					Event_SetName( const char *name );
	void					Event_IsType ( const char *pstr_typeName );
	void					Event_FindTargets( void );
	void					Event_ActivateTargets( idEntity *activator );
	void					Event_NumTargets( void );
	void					Event_GetTarget( float index );
	void					Event_RandomTarget( const char *ignore );
	void					Event_Bind( idEntity *master );
	void					Event_BindPosition( idEntity *master );
	void					Event_BindToJoint( idEntity *master, const char *jointname, float orientated );
	void					Event_Unbind( void );
	void					Event_RemoveBinds( void );
	void					Event_SpawnBind( void );
	void					Event_SetOwner( idEntity *owner );
	void					Event_SetModel( const char *modelname );
	void					Event_SetSkin( const char *skinname );
	void					Event_GetShaderParm( int parmnum );
	void					Event_SetShaderParm( int parmnum, float value );
	void					Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_SetColor( float red, float green, float blue );
	void					Event_GetColor( void );
	void					Event_IsHidden( void );
	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_CacheSoundShader( const char *soundName );
	void					Event_StartSoundShader( const char *soundName, int channel );
	void					Event_StopSound( int channel, int netSync );
	void					Event_StartSound( const char *soundName, int channel, int netSync );
	void					Event_FadeSound( int channel, float to, float over );
	void					Event_GetWorldOrigin( void );
	void					Event_SetWorldOrigin( idVec3 const &org );
	void					Event_GetOrigin( void );
	void					Event_SetOrigin( const idVec3 &org );
	void					Event_GetAngles( void );
	void					Event_SetAngles( const idAngles &ang );
	void					Event_SetLinearVelocity( const idVec3 &velocity );
	void					Event_GetLinearVelocity( void );
	void					Event_SetAngularVelocity( const idVec3 &velocity );
	void					Event_GetAngularVelocity( void );
	void					Event_SetSize( const idVec3 &mins, const idVec3 &maxs );
	void					Event_GetSize( void );
	void					Event_GetMins( void );
	void					Event_GetMaxs( void );
	void					Event_Touches( idEntity *ent );
	void					Event_GetNextKey( const char *prefix, const char *lastMatch );
	void					Event_SetKey( const char *key, const char *value );
	void					Event_GetKey( const char *key );
	void					Event_GetIntKey( const char *key );
	void					Event_GetFloatKey( const char *key );
	void					Event_GetVectorKey( const char *key );
	void					Event_GetEntityKey( const char *key );
	void					Event_RemoveKey( const char *key );
	void					Event_RestorePosition( void );
	void					Event_UpdateCameraTarget( void );
	void					Event_DistanceTo( idEntity *ent );
	void					Event_DistanceToPoint( const idVec3 &point );
	void					Event_StartFx( const char *fx );
	void					Event_WaitFrame( void );
	void					Event_Wait( float time );
	void					Event_HasFunction( const char *name );
	void					Event_CallFunction( const char *name );
	void					Event_SetNeverDormant( int enable );

	void					Event_InPVS( void );
	void					Event_WaitForRender( void );

	/**
	* greebo: This is the method that can be invoked from scripts. It basically
	* passes the call to the member method Damage().
	*
	* This Deals damage to this entity (gets translated into the idEntity::Damage() method within the SDK).
	*
	* @inflictor: This is the entity causing the damage (maybe a projectile)
	* @attacker: This is the "parent" entity of the inflictor, the one that is responsible for the inflictor (can be the same)
	* @dir: The direction the attack is coming from.
	* @damageDefName: The name of the damage entityDef to know what damage is being dealt to <self> (e.g. "damage_lava")
	* @damageScale: The scale of the damage (pass 1.0 as default, this should be ok).
	*/
	void					Event_Damage( idEntity *inflictor, idEntity *attacker, 
										  const idVec3 &dir, const char *damageDefName, 
										  const float damageScale );
	/**
	* greebo: This method heals this entity using the values defined in the
	* the entityDef specified by <healDefName>.
	*/
	void					Event_Heal( const char *healDefName, const float healScale );
	void					Event_SetGui( int handle, const char *guiFile );
	void					Event_GetGui( int handle );
	void					Event_SetGuiString( int handle, const char *key, const char *val );
	void					Event_GetGuiString( int handle, const char *key );
	void					Event_SetGuiFloat( int handle, const char *key, float f );
	void					Event_GetGuiFloat( int handle, const char *key );
	void					Event_SetGuiInt( int handle, const char *key, int n );
	void					Event_GetGuiInt( int handle, const char *key );
	void					Event_SetGuiStringFromKey( int handle, const char *key, idEntity *src, const char *spawnArg );
	void					Event_CallGui( int handle, const char *namedEvent );

	void					Event_LoadExternalData( const char *xdFile, const char* prefix );

	void					Event_GetInventory();
	void					Event_ReplaceItem(idEntity *old_item, idEntity *new_item);
	void					Event_GetNextItem(int WrapAround);
	void					Event_GetPrevItem(int WrapAround);
	void					Event_SetCursorGroup(const char *groupname);
	void					Event_SetCursorGroupItem(const char *itemname, const char *groupname);
	void					Event_SetCursorItem(const char *itemname);
	void					Event_GetCursorGroup(void);
	void					Event_GetCursorItem(void);
	void					Event_AddItem(idEntity *item);
	void					Event_GetGroupItem(const char *name, const char *group);
	void					Event_GetItem(const char *name);
	void					Event_GetLoot(int LootType);
	virtual void			inventoryChangeSelection(idUserInterface *_hud, bool bUpdate = false, CInventoryItem *Prev = NULL);

	void					StimAdd(int Type, float Radius);
	void					StimRemove(int Type);
	void					StimEnable(int Type, int State);
	void					StimClearIgnoreList (int Type);
	void					ResponseAdd(int Type);
	void					ResponseRemove(int Type);
	void					ResponseEnable(int Type, int State);

	/**
	* This triggers a stand-alone response (without an actual Stim) on this entity.
	*
	* @source: The entity that caused the response.
	* @stimType: the according stim index (e.g. ST_FROB)
	*/
	void					ResponseTrigger(idEntity* source, int stimType);

	// Add/Remove the response to/from the stim with the given type
	void					ResponseIgnore(int StimType, idEntity *);
	void					ResponseAllow(int StimType, idEntity *);
	void					ResponseSetAction(int StimType, const char *Action);

	void					Event_TimerCreate(int StimType, int Hour, int Minute, int Seconds, int Milisecond);
	void					Event_TimerStop(int StimType);
	void					Event_TimerStart(int StimType);
	void					Event_TimerRestart(int StimType);
	void					Event_TimerReset(int StimType);
	void					Event_TimerSetState(int StimType, int State);

	/**
	* Used to propagate a sound directly via scripting, without playing the audible sound
	* VolModIn is a volume modifier added to the sound's volume.
	**/
	void					Event_PropSoundMod( const char *sndName, float VolModIn = 0.0 );

	void					Event_PropSound( const char *sndName );

#ifdef MOD_WATERPHYSICS

	void					Event_GetMass( int body );	// MOD_WATERPHYSICS

	void					Event_IsInLiquid( void );	// MOD_WATERPHYSICS

#endif		// MOD_WATERPHYSICS
	
	void					Event_RangedThreatTo(idEntity*);

	/**
	* greebo: Returns true if the target entity can be seen 
	*
	* @useLighting: If set to TRUE, takes lighting into account.
	*/
	bool					canSeeEntity(idEntity* target, int useLighting);

	/**
	* greebo: script event wrapper for the above canSeeEntity() method.
	*/
	void					Event_CanSeeEntity(idEntity* target, int useLighting);

public:			// events that need to have an accessible counterpart
	void					SetGuiString(int handle, const char *key, const char *val);
	const char				*GetGuiString(int handle, const char *key);
	void					SetGuiFloat(int handle, const char *key, float f);
	float					GetGuiFloat(int handle, const char *key);
	void					SetGuiInt(int handle, const char *key, int n);
	int						GetGuiInt(int handle, const char *key);
	void					CallGui(int handle, const char *namedEvent);
};

/*
===============================================================================

	Animated entity base class.
	
===============================================================================
*/

typedef struct damageEffect_s {
	jointHandle_t			jointNum;
	idVec3					localOrigin;
	idVec3					localNormal;
	int						time;
	const idDeclParticle*	type;
	struct damageEffect_s *	next;
} damageEffect_t;

class idAnimatedEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idAnimatedEntity );

							idAnimatedEntity();
							~idAnimatedEntity();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			ClientPredictionThink( void );
	virtual void			Think( void );

	void					UpdateAnimation( void );

	virtual idAnimator *	GetAnimator( void );
	virtual void			SetModel( const char *modelname );

	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int currentTime, idVec3 &offset, idMat3 &axis ) const;

	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName );
	void					AddLocalDamageEffect( jointHandle_t jointNum, const idVec3 &localPoint, const idVec3 &localNormal, const idVec3 &localDir, const idDeclEntityDef *def, const idMaterial *collisionMaterial );
	void					UpdateDamageEffects( void );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	/**
	* Overloads idEntity::Attach to bind to a joint
	**/
	virtual void			Attach( idEntity *ent );

	enum {
		EVENT_ADD_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

protected:
	idAnimator				animator;
	damageEffect_t *		damageEffects;

private:
	void					Event_GetJointHandle( const char *jointname );
	void 					Event_ClearAllJoints( void );
	void 					Event_ClearJoint( jointHandle_t jointnum );
	void 					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void 					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void 					Event_GetJointAngle( jointHandle_t jointnum );
};

#endif /* !__GAME_ENTITY_H__ */
