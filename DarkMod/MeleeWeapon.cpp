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
	m_Owner = NULL;
	m_WeapClip = NULL;
	m_bAttacking = false;
	m_bParrying = false;

	m_bClipAxAlign = true;
	m_bWorldCollide = false;

	m_MeleeType = MELEETYPE_OVERHEAD;
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
		// TODO: We shouldn't set the owner every time, should only have to set once
		m_Owner = ActOwner;
		m_bWorldCollide = spawnArgs.GetBool(va("att_world_collide_%s", AttName));

		if( spawnArgs.GetBool(va("att_mod_cm_%s", AttName) ) )
		{
			SetupClipModel();
		}

		m_OldOrigin = GetPhysics()->GetOrigin();
		m_OldAxis = GetPhysics()->GetAxis();
	}
	else
	{
		DM_LOG(LC_WEAPON, LT_WARNING)LOGSTRING("Did not find attack %s on melee weapon %s\r", AttName, name.c_str());
	}
}

void CMeleeWeapon::DeactivateAttack( void )
{
	if( m_bAttacking )
	{
		m_bAttacking = false;
		ClearClipModel();
	}
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
		// TODO: LOG INVALID MELEEDEF ERROR
	}

}

void CMeleeWeapon::DeactivateParry( void )
{
	if( m_bParrying )
	{
		m_bParrying = false;
		ClearClipModel();
	}
}

void CMeleeWeapon::ClearClipModel( void )
{
	if( m_WeapClip )
	{
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
	idMoveable::Think();

	// Move the custom clipmodel around to match the weapon
	if( m_WeapClip )
	{
		idMat3 CMaxis;

		if( m_bClipAxAlign )
			CMaxis = GetPhysics()->GetAxis();
		else
			CMaxis = mat3_identity;

		// Is id = 0 correct here, or will that cause problems with
		// the other clipmodel, which is also zero?
		
		if( m_bParrying )
			m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), CMaxis );
	}

	// Run the checks for an attack, handle what happens when something is hit
	// TODO: Time interleave this test?
	if( m_bAttacking )
	{
		CheckAttack( m_OldOrigin, m_OldAxis );

		m_OldOrigin = GetPhysics()->GetOrigin();
		m_OldAxis = GetPhysics()->GetAxis();
	}

	// TODO: Debug display of the parry clipmodel
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
		ClipMask = MASK_SHOT_RENDERMODEL | CONTENTS_MELEEWEAP | CONTENTS_CORPSE;
	else
		ClipMask = CONTENTS_MELEEWEAP | CONTENTS_CORPSE | CONTENTS_BODY; // parries and AI
	// TODO: Is CONTENTS_BODY going to hit the outer collision box and cause problems?

	// Hack: We really want to ignore more than one entity, but we can't do that,
	// so temporarily set our CONTENTS so that we don't hit ourself
	int contentsEnt = GetPhysics()->GetContents();
	GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );

// Uncomment for debugging
/*
	DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING
		(
			"Check Attack called, old origin %s, new origin %s, old axis %s, new axis %s\r", 
			OldOrigin.ToString(), NewOrigin.ToString(), OldAxis.ToString(), NewAxis.ToString()
		);
*/

	if( cv_melee_debug.GetBool() )
		collisionModelManager->DrawModel( pClip->Handle(), NewOrigin, NewAxis, gameLocal.GetLocalPlayer()->GetEyePosition(), idMath::INFINITY );

	gameLocal.clip.Motion
	(
		tr, OldOrigin, NewOrigin, 
		rotation, pClip, OldAxis, 
		ClipMask, m_Owner.GetEntity()
	);

// APPROXIMATION: Translation by itself can hit rendermodels, but rotations can't...
// This doesn't work well, and hits AI way too far away for some reason.
//	gameLocal.clip.Translation( tr, OldOrigin, NewOrigin, pClip, NewAxis, ClipMask, m_Owner.GetEntity() );
	
	GetPhysics()->SetContents( contentsEnt );
	
	// hit something 
	if( tr.fraction < 1.0f )
	{
		idEntity *other = gameLocal.entities[ tr.c.entityNum ];
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit entity %s\r", other->name.c_str());
		// Show the initial trace collision point
		if( cv_melee_debug.GetBool() )
			gameRenderWorld->DebugArrow( colorBlue, m_OldOrigin, tr.c.point, 3, 1000 );  

		// Secondary trace for when we hit the AF structure of an AI and want
		// to see where we would hit on the actual model
		if(	(tr.c.contents & CONTENTS_CORPSE)
			&& other->IsType(idAnimatedEntity::Type) )
		{
			trace_t tr2;

			idVec3 delta = tr.c.normal;
			idVec3 start = tr.c.point + 8.0f * delta;

			int contentsEnt = GetPhysics()->GetContents();
			GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
			gameLocal.clip.TracePoint
				( 
					tr2, start, tr.c.point - 8.0f * delta, 
					CONTENTS_RENDERMODEL, m_Owner.GetEntity() 
				);
			GetPhysics()->SetContents( contentsEnt );

			if( tr2.fraction < 1.0f )
			{
				tr = tr2;
				other = gameLocal.entities[ tr.c.entityNum ];
				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: CONTENTS_CORPSE secondary trace hit entity %s\r", other->name.c_str());

				// Draw the new collision point
				if( cv_melee_debug.GetBool() )
					gameRenderWorld->DebugArrow( colorRed, start, tr2.c.point, 3, 1000 );
			}
			else
			{
				// If we failed to find anything, draw the attempted trace in green
				// TODO: Also see if we can at least set up damage groups correctly from the AF body hit
				// What about armour setting?
				if( cv_melee_debug.GetBool() )
					gameRenderWorld->DebugArrow( colorGreen, start, tr.c.point - 8.0f * delta, 3, 1000 );
			}
		}


		// TODO: Incorporate angular momentum into knockback/impulse dir?
		idVec3 dir = NewOrigin - OldOrigin;
		dir.NormalizeFast();

		idEntity *AttachOwner(NULL);
		if( other->IsType(idAFAttachment::Type) )
			AttachOwner = static_cast<idAFAttachment *>(other)->GetBody();

		// Hit an actor, or an AF attachment that is part of an actor
		if( other->IsType(idActor::Type) || AttachOwner != NULL )
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit actor or part of actor %s\r", other->name.c_str());
			// Don't do anything if we hit our own AF attachment
			if( AttachOwner != m_Owner.GetEntity() )
			{
				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit AI other than ourselves.\r");
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
			else
			{
				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit yourself.  Stop hitting yourself!\r");
			}
		}

		// Hit a melee parry or held object 
		// (for some reason tr.c.contents erroneously returns CONTENTS_MELEEWEAP for everything)
		else if( other->IsType(CMeleeWeapon::Type) 
			|| (other->GetPhysics() && ( other->GetPhysics()->GetContents(tr.c.id) & CONTENTS_MELEEWEAP) ) )
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit someting with CONTENTS_MELEEWEAP\r");
			// hit a parry (make sure we don't hit our own other melee weapons)
			if( other->IsType(CMeleeWeapon::Type)
				&& static_cast<CMeleeWeapon *>(other)->GetOwner() != m_Owner.GetEntity() )
			{
				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING
					("MeleeWeapon: Hit a melee parry put up by %s\r", 
					  static_cast<CMeleeWeapon *>(other)->GetOwner()->name.c_str() );
				// Test our attack against their parry
				TestParry( static_cast<CMeleeWeapon *>(other), dir, &tr );
			}
			// hit a held object
			else if( other == g_Global.m_DarkModPlayer->grabber->GetSelected() )
			{
				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit an object held by the player\r");
				
				MeleeCollision( other, dir, &tr );

				// TODO: Message the grabber that the grabbed object has been hit
				// So that it can fly out of player's hands if desired

				// TODO: Message the attacking AI to play a bounce off animation if appropriate

				DeactivateAttack();
			}
			else
			{
				DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit something else with CONTENTS_MELEEWEAP (this shouldn't happen!!)\r");
				MeleeCollision( other, dir, &tr );

				DeactivateAttack();
			}
		}
		// Hit something else in the world (only happens to the player)
		else
		{
			DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Hit a non-AI object: %s\r", other->name.c_str());
			MeleeCollision( other, dir, &tr );
			
			// TODO: Message the attacking actor to play a bounce off animation if appropriate

			DeactivateAttack();
		}
	}
}

void CMeleeWeapon::MeleeCollision( idEntity *other, idVec3 dir, trace_t *tr )
{
	const char *DamageDefName;
	const idDict *DmgDef;
	float push(0.0f);
	idVec3 impulse(vec3_zero);
	idStr hitSound, sndName;

	DamageDefName = spawnArgs.GetString( va("def_damage_%s", m_ActionName.c_str()) );
	DmgDef = gameLocal.FindEntityDefDict( DamageDefName, false );

	if( !DmgDef )
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Did not find damage def %s\r", DamageDefName);
		goto Quit;
	}

	// Physical impulse
	push = DmgDef->GetFloat( "push" );
	impulse = -push * tr->c.normal;
	other->ApplyImpulse( this, tr->c.id, tr->c.point, impulse );

	// Damage
	if( other->fl.takedamage )
	{
		DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeWeapon: Applying damage at body id %d, joint handle %d\r", tr->c.id, CLIPMODEL_ID_TO_JOINT_HANDLE(tr->c.id) );
		// TODO: Damage scaling - on the weapon * melee proficiency on the actor
		other->Damage
		(
			m_Owner.GetEntity(), m_Owner.GetEntity(), 
			dir, DamageDefName,
			1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(tr->c.id), tr 
		);
	}

	// apply a LARGE tactile alert to AI
	if( other->IsType(idAI::Type) )
	{
		static_cast<idAI *>(other)->TactileAlert( m_Owner.GetEntity(), 100 );
	}

	// copied from idWeapon, not necessarily what we want
	// Moved impact_damage_effect to DmgDef instead of weapon ent spawnargs
	if ( DmgDef->GetBool( "impact_damage_effect" ) ) 
	{
		if ( other->spawnArgs.GetBool( "bleed" ) ) 
		{
			hitSound = DmgDef->GetString( "snd_hit" );
			sndName = "snd_hit";

			// places wound overlay, also tries to play another sound that's usually not there?
			// on AI, also does the blood spurt particle
			other->AddDamageEffect( *tr, impulse, DmgDef->GetString( "classname" ) );
		} else 
		{
			// we hit an entity that doesn't bleed, 
			// decals, sound and smoke are handled here instead
			// TODO: Change this so that above is only executed if it bleeds AND we hit flesh?

			idStr materialType;
			int type = tr->c.material->GetSurfaceType();

			if ( type == SURFTYPE_NONE ) 
				materialType = gameLocal.sufaceTypeNames[ SURFTYPE_METAL ];
			else
				g_Global.GetSurfName( tr->c.material, materialType );

			// Uncomment for surface type debugging
			// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Surface hit was %s\r", tr->c.material->GetName() );
			// DM_LOG(LC_WEAPON,LT_DEBUG)LOGSTRING("MeleeCollision: Material type name was %s\r", materialType.c_str() );

			// start impact sound based on material type
			hitSound = DmgDef->GetString( va( "snd_%s", materialType.c_str() ) );
			sndName = va( "snd_%s", materialType.c_str() );

			if ( hitSound.IsEmpty() ) 
			{
				hitSound = DmgDef->GetString( "snd_metal" );
				sndName = "snd_metal";
			}

			// project decal 
			// ishtvan: got rid of min time between decals, let it be up to anim
			const char *decal;
			decal = DmgDef->GetString( "mtr_strike" );
			if ( decal && *decal ) 
			{
				gameLocal.ProjectDecal( tr->c.point, -tr->c.normal, 8.0f, true, 6.0, decal );
			}

			// TODO: Strike smoke FX (sparks.. blood is handled above for now?)

			// strike smoke FX (how is this different from AddDamageEffect?

			// strikeSmokeStartTime = gameLocal.time;
			// strikePos = tr.c.point;
			// strikeAxis = -tr.endAxis;
		}
	}

	if ( !hitSound.IsEmpty() ) 
	{
		const idSoundShader *snd = declManager->FindSound( hitSound.c_str() );
		StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
		
		// Propagate the sound to AI, must find global sound first because it's on a different dict
		sndName.StripLeading("snd_");
		sndName = DmgDef->GetString( va("sprS_%s", sndName.c_str()) );
		if( !sndName.IsEmpty() )
			PropSoundDirect( sndName.c_str(), false, false );
	}

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
	
	// Only parries need a linked and auto-upated clipmodel
	if( m_bParrying )
		m_WeapClip->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	// TODO: Set default contents of what??  
	// We don't need to set meleeweapon since that's done explicitly in the trace
	// Temporary test:
	// m_WeapClip->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
}





