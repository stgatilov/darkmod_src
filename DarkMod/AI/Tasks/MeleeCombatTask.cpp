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

#include "MeleeCombatTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

/** Max time hold a parry waiting for a player to make an attack **/
// TODO: Make this a spawnarg?
// TODO: Randomize
const int MAX_PARRY_WAIT = 3000;

// Get the name of this task
const idStr& MeleeCombatTask::GetName() const
{
	static idStr _name(TASK_MELEE_COMBAT);
	return _name;
}

void MeleeCombatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_enemy = owner->GetEnemy();
}

bool MeleeCombatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Melee Combat Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
/*
	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		return true; // terminate me
	}
*/
	// Perform the task according to the current action
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	EMeleeActState ActState = pStatus->m_ActionState;

	if( ActState == MELEEACTION_ATTACK )
		PerformAttack(owner);
	else if( ActState == MELEEACTION_PARRY )
		PerformParry(owner);
	else
		PerformReady(owner);

	return false; // not finished yet
}

void MeleeCombatTask::PerformReady(idAI* owner)
{
	// the "ready" state is where we decide whether to attack or parry
	// For now, always attack
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	CMeleeStatus *pEnStatus = &_enemy.GetEntity()->m_MeleeStatus;

	idStr debugText = "MeleeAction: Ready";
	gameRenderWorld->DrawText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );

	// TODO: Different recovery time based on what just happened (was parried, got hit, etc)
	int NextAttTime = pStatus->m_LastActTime + owner->m_MeleeCurrentAttackRecovery;
	int NextParTime = pStatus->m_LastActTime + owner->m_MeleeCurrentParryRecovery;

	// Can we attack and is the enemy in range?
	if (gameLocal.time > NextAttTime && owner->GetMemory().canHitEnemy )
	{
		StartAttack(owner);
	}

	// if we can't attack and our enemy is attacking us at a threatening range, parry
	// TODO: Figure out how to switch enemies to face & parry a new one
	// TODO: May need different range other than canHitEnemy, if enemy reach exceeds our own
	else if
		( 
			pStatus->m_bCanParry
			&& gameLocal.time > NextParTime
			&& owner->GetMemory().canHitEnemy // then they can hit us?
			&& (pEnStatus->m_ActionState == MELEEACTION_ATTACK)
		)
	{
		StartParry(owner);
	}

	// If we can't attack or parry, wait until we can
}

void MeleeCombatTask::PerformAttack(idAI* owner)
{
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	EMeleeActPhase phase = pStatus->m_ActionPhase;
	
	if( phase == MELEEPHASE_PREPARING )
	{
		idStr debugText = "MeleeAction: Attack, Phase: Preparing";
		gameRenderWorld->DrawText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );
		// don't do anything, animation will update status when it reaches hold point
		return;
	}
	else if( phase == MELEEPHASE_HOLDING )
	{
		// TODO: Decide whether to keep holding the attack or release
		// if player is out of range but close, maybe hold the swing and charge at them for a little while?

		idStr debugText = "MeleeAction: Attack, Phase: Holding";
		gameRenderWorld->DrawText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );
		
		// wait some finite time before releasing (for difficulty tweaking)
		if( (gameLocal.time - pStatus->m_PhaseChangeTime) > owner->m_MeleeCurrentHoldTime )
		{
			owner->Event_PauseAnim( ANIMCHANNEL_TORSO, false );
			owner->Event_MeleeActionReleased();
		}
	}
	// MELEEPHASE_FINISHING
	else
	{
		// check if animation is finished (script will set this when it is)
		idStr debugText = "MeleeAction: Attack, Phase: Finishing";
		gameRenderWorld->DrawText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );

		idStr waitState( owner->WaitState() );
		if( waitState != "melee_action" )
		{
			// TODO: This needs to get set differently if the attack was parried
			// will be set in melee weapon, for now set it to 'missed' here for testing purposes
			pStatus->m_ActionResult = MELEERESULT_AT_MISSED;
			owner->Event_MeleeActionFinished();
		}
	}
}

void MeleeCombatTask::PerformParry(idAI* owner)
{
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	CMeleeStatus *pEnStatus = &_enemy.GetEntity()->m_MeleeStatus;
	EMeleeActPhase phase = pStatus->m_ActionPhase;
	
	if( phase == MELEEPHASE_PREPARING )
	{
		// don't do anything, animation will update status when it reaches hold point
		// TODO: Need some way of differentiating attacks that have a "hold point" from those that don't
		// as a quick hack for now, can just put melee_hold at early frame in the animation
		return;
	}
	else if( phase == MELEEPHASE_HOLDING )
	{
		// Decide whether to keep holding the parry or to release
		bool bRelease = false;
		// If our enemy is no longer attacking, release
		if( pEnStatus->m_ActionState != MELEEACTION_ATTACK )
			bRelease = true;
		// or if enemy is holding for over some time (for now hardcoded)
		else if( pEnStatus->m_ActionPhase == MELEEPHASE_HOLDING
				 && (gameLocal.time - pEnStatus->m_PhaseChangeTime) > MAX_PARRY_WAIT )
			bRelease = true;
		// TODO: Check if enemy is dead or beyond some max range, then stop parrying?
		else
		{
			// otherwise, keep holding the parry
			bRelease = false;
		}

		if( bRelease )
		{
			owner->Event_PauseAnim( ANIMCHANNEL_TORSO, false );
			owner->Event_MeleeActionReleased();
		}
	}
	// MELEEPHASE_FINISHING
	else
	{
		// check if animation is finished (script will set this when it is)
		idStr waitState( owner->WaitState() );
		if( waitState != "melee_action" )
		{
			// TODO: This should be set elsewhere (in CMeleeWeapon probably)
			// for now, set it here
			pStatus->m_ActionResult = MELEERESULT_PAR_ABORTED;
			owner->Event_MeleeActionFinished();
		}
	}
}

void MeleeCombatTask::StartAttack(idAI* owner)
{
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	CMeleeStatus *pEnStatus = &_enemy.GetEntity()->m_MeleeStatus;

	// create subset of possible attacks:
	idList<EMeleeType> attacks = pStatus->m_attacks;
	// if our enemy is parrying a direction, attack along a different direction
	if( pEnStatus->m_ActionState == MELEEACTION_PARRY )
	{
		if( pEnStatus->m_ActionType != MELEETYPE_BLOCKALL )
		{
			attacks.Remove( pEnStatus->m_ActionType );
		}
		// TODO: Shield parries need special handling
		// Either attack the shield to destroy it or wait until it's dropped or flank the parry with footwork
	}

	if (attacks.Num() > 0)
	{
		// choose a random attack from our possible attacks
		int i = gameLocal.random.RandomInt( attacks.Num() );
		i = attacks[i];
		const char *suffix = idActor::MeleeTypeNames[i];

		// update the melee status
		owner->Event_MeleeAttackStarted( i );

		// Set the waitstate, this gets cleared by 
		// the script function when the animation is done.
		owner->SetWaitState("melee_action");

		// script state plays the animation, clearing wait state when done
		// TODO: Why did we have 5 blend frames here?
		owner->SetAnimState(ANIMCHANNEL_TORSO, va("Torso_Melee_%s",suffix), 5);
	}
	else
	{
		// all of our possible attacks are currently being parried
		// TODO: Decide what to do in this case
		// Wait forever?  Attack anyway?  Attack another opponent in our FOV?
	}
}

void MeleeCombatTask::StartParry(idAI* owner)
{
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	CMeleeStatus *pEnStatus = &_enemy.GetEntity()->m_MeleeStatus;
	
	EMeleeType AttType = pEnStatus->m_ActionType;
	EMeleeType ParType;

	// Universal (shield) parry is the best option if we can
	if( pStatus->m_bCanParryAll )
		ParType = MELEETYPE_BLOCKALL;
	else
		ParType = AttType; // match the attack

	const char *suffix = idActor::MeleeTypeNames[ParType];

	// update the melee status
	owner->Event_MeleeParryStarted( ParType );

	// Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("melee_action");

	// script state plays the animation, clearing wait state when done
	// TODO: Why did we have 5 blend frames here?
	owner->SetAnimState(ANIMCHANNEL_TORSO, va("Torso_Parry_%s",suffix), 5);
}

void MeleeCombatTask::OnFinish(idAI* owner)
{
	// ishtvan TODO: Will need different code for when attack is finish vs. parry?
	// TODO: Also need to figure out if we hit or miss, etc.
	CMeleeStatus *pStatus = &owner->m_MeleeStatus;
	pStatus->m_ActionState = MELEEACTION_READY;
	pStatus->m_ActionPhase = MELEEPHASE_PREPARING;

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetWaitState("");
}

void MeleeCombatTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
}

void MeleeCombatTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
}

MeleeCombatTaskPtr MeleeCombatTask::CreateInstance()
{
	return MeleeCombatTaskPtr(new MeleeCombatTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar meleeCombatTaskRegistrar(
	TASK_MELEE_COMBAT, // Task Name
	TaskLibrary::CreateInstanceFunc(&MeleeCombatTask::CreateInstance) // Instance creation callback
);

} // namespace ai
