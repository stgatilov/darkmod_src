/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1998 $
 * $Date: 2008-01-18 10:02:26 -0800 (Fri, 18 Jan 2008) $
 * $Author: ishtvan $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: MeleeWeapon.cpp 1998 2008-01-18 18:02:26Z greebo $", init_version);

#include "../game/game_local.h"
#include "DarkModGlobals.h"
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
		// lookup CM for given attack name
		m_WeapClip = new idClipModel( /* stuff */ );
		m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
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
		m_WeapClip = new idClipModel( /* stuff */ );
		m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
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

	// Attack test:
	if( m_bAttacking )
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
			ClipMask = CONTENTS_MELEEWEAP;

		// NOTE: We really want two ignore entities, but we can't do that,
		// so temporarily set our CONTENTS to none so we don't hit ourself

		int contents = pClip->GetContents();
		pClip->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );

		gameLocal.clip.Motion
		( 
			tr, OldOrigin, NewOrigin, 
			rotation, pClip, OldAxis, 
			ClipMask, m_Owner.GetEntity()
		);

		// Call Collide on both us and other ent if we hit

		pClip->SetContents( contents );
	}
}






