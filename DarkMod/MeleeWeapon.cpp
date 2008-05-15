/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"
#include "MeleeWeapon.h"

CLASS_DECLARATION( idMoveable, CMeleeWeapon )
END_CLASS

CMeleeWeapon::CMeleeWeapon( void ) 
{
}

CMeleeWeapon::~CMeleeWeapon( void )
{
	ClearClipModel();
}

void CMeleeWeapon::ActivateAttack
( 
	idActor *ActOwner, idStr AttName, idDict *DamageDef,
	bool bChangeCM, bool bWorldCollide
)
{
	m_bAttacking = true;
	m_Owner = ActOwner;
	m_bWorldCollide = bWorldCollide;

	if( bChangeCM )
	{
		SetupClipModel();
	}
}

void CMeleeWeapon::DeactivateAttack( void )
{
	m_bAttacking = false;
	ClearClipModel();
}


void CMeleeWeapon::ActivateParry( idActor *ActOwner, idStr ParryName, bool bChangeCM )
{
	m_bParrying = true;
	idClipModel *pClip;

	if( bChangeCM )
	{
		// lookup CM for given parry name
		SetupClipModel();
		pClip = m_WeapClip;
	}
	else
		pClip = GetPhysics()->GetClipModel();

	pClip->SetContents( m_WeapClip->GetContents() | CONTENTS_MELEEWEAP );
}

void CMeleeWeapon::DeactivateParry( void )
{
	m_bParrying = false;
	ClearClipModel();
}

void CMeleeWeapon::ClearClipModel( void )
{
	if( m_WeapClip )
	{
		delete m_WeapClip;
		m_WeapClip = NULL;
	}
	else
	{
		idClipModel *pClip = GetPhysics()->GetClipModel();
		pClip->SetContents( m_WeapClip->GetContents() & (~CONTENTS_MELEEWEAP) );
	}

	m_bClipAxAlign = true;
}

void CMeleeWeapon::Think( void )
{
	// TODO: Figure out how to skip this initialization if we're not active
	idVec3 OldOrigin;
	idMat3 OldAxis;

	if( m_bAttacking )
	{
		OldOrigin = GetPhysics()->GetOrigin();
		OldAxis = GetPhysics()->GetAxis();
	}

	idMoveable::Think();

	// Move the custom clipmodel around to match the weapon
	if( m_WeapClip )
	{
		idMat3 CMaxis;

		if( m_bClipAxAlign )
			CMaxis = GetPhysics()->GetAxis();
		else
			CMaxis = mat3_identity;

		m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), CMaxis );
	}

	// Run the checks for an attack, handle what happens when something is hit
	if( m_bAttacking )
		CheckAttack( OldOrigin, OldAxis );
}

void CMeleeWeapon::TestParry( CMeleeWeapon *other, idVec3 dir, trace_t *trace )
{
	// TODO: Check parry type (slash, thrust) against attack type

	// Notify ourself of hitting a parry, and the other AI of making a parry
}

void CMeleeWeapon::CheckAttack( idVec3 OldOrigin, idMat3 OldAxis )
{
	trace_t tr;
	idMat3 axis;
	idClipModel *pClip;
	int ClipMask;

	if( m_WeapClip )
		pClip = m_WeapClip;
	else
		pClip = GetPhysics()->GetClipModel();

	idVec3 NewOrigin = GetPhysics()->GetOrigin();
	idMat3 NewAxis = GetPhysics()->GetAxis();
	TransposeMultiply( OldAxis, NewAxis, axis );

	idRotation rotation = axis.ToRotation();
	rotation.SetOrigin( OldOrigin );

	if( m_bWorldCollide )
		ClipMask = MASK_SHOT_RENDERMODEL;
	else
		ClipMask = CONTENTS_MELEEWEAP | CONTENTS_BODY; // parries and AI

	// NOTE: We really want two ignore entities, but we can't do that,
	// so temporarily set our CONTENTS so that we don't hit ourself
	int contents = pClip->GetContents();
	pClip->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );

	gameLocal.clip.Motion
	( 
		tr, OldOrigin, NewOrigin, 
		rotation, pClip, OldAxis, 
		ClipMask, m_Owner.GetEntity()
	);
	pClip->SetContents( contents );
	
	// hit something 
	if( tr.fraction < 1.0f )
	{
		idEntity *other = gameLocal.entities[ tr.c.entityNum ];

		// TODO: Incorporate angular momentum into dir?
		idVec3 dir = NewOrigin - OldOrigin;
		dir.NormalizeFast();

		idEntity *AttachOwner(NULL);
		if( other->IsType(idAFAttachment::Type) )
			AttachOwner = static_cast<idAFAttachment *>(other)->GetBody();

		// Hit an actor, or an AF attachment that is part of an actor
		if( other->IsType(idActor::Type) || AttachOwner != NULL )
		{
			// Don't do anything if we hit our own AF attachment
			if( AttachOwner != m_Owner.GetEntity() )
			{
				// TODO: Scale damage with instantaneous velocity of the blade?
				MeleeCollision( other, dir, &tr );

				// apply a LARGE tactile alert to AI
				if( other->IsType(idAI::Type) )
					static_cast<idAI *>(other)->TactileAlert( GetOwner(), 100 );
				else if( AttachOwner && AttachOwner->IsType(idAI::Type) )
					static_cast<idAI *>(AttachOwner)->TactileAlert( GetOwner(), 100 );

				DeactivateAttack();
				// TODO: Message owner AI that they hit an AI
			}
		}

		// Hit a melee parry or held object
		else if( tr.c.contents & CONTENTS_MELEEWEAP )
		{
			// hit a parry (make sure we don't hit our own other melee weapons)
			if( other->IsType(CMeleeWeapon::Type)
				&& static_cast<CMeleeWeapon *>(other)->GetOwner() != m_Owner.GetEntity() )
			{
				// Test our attack against their parry
				TestParry( static_cast<CMeleeWeapon *>(other), dir, &tr );
			}
			// hit a held object
			else if( other == g_Global.m_DarkModPlayer->grabber->GetSelected() )
			{
				MeleeCollision( other, dir, &tr );

				// TODO: Message the grabber that the grabbed object has been hit
				// So that it can fly out of player's hands if desired

				// TODO: Message the attacking AI to play a bounce off animation if appropriate

				DeactivateAttack();
			}
		}
		// Hit something else in the world (only happens to the player)
		else
		{
			MeleeCollision( other, dir, &tr );
			
			// TODO: Message the attacking player to play a bounce off animation if appropriate

			DeactivateAttack();
		}
	}
}

void CMeleeWeapon::MeleeCollision( idEntity *other, idVec3 dir, trace_t *tr )
{
	// Physical impulse
	float push = m_MeleeDef->dict.GetFloat( "push" );
	idVec3 impulse = -push * tr->c.normal;
	
	other->ApplyImpulse( this, tr->c.id, tr->c.point, impulse );

	// Damage
	if( other->fl.takedamage )
	{
		other->Damage
		(
			m_Owner.GetEntity(), m_Owner.GetEntity(), 
			dir, m_MeleeDef->GetName(),
			1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(tr->c.id), tr 
		);
	}

	// TODO: Do the particle FX, damage FX and sound
}

void CMeleeWeapon::SetupClipModel( void )
{
	float size;
	idBounds CMBounds;
	idTraceModel trm;
	int numSides(0);

	idDict *args = &m_MeleeDef->dict;

	// TODO: Allow a custom brush-based CM to be used instead of this

	if ( args->GetVector( "cm_mins", NULL, CMBounds[0] ) &&
				args->GetVector( "cm_maxs", NULL, CMBounds[1] ) )
	{
		if ( CMBounds[0][0] > CMBounds[1][0] || CMBounds[0][1] > CMBounds[1][1] || CMBounds[0][2] > CMBounds[1][2] )
		{
			gameLocal.Error( "Invalid melee CM bounds '%s'-'%s' on entity '%s'", CMBounds[0].ToString(), CMBounds[1].ToString(), name.c_str() );
		}
	} 
	else 
	{
		spawnArgs.GetFloat( "cm_size", "10.0", size );
		CMBounds.Zero();
		CMBounds.ExpandSelf( size );
	}

	if ( spawnArgs.GetInt( "cm_cylinder", "0", numSides ) && numSides > 0 )
		trm.SetupCylinder( CMBounds, numSides < 3 ? 3 : numSides );
	else if ( spawnArgs.GetInt( "cm_cone", "0", numSides ) && numSides > 0 )
		trm.SetupCone( CMBounds, numSides < 3 ? 3 : numSides );
	else
		trm.SetupBox( CMBounds );

	m_WeapClip = new idClipModel( trm );
	m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
}





