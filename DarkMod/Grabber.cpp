/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// TODO: Make sure drag to point is not within a solid
// TODO: Detecting stuck items (distance + angular offset)
// TODO: Handling stuck items (initially stop the player's motion, then if they continue that motion, drop the item)

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
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
CGrabber::CGrabber( void ) 
{
	Clear();
}

/*
==============
CGrabber::~CGrabber
==============
*/
CGrabber::~CGrabber( void ) 
{
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
	
	m_bAFOffGround = false;
	m_DragUpTimer = 0;
	m_AFBodyLastZ = 0.0f;

	m_DistanceCount = 0;
	m_MinHeldDist	= 0;
	m_MaxDistCount	= DIST_GRANULARITY;
	m_LockedHeldDist = 0;

	while( this->HasClippedEntity() )
		this->RemoveFromClipList( 0 );
}

void CGrabber::Save( idSaveGame *savefile ) const
{
	idEntity::Save(savefile);
	
	m_dragEnt.Save(savefile);
	savefile->WriteJoint(m_joint);
	savefile->WriteInt(m_id);
	savefile->WriteVec3(m_localEntityPoint);
	m_player.Save(savefile);

	m_drag.Save(savefile);

	// Save the three relevant values of the idRotation object
	savefile->WriteVec3(m_rotation.GetOrigin());
	savefile->WriteVec3(m_rotation.GetVec());
	savefile->WriteFloat(m_rotation.GetAngle());


	savefile->WriteInt(m_rotationAxis);
	savefile->WriteVec2(m_mousePosition);
	
	savefile->WriteInt(m_clipList.Num());
	for (int i = 0; i < m_clipList.Num(); i++)
	{
		m_clipList[i].m_ent.Save(savefile);
		savefile->WriteInt(m_clipList[i].m_clipMask);
		savefile->WriteInt(m_clipList[i].m_contents);
	}

	savefile->WriteBool(m_bAttackPressed);
	savefile->WriteInt(m_ThrowTimer);
	savefile->WriteInt(m_DragUpTimer);
	savefile->WriteFloat(m_AFBodyLastZ);
	savefile->WriteBool(m_bAFOffGround);
	savefile->WriteInt(m_DistanceCount);
	savefile->WriteInt(m_MaxDistCount);
	savefile->WriteInt(m_MinHeldDist);
	savefile->WriteInt(m_LockedHeldDist);
}

void CGrabber::Restore( idRestoreGame *savefile )
{
	idEntity::Restore(savefile);

	m_dragEnt.Restore(savefile);
	savefile->ReadJoint(m_joint);
	savefile->ReadInt(m_id);
	savefile->ReadVec3(m_localEntityPoint);
	m_player.Restore(savefile);

	m_drag.Restore(savefile);

	// Read the three relevant values of the idRotation object
	idVec3 origin;
	idVec3 vec;
	float angle;
	savefile->ReadVec3(origin);
	savefile->ReadVec3(vec);
	savefile->ReadFloat(angle);
	m_rotation = idRotation(origin, vec, angle);

	savefile->ReadInt(m_rotationAxis);
	savefile->ReadVec2(m_mousePosition);

	int num;
	savefile->ReadInt(num);
	m_clipList.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_clipList[i].m_ent.Restore(savefile);
		savefile->ReadInt(m_clipList[i].m_clipMask);
		savefile->ReadInt(m_clipList[i].m_contents);
	}

	savefile->ReadBool(m_bAttackPressed);
	savefile->ReadInt(m_ThrowTimer);
	savefile->ReadInt(m_DragUpTimer);
	savefile->ReadFloat(m_AFBodyLastZ);
	savefile->ReadBool(m_bAFOffGround);
	savefile->ReadInt(m_DistanceCount);
	savefile->ReadInt(m_MaxDistCount);
	savefile->ReadInt(m_MinHeldDist);
	savefile->ReadInt(m_LockedHeldDist);
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
	
	m_bAFOffGround = false;
	m_DragUpTimer = 0;
	m_AFBodyLastZ = 0.0f;

	m_DistanceCount = 0;
	m_dragEnt = NULL;

	if(m_player.GetEntity())
	{
		m_player.GetEntity()->m_bDraggingBody = false;
		m_player.GetEntity()->m_bGrabberActive = false;
		m_player.GetEntity()->SetImmobilization( "Grabber", 0 );
		m_player.GetEntity()->SetHinderance( "Grabber", 1.0f, 1.0f );
		m_player.GetEntity()->RaiseWeapon();
	}
}

/*
==============
CGrabber::Update
==============
*/
void CGrabber::Update( idPlayer *player, bool hold ) 
{
	idVec3 viewPoint(vec3_zero), origin(vec3_zero);
	idVec3 COM(vec3_zero), COMWorld(vec3_zero);
	idVec3 draggedPosition(vec3_zero), vPlayerPoint(vec3_zero);
	idMat3 viewAxis(mat3_identity), axis(mat3_identity);
	trace_t trace;
	idAnimator *dragAnimator;
	renderEntity_t *renderEntity;
	float distFactor;
	bool bAttackHeld;
	idEntity *drag;
	idPhysics_Player *playerPhys;
	
	m_player = player;

	// if there is an entity selected, we let it go and exit
	if( !hold && m_dragEnt.GetEntity() ) 
	{
		ClampVelocity( MAX_RELEASE_LINVEL, MAX_RELEASE_ANGVEL, m_id );

		this->StopDrag();
		
		goto Quit;
	}

	/* idPhysics_Player* */ playerPhys = static_cast<idPhysics_Player *>(player->GetPhysics());
	// if the player is climbing a rope or ladder, don't let them grab things
	if( playerPhys->OnRope() || playerPhys->OnLadder() )
		goto Quit;

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity is currently selected for dragging, start grabbing the frobbed entity
    if ( !m_dragEnt.GetEntity() ) 
		StartDrag( player );

	// if there's still not a valid ent, don't do anything
	/* idEntity* */ drag = m_dragEnt.GetEntity();
	if ( !drag || !m_dragEnt.IsValid() )
		goto Quit;

	// Check for throwing:
	/* bool */ bAttackHeld = player->usercmd.buttons & BUTTON_ATTACK;

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

	vPlayerPoint.x = 1.0f; // (1, 0, 0)
	/* float */ distFactor = (float) m_DistanceCount / (float) m_MaxDistCount;
	vPlayerPoint *= m_MinHeldDist + (m_dragEnt.GetEntity()->m_FrobDistance - m_MinHeldDist) * distFactor;

	draggedPosition = viewPoint + vPlayerPoint * viewAxis;

// ====================== AF Grounding Testing ===============================
	// If dragging a body with a certain spawnarg set, you should only be able to pick
	// it up so far off the ground
	if( drag->IsType(idAFEntity_Base::Type) && (cv_drag_AF_free.GetBool() == false) )
	{
		idAFEntity_Base *AFPtr = (idAFEntity_Base *) drag;
		
		if( AFPtr->IsActiveAF() && AFPtr->m_bGroundWhenDragged )
		{
			// Poll the critical AF bodies and see how many are off the ground
			int OnGroundCount = 0;
			for( int i=0; i<AFPtr->m_GroundBodyList.Num(); i++ )
			{
				if( AFPtr->GetAFPhysics()->HasGroundContacts( AFPtr->m_GroundBodyList[i] ) )
					OnGroundCount++;
			}

			// check if the minimum number of these critical bodies remain on the ground
			if( OnGroundCount < AFPtr->m_GroundBodyMinNum )
			{
				m_DragUpTimer = gameLocal.time;

				// do not allow translation higher than current vertical position
				idVec3 bodyOrigin = AFPtr->GetAFPhysics()->GetOrigin( m_id );
				idVec3 UpDir = -AFPtr->GetPhysics()->GetGravityNormal();

				// If the AF just went off the ground, copy the last valid Z
				if( !m_bAFOffGround )
				{
					m_AFBodyLastZ = bodyOrigin * UpDir;
					m_bAFOffGround = true;
				}

				float deltaVert = draggedPosition * UpDir - m_AFBodyLastZ;
				if( deltaVert > 0 )
					draggedPosition -= deltaVert * UpDir;
			}
			else if( (gameLocal.time - m_DragUpTimer) < cv_drag_AF_ground_timer.GetInteger() )
			{
				idVec3 bodyOrigin = AFPtr->GetAFPhysics()->GetOrigin( m_id );
				idVec3 UpDir = -AFPtr->GetPhysics()->GetGravityNormal();
				float deltaVert = draggedPosition * UpDir - m_AFBodyLastZ;
				if( deltaVert > 0 )
				{
					float liftTimeFrac = (float)(gameLocal.time - m_DragUpTimer)/(float)cv_drag_AF_ground_timer.GetInteger();
					// try a quadratic buildup, sublinear for frac < 1
					liftTimeFrac *= liftTimeFrac;
					draggedPosition -= deltaVert * UpDir * (1.0 - liftTimeFrac);
				}

				m_bAFOffGround = false;
			}
			else
			{
				// If somehow this is not set
				m_bAFOffGround = false;
			}
			
		}
	}

	m_drag.SetDragPosition( draggedPosition );

	// evaluate physics
	// Note: By doing these operations in this order, we overwrite idForce_Drag angular velocity
	// calculations which is what we want so that the manipulation works properly
	m_drag.Evaluate( gameLocal.time );
	this->ManipulateObject( player );

	/* renderEntity_t* */ renderEntity = drag->GetRenderEntity();
	/* idAnimator* */ dragAnimator = drag->GetAnimator();

	if ( m_joint != INVALID_JOINT && renderEntity && dragAnimator ) 
	{
		dragAnimator->GetJointTransform( m_joint, gameLocal.time, draggedPosition, axis );
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
		FrobEnt = g_Global.m_DarkModPlayer->m_FrobEntity.GetEntity();
		if( !FrobEnt )
#ifdef __linux__
			return;
#else
			goto Quit;
#endif

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
#ifdef __linux__
		return;
#else
		goto Quit;
#endif
	
// Set up the distance and orientation and stuff

	// TODO: The !idStaticEntity here is temporary to fix a bug that should go away once frobbing moveables works again
	if( newEnt->IsType(idStaticEntity::Type) )
#ifdef __linux__
		return;
#else
		goto Quit;
#endif

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
	m_MinHeldDist = int(newEnt->spawnArgs.GetFloat("hold_distance_min", "-1" ));
	if( m_MinHeldDist < 0 )
		m_MinHeldDist = int(MIN_HELD_DISTANCE);

	delta2 = COMWorld - viewPoint;
	m_DistanceCount = int(idMath::Floor( m_MaxDistCount * (delta2.Length() - m_MinHeldDist) / (newEnt->m_FrobDistance - m_MinHeldDist ) ));
	m_DistanceCount = idMath::ClampInt( 0, m_MaxDistCount, m_DistanceCount );

	// prevent collision with player
	// set the clipMask so that the objet only collides with the world
	AddToClipList( m_dragEnt.GetEntity() );

	// signal object manipulator to update drag position so it's relative to the objects
	// center of mass instead of its origin
	m_rotationAxis = -1;

	if( newEnt->IsType(idAFEntity_Base::Type)
		&& static_cast<idAFEntity_Base *>(newEnt)->m_bDragAFDamping == true )
	{
		m_drag.Init( cv_drag_damping_AF.GetFloat() );
	}
	else
		m_drag.Init( cv_drag_damping.GetFloat() );

	m_drag.SetPhysics( phys, m_id, m_localEntityPoint );

	player->m_bGrabberActive = true;
	// don't let the player switch weapons or items
	player->SetImmobilization( "Grabber", EIM_ITEM_SELECT | EIM_WEAPON_SELECT );

	// Set movement encumbrance
	player->SetHinderance( "Grabber", 1.0f, m_dragEnt.GetEntity()->spawnArgs.GetFloat("grab_encumbrance", "1.0") );

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
	idVec3 rotationVec(0.0, 0.0, 0.0);
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
	if( idMath::Fabs( m_player.GetEntity()->usercmd.mx - m_mousePosition.x ) > MOUSE_DEADZONE ||
		idMath::Fabs( m_player.GetEntity()->usercmd.my - m_mousePosition.y ) > MOUSE_DEADZONE )
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
	int contents = phys->GetContents();

	obj.m_ent = ent;
	obj.m_clipMask = clipMask;
	obj.m_contents = contents;

	m_clipList.AddUnique( obj );

	// set the clipMask so that the player won't colide with the object but it still
	// collides with the world
	phys->SetClipMask( clipMask & (~MASK_PLAYERSOLID) );
	phys->SetClipMask( phys->GetClipMask() | CONTENTS_SOLID );

	phys->SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );

	if( HasClippedEntity() ) 
	{
		PostEventMS( &EV_Grabber_CheckClipList, CHECK_CLIP_LIST_INTERVAL );
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
	if( index != -1)
	{
		if (m_clipList[index].m_ent.GetEntity() != NULL)
		{
			m_clipList[index].m_ent.GetEntity()->GetPhysics()->SetClipMask( m_clipList[index].m_clipMask );
			m_clipList[index].m_ent.GetEntity()->GetPhysics()->SetContents( m_clipList[index].m_contents );
		}
		m_clipList.RemoveIndex( index );
	}

	if( !this->HasClippedEntity() )
	{
		// cancel CheckClipList because the list is empty
		this->CancelEvents( &EV_Grabber_CheckClipList );
	}
}

void CGrabber::RemoveFromClipList(idEntity* entity)
{
	for (int i = 0; i < m_clipList.Num(); i++) {
		if (m_clipList[i].m_ent.GetEntity() == entity) {
			m_clipList.RemoveIndex(i);
			break;
		}
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
	num = gameLocal.clip.EntitiesTouchingBounds( m_player.GetEntity()->GetPhysics()->GetAbsBounds(), CONTENTS_SOLID, ent, MAX_GENTITIES );
	for( i = 0; i < m_clipList.Num(); i++ ) 
	{
		// Check clipEntites against entities touching player

		// We keep an entity if it is the one we're dragging 
		if( this->GetSelected() == m_clipList[i].m_ent.GetEntity() ) 
		{
			keep = true;
		}
		else 
		{
			keep = false;

			// OR if it's touching the player and still in the clipList
			for( j = 0; !keep && j < num; j++ ) 
			{
				if( m_clipList[i].m_ent.GetEntity() == ent[j] ) 
					keep = true;
			}
		}

		// Note we have to decrement i otherwise we skip entities
		if( !keep ) 
		{
			this->RemoveFromClipList( i );
			i -= 1;
		}
	}

	if( this->HasClippedEntity() ) 
	{
		this->PostEventMS( &EV_Grabber_CheckClipList, CHECK_CLIP_LIST_INTERVAL );
	}
}

void CGrabber::SetPhysicsFromDragEntity() 
{
	if (m_dragEnt.GetEntity() != NULL)
	{
		m_drag.SetPhysics(m_dragEnt.GetEntity()->GetPhysics(), m_id, m_localEntityPoint );
	}
}

/*
==============
CGrabber::IsInClipList
==============
*/
// TODO / FIXME: Will this work if we don't set the matching contents and clipmask also?
bool CGrabber::IsInClipList( idEntity *ent ) const 
{
	CGrabbedEnt obj;
	
	obj.m_ent = ent;

	// check if the entity is in the clipList
	if( m_clipList.FindIndex( obj ) == -1 ) 
	{
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
	ImpulseVec = m_player.GetEntity()->firstPersonViewAxis[0];
	ImpulseVec.Normalize();

	FracPower = (float) HeldTime / (float) cv_throw_time.GetInteger();

	if( FracPower > 1.0 )
		FracPower = 1.0;

	// Try out a linear scaling between max and min
	ThrowImpulse = cv_throw_min.GetFloat() + (cv_throw_max.GetFloat() - cv_throw_min.GetFloat()) * FracPower;
	ImpulseVec *= ThrowImpulse;  

	ClampVelocity( MAX_RELEASE_LINVEL, MAX_RELEASE_ANGVEL, m_id );

	// Only apply the impulse for throwable items
	if (ent->spawnArgs.GetBool("throwable", "1")) {
		ent->ApplyImpulse( m_player.GetEntity(), m_id, ent->GetPhysics()->GetOrigin(), ImpulseVec );
	}

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

	gameLocal.clip.TraceBounds( trace, viewPoint, orig, ent->GetPhysics()->GetBounds(), ContentsMask, player );

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

bool CGrabber::FitsInHands(idEntity *ent, idPlayer *player, int bodyID)
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

	gameLocal.clip.TraceBounds( trace, viewPoint, orig, ent->GetPhysics()->GetBounds(), ContentsMask, player );

	if( trace.fraction < 1.0f )
		bReturnVal = false;
	else
		bReturnVal = true;

	ent->SetOrigin( initOrigin );

Quit:
	if(bStartedHidden)
		ent->Hide();

	return bReturnVal;
}
