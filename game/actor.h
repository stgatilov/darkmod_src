/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

/*
===============================================================================

	idActor

===============================================================================
*/

extern const idEventDef AI_EnableEyeFocus;
extern const idEventDef AI_DisableEyeFocus;
extern const idEventDef EV_Footstep;
extern const idEventDef EV_FootstepLeft;
extern const idEventDef EV_FootstepRight;
extern const idEventDef EV_EnableWalkIK;
extern const idEventDef EV_DisableWalkIK;
extern const idEventDef EV_EnableLegIK;
extern const idEventDef EV_DisableLegIK;
extern const idEventDef AI_SetAnimPrefix;
extern const idEventDef AI_PlayAnim;
extern const idEventDef AI_PlayCycle;
extern const idEventDef AI_AnimDone;
extern const idEventDef AI_SetBlendFrames;
extern const idEventDef AI_GetBlendFrames;

class idDeclParticle;

class idAnimState {
public:
	bool					idleAnim;
	idStr					state;
	int						animBlendFrames;
	int						lastAnimBlendFrames;		// allows override anims to blend based on the last transition time

public:
							idAnimState();
							~idAnimState();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Init( idActor *owner, idAnimator *_animator, int animchannel );
	void					Shutdown( void );
	void					SetState( const char *name, int blendFrames );
	void					SetFrame( int anim, int frame );
	void					StopAnim( int frames );
	void					PlayAnim( int anim );
	void					CycleAnim( int anim );
	void					BecomeIdle( void );
	bool					UpdateState( void );
	bool					Disabled( void ) const;
	void					Enable( int blendFrames );
	void					Disable( void );
	bool					AnimDone( int blendFrames ) const;
	bool					IsIdle( void ) const;
	animFlags_t				GetAnimFlags( void ) const;
	idAnimator*				GetAnimator();
	void					FinishAction(const idStr& actionname);
	const char *			WaitState( void ) const;
	void					SetWaitState( const char *_waitstate );

private:
	idStr					waitState;
	idActor *				self;
	idAnimator *			animator;
	idThread *				thread;
	int						channel;
	bool					disabled;
};

typedef struct {
	jointModTransform_t		mod;
	jointHandle_t			from;
	jointHandle_t			to;
} copyJoints_t;

class idActor : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idActor );

	int						team;
	int						rank; // monsters don't fight back if the attacker's rank is higher
	/**
	* TDM: Defines the type of the AI (human, beast, undead, bot, etc)
	**/
	int						m_AItype;
	/**
	* TDM: Whether this actor is considered a non-combatant
	**/
	bool					m_Innocent;
	idMat3					viewAxis;			// view axis of the actor

	idLinkList<idActor>		enemyNode;			// node linked into an entity's enemy list for quick lookups of who is attacking him
	idLinkList<idActor>		enemyList;			// list of characters that have targeted the player as their enemy

public:
							idActor( void );
	virtual					~idActor( void );

	void					Spawn( void );
	virtual void			Restart( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	virtual bool			LoadAF( void );
	void					SetupBody( void );

	void					CheckBlink( void );

	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

							// script state management
	void					ShutdownThreads( void );
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual idThread *		ConstructScriptObject( void );
	void					UpdateScript( void );
	const function_t		*GetScriptFunction( const char *funcname );
	void					SetState( const function_t *newState );
	void					SetState( const char *statename );
	
							// vision testing
	void					SetEyeHeight( float height );
	float					EyeHeight( void ) const;
	idVec3					EyeOffset( void ) const;
	idVec3					GetEyePosition( void ) const;
	virtual void			GetViewPos( idVec3 &origin, idMat3 &axis ) const;
	/**
	* Sets the actor's field of view (doesn't apply to players)
	* Input is horizontal FOV, vertical FOV.
	* If no vertical is provided, it is assumed the same as the vertical
	*
	* Sets the FOV dot products to those of the half-angles of the full fov
	**/
	void					SetFOV( float fovHoriz, float fovVert = -1 );
	virtual bool			CheckFOV( const idVec3 &pos ) const;

	/**
	 * greebo: This method returns TRUE, if the given entity <ent> is visible by this actor.
	 *         For actor entities, the eye position is taken into account, for ordinary entities,
	 *         the origin is taken.
	 *         First, the origin/eye position is checked against the FOV of this actor, and second,
	 *         a trace is performed from eye (this) to origin/eye (other entity). If the trace is
	 *         blocked, the entity is considered hidden and the method returns FALSE.
	 */
	virtual bool			CanSee( idEntity *ent, bool useFOV ) const;
	bool					PointVisible( const idVec3 &point ) const;
	virtual void			GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos );

							// damage
	void					SetupDamageGroups( void );
	// DarkMod: Added trace reference to damage
	virtual	void			Damage
							( 
								idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName, const float damageScale, const int location,
								trace_t *collision = NULL
							);

	/****************************************************************************************
	=====================
	idActor::CrashLand
	handle collision(Falling) damage to AI/Players
	Added by Richard Day
	=====================
	****************************************************************************************/
	virtual float		    CrashLand( const idPhysics_Actor& physicsObj, const idVec3 &oldOrigin, const idVec3 &oldVelocity );
	
	int						GetDamageForLocation( int damage, int location );
	const char *			GetDamageGroup( int location );
	void					ClearPain( void );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

							// model/combat model/ragdoll
	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );
	bool					StartRagdoll( void );
	void					StopRagdoll( void );
	virtual bool			UpdateAnimationControllers( void );

							// delta view angles to allow movers to rotate the view of the actor
	const idAngles &		GetDeltaViewAngles( void ) const;
	void					SetDeltaViewAngles( const idAngles &delta );

	bool					HasEnemies( void ) const;
	idActor *				ClosestEnemyToPoint( const idVec3 &pos );
	idActor *				EnemyWithMostHealth();

	virtual bool			OnLadder( void ) const;

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;

	/**
	* Called when the given ent is about to be bound/attached to this actor.
	**/
	void					BindNotify( idEntity *ent );
	
	/**
	* Called when the given ent is about to be unbound/detached from this actor.
	**/
	virtual void			UnbindNotify( idEntity *ent );

	/**
	* Attach an entity.  Entity spawnArgs checked for attachments are:
	* "origin", "angles", and "joint".
	* AttName is the optional name of the attachment for indexing purposes (e.g., "melee_weapon")
	* Ent is the entity being attached
	* PosName is the optional position name to attach to.
	**/
	virtual void			Attach( idEntity *ent, const char *PosName = NULL, const char *AttName = NULL );

	/**
	* Reattach an existing attachment
	* Effects the attachment at index ind (where the first attachment starts at 1)
	* For example to effect the first item that was attached, ind should be = 1
	* 
	* The next arguments are the name of the joint to attach to, the translation
	* offset from that joint, and a (pitch, yaw, roll) angle vector that defines the 
	* rotation of the attachment relative to the joint's orientation.
	**/
	void ReAttach( int ind, idStr jointName, idVec3 offset, idAngles angles );

	/**
	* Show or hide an attachment.  Index works the same as in ReAttach described above.
	**/
	void ShowAttachment( int ind, bool bShow );

	/**
	* Drop an attached item.  This is different from def_drop dropping
	* Because the entity is actually the same one that's attached,
	* it just gets unbound and falls to the ground from its current position.
	**/
	void DropAttachment( int ind );

	/**
	* Store the attachment info in the argument references given.
	* Returns false if the attachment index was invalid.
	* (Note: The attachment index starts at 1)
	**/
	bool GetAttachInfo( int ind, idStr &joint, idVec3 &offset, idAngles &angles );

	/**
	* Retrieve the attached entity at index ind.  NOTE: ind starts at 1, not zero
	* Returns NULL if ind or entity is invalid.
	**/
	idEntity *GetAttachedEnt( int ind );

	/**
	 * greebo: Returns the number of attached melee weapons. These are the entities
	 *         with the according spawnarg "is_weapon_melee" etc. set to "1".
	 */
	int GetNumMeleeWeapons();
	int GetNumRangedWeapons();

	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

	virtual	renderView_t *	GetRenderView();	
	
							// animation state control
	int						GetAnim( int channel, const char *name );
	idAnimator*				GetAnimatorForChannel(int channel);
	const char*				LookupReplacementAnim( const char *name );
	void					StopAnim(int channel, int frames);
	void					UpdateAnimState( void );
	void					SetAnimState( int channel, const char *name, int blendFrames );
	const char *			GetAnimState( int channel ) const;
	bool					InAnimState( int channel, const char *name ) const;
	const char *			WaitState( void ) const;
	const char *			WaitState( int channel ) const;
	void					SetWaitState( const char *_waitstate );
	void					SetWaitState( int channel, const char *_waitstate );
	bool					AnimDone( int channel, int blendFrames ) const;
	virtual void			SpawnGibs( const idVec3 &dir, const char *damageDefName );

	/**
	* Returns the modification to movement volume based on the movement type
	* (crouch walk, creep, run, etc)
	* Called in derived classes idPlayer and idAI.
	**/
	virtual float			GetMovementVolMod( void ) { return 0; };

	virtual bool			IsKnockedOut( void ) { return false; };

	/** 
	 * greebo: This gets called by the pathing routine to let the actor
	 * reconsider the "forbidden" status of the given area. After some time
	 * AI should be able to re-try opening locked doors otherwise the door's AAS area
	 * stays flagged as "forbidden" for all times.
	 *
	 * Note: This method is overriden by the AI class.
	 *
	 * @returns: TRUE if the area is now considered as "allowed", FALSE otherwise.
	 */
	virtual bool			ReEvaluateArea(int areaNum);

protected:

	/*	CrashLand variables	Added by Richard Day	*/

	float m_delta_fatal; ///< any value above this is death
	float m_delta_scale; ///< scale the damage based on this. delta is divide by this
	float m_delta_min;   ///< min delta anything at or below this does 0 damage
	

	friend class			idAnimState;

	float					m_fovDotHoriz;		// cos( Horizontal fov [degrees] )
	float					m_fovDotVert;		// cos( Vertical fov [degrees] )
	idVec3					eyeOffset;			// offset of eye relative to physics origin
	idVec3					modelOffset;		// offset of visual model relative to the physics origin
	idVec3					mHeadModelOffset;	// offset for the head

	idAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						pain_debounce_time;	// next time the actor can show pain
	int						pain_delay;			// time between playing pain sound
	int						pain_threshold;		// how much damage monster can take at any one time before playing pain animation

	idStrList				damageGroups;		// body damage groups
	idList<float>			damageScale;		// damage scale per damage gruop

	bool						use_combat_bbox;	// whether to use the bounding box for combat collision
	idEntityPtr<idAFAttachment>	head;
	idList<copyJoints_t>		copyJoints;			// copied from the body animation to the head model

	// state variables
	const function_t		*state;
	const function_t		*idealState;
	
protected:

	// joint handles
	jointHandle_t			leftEyeJoint;
	jointHandle_t			rightEyeJoint;
	jointHandle_t			soundJoint;

	idIK_Walk				walkIK;

	idStr					animPrefix;
	idStr					painAnim;

	// blinking
	int						blink_anim;
	int						blink_time;
	int						blink_min;
	int						blink_max;

	// script variables
	idThread *				scriptThread;
	idStr					waitState;
	idAnimState				headAnim;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;

	bool					allowPain;
	bool					allowEyeFocus;
	bool					finalBoss;

	int						painTime;

	idList<CAttachInfo>	m_attachments;
	
	// Maps animation names to the names of their replacements
	idDict					m_replacementAnims;

	/**
	* Movement volume modifiers.  Ones for the player are taken from 
	* cvars (for now), ones for AI are taken from spawnargs.
	* Walking and not crouching is the default volume.
	**/

	float					m_stepvol_walk;
	float					m_stepvol_run;
	float					m_stepvol_creep;

	float					m_stepvol_crouch_walk;
	float					m_stepvol_crouch_creep;
	float					m_stepvol_crouch_run;

	virtual void			Gib( const idVec3 &dir, const char *damageDefName );

							// removes attachments with "remove" set for when character dies
	void					RemoveAttachments( void );

							// copies animation from body to head joints
	void					CopyJointsFromBodyToHead( void );

	/**
	* Updates the volume offsets for various movement modes
	* (eg walk, run, creep + crouch ).  
	* Used by derived classes idPlayer and idAI.
	**/
	virtual void			UpdateMoveVolumes( void ) {};

	/**
	* TestKnockoutBlow, only defined in derived classes
	* Returns true if going from conscious to unconscious
	**/
	virtual bool TestKnockoutBlow( idEntity* attacker, idVec3 dir, trace_t *tr, bool bIsPowerBlow ) {return false;} ;

	/**
	 * greebo: Plays the footstep sound according to the current movement type.
	 *         Note: AI and Player are overriding this method.
	 */
	virtual void PlayFootStepSound(); // empty default implementation

private:
	void					SyncAnimChannels( int channel, int syncToChannel, int blendFrames );
	void					FinishSetup( void );
	void					SetupHead( void );

public:
	void					Event_EnableEyeFocus( void );
	void					Event_DisableEyeFocus( void );
	void					Event_Footstep( void );
	void					Event_EnableWalkIK( void );
	void					Event_DisableWalkIK( void );
	void					Event_EnableLegIK( int num );
	void					Event_DisableLegIK( int num );
	void					Event_SetAnimPrefix( const char *name );
	void					Event_LookAtEntity( idEntity *ent, float duration );
	void					Event_PreventPain( float duration );
	void					Event_DisablePain( void );
	void					Event_EnablePain( void );
	void					Event_GetPainAnim( void );
	void					Event_StopAnim( int channel, int frames );
	void					Event_PlayAnim( int channel, const char *name );
	void					Event_PlayCycle( int channel, const char *name );
	void					Event_IdleAnim( int channel, const char *name );
	void					Event_SetSyncedAnimWeight( int channel, int anim, float weight );
	void					Event_OverrideAnim( int channel );
	void					Event_EnableAnim( int channel, int blendFrames );
	void					Event_DisableAnimchannel( int channel );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_AnimState( int channel, const char *name, int blendFrames );
	void					Event_GetAnimState( int channel );
	void					Event_InAnimState( int channel, const char *name );
	void					Event_FinishAction( const char *name );
	void					Event_FinishChannelAction(int channel, const char *name );
	void					Event_AnimDone( int channel, int blendFrames );
	void					Event_HasAnim( int channel, const char *name );
	void					Event_CheckAnim( int channel, const char *animname );
	void					Event_ChooseAnim( int channel, const char *animname );
	void					Event_AnimLength( int channel, const char *animname );
	void					Event_AnimDistance( int channel, const char *animname );
	void					Event_HasEnemies( void );
	void					Event_NextEnemy( idEntity *ent );
	void					Event_ClosestEnemyToPoint( const idVec3 &pos );
	void					Event_StopSound( int channel, int netsync );
	void					Event_SetNextState( const char *name );
	void					Event_SetState( const char *name );
	void					Event_GetState( void );
	void					Event_GetHead( void );
	void					Event_GetEyePos( void );
	/**
	* Attaches the entity and also returns its index in the attachment list
	**/
	void					Event_Attach( idEntity *ent );
	void					Event_GetAttachment( int ind );
	void					Event_GetNumAttachments( void );

	// Returns the number of ranged/melee weapons attached to the calling script
	void					Event_GetNumMeleeWeapons();
	void					Event_GetNumRangedWeapons();

	/**
	 * greebo: Script events to get/set the team of this actor.
	 */
	void					Event_GetTeam();
	void					Event_SetTeam(int newTeam);
};

#endif /* !__GAME_ACTOR_H__ */
