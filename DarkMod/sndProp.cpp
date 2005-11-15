
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
 *
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

// NOTES:
// ALL LOSSES ARE POSITIVE (ie, loss of +10dB subtracts 10dB from vol)

// ALL INITIAL VOLUMES ARE SWL [dB] (power level of the source, 10*log(power/1E-12 Watts))

// TODO: Add volInit to propParms instead of continually passing it as an argument

const float s_DOOM_TO_METERS = 0.0254f;					// doom to meters
const float s_METERS_TO_DOOM = (1.0f/DOOM_TO_METERS);	// meters to doom

/**
* Max number of areas to flood when doing wavefront expansion
*
* When the expansion goes above this number, it is terminated
*
* TODO: Read this from soundprop def file!
**/
const int s_MAX_FLOODNODES = 200;

/**
* Volume ( SPL [dB] ) threshold after which the sound stops propagating
* 
* Should correspond to the absolute lowest volume we want AI to be
* able to detect
*
* TODO: Read this from soundprop def file!
**/
const float s_MIN_AUD_THRESH = 15;

/**
* Max number of expansion nodes within which to use detailed path minimization
* (That is, trace the path back from AI thru the portals to find the optimum
*  points on the portal surface the path travels thru).
*
* TODO: Read this from soundprop def file!
**/
const float s_MAX_DETAILNODES = 3;

/**
* 1/log(10), useful for change of base between log and log10
**/
const float s_invLog10 = 0.434294482f;


/**************************************************
* BEGIN CsndProp Implementation
***************************************************/

CsndProp::CsndProp ( void )
{
	m_bLoadSuccess = false;
	m_bDefaultSpherical = false;

	m_EventAreas = NULL;
	m_PopAreas = NULL;
}

void CsndProp::Clear( void )
{
	SPortEvent *pPortEv;

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Clearing sound prop gameplay object.\r");

	m_AreaPropsG.Clear();

	m_bLoadSuccess = false;
	m_bDefaultSpherical = false;

	if( m_EventAreas != NULL )
	{
		// delete portal event data array
		for( int i=0; i < m_numAreas; i++ )
		{
			pPortEv = m_EventAreas[i].PortalDat;
			if( pPortEv != NULL )
				delete[] pPortEv;
		}

		delete[] m_EventAreas;
		m_EventAreas = NULL;
	}

	if( m_PopAreas != NULL )
	{
		for( int i=0; i < m_numAreas; i++ )
		{
			m_PopAreas[i].AIContents.Clear();
			m_PopAreas[i].VisitedPorts.Clear();
		}

		delete[] m_PopAreas;
		m_PopAreas = NULL;
	}

	// delete m_sndAreas
	DestroyAreasData();
}

CsndProp::~CsndProp ( void )
{
	Clear();
}


void CsndProp::SetupFromLoader( const CsndPropLoader *in )
{
	SAreaProp	defaultArea;
	int			tempint(0);
	int			numPorts;
	SEventArea *pEvArea;

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

		//TODO : Need better default behavior for bad file, this isn't going to work
		defaultArea.area = 0;
		defaultArea.LossMult = 1.0 * m_SndGlobals.kappa0;;
		defaultArea.SpherSpread = 0;

		m_AreaPropsG.Append( defaultArea );
		m_AreaPropsG.Condense();

		goto Quit;
	}

	m_bLoadSuccess = true;

	m_numAreas = in->m_numAreas;

	// copy the connectivity database from sndPropLoader
	if( (m_sndAreas = new SsndArea[m_numAreas]) == NULL )
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when copying area connectivity database to gameplay object\r");
		goto Quit;
	}

	// copy the areas array, element by element
	for( int i=0; i < m_numAreas; i++ )
	{
		m_sndAreas[i].LossMult = in->m_sndAreas[i].LossMult;
		m_sndAreas[i].SpherSpread = in->m_sndAreas[i].SpherSpread;
		
		tempint = in->m_sndAreas[i].numPortals;
		m_sndAreas[i].numPortals = tempint;

		m_sndAreas[i].center = in->m_sndAreas[i].center;

		m_sndAreas[i].portals = new SsndPortal[ tempint ];
		for( int k=0; k < tempint; k++ )
			m_sndAreas[i].portals[k] = in->m_sndAreas[i].portals[k];

		m_sndAreas[i].portalDists = new CMatRUT<float>;
		m_sndAreas[i].portalDists->Copy( in->m_sndAreas[i].portalDists );
	}


	m_bDefaultSpherical = in->m_bDefaultSpherical;
	m_AreaPropsG = in->m_AreaPropsG;

	m_DoorRefs = in->m_DoorRefs;
	
	// fill in door entity pointers on portals in m_sndAreas
	FillDoorEnts();


	// initialize Event Areas
	if( (m_EventAreas = new SEventArea[m_numAreas]) == NULL )
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when initializing m_EventAreas\r");
		goto Quit;
	}

	// initialize Populated Areas
	if( (m_PopAreas = new SPopArea[m_numAreas]) == NULL )
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when initializing m_PopAreas\r");
		goto Quit;
	}

	// Initialize the timestamp in Populated Areas

	for ( int k=0; k<m_numAreas; k++ )
	{
		m_PopAreas[k].addedTime = 0;
	}
	
	// initialize portal loss arrays within Event Areas
	for( int j=0; j<m_numAreas; j++ )
	{
		pEvArea = &m_EventAreas[j];

		pEvArea->bVisited = false;

		numPorts = m_sndAreas[j].numPortals;

		if( (pEvArea->PortalDat = new SPortEvent[ numPorts ])
			== NULL )
		{
			DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when initializing portal data array for area %d in m_EventAreas\r", j);
			goto Quit;
		}

		// point the event portals to the m_sndAreas portals
		for( int l=0; l<numPorts; l++ )
		{
			SPortEvent *pEvPtr = &pEvArea->PortalDat[l];
			pEvPtr->ThisPort = &m_sndAreas[j].portals[l];
		}
	}

Quit:
	return;
}

void CsndProp::FillDoorEnts( void )
{
	idEntity *gent;
	int anum, pnum;

	for ( int i = 0; i < m_DoorRefs.Num(); i++ )
	{
	gent = gameLocal.FindEntity( m_DoorRefs[i].doorName );

	if (gent == NULL)
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Could not find door ent for doorname %s.\r", m_DoorRefs[i].doorName );
		continue;
	}

	// Add to m_sndAreas
	anum = m_DoorRefs[i].area;
	pnum = m_DoorRefs[i].portalNum;

	m_sndAreas[ anum ].portals[ pnum ].doorEnt = gent;

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("FillDoorEnts: Added pointer for door %s to area %d, portal %d\r", m_DoorRefs[i].doorName, anum, pnum);
	}
}

// NOTE: Propagate does not call CheckSound.  CheckSound should be called before
//	calling Propagate, in order to make sure the sound exists somewhere.

void CsndProp::Propagate 
	( float volMod, float durMod, idStr sndName,
	 idVec3 origin, idEntity *maker,
	 USprFlags *addFlags )

{
	bool bValidTeam(false), bSameArea(false), bExpandFinished(false);
	int			mteam;
	float		range, vol0, propVol(0), noise(0);
	
	idTimer		timer_Prop;
	UTeamMask	tmask, compMask;
	SSprParms	propParms;
	idBounds	bounds(origin), envBounds(origin);
	idEntity *			testEnt;
	idAI				*testAI;
	idList<idEntity *>	validTypeEnts, validEnts;
	const idDict *		parms;
	SPopArea			*pPopArea;

	timer_Prop.Clear();
	timer_Prop.Start();

	m_TimeStamp = gameLocal.time;

	if( cv_spr_debug.GetBool() )
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("PROPAGATING: From entity %s, sound \"%s\", volume modifier %f, duration modifier %f \r", maker->name.c_str(), sndName.c_str(), volMod, durMod );
		gameLocal.Printf("PROPAGATING: From entity %s, sound \"%s\", volume modifier %f, duration modifier %f \n", maker->name.c_str(), sndName.c_str(), volMod, durMod );
	}

	// clear the old populated areas list
	m_PopAreasInd.Clear();

	// initialize the comparison team mask
	compMask.m_field = 0;
	
	// find the dict def for the specific sound
	parms = gameLocal.FindEntityDefDict( va("sprGS_%s", sndName.c_str() ), false );

	// redundancy
	if(!parms)
		goto Quit;

	propParms.name = sndName.c_str();

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

	propParms.origin = origin;

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

	range = pow((float)2, ((vol0 - m_SndGlobals.MaxRangeCalVol) / 7) ) * m_SndGlobals.MaxRange * s_METERS_TO_DOOM;

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

	// Don't bother propagation if no one is in range
	if (validEnts.Num() == 0)
		goto Quit;

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Beginning propagation to %d targets\r", validEnts.Num() );


// ======================== BEGIN WAVEFRONT EXPANSION ===================

// Populate the AI lists in the m_PopAreas array, use timestamp method to check if it's the first visit
	
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Filling populated areas array with AI\r" );
	for(int j = 0; j < validEnts.Num(); j++)
	{
		int AIAreaNum = gameRenderWorld->PointInArea( validEnts[j]->GetPhysics()->GetOrigin() );
		
		//Sometimes PointInArea returns -1, don't know why
		if (AIAreaNum < 0)
			continue;

		pPopArea = &m_PopAreas[AIAreaNum];
		
		if( pPopArea == NULL )
			continue;

		//DM_LOG(LC_SOUND, LT_DEBUG).LogString("TimeStamp Debug: Area timestamp %d, new timestamp %d \r", pPopArea->addedTime, m_TimeStamp);

		// check if this is the first update to the pop. area in this propagation
		if( pPopArea->addedTime != m_TimeStamp )
		{
			//update the timestamp
			pPopArea->addedTime = m_TimeStamp;

			pPopArea->bVisited = false;
			pPopArea->AIContents.Clear();
			pPopArea->VisitedPorts.Clear();

			// add the first AI to the contents list
			pPopArea->AIContents.Append( static_cast< idAI * >(validEnts[j]) );

			// append the index of this area to the popAreasInd list for later processing
			m_PopAreasInd.Append( AIAreaNum );
		}
		else
		{
		// This area has already been updated in this propagation, just add the next AI
			pPopArea->AIContents.Append( static_cast< idAI * >(validEnts[j]) );
		}
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Processed AI %s in area %d\r", validEnts[j]->name.c_str(), AIAreaNum );
	}

	bExpandFinished = ExpandWave( vol0, origin, &propParms );

	//TODO: If bExpandFinished == false, either fake propagation or
	// delay further expansion until later frame
	if(bExpandFinished == false)
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Expansion was stopped when max node number %d was exceeded\r", s_MAX_FLOODNODES );


	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Expansion done, processing AI\r" );
	ProcessPopulated( vol0, origin, &propParms );

	timer_Prop.Stop();
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Total TIME for propagation: %d [ms]\r", (int) timer_Prop.Milliseconds() );

Quit:

	return;

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

float CsndProp::GetDoorLoss( idEntity *doorEnt )
{
	float doorLoss(0);

	if( doorEnt == NULL )		
	{
		// do not log this, since it's supposed to be NULL when no door exists
		doorLoss = 0;
		goto Quit;
	}
		
	if( doorEnt->IsType(CFrobDoor::Type) )
	{
		doorLoss += static_cast<CFrobDoor *>( doorEnt )->GetSoundLoss();
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Added door loss %f from door %s\r", doorLoss, doorEnt->name.c_str());
	}
	
	else
	{
		//TODO : Modify func_door to store a loss when open/closed?  Maybe an FM author
		//	wants to have a door that is not a CFrobDoor, but wants unique acoustical loss

		doorLoss += m_SndGlobals.DefaultDoorLoss;
	}

Quit:

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

bool CsndProp::ExpandWave( float volInit, idVec3 origin, 
							SSprParms *propParms )
{
	bool				returnval;
	int					popIndex(-1), floods(1), nodes(0), area, LocalPort;
	float				tempDist(0), tempAtt(1), tempLoss(0), AddedDist(0);
	idList<SExpQue>		NextAreas; // expansion queue
	idList<SExpQue>		AddedAreas; // temp storage for next expansion queue
	SExpQue				tempQEntry;
	SPortEvent			*pPortEv; // pointer to portal event data
	SPopArea			*pPopArea; // pointer to populated area data

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Starting wavefront expansion\r" );

	// clear the visited settings on m_EventAreas from previous propagations
	for(int i=0; i < m_numAreas; i++)
		m_EventAreas[i].bVisited = false;

	NextAreas.Clear();
	AddedAreas.Clear();

	
	// ======================== Handle the initial area =========================

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Processing initial area\r" );

	int initArea = gameRenderWorld->PointInArea( origin );

	m_EventAreas[ initArea ].bVisited = true;

	// Update m_PopAreas to show that the area has been visited
	m_PopAreas[ initArea ].bVisited = true;

	// array index pointers to save on calculation
	SsndArea *pSndAreas = &m_sndAreas[ initArea ];
	SEventArea *pEventAreas = &m_EventAreas[ initArea ];

	// calculate initial portal losses from the sound origin point
	for( int i2=0; i2 < pSndAreas->numPortals; i2++)
	{
		idVec3 portalCoord = pSndAreas->portals[i2].center;

		tempDist = (origin - portalCoord).LengthFast() * s_DOOM_TO_METERS;
		// calculate and set initial portal losses
		tempAtt = m_AreaPropsG[ initArea ].LossMult * tempDist;
		
		// add the door loss
		tempAtt += GetDoorLoss( pSndAreas->portals[i2].doorEnt );

		// get the current loss
		tempLoss = m_SndGlobals.Falloff_Ind * s_invLog10*idMath::Log16(tempDist) + tempAtt + 8;

		pPortEv = &pEventAreas->PortalDat[i2];

		pPortEv->Loss = tempLoss;
		pPortEv->Dist = tempDist;
		pPortEv->Att = tempAtt;
		pPortEv->Floods = 1;
		pPortEv->PrevPort = NULL;

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Loss at portal %d is %f [dB]\r", i2, tempLoss);
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Dist at portal %d is %f [m]\r", i2, tempDist);


		// add the portal destination to flooding queue if the sound has
		//	not dropped below threshold at the portal
		if( (volInit - tempLoss) > s_MIN_AUD_THRESH )
		{
			tempQEntry.area = pSndAreas->portals[i2].to;
			tempQEntry.curDist = tempDist;
			tempQEntry.curAtt = tempAtt;
			tempQEntry.curLoss = tempLoss;
			tempQEntry.portalH = pSndAreas->portals[i2].handle;
			tempQEntry.PrevPort = NULL;

			NextAreas.Append( tempQEntry );
		}
		else
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Wavefront intensity dropped below threshold at portal %d\r", i2);
	}

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Starting main loop\r" );
	
	
// done with initial area, begin main loop

	while( NextAreas.Num() > 0 && nodes < s_MAX_FLOODNODES )
	{
		floods++;

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Expansion loop, iteration %d\r", floods);

		AddedAreas.Clear();

		for(int j=0; j < NextAreas.Num(); j++)
		{
			nodes++;

			area = NextAreas[j].area;

			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Flooding area %d thru portal handle %d\r", area, NextAreas[j].portalH);

			// array index pointers to save on calculation
			pSndAreas = &m_sndAreas[ area ];
			pEventAreas = &m_EventAreas[ area ];
			pPopArea = &m_PopAreas[area];

			// find the local portal number associated with the portal handle
			LocalPort = -1;
			for( int ind = 0; ind < pSndAreas->numPortals; ind++ )
			{
				if( pSndAreas->portals[ind].handle == NextAreas[j].portalH )
				{
					LocalPort = ind;
					break;
				}
			}
			
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Identified portal handle with local port %d\r", LocalPort );

			if( LocalPort == -1 )
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Couldn't find portal handle %d in area %d\r", NextAreas[j].portalH, area);
				goto Quit;
			}

			pPortEv = &pEventAreas->PortalDat[ LocalPort ];

			// copy information from the portal's other side
			pPortEv->Dist = NextAreas[j].curDist;
			pPortEv->Att = NextAreas[j].curAtt;
			pPortEv->Loss = NextAreas[j].curLoss;
			pPortEv->Floods = floods - 1;
			pPortEv->PrevPort = NextAreas[j].PrevPort;


			// Updated the Populated Areas to show that it's been visited
			// Only do this for populated areas that matter (ie, they've been updated
			//	on this propagation
			
			if ( pPopArea->addedTime == m_TimeStamp )
			{
				pPopArea->bVisited = true;
				// note the portal flooded in on for later processing
				pPopArea->VisitedPorts.AddUnique( LocalPort );
			}

			// Flood to portals in this area
			for( int i=0; i < pSndAreas->numPortals; i++)
			{
				// do not flood back thru same portal we came in
				if( LocalPort == i)
					continue;

				// set up the portal event pointer
				pPortEv = &pEventAreas->PortalDat[i];

				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Calculating loss from portal %d to portal %d in area %d\r", LocalPort, i, area);
		
				// Obtain loss at this portal and store in temp var
				tempDist = NextAreas[j].curDist;
				AddedDist = *pSndAreas->portalDists->GetRev( LocalPort, i );
				tempDist += AddedDist;

				tempAtt = NextAreas[j].curAtt;
				tempAtt += AddedDist * m_AreaPropsG[ area ].LossMult;
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Total distance now %f\r", tempDist );
				
				// add the door loss
				tempAtt += GetDoorLoss( pSndAreas->portals[i].doorEnt );
	

				tempLoss = m_SndGlobals.Falloff_Ind * s_invLog10*idMath::Log16(tempDist) + tempAtt + 8;

				// check if we've visited the area, and do not add destination area 
				//	if loss is greater this time
				if( pEventAreas->bVisited 
					&& tempLoss >= pPortEv->Loss )
				{
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("Cancelling flood thru portal %d in previously visited area %d\r", i, area);
					continue;
				}

				if( ( volInit - tempLoss ) < s_MIN_AUD_THRESH )
				{
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("Wavefront intensity dropped below abs min audibility at portal %d in area %d\r", i, area);
					continue;
				}

				// path has been determined to be minimal loss, above cutoff intensity
				// store the loss value

				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Further expansion valid thru portal %d in area %d\r", i, area);
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Set loss at portal %d to %f [dB]\r", i, tempLoss);
				
				pPortEv->Loss = tempLoss;
				pPortEv->Dist = tempDist;
				pPortEv->Att = tempAtt;
				pPortEv->Floods = floods;
				pPortEv->PrevPort = &pEventAreas->PortalDat[ LocalPort ];

				// add the portal destination to flooding queue
				tempQEntry.area = pSndAreas->portals[i].to;
				tempQEntry.curDist = tempDist;
				tempQEntry.curAtt = tempAtt;
				tempQEntry.curLoss = tempLoss;
				tempQEntry.portalH = pSndAreas->portals[i].handle;	
				tempQEntry.PrevPort = pPortEv->PrevPort;

				AddedAreas.Append( tempQEntry );
			
			} // end portal flood loop

			m_EventAreas[j].bVisited = true;
		} // end area flood loop

		// create the next expansion queue
		NextAreas = AddedAreas;

	} // end main loop

	// return true if the expansion died out naturally rather than being stopped
	returnval = ( !NextAreas.Num() );

Quit:
	return returnval;
} // end function

void CsndProp::ProcessPopulated( float volInit, idVec3 origin, 
								SSprParms *propParms )
{
	float LeastLoss, TestLoss, tempDist, tempAtt, tempLoss;
	int area, LoudPort, portNum, i(0), j(0), k(0);
	idAI *AIPtr;
	idVec3 testLoc;
	SPortEvent *pPortEv;
	SPopArea *pPopArea;
	idList<idVec3> showPoints;
	
	int initArea = gameRenderWorld->PointInArea( origin );

	for( i=0; i < m_PopAreasInd.Num(); i++ )
	{
		area = m_PopAreasInd[i];

		pPopArea = &m_PopAreas[area];

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Processing pop. area %d\r", area);
		
		// Special case: AI area = initial area - no portal flooded in on in this case
		if( area == initArea )
		{
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Special case: AI in initial area %d\r", area);

			propParms->bSameArea = true;
			propParms->direction = origin;

			for( j=0; j < pPopArea->AIContents.Num(); j++)
			{
				AIPtr = pPopArea->AIContents[j];
				
				if (AIPtr == NULL)
					continue;

				tempDist = (origin - AIPtr->GetEyePosition()).LengthFast() * s_DOOM_TO_METERS;
				tempAtt = tempDist * m_AreaPropsG[ area ].LossMult;
				tempLoss = m_SndGlobals.Falloff_Ind * s_invLog10*idMath::Log16(tempDist) + tempAtt + 8;

				propParms->propVol = volInit - tempLoss;

				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Messaging AI %s in (source area) area %d\r", AIPtr->name.c_str(), area);
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Dist to AI: %f [m], Propagated volume found to be %f [dB]\r", tempDist, propParms->propVol);
				
				ProcessAI( AIPtr, origin, propParms );

				// draw debug lines if show soundprop cvar is set
				if( cv_spr_show.GetBool() )
				{
					showPoints.Clear();
					showPoints.Append( AIPtr->GetEyePosition() );
					showPoints.Append( propParms->origin );
					DrawLines( &showPoints );
				}
			}
		}

		// Normal propagation to a surrounding area
		else if ( pPopArea->bVisited == true )
		{
			propParms->bSameArea = false;

			// figure out the least loss portal
			// May be different for each AI (esp. in large rooms)
			// TODO OPTIMIZATION: Don't do this extra loop for each AI if 
			//		we only visited one portal in the area

			for( int aiNum = 0; aiNum < pPopArea->AIContents.Num(); aiNum++ )
			{
				AIPtr = pPopArea->AIContents[ aiNum ];

				if (AIPtr == NULL)
				{
					DM_LOG(LC_SOUND, LT_WARNING).LogString("NULL AI pointer for AI %d in area %d\r", aiNum, area);
					continue;
				}

				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Calculating least loss for AI %s in area %d\r", AIPtr->name.c_str(), area);

				LeastLoss = 999999999.0f;

				for( k=0; k < pPopArea->VisitedPorts.Num(); k++ )
				{	
					portNum = pPopArea->VisitedPorts[ k ];
					pPortEv = &m_EventAreas[area].PortalDat[ portNum ];

					//DM_LOG(LC_SOUND, LT_DEBUG).LogString("Calculating loss from portal %d, DEBUG k=%d, portsnum = %d\r", portNum, k, m_PopAreas[i].VisitedPorts.Num());

					testLoc = m_sndAreas[area].portals[portNum].center;

					tempDist = (testLoc - AIPtr->GetEyePosition()).LengthFast() * s_DOOM_TO_METERS;
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI Calc: Distance to AI = %f [m]\r", tempDist);

					tempAtt = tempDist * m_AreaPropsG[ area ].LossMult;
					tempDist += pPortEv->Dist;
					tempAtt += pPortEv->Att;

					DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI Calc: Portal %d has total Dist = %f [m]\r", portNum, tempDist);

					// add loss from portal to AI to total loss at portal
					TestLoss = m_SndGlobals.Falloff_Ind * s_invLog10*idMath::Log16(tempDist) + tempAtt + 8;
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("AI Calc: Portal %d has total Loss = %f [dB]\r", portNum, TestLoss);

					if( TestLoss < LeastLoss )
					{
						LeastLoss = TestLoss;
						LoudPort = portNum;
					}
				}
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Portal %d has least loss %f [dB]\r", LoudPort, LeastLoss );

				pPortEv = &m_EventAreas[area].PortalDat[ LoudPort ];
				propParms->floods = pPortEv->Floods;


// Detailed Path Minimization: 

				// check if AI is within the flood range for detailed path minimization	
				if( pPortEv->Floods <= s_MAX_DETAILNODES )
				{
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("Starting detailed path minimization for portal  %d\r", LoudPort );
					propParms->bDetailedPath = true;

					// call detailed path minimization, which writes results to propParms
					DetailedMin( AIPtr, propParms, pPortEv, area, volInit ); 

					// message the AI
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("Propagated volume found to be %f\r", propParms->propVol);
					DM_LOG(LC_SOUND, LT_DEBUG).LogString("Messaging AI %s in area %d\r", AIPtr->name.c_str(), area);
					
					ProcessAI( AIPtr, origin, propParms );
					
					continue;
				}

				propParms->bDetailedPath = false;
				propParms->direction = m_sndAreas[area].portals[ LoudPort ].center;
				propParms->propVol = volInit - LeastLoss;

				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Propagated volume found to be %f\r", propParms->propVol);
				
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Messaging AI %s in area %d\r", AIPtr->name.c_str(), area);
				ProcessAI( AIPtr, origin, propParms );
			}
		}

		// Propagation was stopped before this area was reached
		else if ( pPopArea->bVisited == false )
		{
			// Do nothing for now
			// TODO: Keep track of these areas for delayed calculation?
		}

	}

} // End function

void CsndProp::ProcessAI( idAI *AI, idVec3 origin, SSprParms *propParms )
{
	float noise(0);

	// check AI hearing, get environmental noise, etc
	
	if( AI == NULL )
		goto Quit;

	if( cv_spr_debug.GetBool() )
	{
		gameLocal.Printf("Propagated sound %s to AI %s, from origin %s : Propagated volume %f, Apparent origin of sound: %s \r", 
						  propParms->name, AI->name.c_str(), origin.ToString(), propParms->propVol, propParms->direction.ToString() );

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Propagated sound %s to AI %s, from origin %s : Propagated volume %f, Apparent origin of sound: %s \r", 
											  propParms->name, AI->name.c_str(), origin.ToString(), propParms->propVol, propParms->direction.ToString() );
	}

	// convert the SPL to loudness and store it in parms
	AI->SPLtoLoudness( propParms );

	if (  AI->CheckHearing( propParms ) )
	{
		// TODO: Add env. sound masking check here
		// GetEnvNoise should check all the env. noises on the list we made, plus global ones
		
		// noiseVol = GetEnvNoise( &propParms, origin, AI->GetEyePosition() );
		noise = 0;
		
		//message the AI
		AI->HearSound( propParms, noise, origin );
	}

Quit:
	return;
}

/**
* CsndProp::OptSurfPoint
* PSUEDOCODE
*
* 1. Define the surface coordinates by taking vectors to two consecutive 
* 	points in the winding.
* 
* 2.	Obtain intersection point with winding plane.
* 2.A Test if this point is within the rectangle.  If so, we're done. (goto Quit)
* 
* 3. Obtain a1 and a2, the coordinates when line is resolved onto the axes
* 
* 4. Test sign of a1 and a2 to determine which edge they should intersect
* 	write the lower numbered point of the intersection edge to edgeNum
* 
* 
* 5. Find the point along the edge closest to point A
* -Find line that goes thru point isect, and is also perpendicular to the edge
* 
* 6. Return this point in world coordinates, we're done.
**/
idVec3 CsndProp::OptSurfPoint( idVec3 p1, idVec3 p2, const idWinding *wind, idVec3 WCenter )
{
	//TODO: If the winding is not a rectangle, just return the center coordinate

	idVec3 line, u1, u2, v1, v2, edge, isect, lineSect;
	idVec3 returnVec, tempVec, pointA;
	idPlane WPlane;
	float lenV1, lenV2, lineU1, lineU2, Scale, frac;
	int edgeStart, edgeStop;

	// Want to find a point on the portal rectangle closest to this line:
	line = p2 - p1;

	// define the portal coordinates and extent of the two corners
	u1 = (wind->operator[](0).ToVec3() - WCenter);
	u2 = (wind->operator[](1).ToVec3() - WCenter);
	u1.NormalizeFast();
	u2.NormalizeFast();

	// define other coordinates going to midpoint between two points (useful to see if point is on portal)
	v1 = (wind->operator[](1).ToVec3() + wind->operator[](0).ToVec3()) / 2 - WCenter;
	v2 = (wind->operator[](2).ToVec3() + wind->operator[](1).ToVec3()) / 2 - WCenter;
	lenV1 = v1.LengthFast();
	lenV2 = v2.LengthFast();

	wind->GetPlane(WPlane);

	tempVec = p2-p1;
	tempVec.NormalizeFast();

	// find ray intersection point, in terms of p1 + (p2-p1)*Scale
	WPlane.RayIntersection( p1, tempVec, Scale );

	isect = p1 + Scale * tempVec;
	lineSect = isect - WCenter;

	if( cv_spr_show.GetBool() )
	{
		gameRenderWorld->DebugLine( colorRed, WCenter, (wind->operator[](1).ToVec3() + wind->operator[](0).ToVec3()) / 2, 3000);
		gameRenderWorld->DebugLine( colorRed, WCenter, (wind->operator[](2).ToVec3() + wind->operator[](1).ToVec3()) / 2, 3000);
		//gameRenderWorld->DebugLine( colorYellow, WCenter, isect, 3000);
	}

	// resolve into surface coordinates
	lineU1 = lineSect * u1;
	lineU2 = lineSect * u2;

	// If point is within the rectangular surface boundaries, we're done
	// Use the v axes (going to edge midpoints) to check if point is within rectangle
	if( abs(lineSect * v1/lenV1) <= lenV1 && abs( lineSect * v2/lenV2) <= lenV2 )
	{
//		DM_LOG(LC_SOUND, LT_DEBUG).LogString("MinSurf: Line itersects inside portal surface\r" );
//		DM_LOG(LC_SOUND, LT_DEBUG).LogString("v1/lenV1 = %f, v2/lenV2 = %f\r", abs(lineSect * v1/lenV1)/lenV1, abs( lineSect * v2/lenV2)/lenV2 );
		returnVec = isect;
		goto Quit;
	}

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("MinSurf: Line intersected outside of portal\r");
	// find the edge that the line intersects
	if( lineU1 >= 0 && lineU2 >= 0 )
	{
		edgeStart = 0;
		edgeStop = 1;
	}
	else if( lineU1 <= 0 && lineU2 >= 0 )
	{
		edgeStart = 1;
		edgeStop = 2;
	}
	else if( lineU1 <= 0 && lineU2 <= 0 )
	{
		edgeStart = 2;
		edgeStop = 3;
	}
	else if( lineU1 >= 0 && lineU2 <= 0 )
	{
		edgeStart = 3;
		edgeStop = 0;
	}

	pointA = wind->operator[](edgeStart).ToVec3();
	edge = wind->operator[](edgeStop).ToVec3() - pointA;

	// Find the closest point on the edge to the isect point
	// This is the point we're looking for
	frac = ((isect - pointA) * edge )/ edge.LengthSqr();

	// check if the point is outside the actual edge, if not, set it to
	//	the appropriate endpoint.
	if( frac < 0 )
		returnVec = pointA;
	else if( frac > 1.0 )
		returnVec = pointA + edge;
	else
		returnVec = pointA + frac * edge;

Quit:
	return returnVec;
}

void CsndProp::DetailedMin( idAI* AI, SSprParms *propParms, SPortEvent *pPortEv, int AIArea, float volInit )
{
	idList<idVec3>		pathPoints; // pathpoints[0] = closest path point to the TARGET
	idList<SsndPortal*> PortPtrs; // pointers to the portals along the path
	idVec3				point, p1, p2, AIpos;
	int					floods, curArea;
	float				tempAtt, tempDist, totAtt, totDist, totLoss;
	SPortEvent			*pPortTest;
	SsndPortal			*pPort2nd;

	floods = pPortEv->Floods;
	pPortTest = pPortEv;

	AIpos = AI->GetEyePosition();
	p1 = AIpos;


	// first iteration, populate pathPoints going "backwards" from target to source
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Path min: Performing first iteration for %d floods\r", floods );
	
	for( int i = 0; i < floods; i++ )
	{
		if( pPortEv->ThisPort == NULL)
			DM_LOG(LC_SOUND, LT_ERROR).LogString("ERROR: pPortEv->ThisPort is NULL\r" );

		// calculate optimum point for this leg of the path
		point = OptSurfPoint( p1, propParms->origin, pPortTest->ThisPort->winding, 
							  pPortTest->ThisPort->center );

		pathPoints.Append(point);
		PortPtrs.Append( pPortTest->ThisPort );

		p1 = point;
		pPortTest = pPortTest->PrevPort;
	}

	// check if we have enough floods to perform the 2nd iteration
	if( (floods - 2) < 0 )
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Path min: Skipping second iteration, not enough portals\r" );
		goto Quit;
	}

	// second iteration, going forwards from source to target
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Path min: Performing second iteration\r" );


	p1 = propParms->origin;

	for( int k = 0; k < floods; k++ )
	{
		// recalculate the N-1th point using the Nth point and N-2th point on either side
		pPort2nd = PortPtrs[floods - k - 1];
		if( pPort2nd == NULL)
			DM_LOG(LC_SOUND, LT_ERROR).LogString("ERROR: pPort2nd is NULL\r" );

		// the last point tested must be the AI position
		if( (floods - k - 2) >= 0 )
			p2 = pathPoints[floods - k - 2];
		else
			p2 = AIpos;
		
		// DM_LOG(LC_SOUND, LT_DEBUG).LogString("2nd iter: Finding optimum surface point\r" );
		point = OptSurfPoint( p1, p2, pPort2nd->winding, pPort2nd->center );
		pathPoints[floods - k -1] = point;

		p1 = point;
	}

Quit:

// Now go through and calculate the new loss

	// get loss to 1st point on the path
	totDist = (AIpos - pathPoints[0]).LengthFast() * s_DOOM_TO_METERS;
	curArea = PortPtrs[0]->to;
	totAtt = totDist * m_AreaPropsG[ curArea ].LossMult;

	// add the rest of the loss from the path points
	for( int j = 0; j < (floods - 1); j++ )
	{
		tempDist = (pathPoints[j+1] - pathPoints[j]).LengthFast() * s_DOOM_TO_METERS;
		curArea = PortPtrs[j]->to;
		tempAtt = tempDist * m_AreaPropsG[ curArea ].LossMult;
		
		totDist += tempDist;
		totAtt += tempAtt;
	}

	// add the loss from the final path point to the source
	tempDist = ( pathPoints[floods - 1] - propParms->origin ).LengthFast() * s_DOOM_TO_METERS;
	curArea = PortPtrs[ floods - 1 ]->to;
	tempAtt = tempDist * m_AreaPropsG[ curArea ].LossMult;

	totDist += tempDist;
	totAtt += tempAtt;

	// Finally, convert to acoustic spreading + attenuation loss in dB
	totLoss = m_SndGlobals.Falloff_Ind * s_invLog10*idMath::Log16(totDist) + totAtt + 8;

	propParms->direction = pathPoints[0];
	propParms->propVol = volInit - totLoss;

	// draw debug lines if show soundprop cvar is set
	if( cv_spr_show.GetBool() )
	{
		pathPoints.Insert(AIpos, 0);
		pathPoints.Append(propParms->origin);
		DrawLines( &pathPoints );
	}

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Detailed path minimization for AI %s finished\r", AI->name.c_str() );

	return;
}

void CsndProp::DrawLines( idList<idVec3> *pointlist )
{
	for( int i=0; i<( pointlist->Num() - 1 ); i++)
	{
		gameRenderWorld->DebugLine( colorGreen, pointlist->operator[](i), pointlist->operator[](i+1), 3000);
	}
}
	

