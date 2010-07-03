// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3958 $
 * $Date: 2010-06-20 15:16:22 +0200 (Sun, 20 Jun 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod)

#ifndef __GAME_LODE_H__
#define __GAME_LODE_H__

/*
===============================================================================

  Lode - Level Of Detail Entity
  
  Automatically creates/culls entities based on distance from player.

===============================================================================
*/

// Defines one entity class to be spawned/culled
struct lode_class_t {
	idStr					classname;		// Entity class to respawn entities
	idStr					skin;			// Either "skinname" or "random:skinname1,skinname2" etc.
	int						score;			// to find out how many entities (calculate at spawn time from score)
	idVec3					origin;			// origin of the original target entity, useful for "flooring"
	float					cullDist;		// distance after where we remove the entity from the world
	float					spawnDist;		// distance where we respawn the entity
	float					spacing;		// min. distance between entities of this class
	float					bunching;		// bunching threshold (0 - none, 1.0 - all)
	bool					floor;			// if true, the entities will be floored (on by default, use
											// "lode_floor" "0" to disable, then entities will be positioned
											// at "z" where the are in the editor
	bool					stack;			// if true, the entities can stack on top of each other
	int						nocollide;		// should this entity collide with:
   											// 1 other auto-generated entities from the same class?
											// 2 other auto-generated entities (other classes)
											// 4 other static entities already present
											// 8 world geometry
	int						falloff;		// Entity random distribution method
											// 0 - none, 1 - cutoff, 2 - square, 3 - exponential
	idVec3					size;			// size of the model for collision tests
};

// Defines one area that inhibits entity spawning
struct lode_inhibitor_t {
	idVec3					origin;			// origin of the area
	idBox					box;			// oriented box of the area
};

// Defines one entity to be spawned/culled
struct lode_entity_t {
	idStr					skin;			// the final skin for this entity (might be randomly choosen)
	idVec3					origin;			// (semi-random) origin relatively to LODE origin
	idAngles				angles;			// zyx (yaw, pitch, roll) (semi-random) angles
	bool					hidden;			// hidden?
	bool					exists;			// false if culled
	int						entity;			// nr of the entity if exists == true
	int						classIdx;		// index into Classes
	// only needed during construct, so we use a temp. idList to save memory
	//idBounds				bounds;			// bounds of the model, for fast collision tests
};

extern const idEventDef EV_Deactivate;
extern const idEventDef EV_CullAll;

class Lode : public idStaticEntity {
public:
	CLASS_PROTOTYPE( Lode );

						Lode( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	virtual void		Think( void );

	/**
	* Stop thinking and no longer cull/spawn entities.
	*/
	void				Event_Deactivate( idEntity *activator );
	/*
	* Cull all entities. Only useful after Deactivate().
	*/
	void				Event_CullAll( void );

private:
	void				Event_Activate( idEntity *activator );

	/**
	* Look at our targets and create the entity classes. Calls PrepareEntities().
	*/
	void				Prepare( void );

	/**
	* Create the entity positions.
	*/
	void				PrepareEntities( void );

	/**
	* In the range 0.. 1.0, using our own m_iSeed value.
	*/
	float				RandomFloat();

	/**
	* Squared falloff
	*/
	float				RandomFloatSqr( void );

	/**
	* Exponential falloff
	*/
	float				RandomFloatExp( const float lambda );

	/**
	* Spawn the entity with the given index, return true if it could be spawned.
	* If managed is true, the LODE will take care of this entity.
	*/
	bool				spawnEntity( const int idx, const bool managed );

	/**
	* Take the given entity as template and add a class from its values. Returns
	* the floor-space-size of this entity class.
	*/
	float				addClassFromEntity( idEntity *ent, const int iEntScore );

	/**
	* Generate a scaling factor depending on the GUI setting.
	*/
	float				LODBIAS ( void );

	/**
	* Set m_iNumEntities from spawnarg, or density, taking GUI setting into account.
	*/
	void				ComputeEntityCount( void );


	/* *********************** Members *********************/

	bool				active;

	/**
	* If true, we need still to prepare our entity list.
	*/
	bool				m_bPrepared;

	/**
	* Set to true if whether this static entity should hide
	* or change models/skins when outside a certain distance from the player.
	* If true, this entity will manage the LOD settings for entities, which
	* means these entities should have a "m_DistCheckInterval" of 0.
	**/
	bool				m_bDistDependent;

	/**
	* Entities further from player than "hide_distance" + m_fCullRange will be culled.
	* Set to 0 to disable.
	**/
	float				m_fCullRange;

	/**
	* Current seed value for the random generator, which generates the sequence used to place
	* entities. Same seed value gives same sequence, thus same placing every time.
	**/
	int					m_iSeed;

	/**
	* Seed start value for the random generator, used to restart the sequence.
	**/
	int					m_iOrgSeed;

	/**
	* Sum of all entity scores, used to distribute the entities according to their score.
	**/
	int					m_iScore;

	/**
	* Number of entities to manage overall.
	**/
	int					m_iNumEntities;

	/**
	* Number of entities currently visible.
	**/
	int					m_iNumVisible;

	/**
	* Number of entities currently existing (visible or not).
	**/
	int					m_iNumExisting;

	/**
	* If true, the LOD distance check will only consider distance
	* orthogonal to gravity.  This can be useful for things like
	* turning on rainclouds high above the player.
	**/
	bool				m_bDistCheckXYOnly;
	
	/**
	* Timestamp and interval between distance checks, in milliseconds
	**/
	int					m_DistCheckTimeStamp;
	int					m_DistCheckInterval;

	/**
	* The classes of entities that we need to construct.
	**/
	idList<lode_class_t>		m_Classes;

	/**
	* The entities that inhibit us from spawning inside their area.
	**/
	idList<lode_inhibitor_t>	m_Inhibitors;

	/**
	* Info about each entitiy that we spawn or cull.
	**/
	idList<lode_entity_t>		m_Entities;

	/**
	* A copy of cv_lod_bias, to detect changes during runtime.
	*/
	float						m_fLODBias;

	/**
	* Average floorspace of all classes
	*/
	float						m_fAvgSize;

	static const unsigned long	IEEE_ONE  = 0x3f800000;
	static const unsigned long	IEEE_MASK = 0x007fffff;

	static const unsigned long	NOCOLLIDE_SAME   = 0x01;
	static const unsigned long	NOCOLLIDE_OTHER  = 0x02;
	static const unsigned long	NOCOLLIDE_STATIC = 0x04;
	static const unsigned long	NOCOLLIDE_WORLD  = 0x08;
	static const unsigned long	NOCOLLIDE_ATALL  = NOCOLLIDE_WORLD + NOCOLLIDE_STATIC + NOCOLLIDE_OTHER + NOCOLLIDE_SAME;
	static const unsigned long	COLLIDE_WORLD    = NOCOLLIDE_STATIC + NOCOLLIDE_OTHER + NOCOLLIDE_SAME;
	static const unsigned long	COLLIDE_ALL  	 = 0x00;
};

#endif /* !__GAME_LODE_H__ */

