/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.13  2007/02/11 20:59:57  ishtvan
 * comments updated for better documentation
 *
 * Revision 1.12  2006/11/20 05:35:17  ishtvan
 * simplified rotation code
 *
 * Revision 1.11  2006/08/14 01:06:28  ishtvan
 * PutInHands added
 *
 * fixed member vars to conform to naming conventions
 *
 * Revision 1.10  2006/08/07 06:52:08  ishtvan
 * *) added distance control
 *
 * *) Grabber now always grabs the center of mass
 *
 * *) StartGrab function added that may be called by the inventory to drop stuff to hands
 *
 * Revision 1.9  2006/08/04 10:53:26  ishtvan
 * preliminary grabber fixes
 *
 * Revision 1.8  2006/08/02 07:49:30  ishtvan
 * manipulation - rotation updates/fixes
 *
 * Revision 1.7  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.6  2006/02/23 10:20:19  ishtvan
 * throw implemented
 *
 * Revision 1.5  2005/12/11 18:08:05  ishtvan
 * disabled player view changes when using mouse axes to rotate
 *
 * Revision 1.4  2005/12/09 05:12:48  lloyd
 * Various bug fixes (AF grabbing, mouse deadzone, mouse sensitivty, ...)
 *
 * Revision 1.3  2005/12/02 18:21:04  lloyd
 * Objects start oriented with player
 *
 * Revision 1.1.1.1  2005/09/22 15:52:33  Lloyd
 * Initial release
 *
 ***************************************************************************/

#include "....//idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"
#include "DarkModGlobals.h"
#include "PlayerData.h"

#include "Grabber.h"

/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

const idEventDef EV_Grabber_CheckClipList( "<checkClipList>", NULL, NULL );

const int CHECK_CLIP_LIST_INTERVAL =	1000;

const int MOUSE_DEADZONE =				5;
const float MOUSE_SCALE =				0.7f;

const float MAX_PICKUP_DISTANCE =		1000.0f;
//const float ROTATION_SPEED =			0.9f;
const float ROTATION_SPEED	=			1.0f;
const float ROTATION_DAMPER =			0.9f;
const float MAX_ROTATION_SPEED =		30.0f;
// when you let go of an item, the velocity is clamped to this value
const float MAX_RELEASE_LINVEL =		30.0f;
// when you let go of an item, the angular velocity is clamped to this value
const float MAX_RELEASE_ANGVEL =		10.0f;
// limits how close you can hold an item to the player view point
const float MIN_HELD_DISTANCE  =		35.0f;
// granularity of the distance control
const int	DIST_GRANULARITY	=		12;

const idVec3 rotateMin( -MAX_ROTATION_SPEED, -MAX_ROTATION_SPEED, -MAX_ROTATION_SPEED );
const idVec3 rotateMax( MAX_ROTATION_SPEED, MAX_ROTATION_SPEED, MAX_ROTATION_SPEED );



CLASS_DECLARATION( idEntity, CGrabber )

	EVENT( EV_Grabber_CheckClipList, 	CGrabber::Event_CheckClipList )

END_CLASS


/*
==============
CGrabber::CGrabber
==============
*/
CGrabber::CGrabber( void ) {
	Clear();
}

/*
==============
CGrabber::~CGrabber
==============
*/
CGrabber::~CGrabber( void ) {
	StopDrag();
	Clear();
}


/*
==============
CGrabber::Clear
==============
*/
void CGrabber::Clear( void ) 
{
	m_dragEnt			= NULL;
	m_player			= NULL;
	m_joint			= INVALID_JOINT;
	m_id				= 0;
	m_localEntityPoint.Zero();
	m_bAttackPressed = false;
	m_ThrowTimer = 0;
	m_bIsColliding = false;
	m_bPrevFrameCollided = false;

	m_DistanceCount = 0;
	m_MinHeldDist	= 0;
	m_MaxDistCount	= DIST_GRANULARITY;
	m_LockedHeldDist = 0;

	while( this->HasClippedEntity() )
		this->RemoveFromClipList( 0 );
}

/*
==============
CGrabber::Spawn
==============
*/
void CGrabber::Spawn( void ) 
{
	//TODO: Change constants at the start of the file and assign them here
	//	using spawnArgs.
	//
	// This will also require moving the values into the class def .h file
}

/*
==============
CGrabber::StopDrag
==============
*/
void CGrabber::StopDrag( void ) 
{
	m_bIsColliding = false;
	m_bPrevFrameCollided = false;
	m_DistanceCount = 0;
	m_dragEnt = NULL;

	if(m_player)
	{
		m_player->m_bDraggingBody = false;
		m_player->m_bGrabberActive = false;
		m_player->SetImmobilization( "Grabber", 0 );
		m_player->RaiseWeapon();
	}
}

/*
==============
CGrabber::Update
==============
*/
void CGrabber::Update( idPlayer *player, bool hold ) 
{
	idVec3 viewPoint, origin, COM, COMWorld;
	idMat3 viewAxis, axis;
	trace_t trace;
//	idEntity *newEnt(NULL);
//	jointHandle_t newJoint(INVALID_JOINT);

	m_player = player;

	// if there is an entity selected, we let it go and exit
	if( !hold && m_dragEnt.GetEntity() ) 
	{
		ClampVelocity( MAX_RELEASE_LINVEL, MAX_RELEASE_ANGVEL, m_id );

		this->StopDrag();
		
		goto Quit;
	}

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity is currently selected for dragging, start grabbing the frobbed entity
    if ( !m_dragEnt.GetEntity() ) 
		StartDrag( player );

	idEntity *drag = m_dragEnt.GetEntity();
	if ( drag ) 
	{
		idVec3 draggedPosition;

		// Check for throwing:
		bool bAttackHeld = player->usercmd.buttons & BUTTON_ATTACK;

		if( m_bAttackPressed && !bAttackHeld )
		{
			int HeldTime = gameLocal.time - m_ThrowTimer;

			Throw( HeldTime );
			m_bAttackPressed = false;

			goto Quit;
		}

		if( !m_bAttackPressed && bAttackHeld )
		{
			m_bAttackPressed = true;

			// start the throw timer
			m_ThrowTimer = gameLocal.time;
		}

		// Update the held distance

		// Lock the held distance to +/- a few increments around the current held dist
		// when collision occurs.
		// Otherwise the player would have to increment all the way back
		if(m_bIsColliding)
		{
			if(!m_bPrevFrameCollided)
				m_LockedHeldDist = m_DistanceCount;

			m_DistanceCount = idMath::ClampInt( (m_LockedHeldDist-2), (m_LockedHeldDist+1), m_DistanceCount );
		}

		idVec3 vPlayerPoint(1.0f,0.0f,0.0f);
		float distFactor = (float) m_DistanceCount / (float) m_MaxDistCount;
		vPlayerPoint *= m_MinHeldDist + (m_dragEnt.GetEntity()->m_FrobDistance - m_MinHeldDist) * distFactor;

		draggedPosition = viewPoint + vPlayerPoint * viewAxis;


		m_drag.SetDragPosition( draggedPosition );

		// evaluate physics
		// Note: By doing these operations in this order, we overwrite idForce_Drag angular velocity
		// calculations which is what we want so that the manipulation works properly
		m_drag.Evaluate( gameLocal.time );
		this->ManipulateObject( player );

		renderEntity_t *renderEntity = drag->GetRenderEntity();
		idAnimator *dragAnimator = drag->GetAnimator();

		if ( m_joint != INVALID_JOINT && renderEntity && dragAnimator ) {
			dragAnimator->GetJointTransform( m_joint, gameLocal.time, draggedPosition, axis );
		}
	}

Quit:
	m_bPrevFrameCollided = m_bIsColliding;
	
	// reset this, it gets set by physics if the object collides
	m_bIsColliding = false;
	return;
}

void CGrabber::StartDrag( idPlayer *player, idEntity *newEnt, int bodyID )
{
	idVec3 viewPoint, origin, COM, COMWorld, delta2;
	idEntity *FrobEnt;
	idMat3 viewAxis, axis;
	trace_t trace;
	jointHandle_t newJoint;

	// Just in case we were called while holding another item:
	StopDrag();

	player->GetViewPos( viewPoint, viewAxis );

	// If an entity was not explictly passed in, use the frob entity
    if ( !newEnt ) 
	{
		FrobEnt = g_Global.m_DarkModPlayer->m_FrobEntity;
		if( !FrobEnt )
			goto Quit;

		newEnt = FrobEnt;

		trace = g_Global.m_DarkModPlayer->m_FrobTrace;
		
		// If the ent was not hit directly and is an AF, we must fill in the joint and body ID
		if( trace.c.entityNum != FrobEnt->entityNumber && FrobEnt->IsType(idAFEntity_Base::Type) )
		{
			static_cast<idPhysics_AF *>(FrobEnt->GetPhysics())->NearestBodyOrig( trace.c.point, &trace.c.id );
		}
	}
	else
	{
		// An entity was passed in to grab directly
		trace.c.id = bodyID;
	}

	if ( newEnt->GetBindMaster() ) 
	{
		if ( newEnt->GetBindJoint() ) 
		{
		trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
		} else 
		{
			trace.c.id = newEnt->GetBindBody();
		}
		newEnt = newEnt->GetBindMaster();			
	}

	if ( newEnt->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base *>(newEnt)->IsActiveAF() ) 
	{
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>(newEnt);

		// joint being dragged
		newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
		// get the body id from the trace model id which might be a joint handle
		trace.c.id = af->BodyForClipModelId( trace.c.id );
	} 
	else if ( !newEnt->IsType( idWorldspawn::Type ) ) 
	{
		if ( trace.c.id < 0 ) 
		{
			newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
		} else 
		{
			newJoint = INVALID_JOINT;
		}
	} else 
	{
			newJoint = INVALID_JOINT;
			newEnt = NULL;
	}
	
	// If the new entity still wasn't set at this point, it was a frobbed
	// ent but was found to be invalid.

	if ( !newEnt ) 
		goto Quit;
	
// Set up the distance and orientation and stuff

	// TODO: The !idStaticEntity here is temporary to fix a bug that should go away once frobbing moveables works again
	if( newEnt->IsType(idStaticEntity::Type) )
		goto Quit;

	// get the center of mass
	idPhysics *phys = newEnt->GetPhysics();
	idClipModel *clipModel;

	clipModel = phys->GetClipModel( m_id );
	if( clipModel && clipModel->IsTraceModel() )
	{
		float mass;
		idMat3 inertiaTensor;
		clipModel->GetMassProperties( 1.0f, mass, COM, inertiaTensor );
	} else 
	{
		// don't drag it if its clipmodel is not a trace model
		goto Quit;
	}
	COMWorld = phys->GetOrigin( m_id ) + COM * phys->GetAxis( m_id );

	m_dragEnt = newEnt;
	m_joint = newJoint;
	m_id = trace.c.id;

	origin = phys->GetOrigin( m_id );
	axis = phys->GetAxis( m_id );
	m_localEntityPoint = COM;

	// find the nearest distance and set it to that
	m_MinHeldDist = newEnt->spawnArgs.GetFloat("hold_distance_min", "-1" );
	if( m_MinHeldDist < 0 )
		m_MinHeldDist = MIN_HELD_DISTANCE;

	delta2 = COMWorld - viewPoint;
	m_DistanceCount = idMath::Floor( m_MaxDistCount * (delta2.Length() - m_MinHeldDist) / (newEnt->m_FrobDistance - m_MinHeldDist ) );
	m_DistanceCount = idMath::ClampInt( 0, m_MaxDistCount, m_DistanceCount );

	// prevent collision with player
	// set the clipMask so that the objet only collides with the world
	this->AddToClipList( m_dragEnt.GetEntity() );

	// signal object manipulator to update drag position so it's relative to the objects
	// center of mass instead of its origin
	m_rotationAxis = -1;

	this->m_drag.Init( g_dragDamping.GetFloat() );
	this->m_drag.SetPhysics( phys, m_id, m_localEntityPoint );

	player->m_bGrabberActive = true;
	// don't let the player switch weapons or items
	player->SetImmobilization( "Grabber", EIM_ITEM_SELECT | EIM_WEAPON_SELECT );

Quit:
	return;
}

/*
==============
CGrabber::ManipulateObject
==============
*/
void CGrabber::ManipulateObject( idPlayer *player ) {
	idVec3 viewPoint;
	idMat3 viewAxis;

	player->GetViewPos( viewPoint, viewAxis );

	idEntity *ent;
	idVec3 angularVelocity;
	idPhysics *physics;
	idVec3 rotationVec;
	bool rotating;

	ent = m_dragEnt.GetEntity();
	if( !ent ) {
		return;
	}

	physics = ent->GetPhysics();
	if ( !physics ) {
		return;
	}

	angularVelocity = vec3_origin;

	// NOTES ON OBJECT ROTATION
	// 
	// The way the object rotation works is as follows:
	//	1) Player must be holding BUTTON_ZOOM
	//	2) if the player is holding BUTTON_RUN, rotate about the z-axis
	//	   else then if the mouse first moves along the x axis, rotate about the x-axis
	//				 else if the mouse first moves along the y axis, rotate about the y-axis
	//
	// This system may seem complicated but I found after playing with it for a few minutes
	// it's quite easy to use.  It also offers some throttling of rotation speed. (Besides, 
	// who uses the ZOOM button anyway?)
	//
	// If the player releases the ZOOM button rotation slows down.
	// To sum it all up...
	//
	// If the player holds ZOOM, make the object rotated based on mouse movement.
	if( !ent->IsType( idAFEntity_Base::Type ) && player->usercmd.buttons & BUTTON_ZOOM ) 
	{

		float angle = 0.0f;
		rotating = true;

		// Disable player view change while rotating
		player->SetImmobilization( "Grabber", player->GetImmobilization("Grabber") | EIM_VIEW_ANGLE );
		
		if( !this->DeadMouse() ) 
		{
			switch( m_rotationAxis ) 
			{
				case 1:
					angle = idMath::Fabs( player->usercmd.mx - m_mousePosition.x ) - MOUSE_DEADZONE;
					if( player->usercmd.mx < m_mousePosition.x )
						angle = -angle;

					rotationVec.Set( 1.0f, 0.0f, 0.0f );
					m_rotationAxis = 1;

					break;

				case 2:
					angle = idMath::Fabs( player->usercmd.my - m_mousePosition.y ) - MOUSE_DEADZONE;
					if( player->usercmd.my < m_mousePosition.y )
						angle = -angle;

					rotationVec.Set( 0.0f, -1.0f, 0.0f );
					m_rotationAxis = 2;

					break;

				case 3:
					angle = idMath::Fabs( player->usercmd.mx - m_mousePosition.x ) - MOUSE_DEADZONE;
					if( player->usercmd.mx < m_mousePosition.x )
						angle = -angle;

					rotationVec.Set( 0.0f, 0.0f, 1.0f );
					m_rotationAxis = 3;

					break;

				default:
					// wait for motion on the x-axis, if nothing, check the y-axis.
					if( idMath::Fabs( player->usercmd.mx - m_mousePosition.x ) > idMath::Fabs( player->usercmd.my - m_mousePosition.y ) ) {
						// if BUTTON_RUN, then toggle rotating the x-axis, else just do the z-axis
						if( player->usercmd.buttons & BUTTON_RUN ) {
							m_rotationAxis = 3;
						}
						else 
						{
							m_rotationAxis = 1;
						}
					}
					else 
					{
						m_rotationAxis = 2;
					}

					rotationVec.Set( 0.0f, 0.0f, 0.0f );
			}
		}

		angle = idMath::ClampFloat( -MAX_ROTATION_SPEED, MAX_ROTATION_SPEED, angle * MOUSE_SCALE );

		idAngles viewAnglesXY = viewAxis.ToAngles();
		// ignore the change in player pitch angles
		viewAnglesXY[0] = 0;
		idMat3 viewAxisXY = viewAnglesXY.ToMat3();
		
		angularVelocity = rotationVec * viewAxisXY * angle;
	}
	else 
	{
		rotating = false;

		// Ishtvan: Enable player view change
		player->SetImmobilization( "Grabber", player->GetImmobilization("Grabber") & (~EIM_VIEW_ANGLE) );

		// reset these coordinates so that next time they press zoom the rotation will be fresh
		m_mousePosition.x = player->usercmd.mx;
		m_mousePosition.y = player->usercmd.my;

		// reset rotation information so when the next zoom is pressed we can freely rotate again
		if( m_rotationAxis )
			m_rotationAxis = 0;

		angularVelocity = vec3_zero;
	}


// ============== rotate object so it stays oriented with the player ===========
	if( !ent->IsType( idAFEntity_Base::Type ) && !rotating && !m_bIsColliding ) 
	{
		idVec3	normal;
		float	deltaYawAng(0);
		normal = physics->GetGravityNormal();

		deltaYawAng = static_cast<idPhysics_Player *>(player->GetPhysics())->GetDeltaViewYaw();
		m_rotation.Set(m_drag.GetCenterOfMass(), normal, deltaYawAng );
		
		// rotate the object directly
		trace_t trResults;
		physics->ClipRotation( trResults, m_rotation, NULL );
		physics->Rotate( m_rotation * trResults.fraction );
		
		//I can't seem to get setting the angular velocity to work here for some reason
		// Might be due to disparity between actual frame integration time and 1/60 sec

// TODO: This may work now that setting angular velocity has been fixed
		//angularVelocity += m_rotation.ToAngularVelocity() / MS2SEC( USERCMD_MSEC );
	}

	physics->SetAngularVelocity( angularVelocity, m_id );
}

/*
==============
CGrabber::DeadMouse
==============
*/
bool CGrabber::DeadMouse( void ) 
{
	// check mouse is in the deadzone along the x-axis or the y-axis
	if( idMath::Fabs( m_player->usercmd.mx - m_mousePosition.x ) > MOUSE_DEADZONE ||
		idMath::Fabs( m_player->usercmd.my - m_mousePosition.y ) > MOUSE_DEADZONE )
		return false;

	return true;
}

/*
==============
CGrabber::AddToClipList
==============
*/
void CGrabber::AddToClipList( idEntity *ent ) 
{
	CGrabbedEnt obj;
	idPhysics *phys = ent->GetPhysics();
	int clipMask = phys->GetClipMask();

	obj.ent = ent;
	obj.clipMask = clipMask;

	m_clipList.AddUnique( obj );

	// set the clipMask so that the player won't colide with the object but it still
	// collides with the world
	phys->SetClipMask( clipMask & (~MASK_PLAYERSOLID) );
	phys->SetClipMask( phys->GetClipMask() | CONTENTS_SOLID );

	if( this->HasClippedEntity() ) {
		this->PostEventMS( &EV_Grabber_CheckClipList, CHECK_CLIP_LIST_INTERVAL );
	}
}

/*
==============
CGrabber::RemoveFromClipList
==============
*/
void CGrabber::RemoveFromClipList( int index ) 
{
	// remove the entity and reset the clipMask
	if( index != -1 ) {
		m_clipList[index].ent->GetPhysics()->SetClipMask( m_clipList[index].clipMask );
		m_clipList.RemoveIndex( index );
	}

	if( !this->HasClippedEntity() ) {
		// cancel CheckClipList because the list is empty
		this->CancelEvents( &EV_Grabber_CheckClipList );
	}
}

/*
==============
CGrabber::Event_CheckClipList
==============
*/
void CGrabber::Event_CheckClipList( void ) 
{
	idEntity *ent[MAX_GENTITIES];
	bool keep;
	int i, j, num;	

	// Check for any entity touching the players bounds
	// If the entity is not in our list, remove it.
	num = gameLocal.clip.EntitiesTouchingBounds( m_player->GetPhysics()->GetAbsBounds(), CONTENTS_SOLID, ent, MAX_GENTITIES );
	for( i = 0; i < m_clipList.Num(); i++ ) {
		// Check clipEntites against entites touching player

		// We keep an entity if it is the one we're dragging 
		if( this->GetSelected() == m_clipList[i].ent ) {
			keep = true;
		}
		else {
			keep = false;

			// OR if it's touching the player and still in the clipList
			for( j = 0; !keep && j < num; j++ ) {
				if( m_clipList[i].ent == ent[j] ) {
					keep = true;
				}
			}
		}

		// Note we have to decrement i otherwise we skip entities
		if( !keep ) {
			this->RemoveFromClipList( i );
			i -= 1;
		}
	}

	if( this->HasClippedEntity() ) {
		this->PostEventMS( &EV_Grabber_CheckClipList, CHECK_CLIP_LIST_INTERVAL );
	}
}

/*
==============
CGrabber::IsInClipList
==============
*/
bool CGrabber::IsInClipList( idEntity *ent ) const {
	CGrabbedEnt obj;
	
	obj.ent = ent;
	
	// check if the entity is in the clipList
	if( m_clipList.FindIndex( obj ) == -1 ) {
		return false;
	}
	return true;
}


/*
==============
CGrabber::HasClippedEntity
==============
*/
bool CGrabber::HasClippedEntity( void ) const {
	if( m_clipList.Num() > 0 ) {
		return true;
	}
	return false;
}

/*
==============
CGrabber::Throw
==============
*/
void CGrabber::Throw( int HeldTime )
{
	float ThrowImpulse(0), FracPower(0);
	idVec3 ImpulseVec(vec3_zero), IdentVec( 1, 0, 1);

	idEntity *ent = m_dragEnt.GetEntity();
	ImpulseVec = m_player->firstPersonViewAxis[0];
	ImpulseVec.Normalize();

	FracPower = (float) HeldTime / (float) cv_throw_time.GetInteger();

	if( FracPower > 1.0 )
		FracPower = 1.0;

	// Try out a linear scaling between max and min
	ThrowImpulse = cv_throw_min.GetFloat() + (cv_throw_max.GetFloat() - cv_throw_min.GetFloat()) * FracPower;
	ImpulseVec *= ThrowImpulse;  

	ClampVelocity( MAX_RELEASE_LINVEL, MAX_RELEASE_ANGVEL, m_id );
	ent->ApplyImpulse( m_player, m_id, ent->GetPhysics()->GetOrigin(), ImpulseVec );

	StopDrag();
}

void CGrabber::ClampVelocity(float maxLin, float maxAng, int idVal)
{
	idVec3 linear, angular;
	float  lengthLin(0), lengthAng(0), lengthLin2(0), lengthAng2(0);

	if( m_dragEnt.GetEntity() )
	{
		idEntity *ent = m_dragEnt.GetEntity();
		linear = ent->GetPhysics()->GetLinearVelocity( idVal );
		angular = ent->GetPhysics()->GetAngularVelocity( idVal );
		// only do this when we let go or throw, so can afford to do a sqrt here
		lengthLin = linear.Length();
		lengthAng = angular.Length();
		lengthLin2 = idMath::ClampFloat(0.0f, maxLin, lengthLin );
		lengthAng2 = idMath::ClampFloat(0.0f, maxAng, lengthAng );

		if( lengthLin > 0 )
			ent->GetPhysics()->SetLinearVelocity( linear * (lengthLin2/lengthLin), idVal );
		if( lengthAng > 0 )
			ent->GetPhysics()->SetAngularVelocity( angular * (lengthAng2/lengthAng), idVal );
	}
}

void CGrabber::IncrementDistance( bool bIncrease )
{
	int increment = 1;
	if( !m_dragEnt.GetEntity() )
		goto Quit;
	
	if( !bIncrease )
		increment *= -1;

	m_DistanceCount += increment;
	m_DistanceCount = idMath::ClampInt( 0, m_MaxDistCount, m_DistanceCount );

Quit:
	return;
}

bool CGrabber::PutInHands(idEntity *ent, idPlayer *player, int bodyID)
{
	idClipModel *ClipModel = NULL;
	bool bReturnVal = false, bStartedHidden(false);
	int ContentsMask = 0;
	float HeldDist = 0.0f;
	trace_t trace;
	idVec3 orig(vec3_zero), targetCOM(vec3_zero), COMLocal(vec3_zero);
	idVec3 forward(1.0f, 0.0f, 0.0f), viewPoint, initOrigin;
	idMat3 viewAxis;
	idVec3 FarAway(0.0f, 0.0f, -4096.0f);

	if( !ent || !player )
		goto Quit;

	player->GetViewPos( viewPoint, viewAxis );

	bStartedHidden = ent->IsHidden();
	// Momentarily show the entity if it was hidden, to enable the clipmodel
	ent->Show();

	// calculate where the origin should end up based on center of mass location and orientation
	// also based on the minimum held distance
	HeldDist = ent->spawnArgs.GetFloat("hold_distance_min", "-1" );
	if( HeldDist < 0 )
		HeldDist = MIN_HELD_DISTANCE;

	// get the center of mass
	ClipModel = ent->GetPhysics()->GetClipModel( bodyID );
	if( ClipModel && ClipModel->IsTraceModel() ) 
	{
		float mass;
		idMat3 inertiaTensor;
		ClipModel->GetMassProperties( 1.0f, mass, COMLocal, inertiaTensor );
	} else 
	{
		COMLocal.Zero();
	}

	targetCOM = (HeldDist * forward ) * viewAxis;
	targetCOM += viewPoint;

	orig = targetCOM - ( ent->GetPhysics()->GetAxis( bodyID ) * COMLocal );

	ContentsMask = CONTENTS_SOLID | CONTENTS_CORPSE | CONTENTS_RENDERMODEL;
	
	// This is a hack:  We want the trace to ignore both the entity itself and the player
	// But can only pass one entity in to ignore.  Therefore, we teleport it far away before doing the trace
	// And put it back afterwards
	initOrigin = ent->GetPhysics()->GetOrigin();
	ent->SetOrigin( FarAway );

	gameLocal.clip.TraceBounds( trace, orig, orig, ent->GetPhysics()->GetBounds(), ContentsMask, player );

	if( trace.fraction < 1.0f )
	{
		// object collided, return false, hide it
		if(bStartedHidden)
			ent->Hide();

		ent->SetOrigin( initOrigin );

		goto Quit;
	}

	bReturnVal = true;
	
	// otherwise teleport in the object
	ent->SetOrigin( orig );
	StartDrag( player, ent, bodyID );

Quit:
	return bReturnVal;
}
		