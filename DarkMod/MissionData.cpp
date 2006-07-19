/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.11  2006/07/19 21:51:03  ishtvan
 * added irreversible behavior, modified some internal functions
 *
 * Revision 1.10  2006/07/19 09:10:09  ishtvan
 * bugfixes
 *
 * Revision 1.9  2006/07/19 05:19:49  ishtvan
 * added enabling objectives and scripts to call when objective completes
 *
 * Revision 1.8  2006/07/17 02:42:25  ishtvan
 * fixes to comp_custom_clocked and comp_distance
 *
 * Revision 1.7  2006/07/17 01:45:59  ishtvan
 * updates: custom objectives, distance objectives, custom clocked objectives
 *
 * Revision 1.6  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.5  2006/06/07 09:56:15  ishtvan
 * fixed CObjecitveLocation so that the clipmodel is actually detected as the bounds
 *
 * Revision 1.4  2006/06/07 09:03:28  ishtvan
 * location component updates
 *
 * Revision 1.3  2006/05/30 06:22:04  ishtvan
 * added parsing of objectives from entity
 *
 * Revision 1.2  2006/05/28 08:41:22  ishtvan
 * mission failure now calls death menu event on player
 *
 * Revision 1.1  2006/05/26 10:24:39  ishtvan
 * Initial release
 *
 *
 *
 ***************************************************************************/

#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#pragma warning(disable : 4996)

#include "MissionData.h"
#include "../game/player.h"

// TODO: Move to config file or player spawnargs
const int s_FAILURE_FADE_TIME = 3000;

CObjectiveComponent::CObjectiveComponent( void )
{
	m_bNotted = false;
	m_bState = false;
	m_bReversible = true;
	m_bLatched = false;
	m_Type = COMP_ITEM;
	m_SpecMethod[0] = SPEC_NONE;
	m_SpecMethod[1] = SPEC_NONE;
	m_SpecIntVal[0] = 0;
	m_SpecIntVal[1] = 0;
	m_IntArgs.Clear();
	m_StrArgs.Clear();

	m_ClockInterval = 1000;
	m_TimeStamp = 0;

	m_Index[0] = 0;
	m_Index[1] = 0;
}

CObjectiveComponent::~CObjectiveComponent( void )
{
	m_SpecStrVal[0].Clear();
	m_SpecStrVal[1].Clear();

	m_IntArgs.Clear();
	m_StrArgs.Clear();
}

bool CObjectiveComponent::SetState( bool bState )
{
	bool bReturnVal(false);

// TODO: Check for irreversible, if it is irreversible and already changed, do not change back

	if( m_bNotted )
		bState = !bState;

	if(bState != m_bState)
	{
		// state has changed, check for latching to see if it can change
		if( !m_bReversible )
		{
			if( !m_bLatched )
			{
				m_bLatched = true;
				m_bState = bState;
				bReturnVal = true;
			}
		}
		else
		{
			m_bState = bState;
			bReturnVal = true;
		}
	}

	return bReturnVal;
}

CMissionData::CMissionData( void )
{
	Clear();

// Initialize Hash indexes used for parsing string names to enum index
	idStrList CompTypeNames, SpecTypeNames;

/**
* Add new component type names here.  Must be in exact same order as EComponentType
*	enum, defined in MissionData.h
**/
	CompTypeNames.Append("kill");
	CompTypeNames.Append("ko");
	CompTypeNames.Append("ai_find_item");
	CompTypeNames.Append("ai_find_body");
	CompTypeNames.Append("alert");
	CompTypeNames.Append("item");
	CompTypeNames.Append("location");
	CompTypeNames.Append("custom");
	CompTypeNames.Append("custom_clocked");
	CompTypeNames.Append("distance");

/**
* Add in new specification types here.  Must be in exact same order as
*	ESpecificationMethod enum, defined in MissionData.h
**/
	SpecTypeNames.Append("none");
	SpecTypeNames.Append("name");
	SpecTypeNames.Append("overall");
	SpecTypeNames.Append("group");
	SpecTypeNames.Append("classname");
	SpecTypeNames.Append("spawnclass");
	SpecTypeNames.Append("ai_type");
	SpecTypeNames.Append("ai_team");
	SpecTypeNames.Append("ai_innocence");

	CompTypeNames.Condense();
	SpecTypeNames.Condense();

	for( int i=0; i < CompTypeNames.Num(); i++ )
	{
		m_CompTypeHash.Add( m_CompTypeHash.GenerateKey( CompTypeNames[i].c_str(), false ), i );
	}
	for( int i=0; i < SpecTypeNames.Num(); i++ )
	{
		m_SpecTypeHash.Add( m_SpecTypeHash.GenerateKey( SpecTypeNames[i].c_str(), false ), i );
	}
}

CMissionData::~CMissionData( void )
{
	Clear();
}

void CMissionData::Clear( void )
{
	m_bObjsNeedUpdate = false;
	m_Objectives.Clear();
	m_ClockedComponents.Clear();

	// Clear all the stats (this is kind've ugly)
	// create a cleared stat to copy to all the SStat fields
	SStat ClearedSt;
	ClearedSt.ByInnocence[0] = 0;
	ClearedSt.ByInnocence[1] = 0;
	ClearedSt.WhileAirborne = 0;
	for( int n=0; n<MAX_TEAMS; n++ )
	{
		ClearedSt.ByTeam[n] = 0;
	}
	for( int n=0; n<MAX_TYPES; n++ )
	{
		ClearedSt.ByType[n] = 0;
	}
	ClearedSt.Overall = 0;

	for( int n=0; n<MAX_AICOMP; n++ )
	{
		m_Stats.AIStats[n] = ClearedSt;
	}
	for( int n=0; n<MAX_ALERTNUMS; n++ )
	{
		m_Stats.AIAlerts[n] = ClearedSt;
	}

	m_Stats.DamageDealt = 0;
	m_Stats.DamageReceived = 0;
	m_Stats.LootOverall = 0;
}




void CMissionData::MissionEvent
	( 
		EComponentType CompType, 
		SObjEntParms *EntDat1, 
		SObjEntParms *EntDat2,
		bool bBoolArg 
	)
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Mission event called \r");
	SStat *pStat(NULL);
	bool bCompState;

	if( !EntDat1 )
	{
		// log error
		goto Quit;
	}

	// Update AI stats, don't add to stats if playerresponsible is false
	// Stas for KOs, kills, body found, item found
	if( ( ( CompType == COMP_KILL && EntDat1->bIsAI ) || CompType == COMP_KO 
		|| CompType == COMP_AI_FIND_BODY || CompType == COMP_AI_FIND_ITEM
		|| CompType == COMP_ALERT ) && bBoolArg )
	{
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Determined AI event \r");
		if( CompType == COMP_ALERT )
		{
			if( EntDat1->value > MAX_ALERTNUMS )
			{
				// log error
				goto Quit;
			}
			// index in this array is determined by alert value
			pStat = &m_Stats.AIAlerts[ EntDat1->value ];
		}
		else
			pStat = &m_Stats.AIStats[ CompType ];

		if( CompType > MAX_AICOMP || !pStat)
		{
			DM_LOG(LC_AI,LT_ERROR)LOGSTRING("Objectives: No AI stat found for comptype %d\r", CompType );
			goto Quit;
		}
		
		// Add to all appropriate stats
		pStat->Overall++;
		pStat->ByTeam[ EntDat1->team ]++;
		pStat->ByType[ EntDat1->type ]++;
		pStat->ByInnocence[ EntDat1->innocence ]++;

		if( EntDat1->bWhileAirborne )
			pStat->WhileAirborne++;

		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Done adding to stats, checking for objectives...\r" );
	}

	for( int i=0; i<m_Objectives.Num(); i++ )
	{
		CObjective *pObj = &m_Objectives[i];

		for( int j=0; j < pObj->m_Components.Num(); j++ )
		{
			CObjectiveComponent *pComp;
			pComp = &pObj->m_Components[j];

			// match component type
			if( pComp->m_Type != CompType )
				continue;
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Matching Component found: %d, %d\r", i+1, j+1 );
			
			// check if the specifiers match, for first spec and second if it exists
			if( !MatchSpec(pComp, EntDat1, 0) )
				continue;
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: First specification check matched: %d, %d\r", i+1, j+1 );

			if( pComp->m_SpecMethod[1] != SPEC_NONE )
			{
				if( !MatchSpec(pComp, EntDat2, 1) )
					continue;
			}
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Second specification check matched or absent: %d, %d\r", i+1, j+1 );

			bCompState = EvaluateObjective( pComp, EntDat1, EntDat2, bBoolArg );
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objective component evaluation result: %d \r", (int) bCompState );

			// notify the component of the current state. If the state changed,
			// this will return true and we must mark this objective for update.
			if( pComp->SetState( bCompState ) )
			{
				DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objective %d, Component %d state changed, needs updating", i+1, j+1 );
				pObj->m_bNeedsUpdate = true;
				m_bObjsNeedUpdate = true;
			}
		}
	}
Quit:
	return;
}

void CMissionData::MissionEvent
	(
	EComponentType CompType,
	idEntity *Ent1, idEntity *Ent2,
	bool bBoolArg, bool bWhileAirborne
	)
{
	SObjEntParms data1, data2;

	// at least the first ent must exist
	if(!Ent1)
	{
		// log error
		goto Quit;
	}
	FillParmsData( Ent1, &data1 );
	data1.bWhileAirborne = bWhileAirborne;

	if( !Ent2 )
		MissionEvent( CompType, &data1, NULL, bBoolArg );
	else
	{
		FillParmsData( Ent2, &data2 );
		MissionEvent( CompType, &data1, &data2, bBoolArg );
	}

Quit:
	return;
}

bool	CMissionData::MatchSpec
			(
			CObjectiveComponent *pComp,
			SObjEntParms *EntDat,
			int ind
			)
{
	bool bReturnVal(false);

	// objectives only have two specified ents at max
	if( !pComp || !EntDat || ind > 1 )
		goto Quit;
	ESpecificationMethod SpecMethod = pComp->m_SpecMethod[ ind ];

	switch( SpecMethod )
	{
		case SPEC_NONE:
			bReturnVal = true;
			break;		
		case SPEC_NAME:
			bReturnVal = ( pComp->m_SpecStrVal[ind] == EntDat->name );
			break;
		case SPEC_OVERALL:
			bReturnVal = true;
			break;
		case SPEC_GROUP:
			bReturnVal = ( pComp->m_SpecStrVal[ind] == EntDat->group );
			break;
		case SPEC_CLASSNAME:
			bReturnVal = ( pComp->m_SpecStrVal[ind] == EntDat->classname );
			break;
		case SPEC_SPAWNCLASS:
			bReturnVal = ( pComp->m_SpecStrVal[ind] == EntDat->spawnclass );
			break;
		case SPEC_AI_TYPE:
			bReturnVal = ( pComp->m_SpecIntVal[ind] == EntDat->type );
			break;
		case SPEC_AI_TEAM:
			bReturnVal = ( pComp->m_SpecIntVal[ind] == EntDat->team );
			break;
		case SPEC_AI_INNOCENCE:
			bReturnVal = ( pComp->m_SpecIntVal[ind] == EntDat->innocence );
			break;
		default:
			break;
	}

Quit:
	return bReturnVal;
}

bool	CMissionData::EvaluateObjective
			(
			CObjectiveComponent *pComp,
			SObjEntParms *EntDat1,
			SObjEntParms *EntDat2,
			bool bBoolArg
			)
{
	bool bReturnVal(false);
	int value(0), index(0);

	EComponentType CompType = pComp->m_Type;
	ESpecificationMethod SpecMeth = pComp->m_SpecMethod[0];

	// LOCATION : If we get this far with location, the specifiers
	// already match, that means it's already true and no further evaluation is needed
	if( CompType == COMP_LOCATION )
	{
		// Return value is set to whether the item entered or left the location
		bReturnVal = bBoolArg;
		goto Quit;
	}

	// AI COMPONENTS:
	if( ( CompType == COMP_KILL || CompType == COMP_KO 
		|| CompType == COMP_AI_FIND_BODY || CompType == COMP_AI_FIND_ITEM
		|| CompType == COMP_ALERT ) )
	{
		int AlertNum = 0;
		if( CompType == COMP_ALERT )
			AlertNum = pComp->m_IntArgs[1];

		if( AlertNum < 0 || AlertNum > MAX_ALERTNUMS )
			goto Quit;

		// name, classname and spawnclass are all one-shot objectives and not counted up (for now)
		if( SpecMeth == SPEC_NONE || SpecMeth == SPEC_NAME || SpecMeth == SPEC_CLASSNAME || SpecMeth == SPEC_SPAWNCLASS )
		{
			bReturnVal = true;
			goto Quit;
		}

		switch(SpecMeth)
		{
			case SPEC_OVERALL:
				value = GetStatOverall( CompType, AlertNum );
				break;
			case SPEC_AI_TYPE:
				index = EntDat1->type;
				value = GetStatByType( CompType, index, AlertNum );
				break;
			case SPEC_AI_TEAM:
				index = EntDat1->team;
				value = GetStatByTeam( CompType, index, AlertNum );
				break;
			case SPEC_AI_INNOCENCE:
				index = EntDat1->innocence;
				value = GetStatByInnocence( CompType, index, AlertNum );
				break;
		}

		bReturnVal = value >= pComp->m_IntArgs[0];
	}

	// ITEMS:
	else if( CompType == COMP_ITEM )
	{
		// name, classname and spawnclass are all one-shot objectives and not counted up (for now)
		if( SpecMeth == SPEC_NONE || SpecMeth == SPEC_NAME || SpecMeth == SPEC_CLASSNAME || SpecMeth == SPEC_SPAWNCLASS )
		{
			// Returnval is set based on whether item is entering or leaving inventory
			bReturnVal = bBoolArg;
			goto Quit;
		}

		switch( SpecMeth )
		{
			// overall loot
			case SPEC_OVERALL:
				value = EntDat1->valueSuperGroup;
				break;
			case SPEC_GROUP:
				value = EntDat1->value;
				break;
			default:
				break;
		}
		bReturnVal = value >= pComp->m_IntArgs[0];
	}

Quit:
	return bReturnVal;
}

void CMissionData::UpdateObjectives( void )
{
	bool bTest(true), bObjEnabled(true);

// =============== Begin Handling of Clocked Objective Components ===============
	
	for( int k=0; k < m_ClockedComponents.Num(); k++ )
	{
		CObjectiveComponent *pComp = m_ClockedComponents[k];

		// check if timer is due to fire
		if( !pComp || (gameLocal.time - pComp->m_TimeStamp < pComp->m_ClockInterval) )
			continue;

		// COMP_DISTANCE - Do a distance check
		else if( pComp->m_Type == COMP_DISTANCE )
		{
			pComp->m_TimeStamp = gameLocal.time;

			idEntity *ent1, *ent2;
			idVec3 delta;
			int dist(0);

			ent1 = gameLocal.FindEntity( pComp->m_StrArgs[0].c_str() );
			ent2 = gameLocal.FindEntity( pComp->m_StrArgs[1].c_str() );
			
			if( !ent1 || !ent2 )
			{
				DM_LOG(LC_AI, LT_WARNING)LOGSTRING("Objective %d, component %d: Distance objective component given bad entity names %s , %s \r", pComp->m_Index[0], pComp->m_Index[1], pComp->m_StrArgs[0], pComp->m_StrArgs[1] ); 
				continue;
			}

			delta = ent1->GetPhysics()->GetOrigin();
			delta = delta - ent2->GetPhysics()->GetOrigin();

			dist = pComp->m_IntArgs[0];
			dist *= dist;

			SetComponentState( pComp, ( delta.LengthSqr() < dist ) );
		}

		// COMP_CUSTOM_CLOCKED
		else if( pComp->m_Type == COMP_CUSTOM_CLOCKED )
		{
			pComp->m_TimeStamp = gameLocal.time;

			function_t *pScriptFun = gameLocal.program.FindFunction( pComp->m_StrArgs[0].c_str() );
			
			if(pScriptFun)
			{
				idThread *pThread = new idThread( pScriptFun );
				pThread->CallFunction( pScriptFun, true );
				pThread->DelayedStart( 0 );
			}
			else
			{
				DM_LOG(LC_AI, LT_WARNING)LOGSTRING("Objective %d, component %d: Custom clocked objective called bad script: %s \r", pComp->m_Index[0], pComp->m_Index[1], pComp->m_StrArgs[0].c_str() );
				gameLocal.Printf("WARNING: Objective %d, component %d: Custom clocked objective called bad script: %s \n", pComp->m_Index[0], pComp->m_Index[1], pComp->m_StrArgs[0].c_str() );
			}
		}

	}

// ============== End Handling of  Clocked Objective Components =============

	// Check if any objective states have changed:
	if( !m_bObjsNeedUpdate )
		goto Quit;
	m_bObjsNeedUpdate = false;

	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Objectives in need of updating \r");

	for( int i=0; i<m_Objectives.Num(); i++ )
	{
		CObjective *pObj = &m_Objectives[i];
		
		// skip objectives that don't need updating
		if( !pObj->m_bNeedsUpdate || pObj->m_state == STATE_INVALID )
			continue;
		pObj->m_bNeedsUpdate = false;
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Found objective in need of update: %d \r", i+1);

// TODO: Implement arbitrary boolean logic here, for mission failure and mission success
// For now, just AND everything, and fail the objective if it's ongoing and not successful
		for( int j=0; j < pObj->m_Components.Num(); j++ )
		{
			bTest = bTest && pObj->m_Components[j].m_bState;
		}

		// Objective was just completed
		if( bTest )
		{
			// Check for enabling objectives
			for( int k=0; k < pObj->m_EnablingObjs.Num(); k++ )
			{
				int ObjNum = pObj->m_EnablingObjs[k] - 1;
				if( ObjNum >= m_Objectives.Num() || ObjNum < 0 )
					continue;

				EObjCompletionState CompState = m_Objectives[ObjNum].m_state;

				bObjEnabled = bObjEnabled && (CompState == STATE_COMPLETE || CompState == STATE_INVALID);
			}
			if( !bObjEnabled )
				goto Quit;

			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Objective %d COMPLETED\r", i+1);
			SetCompletionState( i, STATE_COMPLETE );
		}
// TODO: This is temporary and should be replaced with a failure logic check
// For now: If ANY components of an ongoing objective are false, the objective is failed
		else if( pObj->m_bOngoing && !(pObj->m_state == STATE_INVALID) )
		{
				DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: Objective %d FAILED\r", i);
				SetCompletionState(i, STATE_FAILED );
		}
		else
		{
			pObj->m_state = STATE_INCOMPLETE;
		}
	}

Quit:
	return;
}

void CMissionData::Event_ObjectiveComplete( int ind )
{
	bool bTest(true), bTemp(false);

	// Ongoing objectives don't play the sound or mark off in the GUI as complete during mission
	if( !m_Objectives[ind].m_bOngoing )
	{
		idPlayer *   player;
		player = gameLocal.localClientNum >= 0 ? static_cast<idPlayer *>( gameLocal.entities[ gameLocal.localClientNum ] ) : NULL;

		// TODO: Play this sound on the player, not the world, because global channel will cut off ambients
		player->StartSound("snd_objective_complete", SND_CHANNEL_ANY, 0, false, NULL);

		// call completion script
		function_t *pScriptFun = gameLocal.program.FindFunction( m_Objectives[ind].m_CompletionScript.c_str() );
		if(pScriptFun)
		{
			idThread *pThread = new idThread( pScriptFun );
			pThread->CallFunction( pScriptFun, true );
			pThread->DelayedStart( 0 );
		}		

// TODO: Update the GUI to mark the objective as complete

	}

	// check if all mandatory, valid and active objectives have been completed
	// If so, the mission is complete
	for( int i=0; i<m_Objectives.Num(); i++ )
	{
		CObjective *pObj = &m_Objectives[i];
		bTemp = ( pObj->m_state == STATE_COMPLETE || pObj->m_state == STATE_INVALID 
					 || !pObj->m_bMandatory );
		bTest = bTest && bTemp;
	}

	if( bTest )
		Event_MissionComplete();
}

void CMissionData::Event_ObjectiveFailed( int ind )
{
	// if the objective was mandatory, fail the mission
	if( m_Objectives[ind].m_bMandatory )
		Event_MissionFailed();
	else
	{
		// play an objective failed sound for optional objectives?
		
		// call failure script
		function_t *pScriptFun = gameLocal.program.FindFunction( m_Objectives[ind].m_FailureScript.c_str() );
		if(pScriptFun)
		{
			idThread *pThread = new idThread( pScriptFun );
			pThread->CallFunction( pScriptFun, true );
			pThread->DelayedStart( 0 );
		}
	}
}

void CMissionData::Event_MissionComplete( void )
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: MISSION COMPLETE. \r");
	gameLocal.Printf("MISSION COMPLETED\n");
	
	// TODO: Go to mission successful GUI
	// TODO: Read off which map to go to next

	// for now, just play the sound (later it will be played in the GUI)
	idPlayer *player = gameLocal.GetLocalPlayer();
	if(player)
	{
		player->StartSoundShader( declManager->FindSound( "mission_complete" ), SND_CHANNEL_ANY, 0, false, NULL );
	}
}

void CMissionData::Event_MissionFailed( void )
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objectives: MISSION FAILED. \r");
	gameLocal.Printf("MISSION FAILED\n");
	
	idPlayer *player = gameLocal.GetLocalPlayer();
	if(player)
	{
		player->playerView.Fade( colorBlack, s_FAILURE_FADE_TIME );
		player->PostEventMS( &EV_Player_DeathMenu, s_FAILURE_FADE_TIME + 1 );
	}
}


// ============================== Stats =================================

int CMissionData::GetStatOverall( EComponentType CompType, int AlertNum )
{
	int returnVal(0);
	
	if( AlertNum < 0 || AlertNum > MAX_ALERTNUMS )
		goto Quit;

	if( CompType == COMP_ALERT )
		returnVal = m_Stats.AIAlerts[ AlertNum ].Overall;
	else
		returnVal = m_Stats.AIStats[ CompType ].Overall;

Quit:
	return returnVal;
}

int CMissionData::GetStatByTeam( EComponentType CompType, int index, int AlertNum )
{
	int returnVal(0);
	
	if( AlertNum < 0 || AlertNum > MAX_ALERTNUMS )
		goto Quit;
	if( CompType == COMP_ALERT )
		returnVal = m_Stats.AIAlerts[ AlertNum ].ByTeam[index];
	else
		returnVal = m_Stats.AIStats[ CompType ].ByTeam[index];

Quit:
	return returnVal;
}

int CMissionData::GetStatByType( EComponentType CompType, int index, int AlertNum )
{
	int returnVal(0);
	
	if( AlertNum < 0 || AlertNum > MAX_ALERTNUMS )
		goto Quit;
	if( CompType == COMP_ALERT )
		returnVal = m_Stats.AIAlerts[ AlertNum ].ByType[index];
	else
		returnVal = m_Stats.AIStats[ CompType ].ByType[index];

Quit:
	return returnVal;
}

int CMissionData::GetStatByInnocence( EComponentType CompType, int index, int AlertNum )
{
	int returnVal(0);
	
	if( AlertNum < 0 || AlertNum > MAX_ALERTNUMS )
		goto Quit;
	if( CompType == COMP_ALERT )
		returnVal = m_Stats.AIAlerts[ AlertNum ].ByInnocence[index];
	else
		returnVal = m_Stats.AIStats[ CompType ].ByInnocence[index];

Quit:
	return returnVal;
}

int CMissionData::GetStatAirborne( EComponentType CompType, int AlertNum )
{
	int returnVal(0);
	
	if( AlertNum < 0 || AlertNum > MAX_ALERTNUMS )
		goto Quit;
	if( CompType == COMP_ALERT )
		returnVal = m_Stats.AIAlerts[ AlertNum ].WhileAirborne;
	else
		returnVal = m_Stats.AIStats[ CompType ].WhileAirborne;

Quit:
	return returnVal;
}

void CMissionData::AIDamagedByPlayer( int DamageAmount )
{
	m_Stats.DamageDealt += DamageAmount;
}

void CMissionData::PlayerDamaged( int DamageAmount )
{
	m_Stats.DamageReceived += DamageAmount;
}

int CMissionData::GetDamageDealt( void )
{
	return m_Stats.DamageDealt;
}

int CMissionData::GetDamageReceived( void )
{
	return m_Stats.DamageReceived;
}

// ============================== Misc.  ==============================

void CMissionData::FillParmsData( idEntity *ent, SObjEntParms *parms )
{
	if(!ent || !parms)
		goto Quit;

	parms->name = ent->name;
	// TODO: Replace with whatever method Gildoran uses to assign ents to inventory groups
	parms->group = ent->spawnArgs.GetString("inv_group"); // need this because AI might find an item
	parms->classname = ent->spawnArgs.GetString("classname");
	parms->spawnclass = ent->spawnArgs.GetString("spawnclass");

	if( ent->IsType(idActor::Type) )
	{
		idActor *actor = static_cast<idActor *>(ent);
		
		parms->team = actor->team;
		parms->type = actor->m_AItype;
		parms->innocence = (int) actor->m_Innocent;
		parms->bIsAI = true;
	}

Quit:
	return;
}

void CMissionData::SetComponentState_Ext( int ObjIndex, int CompIndex, bool bState )
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("SetComponentState: Called for obj %d, comp %d, state %d. \r", ObjIndex, CompIndex, (int) bState );

	// Offset the indices into "internal" values (start at 0)
	ObjIndex--;
	CompIndex--;

	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0  )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("SetComponentState: Objective num %d out of bounds. \r", (ObjIndex+1) );
		goto Quit;
	}
	if( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("SetComponentState: Component num %d out of bounds for objective %d. \r", (CompIndex+1), (ObjIndex+1) );
		goto Quit;
	}

	// call internal SetComponentState
	SetComponentState( ObjIndex, CompIndex, bState );

Quit:
	return;
}

void CMissionData::SetComponentState(int ObjIndex, int CompIndex, bool bState)
{
	CObjectiveComponent *pComp(NULL);

	pComp = &m_Objectives[ObjIndex].m_Components[CompIndex];
	
	if( !pComp )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("SetComponentState: NULL component found \r" );
		goto Quit;
	}

	if( pComp->SetState( bState ) )
	{
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("SetComponentState: Objective %d, Component %d state changed, needs updating", (ObjIndex+1), (CompIndex+1) );
		m_Objectives[ObjIndex].m_bNeedsUpdate = true;
		m_bObjsNeedUpdate = true;
	}

Quit:
	return;
}

void CMissionData::SetComponentState( CObjectiveComponent *pComp, bool bState )
{
	if( !pComp )
		goto Quit;

	SetComponentState( pComp->m_Index[0]-1, pComp->m_Index[1]-1, bState );

Quit:
	return;
}

void CMissionData::SetCompletionState( int ObjIndex, int State )
{
	CObjective *pObj = NULL;

	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("Attempt was made to set completion state of invalid objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to set completion state of invalid objective index: %d \n", ObjIndex);
		goto Quit;
	}

	// check if the state int is valid by comparing to highest number in enum
	if( State < 0 || State > STATE_FAILED )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("Attempt was made to set objective index: %d to invalid completion state: %d \r", ObjIndex, State);
		gameLocal.Printf("WARNING: Objective system: Attempt was made to set objective index: %d to invalid completion state: %d \n", ObjIndex, State);
		goto Quit;
	}

	pObj = &m_Objectives[ObjIndex];
	if( !pObj )
	{
		DM_LOG(LC_AI,LT_ERROR)LOGSTRING("SetCompletionState: NULL Objective found for obj %d \r", ObjIndex );
		goto Quit;
	}

	// Don't do anything if we are already in that state
	if( pObj->m_state == State )
		goto Quit;

	// Check for latching:
	if( !pObj->m_bReversible )
	{
		// do not do anything if latched
		if( pObj->m_bLatched )
			goto Quit;

		// Irreversible objectives latch to either complete or failed
		if( State == STATE_COMPLETE || State == STATE_FAILED )
			pObj->m_bLatched = true;
	}


	m_Objectives[ObjIndex].m_state = (EObjCompletionState) State;

	if( State == STATE_COMPLETE )
		Event_ObjectiveComplete( ObjIndex );
	else if( State == STATE_FAILED )
		Event_ObjectiveFailed( ObjIndex );
Quit:
	return;
}

// for scripters:

int CMissionData::GetCompletionState( int ObjIndex )
{
	int returnInt = -1;

	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("GetCompletionState: Bad objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to get completion state of invalid objective index: %d \n", ObjIndex);
		goto Quit;
	}

	returnInt = m_Objectives[ObjIndex].m_state;

Quit:
	return returnInt;
}

bool CMissionData::GetComponentState( int ObjIndex, int CompIndex )
{
	bool bReturnVal(false);

	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0  )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("GetComponentState: Objective num %d out of bounds. \r", (ObjIndex+1) );
		gameLocal.Printf("WARNING: Objective System: GetComponentState: Objective num %d out of bounds. \n", (ObjIndex+1) ); 
		goto Quit;
	}
	if( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("GetComponentState: Component num %d out of bounds for objective %d. \r", (CompIndex+1), (ObjIndex+1) );
		gameLocal.Printf("WARNING: Objective System: GetComponentState: Component num %d out of bounds for objective %d. \n", (CompIndex+1), (ObjIndex+1) );
		goto Quit;
	}

	bReturnVal = m_Objectives[ObjIndex].m_Components[CompIndex].m_bState;

Quit:
	return bReturnVal;
}

void CMissionData::UnlatchObjective( int ObjIndex )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("UnlatchObjective: Bad objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to unlatch an invalid objective index: %d \n", ObjIndex);
		goto Quit;
	}

	m_Objectives[ObjIndex].m_bLatched = false;

Quit:
	return;
}

void CMissionData::UnlatchObjectiveComp(int ObjIndex, int CompIndex )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("UnlatchObjective: Bad objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to unlatch a component of invalid objective index: %d \n", ObjIndex);
		goto Quit;
	}

	if( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("UnlatchObjective: Component num %d out of bounds for objective %d. \r", (CompIndex+1), (ObjIndex+1) );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to unlatch invalid component: %d of objective: %d \n", (CompIndex+1), (ObjIndex+1) );
		goto Quit;
	}

	m_Objectives[ObjIndex].m_Components[CompIndex].m_bLatched = false;

Quit:
	return;
}

void CMissionData::Event_SetObjVisible( int ObjIndex, bool bVal )
{
	if( ObjIndex > m_Objectives.Num() || ObjIndex < 0 )
	{
		// log error
		goto Quit;
	}
	m_Objectives[ObjIndex].m_bVisible = bVal;

Quit:
	return;
}

void CMissionData::Event_SetObjMandatory( int ObjIndex, bool bVal )
{
	if( ObjIndex > m_Objectives.Num() || ObjIndex < 0 )
	{
		// log error
		goto Quit;
	}
	m_Objectives[ObjIndex].m_bMandatory = bVal;

Quit:
	return;
}

void CMissionData::Event_SetObjOngoing( int ObjIndex, bool bVal )
{
	if( ObjIndex > m_Objectives.Num() || ObjIndex < 0 )
	{
		// log error
		goto Quit;
	}
	m_Objectives[ObjIndex].m_bOngoing = bVal;

Quit:
	return;
}


// Objective parsing:
// TODO: Figure out how to parse/"compile" arbitrary boolean logic.  For now, don't
// returns the index of the first objective added, for scripting purposes
int CMissionData::AddObjsFromEnt( idEntity *ent )
{
	CObjective			ObjTemp;
	idLexer				src;
	idToken				token;
	idDict				*args;
	idStr				StrTemp, StrTemp2, TempStr2;
	int					Counter(1), Counter2(1); // objective indices start at 1 and must be offset for the inner code
	int					ReturnVal(-1);

	if( !ent )
		goto Quit;

	args = &ent->spawnArgs;
	if( !args )
		goto Quit;

	// store the first index of first added objective
	ReturnVal = m_Objectives.Num();

	// go thru all the objective-related spawnargs
	while( args->MatchPrefix( va("obj%d_", Counter) ) != NULL )
	{
		ObjTemp.m_Components.Clear();

		StrTemp = va("obj%d_", Counter);
		ObjTemp.m_state = (EObjCompletionState) args->GetInt( StrTemp + "state", "0");
		ObjTemp.m_text = args->GetString( StrTemp + "desc", "" );
		ObjTemp.m_bMandatory = args->GetBool( StrTemp + "mandatory", "1");
		ObjTemp.m_bReversible = !args->GetBool (StrTemp + "irreversible", "0" );
		ObjTemp.m_bVisible = args->GetBool( StrTemp + "visible", "1");
		ObjTemp.m_bOngoing = args->GetBool( StrTemp + "ongoing", "0");
		ObjTemp.m_CompletionScript = args->GetString( StrTemp + "script_complete" );
		ObjTemp.m_FailureScript = args->GetString( StrTemp + "script_failed" );

		// parse in the int list of "enabling objectives"
		TempStr2 = args->GetString( StrTemp + "enabling_objs", "" );
		src.LoadMemory( TempStr2.c_str(), TempStr2.Length(), "" );
		while( src.ReadToken( &token ) )
		{
			if( token.IsNumeric() )
				ObjTemp.m_EnablingObjs.Append( token.GetIntValue() );
		}
		src.FreeSource();

// TODO: Parse difficulty level when that is coded

		// parse objective components
		Counter2 = 1;
		while( args->MatchPrefix( va("obj%d_%d_", Counter, Counter2) ) != NULL )
		{
			StrTemp2 = StrTemp + va("%d_", Counter2);
			CObjectiveComponent CompTemp;
			
			CompTemp.m_bState = args->GetBool( StrTemp2 + "state", "0" );
			CompTemp.m_bNotted = args->GetBool( StrTemp2 + "not", "0" );
			CompTemp.m_bReversible = !args->GetBool( StrTemp2 + "irreversible", "0" );
			
			// use comp. type hash to convert text type to EComponentType
			idStr TypeString = args->GetString( StrTemp2 + "type", "");
			int TypeNum = m_CompTypeHash.First(m_CompTypeHash.GenerateKey( TypeString, false ));
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Parsing objective component type '%s' \r", TypeString.c_str() );
			
			if( TypeNum == -1 )
			{
				DM_LOG(LC_AI,LT_ERROR)LOGSTRING("Unknown objective component type '%s' when adding objective %d, component %d \r", TypeString, Counter, Counter2 );
				gameLocal.Printf("Objective System Error: Unknown objective component type '%s' when adding objective %d, component %d.  Objective component ignored. \n", TypeString, Counter, Counter2 ); 
				continue;
			}
			CompTemp.m_Type = (EComponentType) TypeNum;
			
			for( int ind=0; ind<2; ind++ )
			{
				// Use spec. type hash to convert text specifier to ESpecificationMethod enum
				idStr SpecString = args->GetString(va(StrTemp2 + "spec%d", ind + 1), "none");
				int SpecNum = m_SpecTypeHash.First(m_SpecTypeHash.GenerateKey( SpecString, false ));
				
				if( SpecNum == -1 )
				{
					DM_LOG(LC_AI,LT_ERROR)LOGSTRING("Unknown objective component specification type '%s' when adding objective %d, component %d \r", TypeString, Counter, Counter2 );
					gameLocal.Printf("Objective System Error: Unknown objective component specification type '%s' when adding objective %d, component %d.  Setting default specifier type 'none' \n", TypeString, Counter, Counter2 ); 
					SpecNum = 0;
				}
				CompTemp.m_SpecMethod[ind] = (ESpecificationMethod) SpecNum;
			}

			for( int ind=0; ind < 2; ind++ )
			{
				CompTemp.m_SpecStrVal[ind] = args->GetString( va(StrTemp2 + "spec_strval%d", ind + 1), "" );
				CompTemp.m_SpecIntVal[ind] = args->GetInt( va(StrTemp2 + "spec_intval%d", ind + 1), "0" );
			}

			// Use idLexer to read in string args and int args, space delimited lists
			TempStr2 = args->GetString( StrTemp2 + "args_str", "" );
			src.LoadMemory( TempStr2.c_str(), TempStr2.Length(), "" );
			src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES );
			
			while( src.ReadToken( &token ) )
				CompTemp.m_StrArgs.Append( token.c_str() );
			src.FreeSource();
			// same for int args:
			TempStr2 = args->GetString( StrTemp2 + "args_int", "" );
			src.LoadMemory( TempStr2.c_str(), TempStr2.Length(), "" );
			while( src.ReadToken( &token ) )
			{
				if( token.IsNumeric() )
					CompTemp.m_IntArgs.Append( token.GetIntValue() );
			}
			src.FreeSource();

			// Pad args with dummies to prevent a hard crash when they are read, if otherwise empty
			CompTemp.m_StrArgs.Append("");
			CompTemp.m_StrArgs.Append("");
			CompTemp.m_IntArgs.Append(0);
			CompTemp.m_IntArgs.Append(0);

			CompTemp.m_ClockInterval = (int) 1000 * args->GetFloat( StrTemp2 + "clock_interval", "1.0" );

			CompTemp.m_Index[0] = Counter;
			CompTemp.m_Index[1] = Counter2;

			ObjTemp.m_Components.Append( CompTemp );
			Counter2++;
		}
		
		if( ObjTemp.m_Components.Num() > 0 )
		{
			m_Objectives.Append( ObjTemp );
			ObjTemp.Clear();
		}
		Counter++;
	}

	// Process the objectives and add clocked components to clocked components list
	for( int ind = 0; ind < m_Objectives.Num(); ind++ )
	{
		for( int ind2 = 0; ind2 < m_Objectives[ind].m_Components.Num(); ind2++ )
		{
			CObjectiveComponent *pComp = &m_Objectives[ind].m_Components[ind2];
			if( (pComp->m_Type == COMP_CUSTOM_CLOCKED) || (pComp->m_Type == COMP_DISTANCE) )
				m_ClockedComponents.Append( pComp );
		}	
	}

	// check if any objectives were actually added, if not return -1
	if( m_Objectives.Num() == ReturnVal )
		ReturnVal = -1;
Quit:
	return ReturnVal;
}

/**==========================================================================
* CObjective
*==========================================================================**/

CObjective::CObjective( void )
{
	Clear();
}

CObjective::~CObjective( void )
{
	Clear();
}

void CObjective::Clear( void )
{
	m_state = STATE_INCOMPLETE;
	m_text = "";
	m_bNeedsUpdate = false;
	m_bMandatory = false;
	m_bReversible = true;
	m_bLatched = false;
	m_bVisible = true;
	m_bOngoing = false;
	m_MinDifficulty = 0;
	m_Components.Clear();
	m_EnablingObjs.Clear();
	m_CompletionScript.Clear();
	m_FailureScript.Clear();
}

/*=========================================================================== 
* 
*CObjectiveLocation
*
*============================================================================*/
CLASS_DECLARATION( idEntity, CObjectiveLocation )
END_CLASS

CObjectiveLocation::CObjectiveLocation( void )
{
	m_Interval = 1000;
	m_TimeStamp = 0;

	m_EntsInBounds.Clear();
}

void CObjectiveLocation::Spawn()
{
	m_Interval = (int) 1000.0f * spawnArgs.GetFloat( "interval", "1.0" );
	m_TimeStamp = gameLocal.time;

// Set the contents to a useless trigger so that the collision model will be loaded
// FLASHLIGHT_TRIGGER seems to be the only one that doesn't do anything else we don't want
	GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
	GetPhysics()->EnableClip();

	BecomeActive( TH_THINK );
}

void CObjectiveLocation::Think()
{
	int NumEnts(0);
	idEntity *Ents[MAX_GENTITIES];
	idStrList current, added, missing;
	bool bFound(false);

	// only check on clock ticks
	if( (gameLocal.time - m_TimeStamp) < m_Interval )
		goto Quit;

	m_TimeStamp = gameLocal.time;

	// bounding box test
	NumEnts = gameLocal.clip.EntitiesTouchingBounds(GetPhysics()->GetAbsBounds(), -1, Ents, MAX_GENTITIES);
	for( int i=0; i<NumEnts; i++ )
	{
		if( Ents[i] && Ents[i]->m_bIsObjective )
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objective location %s found entity %s during clock tick. \r", name.c_str(), Ents[i]->name.c_str() );
			current.Append( Ents[i]->name );
		}
	}

	// compare current list to previous cock tick list to generate added list
	for( int i = 0; i < current.Num(); i++ )
	{
		bFound = false;
		for( int j = 0; j < m_EntsInBounds.Num(); j++ )
		{
			if( current[i] == m_EntsInBounds[j] )
			{
                  bFound = true;
                  break;
            }
		}

		if( !bFound )
		{
			added.Append( current[i] );
		}
	}

	// compare again the other way to generate missing list
	for( int i = 0; i < m_EntsInBounds.Num(); i++ )
	{
		bFound = false;
		for( int j = 0; j < current.Num(); j++ )
		{
			if( m_EntsInBounds[i] == current[j] )
			{
                  bFound = true;
                  break;
            }
		}

		if( !bFound )
		{
			missing.Append( m_EntsInBounds[i] );
		}
	}

	// call objectives system for all missing or added ents
	for( int i=0; i<added.Num(); i++ )
	{
		idEntity *Ent = gameLocal.FindEntity( added[i].c_str() );
		if( Ent )
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objective entity %s entered objective location %s \r", Ent->name.c_str(), name.c_str() );
			gameLocal.m_MissionData->MissionEvent( COMP_LOCATION, Ent, this, true );
		}
	}

	for( int j=0; j<missing.Num(); j++ )
	{
		idEntity *Ent2 = gameLocal.FindEntity( missing[j].c_str() );
		if( Ent2 )
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Objective entity %s left objective location %s \r", Ent2->name.c_str(), name.c_str() );
			gameLocal.m_MissionData->MissionEvent( COMP_LOCATION, Ent2, this, false );
		}
	}

	// copy over the list
	m_EntsInBounds.Clear();
	m_EntsInBounds = current;
	
	current.Clear();
	missing.Clear();
	added.Clear();

Quit:
	idEntity::Think();
	return;
}
	


