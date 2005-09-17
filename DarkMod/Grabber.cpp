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

const int MAX_PICKUP_DISTANCE =			200.0f;
const int MOUSE_SCALE =					2.5f;
const int DAMPER =						0.4f;

const idVec3 rotateMin( -20.0f, -20.0f, -20.0f );
const idVec3 rotateMax( 20.0f, 20.0f, 20.0f );

/*
==============
CGrabber::CGrabber
==============
*/
CGrabber::CGrabber( void ) {
	cursor = NULL;
	Clear();
}

/*
==============
CGrabber::~CGrabber
==============
*/
CGrabber::~CGrabber( void ) {
	StopDrag();
	selected = NULL;
	delete cursor;
	cursor = NULL;
}


/*
==============
CGrabber::Clear
==============
*/
void CGrabber::Clear() {
	dragEnt			= NULL;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
	selected		= NULL;
}

/*
==============
CGrabber::StopDrag
==============
*/
void CGrabber::StopDrag( void ) {
	dragEnt = NULL;
	if ( cursor ) {
		cursor->BecomeInactive( TH_THINK );
	}
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
				// grabbed entity so reset rotation and lower player weapon
				player->LowerWeapon();

				dragEnt = newEnt;
				selected = newEnt;
				joint = newJoint;
				id = trace.c.id;
				bodyName = newBodyName;

				if ( !cursor ) {
					cursor = ( idCursor3D * )gameLocal.SpawnEntityType( idCursor3D::Type );
					cursor->showCursor = false;
				}

				idPhysics *phys = dragEnt.GetEntity()->GetPhysics();
				localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
				origin = phys->GetOrigin( id );
				axis = phys->GetAxis( id );
				localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

				cursor->drag.Init( g_dragDamping.GetFloat() );
				cursor->drag.SetPhysics( phys, id, localEntityPoint );
				cursor->Show();

				if ( phys->IsType( idPhysics_AF::Type ) ||
						phys->IsType( idPhysics_RigidBody::Type ) ||
							phys->IsType( idPhysics_Monster::Type ) ) {
					cursor->BecomeActive( TH_THINK );
				}
			}
		}
	}

	// if there is an entity selected for dragging
	idEntity *drag = dragEnt.GetEntity();
	if ( drag ) {
		this->ManipulateObject( player );

		cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
		cursor->SetAxis( viewAxis );

		cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

		renderEntity_t *renderEntity = drag->GetRenderEntity();
		idAnimator *dragAnimator = drag->GetAnimator();

		if ( joint != INVALID_JOINT && renderEntity && dragAnimator ) {
			dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
			cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
		} else {
			cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
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

	// if the player holds ZOOM, make the object rotated based on mouse movement
	if( player->usercmd.buttons & BUTTON_ZOOM ) {

		this->rotatePosition.x = (player->usercmd.my - this->mousePosition.y) * MOUSE_SCALE;
		this->rotatePosition.z = (player->usercmd.mx - this->mousePosition.x) * MOUSE_SCALE;

		this->rotatePosition.Clamp( rotateMin, rotateMax );

		physics->SetAngularVelocity( this->rotatePosition );
	}
	else {
		// reset these coordinates so that next time they press zoom the rotation will be fresh
		this->mousePosition.x = player->usercmd.mx;
		this->mousePosition.y = player->usercmd.my;

		physics->SetAngularVelocity( physics->GetAngularVelocity() * DAMPER );
	}
}