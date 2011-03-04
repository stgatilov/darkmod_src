#include "../idlib/precompiled.h"
#pragma hdrstop

#include "lightgem.h"

// Temporary profiling related macros

#define ENABLE_PROFILING

#ifdef ENABLE_PROFILING
#define PROFILE_BLOCK( block_tag )																				\
class __profile_block {																							\
	idTimer m_timeStamp;																						\
public:																											\
	__profile_block()  {																						\
	m_timeStamp.Start();																						\
	}																											\
	~__profile_block()  {																						\
	m_timeStamp.Stop();																							\
	gameLocal.Printf( #block_tag" : %lf \n\n", m_timeStamp.Milliseconds() );									\
	}																											\
}	_profile_blockInstance_##block_tag;																			\


#define PROFILE_BLOCK_START( block_tag )																		\
	idTimer timer##block_tag;																					\
	timer##block_tag.Start()																					\

// PROFILE_BLOCK_END requires PROFILE_BLOCK_START to be placed before it, to work.
#define PROFILE_BLOCK_END( block_tag )																			\
	timer##block_tag.Stop();																					\
	gameLocal.Printf( #block_tag": %lf \n", timer##block_tag.Milliseconds() )									\

#else

#define PROFILE_BLOCK( block_tag )
#define PROFILE_BLOCK_START( block_tag )
#define PROFILE_BLOCK_END( block_tag )	

#endif
//------------------------
// Construction/Destruction
//----------------------------------------------------
LightGem::LightGem() :
m_LightgemRenderBuffer(NULL),
m_fRetVal (0.0),
m_iRenderPass (0)
{
	m_pfnCurrentState = &LightGem::State_RenderLightGem;
}

LightGem::~LightGem()
{
	Deinitialize();
}


//----------------------------------------------------
// Initialization
//----------------------------------------------------
void LightGem::Initialize()
{
	// Create render pipe - One time initialization only.
	if( !m_LightgemRenderBuffer )
	{
		// Create lightgem render buffer
		m_LightgemRenderBuffer = new idList<char>();

		if (m_LightgemRenderBuffer == NULL) {
			// Out of memory
			DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Out of memory when allocating lightgem render buffer\n");
		}
	}
}

void LightGem::Deinitialize()
{
	if(m_LightgemRenderBuffer)
		delete m_LightgemRenderBuffer;

	m_LightgemRenderBuffer = NULL;
}

void LightGem::Clear()
{
	m_LightgemSurface = NULL;
	m_LightgemShotSpot = 0;
	for(unsigned int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++)
		m_LightgemShotValue[i] = 0.0;
}

void LightGem::SpawnLightGemEntity( idMapFile *	a_mapFile )
{
	static const char *LightgemName = DARKMOD_LG_ENTITY_NAME;
	idMapEntity *mapEnt = NULL;

	mapEnt = a_mapFile->FindEntity(LightgemName);
	if(mapEnt == NULL)
	{
		mapEnt = new idMapEntity();
		a_mapFile->AddEntity(mapEnt);
		mapEnt->epairs.Set("classname", "func_static");
		mapEnt->epairs.Set("name", LightgemName);
		if(strlen(cv_lg_model.GetString()) == 0)
			mapEnt->epairs.Set("model", DARKMOD_LG_RENDER_MODEL);
		else
			mapEnt->epairs.Set("model", cv_lg_model.GetString());
		mapEnt->epairs.Set("origin", "0 0 0");
		mapEnt->epairs.Set("noclipmodel", "1");
	}
}

void LightGem::InitializeLightGemEntity( void )
{
	m_LightgemSurface = gameLocal.FindEntity(DARKMOD_LG_ENTITY_NAME);
	m_LightgemSurface.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = DARKMOD_LG_VIEWID;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noDynamicInteractions = false;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noShadow = true;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noSelfShadow = true;
	DM_LOG(LC_LIGHT, LT_INFO)LOGSTRING("LightgemSurface: [%08lX]\r", m_LightgemSurface.GetEntity());
}

//----------------------------------------------------
// State Persistence 
//----------------------------------------------------

void LightGem::Save( idSaveGame & a_saveGame )
{
	m_LightgemSurface.Save( &a_saveGame );
	a_saveGame.WriteInt(m_LightgemShotSpot);
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++)
	{
		a_saveGame.WriteFloat(m_LightgemShotValue[i]);
	}
}

void LightGem::Restore( idRestoreGame & a_savedGame )
{
	m_LightgemSurface.Restore( &a_savedGame );
	a_savedGame.ReadInt(m_LightgemShotSpot);
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++)
	{
		a_savedGame.ReadFloat(m_LightgemShotValue[i]);
	}

	m_LightgemSurface.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = DARKMOD_LG_VIEWID;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noDynamicInteractions = false;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noShadow = true;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noSelfShadow = true;
	DM_LOG(LC_LIGHT, LT_INFO)LOGSTRING("LightgemSurface: [%08lX]\r", m_LightgemSurface.GetEntity());
}

//----------------------------------------------------
// 
//----------------------------------------------------
float LightGem::Calculate(idPlayer *player)
{
	assert( NULL != m_LightgemRenderBuffer );
	PROFILE_BLOCK( LightGem_Calculate );

	PROFILE_BLOCK_START( LightGem_Calculate_Setup);
	float dist = cv_lg_distance.GetFloat();			// reasonable distance to get a good look at the player/test model
	float fColVal[DARKMOD_LG_MAX_IMAGESPLIT];
	float fRetVal = 0.0;
	int playerid;			// player viewid
	int headid;				// head viewid
	int pdef;				// player modeldef
	int hdef;				// head modeldef
	int psid;				// player shadow viewid
	int hsid;				// head shadow viewid
	int i, nRenderPasses, k, dim, l;
	idStr dps;
	renderView_t rv/*, rv1*/;
	idEntity *lg;
	renderEntity_t *prent;			// Player renderentity
	renderEntity_t *hrent;			// Head renderentity
	renderEntity_t *lgrend;
	const char *dp = NULL;

	lg = m_LightgemSurface.GetEntity();
	idVec3 Cam = player->GetEyePosition();
	idVec3 Pos = player->GetPhysics()->GetOrigin();
	idVec3 LGPos = Pos; // Set the lightgem position to that of the player
	LGPos.x += ( Cam.x - Pos.x ) * 0.3; // Move the lightgem out a fraction along the leaning x vector
	LGPos.y += ( Cam.y - Pos.y ) * 0.3; // Move the lightgem out a fraction along the leaning y vector
	LGPos.z = Cam.z; // Set the lightgem's Z-axis position to that of the player's eyes

	if (LGPos.z < Pos.z + 50 && static_cast<idPhysics_Player*>(player->GetPlayerPhysics())->IsCrouching())
	{
		// Prevent lightgem from clipping into the floor while crouching
		LGPos.z = Pos.z + 50;
	}

	// Adjust the modelposition with userdefined offsets.
	// Move the lightgem testmodel to the players feet based on the eye position
	LGPos.x += cv_lg_oxoffs.GetInteger();
	LGPos.y += cv_lg_oyoffs.GetInteger();
	LGPos.z += cv_lg_ozoffs.GetInteger();
	lg->SetOrigin(LGPos);

	memset(&rv, 0, sizeof(rv));

	for(i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++)
		fColVal[i] = 0.0;

	for(i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
		rv.shaderParms[i] = gameLocal.globalShaderParms[i];

	rv.globalMaterial = gameLocal.GetGlobalMaterial();
	//	rv.width = SCREEN_WIDTH;
	//	rv.height = SCREEN_HEIGHT;
	rv.width = cv_lg_screen_width.GetInteger();
	rv.height = cv_lg_screen_height.GetInteger();
	rv.fov_x = cv_lg_fov.GetInteger();
	rv.fov_y = cv_lg_fov.GetInteger();		// Bigger values means more compressed view
	rv.forceUpdate = false;
	rv.x = 0;
	rv.y = 0;
	rv.time = gameLocal.GetTime();

	nRenderPasses = cv_lg_renderpasses.GetInteger();
	// limit the renderpasses between 1 and 4
	if(nRenderPasses < 1) nRenderPasses = 1;
	if(nRenderPasses > DARKMOD_LG_MAX_RENDERPASSES) nRenderPasses = DARKMOD_LG_MAX_RENDERPASSES;

	k = cv_lg_hud.GetInteger()-1;
	lgrend = lg->GetRenderEntity();

	// Set the viewid to our private screenshot snapshot. If this number is changed 
	// for some reason, it has to be changed in player.cpp as well.
	rv.viewID = DARKMOD_LG_VIEWID;
	lgrend->suppressShadowInViewID = 0;

	if(cv_lg_player.GetBool() == false)
		lgrend->allowSurfaceInViewID = rv.viewID;
	else
		lgrend->allowSurfaceInViewID = 0;

	// Tell the renderengine about the change for this entity.
	prent = lg->GetRenderEntity();
	if((pdef = lg->GetModelDefHandle()) != -1)
		gameRenderWorld->UpdateEntityDef(pdef, prent);

	prent = player->GetRenderEntity();
	hrent = player->GetHeadEntity()->GetRenderEntity();

	playerid = prent->suppressSurfaceInViewID;
	psid = prent->suppressShadowInViewID;
	prent->suppressShadowInViewID = rv.viewID;
	prent->suppressSurfaceInViewID = rv.viewID;

	headid = hrent->suppressSurfaceInViewID;
	hsid = hrent->suppressShadowInViewID;
	hrent->suppressShadowInViewID = rv.viewID;
	hrent->suppressSurfaceInViewID = rv.viewID;

	if((pdef = player->GetModelDefHandle()) != -1)
		gameRenderWorld->UpdateEntityDef(pdef, prent);

	if((hdef = player->GetHeadEntity()->GetModelDefHandle()) != -1)
		gameRenderWorld->UpdateEntityDef(hdef, hrent);

	dim = cv_lg_image_width.GetInteger();
	if(dim <= 0 || dim > 1024)
		dim = DARKMOD_LG_RENDER_WIDTH;

	fRetVal = 0.0;

// 	rv1 = rv;
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("RenderTurn %u", m_LightgemShotSpot);

	PROFILE_BLOCK_END( LightGem_Calculate_Setup);

	for(i = 0; i < nRenderPasses; i++)
	{
		PROFILE_BLOCK( LightGem_Calculate_ForLoop);

// 		rv = rv1;
		rv.vieworg = LGPos;

		// If it's not the turn for this lightgem shot, then
		// we skip over it. We also skip it if the splitting is disabled.
		if(cv_lg_split.GetBool() == true)
		{
			if(m_LightgemShotSpot != i)
				continue;
		}

		m_LightgemShotValue[i] = 0.0;

		PROFILE_BLOCK_START( LightGem_Calculate_ForLoop_switchCase );
		switch(i)
		{
		case 0:	// From the top to bottom
			{
				rv.vieworg.z += cv_lg_zoffs.GetInteger();
				rv.vieworg.z += dist;
				rv.viewaxis = idMat3(	
					0.0, 0.0, -1.0,
					0.0, 1.0, 0.0,
					1.0, 0.0, 0.0
					);
			}
			break;

		case 1:
			{
				// From bottom to top
				rv.vieworg.z -= cv_lg_zoffs.GetInteger();
				rv.vieworg.z -= dist;
				rv.viewaxis = idMat3(	
					0.0, 0.0, 1.0,
					0.0, 1.0, 0.0,
					-1.0, 0.0, 0.0
					);
			}
			break;
		}
		PROFILE_BLOCK_END( LightGem_Calculate_ForLoop_switchCase );

		// If the hud is enabled we either process all of them in case it is set to 0,
		// then we don't care which one is actually displayed (most likely the last or
		// the first one), or we only show the one that should be shown.
		if(k == -1 || k == i)
		{
			// We always use a square image, because we render now an overhead shot which
			// covers all four side of the player at once, using a diamond or pyramid shape.
			// The result is an image that is split in four triangles with an angle of 
			// 45 degree, thus the square shape.
			PROFILE_BLOCK_START	( LightGem_Calculate_ForLoop_RenderScene );
			renderSystem->CropRenderSize(dim, dim, true);
			gameRenderWorld->SetRenderView(&rv);
			gameRenderWorld->RenderScene(&rv);
			PROFILE_BLOCK_END	( LightGem_Calculate_ForLoop_RenderScene );

			PROFILE_BLOCK_START	( LightGem_Calculate_ForLoop_CaptureRenderToFile );
			renderSystem->CaptureRenderToFile(DARKMOD_LG_FILENAME);
			PROFILE_BLOCK_END	( LightGem_Calculate_ForLoop_CaptureRenderToFile );

			dp = cv_lg_path.GetString();
			if(dp != NULL && strlen(dp) != 0)
			{
				sprintf(dps, "%s_%u.tga", dp, i);
				dp = dps.c_str();
				renderSystem->CaptureRenderToFile(dp);
			}
			else
				dp = NULL;

			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Rendering to lightgem render buffer\n");
			renderSystem->UnCrop();

			PROFILE_BLOCK_START	( LightGem_Calculate_ForLoop_AnalyzeRenderImage );
			AnalyzeRenderImage(fColVal);
			PROFILE_BLOCK_END	( LightGem_Calculate_ForLoop_AnalyzeRenderImage );

			PROFILE_BLOCK_START	( LightGem_Calculate_ForLoop_Cleanup );

			// Check which of the images has the brightest value, and this is what we will use.
			for(l = 0; l < DARKMOD_LG_MAX_IMAGESPLIT; l++)
			{
				DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("fColVal[%u] = %f/%f\n", l, fColVal[l], m_LightgemShotValue[i]);
				if(fColVal[l] > m_LightgemShotValue[i])
					m_LightgemShotValue[i] = fColVal[l];
			}
			PROFILE_BLOCK_END	( LightGem_Calculate_ForLoop_Cleanup );
		}
	}

	PROFILE_BLOCK_START	( LightGem_Calculate_UnSetup );

	m_LightgemShotSpot++;
	if(m_LightgemShotSpot >= nRenderPasses)
		m_LightgemShotSpot = 0;

	for(i = 0; i < nRenderPasses; i++)
	{
		if(m_LightgemShotValue[i] > fRetVal)
			fRetVal = m_LightgemShotValue[i];
	}

	prent->suppressSurfaceInViewID = playerid;
	prent->suppressShadowInViewID = psid;
	hrent->suppressSurfaceInViewID = headid;
	hrent->suppressShadowInViewID = hsid;

	// and switch back our renderdefinition.
	if(pdef != -1)
		gameRenderWorld->UpdateEntityDef(pdef, prent);

	if(hdef != -1)
		gameRenderWorld->UpdateEntityDef(hdef, hrent);

	PROFILE_BLOCK_END	( LightGem_Calculate_UnSetup );

	return(fRetVal);
}

void LightGem::AnalyzeRenderImage(float fColVal[DARKMOD_LG_MAX_IMAGESPLIT])
{
	assert(NULL != m_LightgemRenderBuffer);

	CImage *im = &g_Global.m_RenderImage ;
	unsigned long counter[DARKMOD_LG_MAX_IMAGESPLIT];
	int i, in, k, kn, h, x;

	im->LoadImage(*m_LightgemRenderBuffer);
	unsigned char *buffer = im->GetImageData();

	// This is just an errorhandling to inform the player that something is wrong.
	// The lightgem will simply blink if the renderbuffer doesn't work.
	if(buffer == NULL)
	{
		static int indicator = 0;
		static int lasttime;
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to read image from lightgem render-buffer\r");
		for(i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++)
			fColVal[i] = indicator;

		if(gameLocal.time/1000 != lasttime)
		{
			lasttime = gameLocal.time/1000;
			indicator = !indicator;
		}

		goto Quit;
	}

	for(i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++)
		counter[i] = 0;

	// We always assume a BPP 4 here. We also always assume a square image with an even 
	// number of lines. An odd number might have only a very small influence though and
	// most likely get canceled out if a bigger image is used.
	kn = im->m_Height;
	h = kn/2;
	in = im->m_Width;

	// First we do the top half
	for(k = 0; k < h; k++)
	{
		for(i = 0; i < in; i++)
		{
			if(i < k)
				x = 0;
			else if(i > kn-k-1)
				x = 2;
			else
				x = 1;

			// The order is RGBA.
			fColVal[x] += ((buffer[0] * DARKMOD_LG_RED + buffer[1] * DARKMOD_LG_GREEN + buffer[2] * DARKMOD_LG_BLUE) * DARKMOD_LG_SCALE);
			counter[x]++;
			buffer += im->m_Bpp;
		}
	}

	// Then we do the bottom half where the triangles are inverted.
	for(k = (h-1); k >= 0; k--)
	{
		for(i = 0; i < in; i++)
		{
			if(i < k)
				x = 0;
			else if(i > kn-k-1)
				x = 2;
			else
				x = 3;

			// The order is RGBA.
			fColVal[x] += ((buffer[0] * DARKMOD_LG_RED + buffer[1] * DARKMOD_LG_GREEN + buffer[2] * DARKMOD_LG_BLUE) * DARKMOD_LG_SCALE);
			counter[x]++;
			buffer += im->m_Bpp;
		}
	}

	// Calculate the average for each value
	for(i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++)
		fColVal[i] = fColVal[i]/counter[i];

Quit:
	return;
}

float LightGem::Process(idPlayer *a_pPlayer)
{
	PROFILE_BLOCK(LightGem_Process);

	if( cv_lg_timeSlice.GetFloat() > 0.0f )
	{
		this->State_Setup(a_pPlayer);

		while( !m_bDoneProcessingThisFrame )
			(this->*m_pfnCurrentState)();

		this->State_Unsetup();

		return m_fRetVal;
	}
	else
	{
		m_fRetVal = this->Calculate(a_pPlayer);
		return m_fRetVal;
	}
}

void LightGem::State_Setup(idPlayer *a_pPlayer)
{
	PROFILE_BLOCK( LightGem_State_Setup );
	m_bDoneProcessingThisFrame = false;

	m_timer.Clear();

	m_timer.Start();

	// 	float m_fColVal[DARKMOD_LG_MAX_IMAGESPLIT];
	// 	float m_fRetVal = 0.0;
	// 	int m_nPlayerId;				// player viewid
	// 	int m_nHeadId;				// head viewid
	// 	int m_nHandlePlayerModel;			
	// 	int m_nHandleHeadModel;				
	// 	int m_nPlayerShadowId;				// player shadow viewid
	// 	int m_nHeadShadowId;				// head shadow viewid
	int i;
	// 	renderView_t m_renderViewLighgem1, m_renderViewLighgem2;
	idEntity *pEntityLG;
	// 	renderEntity_t *m_pRenderEntityPlayer;			// Player renderentity
	// 	renderEntity_t *m_pRenderEntityHead;			// Head renderentity
	renderEntity_t *pRenderEntityLG;

	pEntityLG = m_LightgemSurface.GetEntity();
	idVec3 vecCam = a_pPlayer->GetEyePosition();
	idVec3 vecPlayerPos = a_pPlayer->GetPhysics()->GetOrigin();
	idVec3 vecLGPos = vecPlayerPos; // Set the lightgem position to that of the player
	vecLGPos.x += ( vecCam.x - vecPlayerPos.x ) * 0.3; // Move the lightgem out a fraction along the leaning x vector
	vecLGPos.y += ( vecCam.y - vecPlayerPos.y ) * 0.3; // Move the lightgem out a fraction along the leaning y vector
	vecLGPos.z = vecCam.z; // Set the lightgem's Z-axis position to that of the player's eyes

	if (vecLGPos.z < vecPlayerPos.z + 50 && static_cast<idPhysics_Player*>(a_pPlayer->GetPlayerPhysics())->IsCrouching())
	{
		// Prevent lightgem from clipping into the floor while crouching
		vecLGPos.z = vecPlayerPos.z + 50;
	}

	// Adjust the modelposition with userdefined offsets.
	// Move the lightgem testmodel to the players feet based on the eye position
	vecLGPos.x += cv_lg_oxoffs.GetInteger();
	vecLGPos.y += cv_lg_oyoffs.GetInteger();
	vecLGPos.z += cv_lg_ozoffs.GetInteger();
	pEntityLG->SetOrigin(vecLGPos);

	memset(&m_renderViewLighgem, 0, sizeof(m_renderViewLighgem));

	for(i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++)
		m_fColVal[i] = 0.0;

	for(i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
		m_renderViewLighgem.shaderParms[i] = gameLocal.globalShaderParms[i];

	m_renderViewLighgem.globalMaterial = gameLocal.GetGlobalMaterial();
	//	rv.width = SCREEN_WIDTH;
	//	rv.height = SCREEN_HEIGHT;
	m_renderViewLighgem.width = cv_lg_screen_width.GetInteger();
	m_renderViewLighgem.height = cv_lg_screen_height.GetInteger();
	m_renderViewLighgem.fov_x = cv_lg_fov.GetInteger();
	m_renderViewLighgem.fov_y = cv_lg_fov.GetInteger();		// Bigger values means more compressed view
	m_renderViewLighgem.forceUpdate = false;
	m_renderViewLighgem.x = 0;
	m_renderViewLighgem.y = 0;
	m_renderViewLighgem.time = gameLocal.GetTime();

	// Note to self- Move this to another state. - J.C.
	// 	k = cv_lg_hud.GetInteger()-1;
	pRenderEntityLG = pEntityLG->GetRenderEntity();

	// Set the viewid to our private screenshot snapshot. If this number is changed 
	// for some reason, it has to be changed in player.cpp as well.
	m_renderViewLighgem.viewID = DARKMOD_LG_VIEWID;
	// 	renderEntity_t *m_pRenderEntityPlayer;			// Player renderentity
	// 	renderEntity_t *m_pRenderEntityHead;			// Head renderentity->suppressShadowInViewID = 0;

	if(cv_lg_player.GetBool() == false)
		pRenderEntityLG->allowSurfaceInViewID = m_renderViewLighgem.viewID;
	else
		pRenderEntityLG->allowSurfaceInViewID = 0;

	// Tell the renderengine about the change for this entity.
	m_pRenderEntityPlayer = pEntityLG->GetRenderEntity();
	if((m_nHandlePlayerModel = pEntityLG->GetModelDefHandle()) != -1)
		gameRenderWorld->UpdateEntityDef(m_nHandlePlayerModel, m_pRenderEntityPlayer );

	m_pRenderEntityPlayer	= a_pPlayer->GetRenderEntity();
	m_pRenderEntityHead		= a_pPlayer->GetHeadEntity()->GetRenderEntity();

	m_nPlayerId										= m_pRenderEntityPlayer->suppressSurfaceInViewID;
	m_nPlayerShadowId								= m_pRenderEntityPlayer->suppressShadowInViewID;
	m_pRenderEntityPlayer->suppressShadowInViewID	= m_renderViewLighgem.viewID;
	m_pRenderEntityPlayer->suppressSurfaceInViewID	= m_renderViewLighgem.viewID;

	m_nHeadId										= m_pRenderEntityHead->suppressSurfaceInViewID;
	m_nHeadShadowId									= m_pRenderEntityHead->suppressShadowInViewID;
	m_pRenderEntityHead->suppressShadowInViewID		= m_renderViewLighgem.viewID;
	m_pRenderEntityHead->suppressSurfaceInViewID	= m_renderViewLighgem.viewID;

	if((m_nHandlePlayerModel = a_pPlayer->GetModelDefHandle()) != -1)
		gameRenderWorld->UpdateEntityDef(m_nHandlePlayerModel, m_pRenderEntityPlayer);

	if((m_nHandleHeadModel = a_pPlayer->GetHeadEntity()->GetModelDefHandle()) != -1)
		gameRenderWorld->UpdateEntityDef(m_nHandleHeadModel, m_pRenderEntityHead);

	// 	dim = cv_lg_image_width.GetInteger();
	// 	if(dim <= 0 || dim > 1024)
	// 		dim = DARKMOD_LG_RENDER_WIDTH;

	m_renderViewLighgem.vieworg = vecLGPos;
};

void LightGem::State_Unsetup()
{
	PROFILE_BLOCK( LightGem_State_Unsetup );

	m_pRenderEntityPlayer->suppressSurfaceInViewID	= m_nPlayerId;
	m_pRenderEntityPlayer->suppressShadowInViewID	= m_nPlayerShadowId;
	m_pRenderEntityHead->suppressSurfaceInViewID	= m_nHeadId;
	m_pRenderEntityHead->suppressShadowInViewID		= m_nHeadShadowId;

	// and switch back our renderdefinition.
	if(m_nHandlePlayerModel != -1)
		gameRenderWorld->UpdateEntityDef(m_nHandlePlayerModel, m_pRenderEntityPlayer);

	if(m_nHandleHeadModel != -1)
		gameRenderWorld->UpdateEntityDef(m_nHandleHeadModel, m_pRenderEntityHead);
};

void LightGem::State_RenderLightGem()
{
	PROFILE_BLOCK( LightGem_State_RenderLightGem );

	float fDist = cv_lg_distance.GetFloat();			// reasonable distance to get a good look at the player/test model
	int	k = cv_lg_hud.GetInteger()-1;
	int	dim = cv_lg_image_width.GetInteger();

	if(dim <= 0 || dim > 1024)
 		dim = DARKMOD_LG_RENDER_WIDTH;


// 	// If it's not the turn for this lightgem shot, then
// 	// we skip over it. We also skip it if the splitting is disabled.
// 	if(cv_lg_split.GetBool() == true)
// 	{
// 		if(m_LightgemShotSpot != i)
// 			return;
// 	}

	m_LightgemShotValue[m_iRenderPass] = 0.0;

	switch(m_iRenderPass)
	{
	case 0:	// From the top to bottom
		{
			m_renderViewLighgem.vieworg.z += cv_lg_zoffs.GetInteger();
			m_renderViewLighgem.vieworg.z += fDist;
			m_renderViewLighgem.viewaxis = idMat3(	
				0.0, 0.0, -1.0,
				0.0, 1.0, 0.0,
				1.0, 0.0, 0.0
				);
		}
		break;

	case 1:
		{
			// From bottom to top
			m_renderViewLighgem.vieworg.z -= cv_lg_zoffs.GetInteger();
			m_renderViewLighgem.vieworg.z -= fDist;
			m_renderViewLighgem.viewaxis = idMat3(	
				0.0, 0.0, 1.0,
				0.0, 1.0, 0.0,
				-1.0, 0.0, 0.0
				);
		}
		break;
	}

	// If the hud is enabled we either process all of them in case it is set to 0,
	// then we don't care which one is actually displayed (most likely the last or
	// the first one), or we only show the one that should be shown.
	if( k == -1 || k == m_iRenderPass )
	{

		// We always use a square image, because we render now an overhead shot which
		// covers all four side of the player at once, using a diamond or pyramid shape.
		// The result is an image that is split in four triangles with an angle of 
		// 45 degree, thus the square shape.
		renderSystem->CropRenderSize(dim, dim, true);
		gameRenderWorld->SetRenderView(&m_renderViewLighgem);
		gameRenderWorld->RenderScene(&m_renderViewLighgem);
	
		renderSystem->UnCrop();
	}

	// If the updated state is not going to be run this frame then we need to store our result.
	UpdateCurrentState( &LightGem::State_CaptureRenderToBuffer );
	m_bCaptureToGPUTexture = m_bDoneProcessingThisFrame;

	if( m_bCaptureToGPUTexture )
	{
		PROFILE_BLOCK_START( LightGem_State_RenderLightGem_CaptureToGPUTexture );
		renderSystem->CropRenderSize(dim, dim, true);	
		renderSystem->CaptureRenderToImage( "_lightGem" );
		renderSystem->UnCrop();
		PROFILE_BLOCK_END( LightGem_State_RenderLightGem_CaptureToGPUTexture );
	}
}

void LightGem::State_CaptureRenderToBuffer()
{
	PROFILE_BLOCK( LightGem_State_CaptureRenderToBuffer );

	int	k = cv_lg_hud.GetInteger()-1;

	if( k == -1 || k == m_iRenderPass )
	{

		int	dim = cv_lg_image_width.GetInteger();

		if(dim <= 0 || dim > 1024)
			dim = DARKMOD_LG_RENDER_WIDTH;

		renderSystem->CropRenderSize(dim, dim, true);

		PROFILE_BLOCK_START( LightGem_State_CaptureRenderToBuffer_DrawGPUTexture );
		if( m_bCaptureToGPUTexture )
			renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 1, 1, 0, declManager->FindMaterial("_lightGem") );
		PROFILE_BLOCK_END( LightGem_State_CaptureRenderToBuffer_DrawGPUTexture );
			
		PROFILE_BLOCK_START( LightGem_State_CaptureRenderToBuffer_capture );
		renderSystem->CaptureRenderToFile(DARKMOD_LG_FILENAME);
		PROFILE_BLOCK_END( LightGem_State_CaptureRenderToBuffer_capture );

		idStr strPath( cv_lg_path.GetString() );
		if(strPath.Length() > 0)
		{
			sprintf( strPath, "%s_%u.tga", strPath, m_iRenderPass );
			renderSystem->CaptureRenderToFile(strPath);
		}

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Rendering to lightgem render buffer \n");
		renderSystem->UnCrop();
	}

	this->UpdateCurrentState( &LightGem::State_AnalyzeLGImage );

}

void LightGem::State_AnalyzeLGImage(void)
{
	PROFILE_BLOCK( LightGem_State_AnalyzeLGImage );

	int	k = cv_lg_hud.GetInteger()-1;
	if( k == -1 || k == m_iRenderPass )
	{
		AnalyzeRenderImage(m_fColVal);

		// Check which of the images has the brightest value, and this is what we will use.
		for(int l = 0; l < DARKMOD_LG_MAX_IMAGESPLIT; l++)
		{
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("fColVal[%u] = %f/%f\n", l, m_fColVal[l], m_LightgemShotValue[m_iRenderPass]);
			if(m_fColVal[l] > m_LightgemShotValue[m_iRenderPass])
				m_LightgemShotValue[m_iRenderPass] = m_fColVal[l];
		}
	}

	//Limit render-passes between 1 - DARKMOD_LG_MAX_RENDERPASSES
	int nRenderPasses = Min( Max( cv_lg_renderpasses.GetInteger(),  1), DARKMOD_LG_MAX_RENDERPASSES );

	if( ++m_iRenderPass >= nRenderPasses )
	{
		m_fRetVal = m_LightgemShotValue[0];
		for(int i = 1; i < nRenderPasses; i++)
		{
			if(m_LightgemShotValue[i] > m_fRetVal )
				m_fRetVal = m_LightgemShotValue[i];
		}

		gameLocal.Printf(" Rendering and analyzing of two lightgem images complete. \n");
		m_bDoneProcessingThisFrame = true;
		m_iRenderPass = 0;
	}

	this->UpdateCurrentState( &LightGem::State_RenderLightGem );
}

void LightGem::UpdateCurrentState( LightGemState a_newState )
{
	if( !m_bDoneProcessingThisFrame )
	{
		double fTimeSlice = (double)cv_lg_timeSlice.GetFloat();

		m_timer.Stop();
		double	fProcessingTime = m_timer.Milliseconds(); 

		if( fProcessingTime > fTimeSlice )
			m_bDoneProcessingThisFrame = true;
		else
			m_timer.Start();
	}
	m_pfnCurrentState = a_newState;
}
