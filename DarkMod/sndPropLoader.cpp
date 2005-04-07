/******************************************************************************/
/*                                                                            */
/*         Dark Mod Sound Propagation (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
* DESCRIPTION: Sound propagation class for compiling sound propagation data
* from a Mapfile and write it to a file.  Also used to read the file on map init.
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
#include "matrixsq.h"

// TODO: Write the mapfile timestamp to the .spr file and compare them

/*********************************************************
*
*	CsndPropBase Implementation
*
**********************************************************/

SLMEntry *CsndPropBase::GetLM(int row, int col, bool *reversed)
{
	bool rev;
	int temp;
	idVec3 tempV;
	SLMEntry *Entry;

	rev = false;

	if (row>col)
	{
		rev = true;
		temp = col;
		col = row;
		row = temp;
	}

	Entry = m_LossMatrix->Get(row, col);

	// Since reverse is an optional argument set to NULL by default,
	// don't set the pointer if an argument was not provided
	if (reversed != NULL)
		reversed = &rev;
	return Entry;
}

void CsndPropBase::GlobalsFromDef( void )
{
	const idDict *def;

	def = gameLocal.FindEntityDefDict( "soundprop_globals", false );

	if(!def)
	{
		gameLocal.Warning("[DarkMod Sound Prop] : Did not find def for soundprop_globals.  Bad or missing soundprop.def file.  Using default values.");
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Did not find def for soundprop_globals.  Using default values.\r");
		DefaultGlobals();
		goto Quit;
	}

	m_SndGlobals.bDebug = def->GetBool("debug", "0");
	m_SndGlobals.AreaPropName = def->GetString("aprop_name", "");
	m_SndGlobals.doorName = def->GetString("door_name", "");
	m_SndGlobals.d3DoorName = def->GetString("d3door_name", "func_door");
	m_SndGlobals.fileExt = def->GetString("file_ext", "spr");

	m_SndGlobals.MaxPaths = def->GetInt("maxpaths", "3");
	m_SndGlobals.DoorExpand = def->GetFloat("doorexpand", "1.0");
	m_SndGlobals.Falloff_Outd = def->GetFloat("falloff_outd", "10.0");
	m_SndGlobals.Falloff_Ind = def->GetFloat("falloff_ind", "9.0");
	m_SndGlobals.kappa0 = def->GetFloat("kappa_dbm", "0.015");

	m_SndGlobals.DefaultDoorLoss = def->GetFloat("default_doorloss", "20");
	m_SndGlobals.MaxRange = def->GetFloat("maxrange", "2.2");
	m_SndGlobals.MaxRangeCalVol = def->GetFloat("maxrange_cal", "30");
	m_SndGlobals.MaxEnvRange = def->GetFloat("max_envrange", "50");
	
	m_SndGlobals.Vol = def->GetFloat("vol_ai", "0.0");
	m_SndGlobals.DefaultThreshold = def->GetFloat("default_thresh", "20.0");

Quit:
	return;
}

void CsndPropBase::DefaultGlobals( void )
{
	m_SndGlobals.bDebug = false;
	m_SndGlobals.AreaPropName = "";
	m_SndGlobals.doorName = "";
	m_SndGlobals.d3DoorName = "func_door";
	m_SndGlobals.fileExt = "spr";

	m_SndGlobals.MaxPaths = 3;
	m_SndGlobals.DoorExpand = 1.0f;
	m_SndGlobals.Falloff_Outd = 10.0f;
	m_SndGlobals.Falloff_Ind = 9.0f;
	m_SndGlobals.kappa0 = 0.015f;

	m_SndGlobals.DefaultDoorLoss = 20.0f;
	m_SndGlobals.MaxRange = 2.2f;
	m_SndGlobals.MaxRangeCalVol = 30;
	m_SndGlobals.MaxEnvRange = 50;
	
	m_SndGlobals.Vol = 0.0;
	m_SndGlobals.DefaultThreshold = 20.0f;
}

void CsndPropBase::UpdateGlobals( void )
{
	// link console vars to globals here
}


/*********************************************************
*
*	CsndPropLoader Implementation
*
**********************************************************/


CsndPropLoader::CsndPropLoader ( void )
{
	m_sndAreas = NULL;
	m_numAreas = 0;
	m_bDefaultSpherical = false;
	m_bLoadSuccess = false;

	m_LossMatrix = new CMatRUT<SLMEntry>;
}

CsndPropLoader::~CsndPropLoader ( void )
{
	// Call shutdown in case it was not called before destruction
	Shutdown();

	delete m_LossMatrix;
}

/**
* MapEntBounds PSUEDOCODE:
* DOORS WITH BRUSHES:
* For each plane: add the direction normal vector * d to the origin.  
* This sould be the point we want to add to bounds for each plane,
* to generate a bounding box containing the planes
**/

bool CsndPropLoader::MapEntBounds( idBounds &bounds, idMapEntity *mapEnt )
{
	bool			returnval;
	idMapPrimitive	*testPrim;
	idMapBrush		*brush(NULL);
	idMapBrushSide	*face;
	idPlane			plane;
	int				numFaces, numPrim;
	idVec3			norm, *addpoints, debugCenter;
	idMat3			rotation;
	float			dist, angle;
	const char      *modelName;
	cmHandle_t		cmHandle; // collision model handle for getting bounds

	idDict args = mapEnt->epairs;
	const idVec3 origin = args.GetVector("origin","0 0 0");
	modelName = args.GetString("model");
	
	// if a door doesn't have a model, the modelname will be the same as the doorname
	if( strcmp(modelName,args.GetString("name")) )
	{
		// NOTE: In the LoadModel call, Precache is set to false currnetly.  If it was set to
		// TRUE, this would force the door model to have a .cm file, otherwise
		// LoadModel would return 0 in this case. (precache = true, no .cm file)
		cmHandle = collisionModelManager->LoadModel( modelName, false );
		if ( cmHandle == 0)
		{
			DM_LOG(LC_SOUND, LT_WARNING).LogString("Failed to load collision model for door %s with model %s.  Door will be ignored.\r", args.GetString("name"), modelName);
			returnval = false;
			goto Quit;
		}

		collisionModelManager->GetModelBounds( cmHandle, bounds );
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found bounds with volume %f for door %s with model %s.\r", bounds.GetVolume(), args.GetString("name"), modelName);
		// Rotation copied from Entity::ParseSpawnArgsToRenderEntity()
		// get the rotation matrix in either full form, or single angle form
		if ( !args.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", rotation ) ) 
			{
				angle = args.GetFloat( "angle" );
				if ( angle != 0.0f ) 
				{
					rotation = idAngles( 0.0f, angle, 0.0f ).ToMat3();
				}
				else 
				{
						rotation.Identity();
				}
			}
		bounds.RotateSelf(rotation);
		// Translate and rotate the bounds to be in sync with the model
		bounds.TranslateSelf(origin);
		// NOTE FOR FUTURE REFERENCE: MUST ROTATE THEN TRANSLATE
		/**
		* Global DoorExpand is applied to correct door bound inaccuracies.
		**/
		ExpandBoundsMinAxis(&bounds, m_SndGlobals.DoorExpand);
		debugCenter = bounds.GetCenter();
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Model Bounds center: %s , MapEntity origin: %s\r",debugCenter.ToString(),origin.ToString());
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Bounds rotation: %s\r", rotation.ToString());
		returnval = true;
		goto Quit;
		// Brian says it's okay if LoadModel is run twice, it just won't load it the 2nd time.
		// Therefore we won't worry about freeing the model now. (this caused a crash when I tried)
	}

	// Continue on if the door does not have a model:

	if( (numPrim = mapEnt->GetNumPrimitives()) == 0 )
	{
		DM_LOG(LC_SOUND, LT_WARNING).LogString("Door %s has no primitive data.  Door will be ignored.\r", args.GetString("name"));
		returnval = false;
		goto Quit;
	}
	for (int j=0; j < numPrim; j++)
	{
		testPrim = mapEnt->GetPrimitive(j);
		if ( testPrim->GetType() == testPrim->TYPE_BRUSH )
		{
			brush = static_cast<idMapBrush *>(testPrim);
			break;
		}
	}
	if (brush == NULL)
	{
		DM_LOG(LC_SOUND, LT_WARNING).LogString("Door %s does not have a brush primitive.  Door will be ignored\r", args.GetString("name"));
		returnval = false;
		goto Quit;
	}

	numFaces = brush->GetNumSides();
	addpoints = new idVec3[numFaces];
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("MapEntBounds: Door %s has %d faces\r", mapEnt->epairs.GetString("name"), numFaces );

	for(int i = 0; i < numFaces; i++)
	{
		face = brush->GetSide(i);
		plane = face->GetPlane();
		norm = plane.Normal();
		dist = plane.Dist();
		norm.Normalize();
		//addpoints[i] = ( norm/norm.Normalize() * dist ) + origin;
		//TODO: Make sure this change works correctly:
		addpoints[i] = ( norm * dist ) + origin;
		//DM_LOG(LC_SOUND, LT_DEBUG).LogString("Added point: %s to bounds for %s\r", addpoints[i].ToString(), mapEnt->epairs.GetString("name"));
	}

	bounds.FromPoints(static_cast<const idVec3*>(addpoints), static_cast<const int>(numFaces));
	
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Entity %s has bounds with volume %f\r", args.GetString("name"), bounds.GetVolume() );
    
	delete[] addpoints;
	
	returnval = true;

Quit:
	return returnval;
}

void CsndPropLoader::ExpandBoundsMinAxis( idBounds *bounds, float percent )
{
	idVec3 points[8], diff, mindiff, addpoints[2], oppPoint;
	bounds->ToPoints(points);
	float diffDist, mindiffDist(100000.0f); // initialize mindiffDist to a really big number
	// find the minimum axis and direction vector between minimum points
	for( int i = 1; i<8; i++ )
	{
		diff = points[0] - points[i];
		diffDist = diff.LengthFast();
		if (diffDist < mindiffDist)
		{
			mindiffDist = diffDist;
			mindiff = diff;
			oppPoint = points[i];
		}
	}
	// expand the axis by adding points along that axis
	addpoints[0] = points[0] + mindiff * percent;
	addpoints[1] = oppPoint - mindiff * percent;
	bounds->AddPoint(addpoints[0]);
	bounds->AddPoint(addpoints[1]);
}


void CsndPropLoader::ParseMapEntities ( idMapFile *MapFile )
{
	int			i, count(0), missedCount(0);
	idDict		args;
	idBounds    doorB;
	bool doorParsed(false);
	SDNEntry     tempEntry;
	
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Soundprop: Parsing Map entities\r");
	for (i = 0; i < ( MapFile->GetNumEntities() ); i++ )
	{
		idMapEntity *mapEnt = MapFile->GetEntity( i );
		args = mapEnt->epairs;
		const char *classname = args.GetString("classname");

		if( !strcmp(classname,m_SndGlobals.AreaPropName) )
		{
			ParseAreaPropEnt(args);
		}
		
		if( !strcmp(classname,"worldspawn") )
		{
			ParseWorldSpawn(args);
		}

		if( !strcmp(classname,m_SndGlobals.doorName) 
			|| !strcmp(classname,m_SndGlobals.d3DoorName) )
		{
			doorB.Clear();
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsing door (entity %d)\r", i);
			if (MapEntBounds( doorB, mapEnt ) )
			{
				doorParsed = ParseMapDoor(args, &doorB);
			}
			if (doorParsed)
			{ 
				tempEntry.name = args.GetString("name");
				tempEntry.doorID = count;
				m_DoorNameTable.Append( static_cast<const SDNEntry>(tempEntry) );
				DM_LOG(LC_SOUND, LT_DEBUG).LogString("Added door %s to table with ID %d\r",args.GetString("name"), count);
				count++;
			}
			else
			{
				missedCount++;
			}
			doorParsed = false;
		}
	}

	m_AreaProps.Condense();
	m_DoorRefs.Condense();
	FillAPGfromAP( gameRenderWorld->NumAreas() );

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Finished parsing map entities\r");
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("DOORS: %d / %d doors parsed successfully\r",count,(count+missedCount));
}

void CsndPropLoader::ParseWorldSpawn ( idDict args )
{
	bool SpherDefault;

	SpherDefault = args.GetBool("outdoor_propmodel","0");

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsing worldspawn sound prop data...\r" );
	if(SpherDefault)
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Using outdoor sound prop model as the default for this map\r");
	}
	else
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Using indoor sound prop model as the default for this map\r" );
	}
	m_bDefaultSpherical = SpherDefault;
}

void CsndPropLoader::ParseAreaPropEnt ( idDict args )
{
	int area;
	float lossMult;
	bool SpherSpread(false);
	SAreaProp propEntry;

	// define these strings for the log reporting 
	// (because I'm too lazy to put in another if statement)
	idStr modelnames[2];
	modelnames[0] = "indoor propagation model";
	modelnames[1] = "outdoor propagation model";
	
	idStr lossvalue = args.GetString("loss_mult");

	if(!( lossvalue.IsNumeric() ))
	{
		lossMult = 1.0;
		DM_LOG(LC_SOUND, LT_WARNING).LogString("Warning: Non-numeric loss_mult value on sound area entity: %s.  Default loss value assumed\r", args.GetString("name") );
	}
	else
	{
		lossMult = fabs(atof(lossvalue));
	}
	
	if ( ( area = gameRenderWorld->PointInArea(args.GetVector("origin")) ) == -1 )
	{
		DM_LOG(LC_SOUND, LT_WARNING).LogString("Warning: Sound area properties entity %s is not placed in any area.  It will be ignored\r", args.GetString("name") );
		goto Quit;
	}
	propEntry.LossMult = lossMult;
	propEntry.area = area;

	SpherSpread = args.GetBool("outdoor_prop","0");
	propEntry.SpherSpread = SpherSpread;

	//add to the area properties list
	m_AreaProps.Append( static_cast<const SAreaProp>(propEntry) );
			
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Entity %s is a sound area entity.  Applied loss multiplier %f, using %s\r", args.GetString("name"), fabs(atof(lossvalue)), modelnames[ int(SpherSpread) ] );
	
Quit:
	return;
}

void CsndPropLoader::FillAPGfromAP ( int numAreas )
{
	int i, j, area(0);

	m_AreaPropsG.Clear();

	m_AreaPropsG.SetNum( numAreas );

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Allocated m_AreaPropsG for %d areas\r", numAreas );

	// set default values on each area
	for (i=0; i<numAreas; i++)
	{
		m_AreaPropsG[i].LossMult = 1.0;
		m_AreaPropsG[i].SpherSpread = m_bDefaultSpherical;
	}

	for(j=0; j < m_AreaProps.Num(); j++)
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Applying area property entity %d to gameplay properties array\r", j );
		area = m_AreaProps[j].area;
		m_AreaPropsG[area].LossMult = m_AreaProps[j].LossMult;
		m_AreaPropsG[area].SpherSpread = m_AreaProps[j].SpherSpread;
	}

	return;
}


bool CsndPropLoader::ParseMapDoor( idDict args, idBounds *b )
{
	bool		returnval(false);
	// BoundsInAreas seems to cause a crash if it finds more areas than
	// the array you give it (even though you specify a max number of areas to find)
	// So, quick hack: initialize the array of areas to contain 10.
	// TODO: Try this again now that code is fixed, to make sure it was in
	// fact BoundsInAreas causing the crash.
	int			boundAreas[10]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; 
	int			numAreas(0), pnum;
	SDoorRef	doorEntry;
	qhandle_t	pHandle = -1;

	numAreas = gameRenderWorld->BoundsInAreas( static_cast<const idBounds>(*b), boundAreas, 10 );
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("DEBUG:numAreas = %d\r", numAreas);

	if ( numAreas == 2 && boundAreas[0] != -1 )
	{
		// find the portal inside bounds b
		pHandle = gameRenderWorld->FindPortal( *b );
		
		if ( pHandle == -1 ) 
		{
			DM_LOG(LC_SOUND, LT_WARNING).LogString("Warning: Sound prop did not find a portal inside func_door object %s.  Door will be ignored.\r", args.GetString("name") );
			goto Quit;
		}
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Found portal handle %d for door %s\r", pHandle, args.GetString("name"));
		pnum = FindSndPortal(boundAreas[0], pHandle);
		
		if ( pnum == -1 )
		{
			DM_LOG(LC_SOUND, LT_WARNING).LogString("Warning: Sound prop failed to match a door name to a portal handle.\r");
			goto Quit;
		}
		
		doorEntry.area = boundAreas[0]; //arbitrarily pick area 0, it doesn't matter since we will get the return portal later
		doorEntry.doorName = args.GetString("name");
		doorEntry.portalH = pHandle;
		doorEntry.portalNum = pnum;
		
		m_DoorRefs.Append( static_cast<const SDoorRef>(doorEntry) );
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Assigned portal number %d in area %d to DoorRef entry for door %s.\r", doorEntry.portalNum, boundAreas[0], doorEntry.doorName);

		returnval = true;
		goto Quit;
	}
	else
	{
		DM_LOG(LC_SOUND, LT_WARNING).LogString("DEBUG: Door object %s contacted %d areas.\r", args.GetString("name"), numAreas );
		if (numAreas < 2)
		{
			DM_LOG(LC_SOUND, LT_WARNING).LogString("Warning: Portal in door %s is not touching two areas.  Door will be ignored.\r", args.GetString("name") );
		}
		if (numAreas > 2)
		{
			DM_LOG(LC_SOUND, LT_WARNING).LogString("Warning: Sound prop found more than two areas for func_door object %s.  Door will be ignored.\r", args.GetString("name") );
		}
	}
Quit:
	return returnval;
}

int CsndPropLoader::FindSndPortal(int area, qhandle_t pHandle)
{
	int np, val(-1);
	exitPortal_t portalTmp;
		
	np = gameRenderWorld->NumPortalsInArea(area);
	for (int i = 0; i < np; i++)
	{
		portalTmp = gameRenderWorld->GetPortal(area,i);
		//DM_LOG(LC_SOUND, LT_DEBUG).LogString("FindSndPortal: Desired handle %d, handle of portal %d: %d\r", pHandle, i, areaP->portals[i].handle ); //Uncomment for portal handle debugging
		if(portalTmp.portalHandle == pHandle)
		{
			val = i;
			goto Quit;
		}
	}
Quit:
	return val;
}

void CsndPropLoader::CreateAreasData ( void )
{
	int i, j, k, np, anum, anum2, propscount(0), numAreas(0);
	sndAreaPtr area;
	exitPortal_t portalTmp;
	idVec3 pCenters;

	numAreas = gameRenderWorld->NumAreas();
	pCenters.Zero();

	if( (m_sndAreas = new SsndArea[numAreas]) == NULL )
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Create Areas: Out of memory when allocating array of %d Areas\r", numAreas);
			goto Quit;
	}
	
	for ( i = 0; i < numAreas; i++ ) 
	{
		area = &m_sndAreas[i];
		area->LossMult = 1.0;
		np = gameRenderWorld->NumPortalsInArea(i);
		area->numPortals = np;

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Number of Portals in Area %d = %d\r", i, np);

		if( (area->portals = new SsndPortal[np]) == NULL )
		{
			DM_LOG(LC_SOUND, LT_ERROR).LogString("Create Areas: Out of memory when building portals array for Area %d\r", i);
			goto Quit;
		}
		for ( j = 0; j < np; j++ ) 
		{
			portalTmp = gameRenderWorld->GetPortal(i,j);
			
			area->portals[j].portalNum = j;
			area->portals[j].handle = portalTmp.portalHandle;
			area->portals[j].from = portalTmp.areas[0]; // areas[0] is the 'from' area
			area->portals[j].to = portalTmp.areas[1];
			area->portals[j].center = portalTmp.w->GetCenter();
			
			pCenters += area->portals[j].center;
			area->portals[j].doorName = NULL; // this will be set later
		}
		
		// average the portal center coordinates to obtain the area center
		if ( np )
		{
			area->center = pCenters / np;
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Area %d has center %s\r", i, area->center.ToString() );
		}
	}

	// Apply special area Properties (loss and indoor/outdoor)

	for(k = 0; k < m_AreaProps.Num(); k++)
	{
		anum = m_AreaProps[k].area;
		m_sndAreas[anum].LossMult = m_AreaProps[k].LossMult;
		m_sndAreas[anum].SpherSpread = m_AreaProps[k].SpherSpread;
		propscount++;
	}

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("%d Area specific losses applied\r", propscount);
	
	// Attach door identifiers to the portals
	for (k = 0; k < m_DoorRefs.Num(); k++)
	{
		// check thru all portals in the area until matching handle is found
		anum2 = m_DoorRefs[k].area;
		sndAreaPtr areaP2 = &m_sndAreas[anum2];
		
		int pInd = m_DoorRefs[k].portalNum;
		areaP2->portals[pInd].doorName = const_cast<char *>( m_DoorRefs[k].doorName );
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Assigned Doorname %s to portal %d in area %d.\r", m_DoorRefs[k].doorName, pInd, anum2);
		
		// Find the portal in the area it leads to and assign it to the door too
		int toArea = areaP2->portals[pInd].to;
		sndAreaPtr areaP3 = &m_sndAreas[toArea];
		int pInd2 = FindSndPortal(toArea, m_DoorRefs[k].portalH);
		
		// Don't need error handling here, since if the 'from' portal matches, this one must also
		areaP3->portals[pInd2].doorName = const_cast<char *>( m_DoorRefs[k].doorName );
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Reverse portal: Assigned Doorname %s to portal %d in area %d.\r", m_DoorRefs[k].doorName, pInd2, toArea);
	}

    DM_LOG(LC_SOUND, LT_DEBUG).LogString("Create Areas arrays finished.\r");
Quit:
	return;
}

void CsndPropLoader::DestroyAreasData( void )
{
	int i;
	SsndPortal *portalPtr;

	if( m_sndAreas == NULL || m_numAreas == 0)
	{
		goto Quit;
	}
	for( i=0; i < m_numAreas; i++ )
	{
		portalPtr = m_sndAreas[i].portals;
		delete[] portalPtr;
	}

	delete[] m_sndAreas;
	m_sndAreas = NULL;
	m_numAreas = 0;

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Destroy Areas data finished.\r");
	
Quit:
	// clear the area properties
	m_AreaProps.Clear();
	return;
}

void CsndPropLoader::CompileMap( idMapFile *MapFile  )
{
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Sound propagation system initializing...\r");

	m_DoorRefs.Clear();
	m_DoorNameTable.Clear();

	// Just in case this was somehow not done before now
	DestroyAreasData();

	m_numAreas = gameRenderWorld->NumAreas();

	ParseMapEntities(MapFile);

	CreateAreasData();

	// Build the Loss Matrix here with pathfinding algorithm!!!

	// Write loss matrix to file

	DestroyAreasData();

	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Sound propagation system finished loading.\r");
}

void CsndPropLoader::Shutdown( void )
{
	//Destroy the AALDB data when switching to a new map, game ends, etc
	DestroyAreasData();
	DM_LOG(LC_SOUND, LT_DEBUG).LogString("Clearing sound propagation data loader.\r");

	if( !m_LossMatrix->IsCleared() )
	{
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Clearing loss matrix.\r");
		m_LossMatrix->Clear();
	}

	m_bDefaultSpherical = false;
	m_bLoadSuccess = false;

	m_AreaPropsG.Clear();
	m_DoorRefs.Clear();
	m_DoorNameTable.Clear();
}

bool CsndPropLoader::LoadSprFile (const char *pFN)
{
	idStr qpath;
	idToken token;
	idLexer src;
	bool inLM(false), inEntry(false), mismatch, success(false);
	int level(0); // Nesting level for brackets
	int dim(0); // dimension of the given loss matrix
	int indices[2], pathNum(-1);
	float *tempLosses;
	int *tempPropmodels;
	SLMEntry LMEntry;
	
	qpath = pFN;
	qpath.StripFileExtension();

	qpath.DefaultPath( "./Maps/" );

	qpath += ".";
	qpath += m_SndGlobals.fileExt;
	
	if ( !src.LoadFile(qpath.c_str()) )
	{
		DM_LOG(LC_SOUND, LT_WARNING).LogString("Could not read file %s\r", qpath.c_str());
		idLib::common->Printf( "Could not read file %s\n", qpath.c_str() );
		goto Quit;
	}

	m_DoorNameTable.Clear();

	while(1)
	{
		if(!src.ReadToken(&token))
			goto Quit;

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Token: [%s]\r", token.c_str());
		if (token == "LossMatrix")
		{
			inLM = true;
			dim = src.ParseInt();
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Loss matrix dimension = %d\r", dim);
			if (!dim)
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Bad or no dimension provided for loss matrix, aborting.\r");
				goto Quit;
			}
			if (!m_LossMatrix->Init(dim))
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Error initializing loss matrix.  Aborting.\r");
				goto Quit;
			}
			continue;
		}

		else if(token == "{")
		{
			level++;
			if ((level == 2 && inEntry) )
			{
				mismatch = true;
				goto Quit;
			}
			continue;
		}
		
		else if(token == "}")
		{
			level--;
			if (inEntry && level == 1 && indices != NULL)
			{
				// put the entry in to the lossmatrix before exiting entry
				m_LossMatrix->Set(indices[0],indices[1], LMEntry);
				inEntry = false;
				pathNum = -1;
			}
			else if(level <= -1)
			{
				mismatch = true;
				goto Quit;
			}
			continue;
		}
		
		// parse an entry
		else if( token.Icmpn("Entry", 5) == 0 && level == 1 && inLM )
		{
			inEntry = true;
			src.Parse1DMatrix( 2, indices );
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsing LM Entry with indices %d, %d\r", indices[0], indices[1]);
			if (!src.ExpectTokenString( "{" ))
			{
				mismatch = true;
				goto Quit;
			}
			level++;
			if (indices == NULL)
				goto Quit;
			if (!src.ExpectTokenString( "nump" ))
				goto Quit;
			LMEntry.from = indices[0];
			LMEntry.to = indices[1];
			LMEntry.numPaths = src.ParseInt();
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Number of paths = %d\r", LMEntry.numPaths);
			if (!LMEntry.numPaths)
				goto Quit;
			if (!(LMEntry.paths = new SPropPath[LMEntry.numPaths]))
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when creating path array for entry %d, %d.\r", indices[0], indices[1]);
				goto Quit;
			}
			continue;
		}
		
		else if( token.Icmpn("path",4) == 0 && level == 2 && inLM && LMEntry.numPaths )
		{
			pathNum++;
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsing Path %d in Entry %d, %d ...\r", pathNum, indices[0], indices[1]);
			if ( !ParsePath( &src, pathNum, indices, &LMEntry ) )
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Error Parsing Path %d in Entry %d, %d, aborting.\r", pathNum, indices[0], indices[1]);
				goto Quit;
			}
			continue;
		}
		
		else if(token.Icmpn("DoorTable", 9) == 0)
		{
			if( ParseDoorTable( &src ) == false )
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Parse door name table failed, aborting.\r");
				goto Quit;
			}
			continue;
		}
		
		else if(token.Icmpn("AreaLossMults", 13) == 0)
		{
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsing area loss multipliers...\r");

			if ((tempLosses = new float[dim]) == NULL)
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when creating area properties array for %d areas (temp losses array)\r", dim);
				goto Quit;
			}

			if( !(src.Parse1DMatrix(dim, tempLosses)) )
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Parse area loss multiplier table failed, aborting.\r");
				goto Quit;
			}

			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsed area loss data for %d areas\r", dim);
			continue;
		}
		else if(token.Icmpn("AreaPropModels", 14) == 0)
		{
			if ((tempPropmodels = new int[dim]) == NULL)
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when creating area properties array for %d areas (temp propmodels array)\r", dim);
				goto Quit;
			}

			if( !(src.Parse1DMatrix(dim, tempPropmodels)) )
			{
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Parse area prop. model table failed, aborting.\r");
				goto Quit;
			}
			
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parsed area prop. model data for %d areas.\r", dim);
			// should be done parsing now
			success = true;
			break;
		}
	}

	if(success)
	{
		//assign the losses and propmodels to the m_AreaPropsG array
		m_AreaPropsG.SetNum( dim );

		for (int ind = 0; ind < dim; ind++)
		{
			m_AreaPropsG[ind].LossMult = tempLosses[ind];
			
			if(tempPropmodels[ind] > 0)
				m_AreaPropsG[ind].SpherSpread = true;
			else
				m_AreaPropsG[ind].SpherSpread = false;
		}

		DM_LOG(LC_SOUND, LT_DEBUG).LogString("Parse sound prop file COMPLETED.\r");
	}

Quit:	
	if(success == false)
	{
		if ( !m_LossMatrix->IsCleared() )
			m_LossMatrix->Clear();
		m_DoorNameTable.Clear();
	}
	if (tempLosses !=NULL)
		delete[] tempLosses;
	if (tempPropmodels != NULL)
		delete[] tempPropmodels;

	// set the member var according to whether we successfully loaded
	m_bLoadSuccess = success;

	return success;
}

bool CsndPropLoader::ParsePath( idLexer *src, int pathNum, int *indices, SLMEntry *pLMEntry )
{
	bool returnval(true), mismatch, floatErr;
	idToken token2;
	SPropPath *pathEntry;
	float loss;
	float start[3], end[3];
	int *doors;
	int num;
	idVec3 Vstart, Vend;

	if((pathEntry = new SPropPath) == NULL)
	{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when creating path %d in entry %d,%d\r", pathNum, indices[0], indices[1]);
		returnval = false;
		goto Quit;
	}
	if ( !src->ExpectTokenString( "{" ) )
	{
		returnval = false;
		mismatch = true;
		goto Quit;
	}
	
	while(1)
	{
		if(!src->ReadToken(&token2))
		{
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Unexpected EOF in sound prop file\r");
		returnval = false;
		goto Quit;
		}
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("[Parse Path] Token: [%s]\r", token2.c_str());

		if(token2.Icmpn("loss", 4) == 0)
		{
			loss = src->ParseFloat(&floatErr);
			if (floatErr)
			{
				DM_LOG(LC_SOUND, LT_WARNING).LogString("Error parsing loss for path %d in entry %d,%d, default huge loss applied\r", pathNum, indices[0], indices[1]);
				loss = 10000.0f;
			}
			pathEntry->loss = loss;
			continue;
		}
		else if(token2.Icmpn("start", 5) == 0)
		{
			if(!src->Parse1DMatrix( 3, start ))
			{
				returnval = false;
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Error parsing start coords of path %d in entry %d,%d\r", pathNum, indices[0], indices[1]);
				goto Quit;
			}
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Added start vector for path %d: ( %f %f %f )\r", pathNum, start[0], start[1], start[2]);
			Vstart.Set(start[0], start[1], start[2]);
			pathEntry->start = Vstart;
			continue;
		}

		else if(token2.Icmpn("end", 3) == 0)
		{
			if(!src->Parse1DMatrix( 3, end ))
			{
				returnval = false;
				DM_LOG(LC_SOUND, LT_ERROR).LogString("Error parsing end coords of path %d in entry %d,%d\r", pathNum, indices[0], indices[1]);
				goto Quit;
			}
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("Added end vector for path %d: ( %f %f %f )\r", pathNum, end[0], end[1], end[2]);
			Vend.Set(end[0], end[1], end[2]);
			pathEntry->end = Vend;
			continue;
		}

		else if(token2.Icmpn("doors", 5) == 0)
		{
			num = src->ParseInt(); // get the number of doors
			DM_LOG(LC_SOUND, LT_DEBUG).LogString("[Parse Path] Number of doors = %d\r", num );
			if (num != 0 )
			{
				if ((doors = new int[num]) == NULL)
				{
					DM_LOG(LC_SOUND, LT_ERROR).LogString("Out of memory when creating door array for path %d in Entry %d, %d\r", pathNum, indices[0], indices[1]);
					returnval = false;
					goto Quit;
				}
				if(!src->Parse1DMatrix(num, doors))
				{
					returnval = false;
					DM_LOG(LC_SOUND, LT_ERROR).LogString("Error parsing door array of path %d in entry %d,%d\r", pathNum, indices[0], indices[1]);
					goto Quit;
				}
				pathEntry->numDoors = num;
				pathEntry->doors = doors;
			}
			// this was the last path variable.  We should be done parsing the path now.
			pLMEntry->paths[pathNum] = *pathEntry;
			break;
		}
	}
	if ( !src->ExpectTokenString( "}" ) ) 
	{
		returnval = false;
		mismatch = true;
	}
Quit:
	if (returnval == false)
	{
		delete pathEntry;
		pathEntry = NULL;
		if(doors)
			delete[] doors;
		if(mismatch)
			DM_LOG(LC_SOUND, LT_WARNING).LogString("Mismatched brace in path %d in Entry %d,%d\r", pathNum, indices[0], indices[1]);
	}
	return returnval;
}

bool CsndPropLoader::ParseDoorTable( idLexer *src )
{
	bool returnval(true);
	idToken token3;
	idStr doorID, *pName(NULL);
	SDNEntry tableEntry;

	if ( !src->ExpectTokenString( "{" ) )
	{
		returnval = false;
		DM_LOG(LC_SOUND, LT_ERROR).LogString("Missing brace in door name table\r");
		goto Quit;
	}
	while( src->ReadToken(&token3) )
	{
		if(token3 == "}")
			goto Quit;
		src->UnreadToken(&token3);
		src->ExpectTokenType( TT_STRING, 0, &token3 );
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("[ParseDoorNames] Token: [%s]\r", token3.c_str());
		if ( token3.IsEmpty() )
		{
			returnval = false;
			DM_LOG(LC_SOUND,LT_ERROR).LogString("Error reading door name string in Door Name table.  (Could also be brace mismatch)\r");
			goto Quit;
		}

		tableEntry.name = token3;
		if( !src->ParseRestOfLine( doorID ) || !doorID.IsNumeric() )
		{
			returnval = false;
			DM_LOG(LC_SOUND, LT_ERROR).LogString("Bad or missing door ID in Door Name table\r");
			goto Quit;
		}
		tableEntry.doorID = atoi(doorID.c_str());
		DM_LOG(LC_SOUND, LT_DEBUG).LogString("[DoorTable] Added door %s with ID %d\r", tableEntry.name.c_str(), tableEntry.doorID);
		m_DoorNameTable.Append( static_cast<const SDNEntry>(tableEntry) );
	}
Quit:
	if(returnval == false)
		m_DoorNameTable.Clear();
	else
		m_DoorNameTable.Condense();
	return returnval;
}

bool CsndPropLoader::WriteSprFile (const char *MapFN, bool fromBasePath, unsigned mapTS )
{
	bool returnval(true);
	int i,j, dim;
	idStr qpath;
	idFile *fp;

	qpath = MapFN;
	qpath.SetFileExtension( m_SndGlobals.fileExt );

	idLib::common->Printf( "Sound prop writing %s...\n", qpath.c_str() );
	if ( fromBasePath ) 
	{
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_devpath" );
	}
	else 
	{
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if ( !fp ) 
	{
		idLib::common->Warning( "Sound prop couldn't open %s for writing\n", qpath.c_str() );
		returnval = false;
		goto Quit;
	}
	if ( m_LossMatrix->IsCleared() || !m_LossMatrix->Dim() )
	{
		idLib::common->Warning( "Loss matrix missing or bad.  Aborting the write of %s.\n", qpath.c_str() );
		returnval = false;	
		goto Quit;
	}

	dim = m_LossMatrix->Dim();
	fp->WriteFloatString( "// Map Time Stamp\n" );
	fp->WriteFloatString( "%d\n", mapTS);

	fp->WriteFloatString( "LossMatrix %d\n", dim );
	fp->WriteFloatString( "{\n" );
	
	//Write RUT matrix entries
	for (i=0; i<( dim - 1); i++)
	{
		for(j=i+1; j<dim; j++)
		{
			if( !(WriteLMEntry(fp, i, j)) )
			{
				returnval = false;
				idLib::common->Warning( "Bad loss matrix element %d, %d.  Aborting the write of %s.\n", i, j, qpath.c_str() );
				goto Quit;
			}
		}
	}
	fp->WriteFloatString( "}\n" );
	fp->WriteFloatString( "\n" );
	fp->WriteFloatString("// End loss matrix, begin door name table\n");
	fp->WriteFloatString( "DoorTable\n{\n" );
	for( int k=0; k < m_DoorNameTable.Num(); k++)
	{
		fp->WriteFloatString( " \"%s\" %d\n", m_DoorNameTable[k].name.c_str(), m_DoorNameTable[k].doorID );
	}
	fp->WriteFloatString( "}\n");
	fp->WriteFloatString( "\n" );
	
	fp->WriteFloatString( "// Area Loss Multipliers\n" );
	fp->WriteFloatString( "AreaLossMults\n" );
	fp->WriteFloatString( "( " );
	for( int u=0; u < dim; u++ )
	{
		fp->WriteFloatString( "%f ", m_AreaPropsG[u].LossMult );
	}
	fp->WriteFloatString( ")\n" );
	
	fp->WriteFloatString( "// Area Sound Propagation Models\n" );
	fp->WriteFloatString( "AreaPropModels\n" );
	fp->WriteFloatString( "( " );
	for( int u2=0; u2 < dim; u2++ )
	{
		fp->WriteFloatString( "%d ", int(m_AreaPropsG[u2].SpherSpread) );
	}
	fp->WriteFloatString( ")\n" );
	// done writing

Quit:
	if( fp )
		idLib::fileSystem->CloseFile( fp );
	return returnval;
}

bool CsndPropLoader::WriteLMEntry( idFile *fp, int i, int j )
{
	bool returnval(true);
	SLMEntry *Entry;
	int dim;
	dim = m_LossMatrix->Dim();
	if( ((Entry = m_LossMatrix->Get(i, j)) == NULL) || (i>j || i>dim) )
	{
		returnval = false;
		goto Quit;
	}
	fp->WriteFloatString( " Entry (%d %d)\n", i, j );
	fp->WriteFloatString( " {\n" );
	fp->WriteFloatString( "  nump %d\n", Entry->numPaths );
	if (Entry->numPaths <= 0)
		goto Quit;
	for (int i=0; i < Entry->numPaths; i++)
	if(!WriteLMPath( fp, Entry, i ))
	{
		returnval = false;
		goto Quit;
	}
	fp->WriteFloatString( " }\n" );
Quit:
	return returnval;
}

bool CsndPropLoader::WriteLMPath( idFile *fp, SLMEntry *Entry, int pathNum )
{
	bool returnval(true);
	SPropPath *path;
	if ( (path = &Entry->paths[pathNum]) == NULL )
	{
		returnval = false;
		idLib::common->Warning( "Bad path %d.\n", pathNum );
		goto Quit;
	}
	fp->WriteFloatString( "  path\n  {\n" );
	fp->WriteFloatString( "   loss\n   %f\n", path->loss );
	fp->WriteFloatString( "   start\n   ");
	fp->WriteFloatString( "(%s)\n", path->start.ToString() );
	fp->WriteFloatString( "   end\n   ");
	fp->WriteFloatString( "(%s)\n", path->end.ToString() );
	fp->WriteFloatString( "   doors %d", path->numDoors );
	if(path->numDoors > 0)
	{
		fp->WriteFloatString("\n   ( ");
		for (int i=0; i < path->numDoors; i++)
		{
			fp->WriteFloatString("%d ", path->doors[i]);
		}
		fp->WriteFloatString(")");
	}
	fp->WriteFloatString("\n  }\n");
Quit:
	return returnval;
}

void CsndPropLoader::testReadWrite( const char *MapFN )
{
	idStr temp = MapFN;

	temp += ".map";
	
	DestroyAreasData();

	m_DoorRefs.Clear();
	m_DoorNameTable.Clear();

	idLib::common->Printf( "Testing read/write with file %s...\n", MapFN );

	const char *OutFN = "sndtest.spr";
	unsigned int testStamp(171717);

	if (LoadSprFile (temp.c_str()) == false)
		goto Quit;

	idLib::common->Printf( "Finished reading in %s, beginning write of %s...\n", MapFN, OutFN );
	
	WriteSprFile( OutFN, false, testStamp );
	idLib::common->Printf( "Finished writing %s.  Sound prop file IO test done.\n", OutFN );
Quit:
	Shutdown();
}