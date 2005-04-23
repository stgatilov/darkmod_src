
/******************************************************************************/
/*                                                                            */
/*         Dark Mod Sound Propagation (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
* DESCRIPTION: Sound propagation class for propagating suspicious sounds to AI
* during gameplay.  Friend class to CsndPropLoader.
*
*****************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 ******************************************************************************/

#pragma hdrstop

#pragma warning(disable : 4996)

#include "sndproploader.h"
#include "sndprop.h"
#include "matrixsq.h"
#include "darkmodglobals.h"
#include "relations.h"
#include "frobDoor.h"
#include "../game/ai/ai.h"

//TODO: Fix adding up of sounds in PropToPoint (30 dB + 40 dB != 70 dB!!!!)

// NOTES:
// ALL LOSSES ARE POSITIVE (ie, loss of +10dB subtracts 10dB from vol)

// ALL INITIAL VOLUMES ARE SWL [dB] (power level of the source, 10*log(power/1E-12 Watts))

const float s_DOOM_TO_METERS = 0.0254f;					// doom to meters
const float s_METERS_TO_DOOM = (1.0f/DOOM_TO_METERS);	// meters to doom


/**************************************************
* BEGIN CsndProp Implementation
***************************************************/

CsndProp::CsndProp ( void )
{
	if ((m_LossMatrix = new CMatRUT<SLMEntry>) == NULL)
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when creating gameplay loss matrix\r");
		//TODO: Call gameLocal.Error as well to force a quit at this point?

	m_bLoadSuccess = false;
	m_bDefaultSpherical = false;
}

void CsndProp::Clear( void )
{
	m_LossMatrix->Clear();
	m_AreaPropsG.Clear();
	m_DoorIDHash.Clear();

	m_bLoadSuccess = false;
	m_bDefaultSpherical = false;
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Clearing sound prop gameplay object.\r");
}

CsndProp::~CsndProp ( void )
{
	Clear();

	delete m_LossMatrix;
}


void CsndProp::SetupFromLoader( const CsndPropLoader *in )
{
	SAreaProp defaultArea;

	Clear();

	m_SndGlobals = in->m_SndGlobals;

	if( !in->m_bLoadSuccess )
	{
		// setup the default sound prop object for failed loads
		DM_LOG(LC_SOUND, LT_WARNING).LogString("SndPropLoader failed to load from the .spr file.\r");
		DM_LOG(LC_SOUND, LT_WARNING).LogString("SndProp is using default (simple, single area) setup\r");

		//TODO: Uncomment these when soundprop from file is fully implemented
		//gameLocal.Warning("[DM SPR] SndPropLoader failed to load from the .spr file.");
		//gameLocal.Warning("[DM SPR] "SndProp is the using default (simple, single area) setup.");

		defaultArea.area = 0;
		defaultArea.LossMult = 1.0;
		defaultArea.SpherSpread = 0;

		m_AreaPropsG.Append( defaultArea );
		m_AreaPropsG.Condense();

		goto Quit;
	}

	m_bLoadSuccess = true;

	// copy the needed members from sndPropLoader
	m_LossMatrix->Copy( in->m_LossMatrix );

	m_bDefaultSpherical = in->m_bDefaultSpherical;
	m_AreaPropsG = in->m_AreaPropsG;
	
	// fill door ID hash index (must be run after doors spawn)
	FillDoorIDHash( in );

Quit:
	return;
}

void CsndProp::FillDoorIDHash( const CsndPropLoader *in )
{
	idEntity *gent;
	int doorID;

	m_DoorIDHash.Clear();

	for ( int i = 0; i < in->m_DoorNameTable.Num(); i++ )
	{
	gent = gameLocal.FindEntity(in->m_DoorNameTable[i].name.c_str());
	doorID = in->m_DoorNameTable[i].doorID;

	m_DoorIDHash.Add(doorID, gent->entityNumber);
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("FillDoorIDHash: Added door %s with ID %d to ID hash with gentity number %d\r", in->m_DoorRefs[i].doorName, doorID, gent->entityNumber);
	}
}

idEntity *CsndProp::GetDoorEnt( int doorID )
{
	int gentNum;
	idEntity *ent;
	if ((gentNum = m_DoorIDHash.First(doorID)) == -1)
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("GetDoorEnt: Failed to find a gentity number for doorID number %d\r", doorID);
		ent = NULL;
		goto Quit;
	}
	if ((ent = gameLocal.entities[gentNum]) == NULL)
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("GetDoorEnt: No gentity for gentity number %d\r", gentNum);
	}
Quit:
	return ent;
}

// note: Does not call checksound itself, so this should be called before
// calling this to make sure the sound exists somewhere.

void CsndProp::Propagate 
	( float volMod, float durMod, idStr sndName,
	 idVec3 origin, idEntity *maker,
	 USprFlags *addFlags )

{
	idBounds	bounds(origin), envBounds(origin);
	
	bool bValidTeam(false), bSameArea(false);
	int			mteam;
	float		range, vol0, propVol(0), noise(0);
	
	UTeamMask	tmask, compMask;
	SSprParms	propParms;
	
	idEntity *			testEnt;
	idAI				*testAI, *AI;
	idList<idEntity *>	validTypeEnts, validEnts;
	const idDict *		parms;

	if( cv_spr_debug.GetBool() )
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("PROPAGATING: From entity %s, sound \"%s\", volume modifier %f, duration modifier %f \r", maker->name.c_str(), sndName.c_str(), volMod, durMod );
		gameLocal.Printf("PROPAGATING: From entity %s, sound \"%s\", volume modifier %f, duration modifier %f \n", maker->name.c_str(), sndName.c_str(), volMod, durMod );
	}

	// initialize the comparison team mask
	compMask.m_field = 0;
	
	// find the dict def for the specific sound
	parms = gameLocal.FindEntityDefDict( va("sprGS_%s", sndName.c_str() ), false );

	// redundancy
	if(!parms)
		goto Quit;

	vol0 = parms->GetFloat("vol","0") + volMod;
	// DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found modified sound volume %f\r", vol0 );

	// scale the volume by some amount that is be a cvar for now for tweaking
	// later we will put a permananet value in the def for globals->Vol
	vol0 += cv_ai_sndvol.GetFloat();

	propParms.duration *= durMod;
	// DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found modified duration %f\r", propParms.duration);

	// set team alert and propagation flags from the parms
	SetupParms( parms, &propParms, addFlags, &tmask );

	propParms.maker = maker;

	if( maker->IsType(idActor::Type) )
	{
		mteam = static_cast<idAI *>(maker)->team;
	}
	else
	{
		mteam = -1; // maker is an object, not an AI
	}

	// Calculate the range, assuming peceived loudness of a sound doubles every 7 dB
	// (we want to overestimate a bit.  With the current settings, cutoff for a footstep
	// at 50dB is ~15 meters ( ~45 ft )

	// keep in mind that due to FOV compression, visual distances in FPS look shorter
	// than they actually are.

	range = pow(2, ((vol0 - m_SndGlobals.MaxRangeCalVol) / 7) ) * m_SndGlobals.MaxRange * s_METERS_TO_DOOM;

	bounds.ExpandSelf( range );

	// get a list of all ents with type idAI's or Listeners
	
	for ( testEnt = gameLocal.spawnedEntities.Next(); 
		  testEnt != NULL; testEnt = testEnt->spawnNode.Next() ) 
	{
	
		// TODO: Put in Listeners later

		if ( testEnt->IsType( idAI::Type ) )
		{
			validTypeEnts.Append( testEnt );
		}
	}

	validTypeEnts.Condense();
	
	if( cv_spr_debug.GetBool() )
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found %d ents with valid type for propagation\r", validTypeEnts.Num() );

	// cull the list by testing distance and valid team flag

	for ( int i=0; i<validTypeEnts.Num(); i++ )
	{
		bValidTeam = false; 

		// TODO : Do something else in the case of Listeners, since they're not AI
		testAI = static_cast<idAI *>( validTypeEnts[i] );

		if( !bounds.ContainsPoint( testAI->GetEyePosition() ) ) 
		{
			if( cv_spr_debug.GetBool() )
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI %s is not within propagation cutoff range %f\r", testAI->name.c_str(), range );
			continue;
		}

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI %s is within propagation cutoff range %f\r", testAI->name.c_str(), range );

		if( mteam == -1 )
		{
			// for now, inanimate objects alert everyone
			bValidTeam = true;
			if( cv_spr_debug.GetBool() )
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Sound was propagated from inanimate object: Alerts all teams\r" );
		}
		else
		{
			// generate the team comparison mask
			testAI = static_cast<idAI *>( validTypeEnts[i] );

			compMask.m_bits.same = ( testAI->team == mteam );
			compMask.m_bits.friendly = gameLocal.m_RelationsManager->IsFriend( testAI->team, mteam );
			compMask.m_bits.neutral = gameLocal.m_RelationsManager->IsNeutral( testAI->team, mteam );
			compMask.m_bits.enemy = gameLocal.m_RelationsManager->IsEnemy( testAI->team, mteam );

			// do the comparison
			if ( tmask.m_field & compMask.m_field )
			{
				bValidTeam = true;
				if( cv_spr_debug.GetBool() )
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI %s has a valid team for soundprop\r", testAI->name.c_str() );
			}
		}

		// TODO : Add another else if for the case of Listeners
		
		// don't alert the AI that caused the sound
		if( bValidTeam && testAI != maker )
		{
			if( cv_spr_debug.GetBool() )
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found a valid propagation target: %s\r", testAI->name.c_str() );
			validEnts.Append( validTypeEnts[i] );
			continue;
		}
		if( cv_spr_debug.GetBool() )
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI %s does not have a valid team for propagation\r", testAI->name.c_str() );

	}

	validEnts.Condense();

	/* handle environmental sounds here

	envBounds = bounds;
	envBounds.Expand( s_MAX_ENV_SNDRANGE * s_METERS_TO_DOOM);

	envBounds -= bounds;

	numEnt = gameLocal.clip.EntitiesTouchingBounds( envBounds, -1, inrangeEnts2, MAX_ENTS ); 

	for( int j =0; j < numEnt; j++)
	{
		// if the entities are in the env. sound hash
		// add them to the list of env. sounds to check for this propagation
	}
	*/

// EFFICIENCY STEP: Note the origin of the propagation
// if soundprop is triggered AGAIN, with an origin very close to the last,
// do not bother recalculating the list of env. sounds to check
// (note, if the env. list is re-used, keep the original origin for which it
// was calculated in memory, to deal with footstep situation.

// EFFICIENCY STEP:
// keep the last propagation TIME in memory too.  If the time and origin have
// not changed significantly, do not update the list of AI's to prop. to
// (must account for bad AI pointers for AI's that get destroyed though)

// EFFICIENCY STEP: Sparhawk suggestion: If an AI is on the very edge of the
// max radius, do not prop to them, but let them know that they "may have"
// heard a sound.  If they get enough of these "maybes," actually prop to them.

// Add all the remaining inrange entities with env. sounds to the list
// For efficiency reasons, we want to count env. sounds in the first radius
// when we go thru all the entities in the 1st bounds
// ideally we want to subtract the first bounds from these 2nd bounds
// so entities aren't counted twice.  I'm not sure if it's possible to
// subtract bounds that way though.

	// begin prop to each AI

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Beginning propagation to %d targets\r", validEnts.Num() );

	for(int j = 0; j < validEnts.Num(); j++)
	{
		AI = static_cast<idAI *>( validEnts[j] );

		// PropToPoint sets propagated sound parameters based on Loss Matrix lookup
		propVol = PropToPoint( vol0, origin, AI->GetEyePosition(), &propParms, &bSameArea );	
	
		if( cv_spr_debug.GetBool() )
		{
			gameLocal.Printf("Propagated sound %s to AI %s, from origin %s : Propagated volume %f, Apparent origin of sound: %s \r", 
							  sndName.c_str(), AI->name.c_str(), origin.ToString(), propVol, propParms.direction.ToString() );

			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Propagated sound %s to AI %s, from origin %s : Propagated volume %f, Apparent origin of sound: %s \r", 
											  sndName.c_str(), AI->name.c_str(), origin.ToString(), propVol, propParms.direction.ToString() );
		}

		// finish filling propParms for ease of passing args to AI
		propParms.name = sndName.c_str();
		propParms.propVol = propVol;
		
		// check if the AI would normally hear the sound before calculating env. sound masking.

		AI->SPLtoLoudness( &propParms );
		
		if (  AI->CheckHearing( &propParms ) )
		{
			// TODO: Add env. sound masking check here
			// GetEnvNoise should check all the env. noises on the list we made, plus global ones
			
			// noiseVol = GetEnvNoise( &propParms, origin, AI->GetEyePosition() );

			noise = 0;
		
			//message the AI
			AI->HearSound( &propParms, noise, origin, bSameArea );
		}

	}

Quit:

// Clean up pointers and idLists
	return;

}

//TODO: Fix adding of sounds coming from different paths here so they add properly!

float CsndProp::PropToPoint
	( float volInit, idVec3 origin, 
	  idVec3 target, SSprParms *propParms,
	  bool *bSameArea )
{
	bool bReversed(false);
	float volTotal(0), volPath, loudestVol(0);
	SLMEntry *Entry;
	int areaOrig, areaTarg, loudestPath;
	
	// if we failed to load the soundprop file for the map,
	// use the default area (0) and do not check Loss Matrix
	if (!m_bLoadSuccess)
	{
		areaOrig = 0;
		areaTarg = 0;
	}
	else
	{
		areaOrig = gameRenderWorld->PointInArea(origin);
		areaTarg = gameRenderWorld->PointInArea(target);
	}

	// if target is in the same area as origin, just use point to point propagation
	// and set the loudest direction to the origin of the sound
	if( areaOrig == areaTarg )
	{
		volTotal = volInit - p2pLoss( origin, target, areaOrig );
		propParms->direction = origin;
		*bSameArea = true;
		goto Quit;
	}

	// Otherwise, do a Loss Matrix lookup
	Entry = GetLM( areaOrig, areaTarg, &bReversed );

	for(int i=0; i < Entry->numPaths; i++)
	{
		// vol and loss are both in dB, so subtract to get new volume
		volPath = volInit - Entry->paths[i].loss;
		volPath -= GetDoorLoss( &Entry->paths[i] );
		
		// add in spatial loss from origin to startportal, endportal to target
		if(bReversed)
		{
			volPath -= p2pLoss( origin, Entry->paths[i].end, areaOrig );
			volPath -= p2pLoss( target, Entry->paths[i].start, areaTarg );
		}
		else
		{
			volPath -= p2pLoss( origin, Entry->paths[i].start, areaOrig );
			volPath -= p2pLoss( target, Entry->paths[i].end, areaTarg );
		}
		
		volTotal += volPath;
		if ( volPath > loudestVol || i == 0)
		{
			loudestVol = volPath;
			loudestPath = i;
		}
	}
	// if indices were reversed (ie, if row > col )
	if( bReversed )
	{
		propParms->direction = Entry->paths[loudestPath].start;
	}
	else
	{
		propParms->direction = Entry->paths[loudestPath].end;
	}

Quit:
	return volTotal;
}

float CsndProp::p2pLoss( idVec3 point1, idVec3 point2, int area )
{
	idVec3 delta;
	float distsqr, kappa, lossTot;
	
	// Multiply the default attenuation constant by the loss multiplier
	kappa = m_AreaPropsG[area].LossMult * m_SndGlobals.kappa0;

	delta = point1 - point2;
	//TODO: If optimization is needed, use delta.LengthFast()
	distsqr = delta.LengthSqr();
	distsqr *= s_DOOM_TO_METERS * s_DOOM_TO_METERS;
	
	if( m_AreaPropsG[area].SpherSpread == TRUE )
	{
		// spherical spreading, hemi-free field
		// assume most sounds take place near the ground => hemi-free field
		lossTot = m_SndGlobals.Falloff_Outd*log10(distsqr) + 8;
	}
	else
	{
		// Indoor propagation:
		lossTot = m_SndGlobals.Falloff_Ind*log10(distsqr) + 8;
	}

	// add in attenuation (material loss)
	// kappa is in dB / meter so multiply distsqr by 1/2
	// (the actual non-dB formula is loss = e^(-k * dist), then take 10log10(that)
	lossTot += 0.5 * kappa * distsqr;

	return lossTot;
}

void CsndProp::SetupParms( const idDict *parms, SSprParms *propParms, USprFlags *addFlags, UTeamMask *tmask )
{
	USprFlags tempflags;
	
	tempflags.m_field = 0;
	tmask->m_field = 0;
	
	DM_LOG(LC_SOUND,LT_DEBUG).LogString("Parsing team alert and propagation flags from propagated_sounds.def\r");
	
	// note: by default, if the key is not found, GetBool returns false
	tempflags.m_bits.same = parms->GetBool("alert_same");
	tempflags.m_bits.friendly = parms->GetBool("alert_friend");
	tempflags.m_bits.neutral = parms->GetBool("alert_neutral");
	tempflags.m_bits.enemy = parms->GetBool("alert_enemy", "1");

	tempflags.m_bits.omni_dir = parms->GetBool("omnidir");
	tempflags.m_bits.unique_loc = parms->GetBool("unique_loc");
	tempflags.m_bits.urgent = parms->GetBool("urgent");
	tempflags.m_bits.global_vol = parms->GetBool("global_vol");
	tempflags.m_bits.check_touched = parms->GetBool("check_touched");

	if( addFlags )
	{
		tempflags.m_field = tempflags.m_field | addFlags->m_field;
		if( cv_spr_debug.GetBool() )
			DM_LOG(LC_SOUND,LT_DEBUG).LogString("Added additional sound propagation flags from local sound \r");
	}
	
	// set the team mask from the sprflags
	tmask->m_bits.same = tempflags.m_bits.same;
	tmask->m_bits.friendly = tempflags.m_bits.friendly;
	tmask->m_bits.neutral = tempflags.m_bits.neutral;
	tmask->m_bits.enemy = tempflags.m_bits.enemy;

	// copy flags to parms
	propParms->flags = tempflags;

	// setup other parms
	propParms->duration = parms->GetFloat("dur","200");
	propParms->frequency = parms->GetFloat("freq","-1");
	propParms->bandwidth = parms->GetFloat("width", "-1");
	
	if( cv_spr_debug.GetBool() )
		DM_LOG(LC_SOUND,LT_DEBUG).LogString("Finished transfering sound prop parms\r");

	return;
}

float CsndProp::GetDoorLoss( SPropPath *path )
{
	bool hadErr(false);
	int i, num;
	idEntity *doorEnt;
	float doorLoss(0);

	num = path->numDoors;

	for( i=0; i < num; i++)
	{
		doorEnt = GetDoorEnt( path->doors[i] );
		if( doorEnt == NULL )
		{
			// report the error
			hadErr = true;
			goto Quit;
		}
		
		if( doorEnt->IsType(CFrobDoor::Type) )
			doorLoss += static_cast<CFrobDoor *>( doorEnt )->GetSoundLoss();
		
		else
			doorLoss += m_SndGlobals.DefaultDoorLoss;
	}

Quit:
	if(hadErr)
		doorLoss = 0;

	return doorLoss;
}

bool CsndProp::CheckSound( const char *sndNameGlobal, bool isEnv )
{
	const idDict *parms;
	bool returnval;

	if (isEnv)
		parms = gameLocal.FindEntityDefDict( va("sprGE_%s", sndNameGlobal ), false );
	else
		parms = gameLocal.FindEntityDefDict( va("sprGS_%s", sndNameGlobal ), false );

	if ( !parms )
	{
		// Don't log this, because it happens all the time.  Most sounds played with idEntity::StartSound are not propagated.
		//if( cv_spr_debug.GetBool() )
			//gameLocal.Warning("[Soundprop] Could not find sound def for sound \"%s\" Sound not propagated.", sndNameGlobal );
		//DM_LOG(LC_SOUND, LT_WARNING).LogString("Could not find sound def for sound \"%s\" Sound not propagated.\r", sndNameGlobal );
		returnval = false;
		goto Quit;
	}
	
	else
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found propagated sound \"%s\" in the def.\r", sndNameGlobal );
		returnval = true;
	}

Quit:
	return returnval;
}