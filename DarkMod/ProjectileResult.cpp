/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.5  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.4  2006/02/04 23:51:08  sparhawk
 * Added a destructor and fixed some stuff for Stim/Response.
 *
 * Revision 1.3  2006/01/31 22:34:44  sparhawk
 * StimReponse first working version
 *
 * Revision 1.2  2006/01/25 22:05:51  sparhawk
 * Added additional entries to support stims on projectiles.
 *
 * Revision 1.1  2006/01/20 08:47:45  ishtvan
 * initial version
 *
 *
 * Revision 1.1  2006/01/06 20:19:12  ishtvan
 * Initial Release
 *
 *
 ***************************************************************************/

// Copyright (C) 2006 Chris Sarantos <csarantos@gmail.com>

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "ProjectileResult.h"
#include "../game/Game_local.h"
#include "../game/projectile.h"
#include "StimResponse.h"
#include "DarkModGlobals.h"

//===============================================================================
//CProjectileResult
//===============================================================================

// TODO: Make sure BINDING works with the new setup (copybind)

/**
* Name of the script to be run when projectile hits activation surface
**/
const char* s_ACTIVE_SCRIPT_NAME = "active";

/**
* Name of script to be run when projectile hits dud surface
**/
const char* s_DUD_SCRIPT_NAME = "dud";

const idEventDef EV_TDM_GetFinalVel( "getFinalVel", NULL, 'v' );
const idEventDef EV_TDM_GetFinalAngVel( "getFinalAngVel", NULL, 'v' );
const idEventDef EV_TDM_GetAxialDir( "getAxialDir", NULL, 'v' );
const idEventDef EV_TDM_GetProjMass( "getProjMass", NULL, 'f' );
const idEventDef EV_TDM_GetSurfType( "getSurfType", NULL, 's' );
const idEventDef EV_TDM_GetSurfNormal( "getSurfNormal", NULL, 'v' );
const idEventDef EV_TDM_GetStruckEnt( "getStruckEnt", NULL, 'e' );
const idEventDef EV_TDM_GetIncidenceAngle( "getIncidenceAngle", NULL, 'f' );

CLASS_DECLARATION( idEntity, CProjectileResult )
	EVENT( EV_TDM_GetFinalVel,				CProjectileResult::Event_GetFinalVel )
	EVENT( EV_TDM_GetFinalAngVel,			CProjectileResult::Event_GetFinalAngVel )
	EVENT( EV_TDM_GetAxialDir,				CProjectileResult::Event_GetAxialDir )
	EVENT( EV_TDM_GetProjMass,				CProjectileResult::Event_GetProjMass )
	EVENT( EV_TDM_GetSurfType,				CProjectileResult::Event_GetSurfType )
	EVENT( EV_TDM_GetSurfNormal,			CProjectileResult::Event_GetSurfNormal )
	EVENT( EV_TDM_GetStruckEnt,				CProjectileResult::Event_GetStruckEnt )
	EVENT( EV_TDM_GetIncidenceAngle,		CProjectileResult::Event_GetIncidenceAngle )
END_CLASS

CProjectileResult::CProjectileResult( void )
{
	// initialize m_ProjData
	m_ProjData.FinalOrigin.Zero(); 
	m_ProjData.FinalAxis.Zero();
	m_ProjData.LinVelocity.Zero();
	m_ProjData.AngVelocity.Zero();
	m_ProjData.AxialDir.Zero();
	m_ProjData.mass = 0;

	m_bActivated = false;

	// initialize the trace data
	m_Collision.endpos.Zero();
	m_Collision.endAxis.Zero();
	m_Collision.c.point.Zero();
	m_Collision.c.normal.Set( 0, 0, 1 );
}

CProjectileResult::~CProjectileResult(void)
{
}


void CProjectileResult::Init
		(
			SFinalProjData *pData, const trace_t &collision,
			idProjectile *pProj, bool bActivate
		)
{
	idVec3 dotprod, LinVelocity;
	float fTemp;
	int StimType = ST_DEFAULT;
	float StimRadius = 10.0; // we use a (hopefully) reasonable default radius if none is set.

	// copy in the data
	m_Collision = collision;
	
	m_ProjData.FinalOrigin = pData->FinalOrigin;
	m_ProjData.FinalAxis = pData->FinalAxis;
	m_ProjData.LinVelocity = pData->LinVelocity;
	m_ProjData.AngVelocity = pData->AngVelocity;
	m_ProjData.AxialDir = pData->AxialDir;
	m_ProjData.mass = pData->mass;
	m_ProjData.SurfaceType = pData->SurfaceType;

	m_bActivated = bActivate;

	// calculate and store the (max) angle of incidence
// NOTE: For now, angle of incidence is based on velocity, not axis
// To base it on axis we would need all projectiles to follow some modeleing
// convention (e.g., z is always the axial direction, or something)
	LinVelocity = m_ProjData.LinVelocity;

	LinVelocity.NormalizeFast();
	fTemp = (LinVelocity * collision.c.normal);
	
	m_ProjData.IncidenceAngle = idMath::Abs( idMath::ACos( fTemp ) );

	// Move to the point of the collision, hide self
	Hide();
	GetPhysics()->SetOrigin( pData->FinalOrigin );
	GetPhysics()->SetAxis( pData->FinalAxis );

	// The stim type of the projectile result is defined on the projectile itself
	// even though it is not used there. Logically, the stim type is a part of the
	// projectile definition though, since this class is only a helper class.
	pProj->spawnArgs.GetInt("stim_type", "-1", StimType);
	if(StimType != ST_DEFAULT)
	{
		CStim *s;
		pProj->spawnArgs.GetFloat("stim_radius", "10", StimRadius);
		s = AddStim(StimType, StimRadius);
		s->m_State = SS_ENABLED;
		idStr Name;
		sprintf(Name, "%08lX_", this);
		if(StimType < ST_USER)
			Name += cStimType[StimType];

		SetName(name.c_str());
        DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("Stim type %u with radius %f added to entity %08lX\r", StimType, StimRadius, this);
	}

	// Handle binding
	if( spawnArgs.GetBool( "copy_bind", "0") && pProj 
		&& pProj->spawnArgs.GetBool("bindOnImpact", "0") )
	{
		Event_CopyBind( pProj );
	}

	// Run scripts:

	RunResultScript();
}


void CProjectileResult::RunResultScript( void )
{
	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Running projectile result script\r" );

	const char *funName;

	if( m_bActivated )
		funName = s_ACTIVE_SCRIPT_NAME;
	else
		funName = s_DUD_SCRIPT_NAME;

	const function_t *pScript = scriptObject.GetFunction( funName );
	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Attempting to run script %s\r", funName );

	// run the script
	if(pScript)
	{
		DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Script found, running...\r" );
		
		idThread *pThread = new idThread;
		pThread->CallFunction(this, pScript, true);
		pThread->DelayedStart(0);
	}
}

void CProjectileResult::Event_GetFinalVel( void ) 
{
	idThread::ReturnVector( m_ProjData.LinVelocity );
}

void CProjectileResult::Event_GetFinalAngVel( void ) 
{
	idThread::ReturnVector( m_ProjData.AngVelocity );
}

void CProjectileResult::Event_GetAxialDir( void ) 
{
	idThread::ReturnVector( m_ProjData.AxialDir );
}

void CProjectileResult::Event_GetProjMass( void ) 
{
	idThread::ReturnFloat( m_ProjData.mass );
}

void CProjectileResult::Event_GetSurfType( void ) 
{
	idThread::ReturnString( m_ProjData.SurfaceType );
}

void CProjectileResult::Event_GetSurfNormal( void ) 
{
	idThread::ReturnVector( m_Collision.c.normal );
}

void CProjectileResult::Event_GetStruckEnt( void ) 
{
	idThread::ReturnEntity( gameLocal.entities[ m_Collision.c.entityNum ] );
}

void CProjectileResult::Event_GetIncidenceAngle( void )
{
	idThread::ReturnFloat( m_ProjData.IncidenceAngle );
}