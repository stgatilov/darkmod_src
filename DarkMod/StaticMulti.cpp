// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4071 $
 * $Date: 2010-07-18 15:57:08 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod Team)

/*
   StaticMulti - a variant of func_static that can use a idPhys_StaticMulti
   				 for the clipmodel, e.g. has more than one clipmodel.
				 Used for entities with megamodels as rendermodel.
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: StaticMulti.cpp 4071 2010-07-18 13:57:08Z tels $", init_version);

#include "StaticMulti.h"

// if defined, draw debug output
//#define M_DEBUG 1

CLASS_DECLARATION( idStaticEntity, CStaticMulti )
	EVENT( EV_Activate,				CStaticMulti::Event_Activate )
END_CLASS

/*
===============
CStaticMulti::CStaticMulti
===============
*/
CStaticMulti::CStaticMulti( void ) {
	active = false;

	m_LOD = NULL;
}

/*
================
CStaticMulti::Think
================
*/
void CStaticMulti::Think( void ) 
{
	// will also do LOD thinking:
	idStaticEntity::Think();

#ifdef M_DEBUG
	int num = GetPhysics()->GetNumClipModels();
//	gameLocal.Printf("CStaticMulti::Think %i clipmodels\n", num);

   	idVec4 markerColor (0.3, 0.8, 1.0, 1.0);
   	idVec3 arrowLength (0.0, 0.0, 50.0);

	idPhysics *p = GetPhysics();

	// DEBUG draw arrows for each part of the physics object
	for (int i = 0; i < num; i++)
	{
		idVec3 org = p->GetOrigin( i );
	//gameLocal.Printf("CStaticMulti::Think clip %i %s\n", i, org.ToString() );
	    gameRenderWorld->DebugArrow
			(
			markerColor,
			org + arrowLength,
			org,
			3,
	    	1 );
	}
#endif
}

/*
================
CStaticMulti::Event_Activate
================
*/
void CStaticMulti::Event_Activate( idEntity *activator ) {

	int spawnTime = gameLocal.time;

	active = !active;

	const idKeyValue *kv = spawnArgs.FindKey( "hide" );
	if ( kv ) {
		if ( IsHidden() ) {
			Show();
		} else {
			Hide();
		}
	}

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( spawnTime );
	renderEntity.shaderParms[5] = active;
	// this change should be a good thing, it will automatically turn on 
	// lights etc.. when triggered so that does not have to be specifically done
	// with trigger parms.. it MIGHT break things so need to keep an eye on it
	renderEntity.shaderParms[ SHADERPARM_MODE ] = ( renderEntity.shaderParms[ SHADERPARM_MODE ] ) ?  0.0f : 1.0f;
	BecomeActive( TH_UPDATEVISUALS );
}

