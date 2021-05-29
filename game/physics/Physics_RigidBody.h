/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __PHYSICS_RIGIDBODY_H__
#define __PHYSICS_RIGIDBODY_H__

/*
===================================================================================

	Rigid body physics

	Employs an impulse based dynamic simulation which is not very accurate but
	relatively fast and still reliable due to the continuous collision detection.

===================================================================================
*/

typedef struct rididBodyIState_s {
	idVec3					position;					// position of trace model
	idMat3					orientation;				// orientation of trace model
	idVec3					linearMomentum;				// translational momentum relative to center of mass
	idVec3					angularMomentum;			// rotational momentum relative to center of mass
} rigidBodyIState_t;

typedef struct rigidBodyPState_s {
	int						atRest;						// set when simulation is suspended
	float					lastTimeStep;				// length of last time step
	idVec3					localOrigin;				// origin relative to master
	idMat3					localAxis;					// axis relative to master
	idVec6					pushVelocity;				// push velocity
	idVec3					externalForce;				// external force relative to center of mass
	idVec3					externalTorque;				// external torque relative to center of mass
	idVec3					externalForcePoint;			// point where the externalForce is being applied at
	rigidBodyIState_t		i;							// state used for integration
} rigidBodyPState_t;

class idPhysics_RigidBody : public idPhysics_Base {

public:

	CLASS_PROTOTYPE( idPhysics_RigidBody );

							idPhysics_RigidBody( void );
							~idPhysics_RigidBody( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// initialisation
	void					SetFriction( const float linear, const float angular, const float contact );
	void					SetBouncyness( const float b );
							// same as above but drop to the floor first
	void					DropToFloor( void );
							// no contact determination and contact friction
	void					NoContact( void );
							// enable/disable activation by impact
	void					EnableImpact( void );
	void					DisableImpact( void );

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	
	/**
	 * greebo: This is similar to ApplyImpulse, although this distributes the impulse
	 *         on all entities in contact with this one in *this* very frame. If 
	 *         no entities are in contact, all the impulse gets applied to this one.
	 */
	bool					PropagateImpulse(const int id, const idVec3& point, const idVec3& impulse);

	void					AddForce( const int id, const idVec3 &point, const idVec3 &force );
	void					Activate( void );
	void					PutToRest( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

    // tels: force and torque that make entitiy "break down" when exceeded
	void					SetMaxForce( const idVec3 &newMaxForce );
	void					SetMaxTorque( const idVec3 &newMaxTorque );

	const idVec3 &			GetMaxForce( void ) const;
	const idVec3 &			GetMaxTorque( void ) const;

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	/**
	 * greebo: Override empty default implementation of idPhysics_Base::GetBlockingEntity().
	 */
	virtual const trace_t*	GetBlockingInfo( void ) const;
	virtual idEntity *		GetBlockingEntity( void ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const idVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

public:
	/**
	 * greebo: "Accessor" method to the internal state. This is a bit hacky, I admit.
	 */
	rigidBodyPState_t&		State() { return current; }

private:
	// state of the rigid body
	rigidBodyPState_t		current;
	rigidBodyPState_t		saved;

	// rigid body properties
	float					linearFriction;			// translational friction
	float					angularFriction;		// rotational friction
	float					contactFriction;		// friction with contact surfaces
	float					bouncyness;				// bouncyness
#ifdef MOD_WATERPHYSICS
	float					volume;					// MOD_WATERPHYSICS object volume 
#endif		// MOD_WATERPHYSICS
	idClipModel *			clipModel;				// clip model used for collision detection

	// tels: if the applied impulse/torque exceeds these values, the entity breaks down
	idVec3					maxForce;				// spawnarg "max_force"
	idVec3					maxTorque;				// spawnarg "max_torque"

	// derived properties
	float					mass;					// mass of body
	float					inverseMass;			// 1 / mass
	idVec3					centerOfMass;			// center of mass of trace model
	idMat3					inertiaTensor;			// mass distribution
	idMat3					inverseInertiaTensor;	// inverse inertia tensor

	idODE *					integrator;				// integrator
	bool					dropToFloor;			// true if dropping to the floor and putting to rest
	bool					testSolid;				// true if testing for solid when dropping to the floor
	bool					noImpact;				// if true do not activate when another object collides
	bool					noContact;				// if true do not determine contacts and no contact friction

	// master
	bool					hasMaster;
	bool					isOrientated;

	/**
	 * greebo: This saved the collision information when this object is in "bind slave mode".
	 */
	trace_t					collisionTrace;
	bool					isBlocked;

	bool					propagateImpulseLock;

#ifdef MOD_WATERPHYSICS
	// buoyancy
	int					noMoveTime;	// MOD_WATERPHYSICS suspend simulation if hardly any movement for this many seconds
#endif

private:
	friend void				RigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives );
	void					Integrate( const float deltaTime, rigidBodyPState_t &next );
	bool					CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision );
public:
	bool					CollisionImpulse( const trace_t &collision, idVec3 &impulse );
private:
	void					ContactFriction( float deltaTime );
	void					SmallMassContactFriction( float deltaTime ); // grayman #3452
	void					DropToFloorAndRest( void );
#ifdef MOD_WATERPHYSICS
	bool					TestIfAtRest( void );
#else 		// MOD_WATERPHYSICS
	bool					TestIfAtRest( void ) const;
#endif 		// MOD_WATERPHYSICS
	void					Rest( void );
	void					DebugDraw( void );

#ifdef MOD_WATERPHYSICS
	// Buoyancy stuff
	// Approximates the center of mass of the submerged portion of the rigid body.
	virtual bool			GetBuoyancy( const idVec3 &pos, const idMat3 &rotation, idVec3 &bCenter, float &percent ) const;	// MOD_WATERPHYSICS
	// Returns rough a percentage of which percent of the body is in water.
	virtual float			GetSubmergedPercent( const idVec3 &pos, const idMat3 &rotation ) const;	// MOD_WATERPHYSICS
#endif
};

#endif /* !__PHYSICS_RIGIDBODY_H__ */
