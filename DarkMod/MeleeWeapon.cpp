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
	// ClearClipModel();
}

void CMeleeWeapon::ActivateAttack( idActor *ActOwner, const char *AttName )
{
	const idKeyValue *key;

	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Activate attack called.  Weapon %s, owner %s, attack name %s.\r",
											name.c_str(), ActOwner->name.c_str(), AttName );

	if( (key = spawnArgs.FindKey(va("att_type_%s", AttName))) != NULL )
	{
		m_MeleeType = (EMeleeTypes) atoi(key->GetValue().c_str());
		m_ActionName = AttName;
		m_bAttacking = true;
		m_Owner = ActOwner;
		m_bWorldCollide = spawnArgs.GetBool(va("att_world_collide_%s", AttName));

		if( spawnArgs.GetBool(va("att_mod_cm_%s", AttName) ) )
		{
			SetupClipModel();
		}
	}
	else
	{
		DM_LOG(LC_WEAPON, LT_WARNING)LOGSTRING("Did not find attack %s on melee weapon %s\r", AttName, name.c_str());
	}
}

void CMeleeWeapon::DeactivateAttack( void )
{
	m_bAttacking = false;
	ClearClipModel();
}


void CMeleeWeapon::ActivateParry( idActor *ActOwner, const char *ParryName )
{
	const idKeyValue *key;
	
	// Test to see if parry exists in our args
	if( (key = spawnArgs.FindKey(va("par_type_%s", ParryName))) != NULL )
	{
		m_MeleeType = (EMeleeTypes) atoi(key->GetValue().c_str());
		m_ActionName = ParryName;
		m_bParrying = true;
		idClipModel *pClip;

		if( spawnArgs.GetBool(va("par_mod_cm_%s", ParryName)) )
		{
			SetupClipModel();
			pClip = m_WeapClip;
		}
		else
			pClip = GetPhysics()->GetClipModel();

		pClip->SetContents( m_WeapClip->GetContents() | CONTENTS_MELEEWEAP );
	}
	else
	{
		// LOG INVALID MELEEDEF ERROR
	}

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
		// causing a crash??
		m_WeapClip->Unlink();
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

	// TODO: Are you sure these coordinates will change if we get moved by a bindmaster calling RunPhysics()?

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
	// TODO: Time interleave this test?
	if( m_bAttacking )
		CheckAttack( OldOrigin, OldAxis );

	// Debug display
	if( m_bAttacking || m_bParrying )
	{
		// For now just look with g_showcollisionmodels 1
	}
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
			
			// TODO: Message the attacking actor to play a bounce off animation if appropriate

			DeactivateAttack();
		}
	}
}

void CMeleeWeapon::MeleeCollision( idEntity *other, idVec3 dir, trace_t *tr )
{
	const char *DamageDefName;
	const idDict *DamageDef;
	float push(0.0f);
	idVec3 impulse(vec3_zero);

	DamageDefName = spawnArgs.GetString( va("def_damage_%s", m_ActionName.c_str()) );
	DamageDef = gameLocal.FindEntityDefDict( DamageDefName, false );

	if( !DamageDef )
		goto Quit;

	// Physical impulse
	push = DamageDef->GetFloat( "push" );
	impulse = -push * tr->c.normal;
	
	other->ApplyImpulse( this, tr->c.id, tr->c.point, impulse );

	// Damage
	if( other->fl.takedamage )
	{
		// TODO: Damage scaling - on the weapon * melee proficiency on the actor
		other->Damage
		(
			m_Owner.GetEntity(), m_Owner.GetEntity(), 
			dir, DamageDefName,
			1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(tr->c.id), tr 
		);
	}

	// TODO: Do the particle FX, damage decals and sound
Quit:
	return;
}

void CMeleeWeapon::SetupClipModel( )
{
	float size(0.0f);
	idBounds CMBounds;
	idTraceModel trm;
	int numSides(0);
	idStr Prefix;
	const char *APrefix, *AName, *cmName;

	if( m_bAttacking )
		APrefix = "att";
	else
		APrefix = "par";

	AName = m_ActionName.c_str();

	// TODO: Allow offset and rotation of CM relative to self origin and axis?
	// Adds CPU cycles and shouldn't be needed with modeled clipmodels

	// Try loading a clipmodel directly from a file
	spawnArgs.GetString( va("%s_clipmodel_%s", APrefix, AName), "", &cmName );
	if ( cmName[0] )
	{
		if ( !collisionModelManager->TrmFromModel( cmName, trm ) ) 
		{
			gameLocal.Error( "CMeleeWeapon '%s': cannot load collision model %s", name.c_str(), cmName );
		}

		// angua: check if the cm is valid
		if (idMath::Fabs(trm.bounds[0].x) == idMath::INFINITY)
		{
			gameLocal.Error( "CMeleeWeapon '%s': invalid collision model %s", name.c_str(), cmName );
		}
	}
	else if ( spawnArgs.GetVector( va("%s_cm_mins_%s", APrefix, AName), NULL, CMBounds[0] ) &&
				spawnArgs.GetVector( va("%s_cm_maxs_%s", APrefix, AName), NULL, CMBounds[1] ) )
	{
		if ( CMBounds[0][0] > CMBounds[1][0] || CMBounds[0][1] > CMBounds[1][1] || CMBounds[0][2] > CMBounds[1][2] )
		{
			gameLocal.Error( "CMeleeWeapon '%s': Invalid clipmodel bounds '%s'-'%s' for action %s", name.c_str(), CMBounds[0].ToString(), CMBounds[1].ToString(), AName );
		}
	} 
	else 
	{
		spawnArgs.GetFloat( va("%s_cm_size_%s", APrefix, AName), "10.0", size );
		CMBounds.Zero();
		CMBounds.ExpandSelf( size );
	}

	if ( spawnArgs.GetInt( va("%s_cm_cylinder_%s", APrefix, AName), "0", numSides ) && numSides > 0 )
		trm.SetupCylinder( CMBounds, numSides < 3 ? 3 : numSides );
	else if ( spawnArgs.GetInt( va("%s_cm_cone_%s", APrefix, AName), "0", numSides ) && numSides > 0 )
		trm.SetupCone( CMBounds, numSides < 3 ? 3 : numSides );
	else
		trm.SetupBox( CMBounds );

	m_WeapClip = new idClipModel( trm );
	m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	// TODO: Set default contents of what??
	// Temporary test:
	m_WeapClip->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
}





