/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "....//idlib/precompiled.h"
#pragma hdrstop

#include "../game/Game_local.h"

#include "Grabber.h"

/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

const idEventDef EV_Grabber_CheckClipList( "<checkClipList>", NULL, NULL );

const int CHECK_CLIP_LIST_INTERVAL =	1000;

const int MAX_PICKUP_DISTANCE =			50.0f;
const int MOUSE_SCALE =					1.0f;
const int ROTATION_DAMPER =				0.9f;
const int MAX_ROTATION_SPEED =			30.0f;

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
}


/*
==============
CGrabber::Clear
==============
*/
void CGrabber::Clear( void ) {
	dragEnt			= NULL;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
}

/*
==============
CGrabber::Spawn
==============
*/
void CGrabber::Spawn( void ) {
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
void CGrabber::StopDrag( void ) {
	this->dragEnt = NULL;
}

/*
==============
CGrabber::Update
==============
*/
void CGrabber::Update( idPlayer *player, bool hold ) {
	idVec3 viewPoint, origin;
	idMat3 viewAxis, axis;
	trace_t trace;
	idEntity *newEnt;
	idAngles angles;
	jointHandle_t newJoint;
	idStr newBodyName;

	// set this just in case we need it later
	this->player = player;

	// if there is an entity selected, we let it go and exit
	if( !hold && this->dragEnt.GetEntity() ) {
		// stop dragging so better put the players gun back
		player->RaiseWeapon();
		this->StopDrag();
		return;
	}

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity selected for dragging
    if ( !dragEnt.GetEntity() ) {
		gameLocal.clip.TracePoint( trace, viewPoint, viewPoint + viewAxis[0] * MAX_PICKUP_DISTANCE, (CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY), player );
		if ( trace.fraction < 1.0f ) {

			newEnt = gameLocal.entities[ trace.c.entityNum ];
			if ( newEnt ) {

				if ( newEnt->GetBindMaster() ) {
					if ( newEnt->GetBindJoint() ) {
						trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
					} else {
						trace.c.id = newEnt->GetBindBody();
					}
					newEnt = newEnt->GetBindMaster();
				}

				if ( newEnt->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base *>(newEnt)->IsActiveAF() ) {
					idAFEntity_Base *af = static_cast<idAFEntity_Base *>(newEnt);

					// joint being dragged
					newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
					// get the body id from the trace model id which might be a joint handle
					trace.c.id = af->BodyForClipModelId( trace.c.id );
					// get the name of the body being dragged
					newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();

				} else if ( !newEnt->IsType( idWorldspawn::Type ) ) {

					if ( trace.c.id < 0 ) {
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
					} else {
						newJoint = INVALID_JOINT;
					}
					newBodyName = "";

				} else {

					newJoint = INVALID_JOINT;
					newEnt = NULL;
				}
			}
			if ( newEnt ) {
				dragEnt = newEnt;
				joint = newJoint;
				id = trace.c.id;
				bodyName = newBodyName;

				idPhysics *phys = dragEnt.GetEntity()->GetPhysics();
				localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
				origin = phys->GetOrigin( id );
				axis = phys->GetAxis( id );
				localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

				// prevent collision with player
				// set the clipMask so that the objet only collides with the world
				this->AddToClipList( this->dragEnt.GetEntity() );

				this->drag.Init( g_dragDamping.GetFloat() );
				this->drag.SetPhysics( phys, id, localEntityPoint );
			}
		}
	}

	// if there is an entity selected for dragging
	idEntity *drag = dragEnt.GetEntity();
	if ( drag ) {
		idVec3 draggedPosition;

		draggedPosition = viewPoint + localPlayerPoint * viewAxis;
		this->drag.SetDragPosition( draggedPosition );

		// evaluate physics
		// Note: By doing these operations in this order, we overwrite idForce_Drag angular velocity
		// calculations which is what we want so that the manipulation works properly
		this->drag.Evaluate( gameLocal.time );
		this->ManipulateObject( player );

		renderEntity_t *renderEntity = drag->GetRenderEntity();
		idAnimator *dragAnimator = drag->GetAnimator();

		if ( joint != INVALID_JOINT && renderEntity && dragAnimator ) {
			dragAnimator->GetJointTransform( joint, gameLocal.time, draggedPosition, axis );
		}
	}
}

/*
==============
CGrabber::ManipulateObject
==============
*/
void CGrabber::ManipulateObject( idPlayer *player ) {
	idEntity *ent;
	idPhysics *physics;

	ent = this->dragEnt.GetEntity();
	if( !ent ) {
		return;
	}

	physics = ent->GetPhysics();
	if ( !physics ) {
		return;
	}

	// NOTES ON OBJECT ROTATION
	// 
	// The way the object rotation works is as follows:
	//	1) Player must be holding BUTTON_ZOOM
	//	2) if the player is holding BUTTON_RUN, rotate about the x-axis
	//	   else then if the mouse first moves along the x axis, rotate about the z-axis
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
	if( player->usercmd.buttons & BUTTON_ZOOM ) {

		switch( this->rotationAxis ) {
			case 1:
				this->rotatePosition.x = (player->usercmd.mx - this->mousePosition.x) * MOUSE_SCALE;				
				this->rotationAxis = 1;
				break;

			case 2:
				this->rotatePosition.y = (player->usercmd.my - this->mousePosition.y) * MOUSE_SCALE;				
				this->rotationAxis = 2;
				break;

			case 3:
				this->rotatePosition.z = (player->usercmd.mx - this->mousePosition.x) * MOUSE_SCALE;
				this->rotationAxis = 3;
				break;

			default:
				// wait for motion on the x-axis, if nothing, check the y-axis.
				if( (player->usercmd.mx - this->mousePosition.x) != 0 ) {
					// if BUTTON_RUN, then toggle rotating the x-axis, else just do the z-axis
					if( player->usercmd.buttons & BUTTON_RUN ) {
						this->rotationAxis = 1;
					}
					else {
						this->rotationAxis = 3;
					}
				}
				else if( (player->usercmd.my - this->mousePosition.y) != 0 ) {
					this->rotationAxis = 2;
				}
		}

		this->rotatePosition.Clamp( rotateMin, rotateMax );
		physics->SetAngularVelocity( this->rotatePosition, this->id );
	}
	else {
		// reset these coordinates so that next time they press zoom the rotation will be fresh
		this->mousePosition.x = player->usercmd.mx;
		this->mousePosition.y = player->usercmd.my;

		// reset rotation information so when the next zoom is pressed we can freely rotate again
		if( this->rotationAxis ) {
			this->rotationAxis = 0;
			this->rotatePosition = vec3_origin;
		}

		physics->SetAngularVelocity( physics->GetAngularVelocity() * ROTATION_DAMPER, this->id );
	}
}

/*
==============
CGrabber::AddToClipList
==============
*/
void CGrabber::AddToClipList( idEntity *ent ) {
	CGrabbedEnt obj;
	idPhysics *phys = ent->GetPhysics();
	int clipMask = phys->GetClipMask();

	obj.ent = ent;
	obj.clipMask = clipMask;

	this->clipList.AddUnique( obj );

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
void CGrabber::RemoveFromClipList( int index ) {
	// remove the entity and reset the clipMask
	if( index != -1 ) {
		this->clipList[index].ent->GetPhysics()->SetClipMask( this->clipList[index].clipMask );
		this->clipList.RemoveIndex( index );
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
void CGrabber::Event_CheckClipList( void ) {
	idEntity *ent[MAX_GENTITIES];
	bool keep;
	int i, j, num;	

	// Check for any entity touching the players bounds
	// If the entity is not in our list, remove it.
	num = gameLocal.clip.EntitiesTouchingBounds( this->player->GetPhysics()->GetAbsBounds(), CONTENTS_SOLID, ent, MAX_GENTITIES );
	for( i = 0; i < this->clipList.Num(); i++ ) {
		// Check clipEntites against entites touching player

		// We keep an entity if it is the one we're dragging 
		if( this->GetSelected() == this->clipList[i].ent ) {
			keep = true;
		}
		else {
			keep = false;

			// OR if it's touching the player and still in the clipList
			for( j = 0; !keep && j < num; j++ ) {
				if( this->clipList[i].ent == ent[j] ) {
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
	if( this->clipList.FindIndex( obj ) == -1 ) {
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
	if( this->clipList.Num() > 0 ) {
		return true;
	}
	return false;
}
