/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "LightGem.h"
#include "Grabber.h"
#include "../renderer/tr_local.h"
#include "../renderer/FrameBuffer.h"

// Temporary profiling related macros

//#define ENABLE_PROFILING

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
LightGem::LightGem()
{
/*	for( int i = 0; i < 2; ++i ) {
		m_LightgemDrawCmds[i].imageBuffer = (byte*)Mem_Alloc16(DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_BPP * sizeof(ILuint));
		m_LightgemDrawCmds[i].cmdHead = m_LightgemDrawCmds[i].cmdTail = nullptr;
	}
	m_LightgemFrontDraw = &m_LightgemDrawCmds[0];
	m_LightgemBackDraw = &m_LightgemDrawCmds[1];*/
	m_LightgemImgBuffer = (byte*)Mem_Alloc16( DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_BPP * sizeof( ILuint ) );
}

LightGem::~LightGem()
{
	/*for( int i = 0; i < 2; ++i ) {
		Mem_Free16( m_LightgemDrawCmds[i].imageBuffer );
	}*/
	Mem_Free16( m_LightgemImgBuffer );
}


//----------------------------------------------------
// Initialization
//----------------------------------------------------
void LightGem::Clear()
{
	m_LightgemSurface = NULL;
	m_LightgemShotSpot = 0;

	memset(m_LightgemShotValue, 0, sizeof(m_LightgemShotValue));
}

void LightGem::SpawnLightGemEntity( idMapFile *	a_mapFile )
{
	static const char *LightgemName = DARKMOD_LG_ENTITY_NAME;
	idMapEntity *mapEnt = a_mapFile->FindEntity(LightgemName);

	if ( mapEnt == NULL ) {
		mapEnt = new idMapEntity();
		a_mapFile->AddEntity(mapEnt);
		mapEnt->epairs.Set("classname", "func_static");
		mapEnt->epairs.Set("name", LightgemName);
		if ( strlen(cv_lg_model.GetString()) == 0 ) {
			mapEnt->epairs.Set("model", DARKMOD_LG_RENDER_MODEL);
		} else {
			mapEnt->epairs.Set("model", cv_lg_model.GetString());
		}
		mapEnt->epairs.Set("origin", "0 0 0");
		mapEnt->epairs.Set("noclipmodel", "1");
	}

	/*m_Lightgem_rv.viewID = VID_LIGHTGEM;
	
	m_Lightgem_rv.width = SCREEN_WIDTH;
	m_Lightgem_rv.height = SCREEN_HEIGHT;
	m_Lightgem_rv.fov_x = m_Lightgem_rv.fov_y = DARKMOD_LG_RENDER_FOV;	// square, TODO: investigate lowering the value to increase performance on tall maps
	m_Lightgem_rv.x = m_Lightgem_rv.y = 0;

	m_Lightgem_rv.viewaxis = idMat3(	
		 0.0f, 0.0f, 1.0f,
		 0.0f, 1.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	);*/

	//m_Lightgem_rv.forceUpdate = false;
	//m_Lightgem_rv.cramZNear = false;			// Needs testing
}

void LightGem::InitializeLightGemEntity( void )
{
	m_LightgemSurface = gameLocal.FindEntity(DARKMOD_LG_ENTITY_NAME);
	m_LightgemSurface.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = VID_LIGHTGEM;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noDynamicInteractions = false;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noShadow = true;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noSelfShadow = true;
	//nbohr1more: #4379 lightgem culling
	m_LightgemSurface.GetEntity()->GetRenderEntity()->isLightgem = true;

	DM_LOG(LC_LIGHT, LT_INFO)LOGSTRING("LightgemSurface: [%08lX]\r", m_LightgemSurface.GetEntity());
}

//----------------------------------------------------
// State Persistence 
//----------------------------------------------------

void LightGem::Save( idSaveGame & a_saveGame )
{
	m_LightgemSurface.Save( &a_saveGame );
	a_saveGame.WriteInt(m_LightgemShotSpot);
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++) {
		a_saveGame.WriteFloat(m_LightgemShotValue[i]);
	}
}

void LightGem::Restore( idRestoreGame & a_savedGame )
{
	m_LightgemSurface.Restore( &a_savedGame );
	a_savedGame.ReadInt(m_LightgemShotSpot);
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++) {
		a_savedGame.ReadFloat(m_LightgemShotValue[i]);
	}

	m_LightgemSurface.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = VID_LIGHTGEM;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noDynamicInteractions = false;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noShadow = true;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noSelfShadow = true;
	//nbohr1more: #4379 lightgem culling
	m_LightgemSurface.GetEntity()->GetRenderEntity()->isLightgem = true;

	DM_LOG(LC_LIGHT, LT_INFO)LOGSTRING("LightgemSurface: [%08lX]\r", m_LightgemSurface.GetEntity());
}

//----------------------------------------------------
// Calculation
//----------------------------------------------------

float LightGem::Calculate(idPlayer *player)
{
	PROFILE_BLOCK( LightGem_Calculate );

	PROFILE_BLOCK_START(LightGem_Calculate_AnalyzeRenderImage);
	// analyze rendered shot from previous frame
	AnalyzeRenderImage();
	m_LightgemShotValue[m_LightgemShotSpot] = 0.0f;
	// Check which of the images has the brightest value, and this is what we will use.
	for (int l = 0; l < DARKMOD_LG_MAX_IMAGESPLIT; l++) {
		if (m_fColVal[l] > m_LightgemShotValue[m_LightgemShotSpot]) {
			m_LightgemShotValue[m_LightgemShotSpot] = m_fColVal[l];
		}
	}
	PROFILE_BLOCK_END(LightGem_Calculate_AnalyzeRenderImage);

	PROFILE_BLOCK_START( LightGem_Calculate_Setup);
	// If player is hidden (i.e the whole player entity is actually hidden)
	if ( player->GetModelDefHandle() == -1 ) {
		return 0.0f;
	}
	
	// Get position for lg
	idEntity* lg = m_LightgemSurface.GetEntity();
	// duzenko #4408 - this happens at map start if no game tics ran in background yet
	if (lg->GetModelDefHandle() == -1) 
		return 0.0f;
	renderEntity_t* lgent = lg->GetRenderEntity();

	const idVec3& Cam = player->GetEyePosition();
	idVec3 LGPos = player->GetPhysics()->GetOrigin();// Set the lightgem position to that of the player

	LGPos.x += (Cam.x - LGPos.x) * 0.3f + cv_lg_oxoffs.GetFloat(); // Move the lightgem out a fraction along the leaning x vector
	LGPos.y += (Cam.y - LGPos.y) * 0.3f + cv_lg_oyoffs.GetFloat(); // Move the lightgem out a fraction along the leaning y vector
	
	// Prevent lightgem from clipping into the floor while crouching
	if ( static_cast<idPhysics_Player*>(player->GetPlayerPhysics())->IsCrouching() ) {
		LGPos.z += 50.0f + cv_lg_ozoffs.GetFloat() ;
	} else {
		LGPos.z = Cam.z + cv_lg_ozoffs.GetFloat(); // Set the lightgem's Z-axis position to that of the player's eyes
	}

	/*m_Lightgem_rv.vieworg = LGPos;
	lg->SetOrigin(LGPos); // Move the lightgem testmodel to the players feet based on the eye position

	gameRenderWorld->UpdateEntityDef(lg->GetModelDefHandle(), lgent); // Make sure the lg is in the updated position

	// Give the rv the current ambient light values - Not all of the other values, avoiding fancy effects.
	m_Lightgem_rv.shaderParms[2] = gameLocal.globalShaderParms[2]; // Ambient R
	m_Lightgem_rv.shaderParms[3] = gameLocal.globalShaderParms[3]; // Ambient G
	m_Lightgem_rv.shaderParms[4] = gameLocal.globalShaderParms[4]; // Ambient B

	// angua: render view needs current time, otherwise it will be unable to see time-dependent changes in light shaders such as flickering torches
	m_Lightgem_rv.time = gameLocal.GetTime();

	// Make sure the player model is hidden in the lightgem renders
	renderEntity_t* prent = player->GetRenderEntity();
	const int pdef = player->GetModelDefHandle();
	const int playerid = prent->suppressSurfaceInViewID;
	const int psid = prent->suppressShadowInViewID;
	prent->suppressShadowInViewID = VID_LIGHTGEM;
	prent->suppressSurfaceInViewID = VID_LIGHTGEM;

	// And the player's head 
	renderEntity_t* hrent = player->GetHeadEntity()->GetRenderEntity();
	const int hdef = player->GetHeadEntity()->GetModelDefHandle();
	const int headid = hrent->suppressSurfaceInViewID;
	const int hsid = hrent->suppressShadowInViewID;
	hrent->suppressShadowInViewID = VID_LIGHTGEM;
	hrent->suppressSurfaceInViewID = VID_LIGHTGEM;

	// Let the game know about the changes
	gameRenderWorld->UpdateEntityDef(pdef, prent); 
	gameRenderWorld->UpdateEntityDef(hdef, hrent);

	// Currently grabbed entities should not cast a shadow on the lightgem to avoid exploits
	int heldDef = 0;
	int heldSurfID = 0;
	int heldShadID = 0;
	renderEntity_t *heldRE = NULL;
	idEntity *heldEnt = gameLocal.m_Grabber->GetSelected();
	if ( heldEnt ) {
		heldDef = heldEnt->GetModelDefHandle();

		// tels: #3286: Only update the entityDef if it is valid
		if (heldDef >= 0)
		{
			heldRE = heldEnt->GetRenderEntity();
			heldSurfID = heldRE->suppressSurfaceInViewID;
			heldShadID = heldRE->suppressShadowInViewID;
			heldRE->suppressShadowInViewID = VID_LIGHTGEM;
			heldRE->suppressSurfaceInViewID = VID_LIGHTGEM;
			gameRenderWorld->UpdateEntityDef( heldDef, heldRE );
		}
	}

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("RenderTurn %u", m_LightgemShotSpot);
	PROFILE_BLOCK_END( LightGem_Calculate_Setup);

	// Render up and down alternately 
	m_Lightgem_rv.viewaxis.TransposeSelf();
		
	// We always use a square image, because we render now an overhead shot which
	// covers all four side of the player at once, using a diamond or pyramid shape.
	// The result is an image that is split in four triangles with an angle of 
	// 45 degree, thus the square shape.
	PROFILE_BLOCK_START( LightGem_Calculate_RenderScene );
	emptyCommand_t *bkpCmdHead = frameData->cmdHead;
	emptyCommand_t *bkpCmdTail = frameData->cmdTail;
	R_ClearCommandChain( frameData );
	renderSystem->CropRenderSize(DARKMOD_LG_RENDER_WIDTH, DARKMOD_LG_RENDER_WIDTH, true, true);
	gameRenderWorld->SetRenderView(&m_Lightgem_rv); // most likely not needed
	gameRenderWorld->RenderScene(m_Lightgem_rv);
	AddRenderToBufferCommand();
	renderSystem->UnCrop();
	m_LightgemFrontDraw->cmdHead = frameData->cmdHead;
	m_LightgemFrontDraw->cmdTail = frameData->cmdTail;
	frameData->cmdHead = bkpCmdHead;
	frameData->cmdTail = bkpCmdTail;
	PROFILE_BLOCK_END( LightGem_Calculate_RenderScene );

	PROFILE_BLOCK_START	( LightGem_Calculate_UnSetup );
	// and switch back our normal render definition - player model and head are returned
	prent->suppressSurfaceInViewID = playerid;
	prent->suppressShadowInViewID = psid;
	hrent->suppressSurfaceInViewID = headid;
	hrent->suppressShadowInViewID = hsid;
	gameRenderWorld->UpdateEntityDef(pdef, prent);
	gameRenderWorld->UpdateEntityDef(hdef, hrent);

	// switch back currently grabbed entity settings
	if( heldEnt ) {
		// tels: #3286: Only update the entityDef if it is valid
		if (heldDef >= 0)
		{
			heldRE->suppressSurfaceInViewID = heldSurfID;
			heldRE->suppressShadowInViewID = heldShadID;
			gameRenderWorld->UpdateEntityDef( heldDef, heldRE );
		}
	}
	PROFILE_BLOCK_END(LightGem_Calculate_UnSetup);*/

	m_LightgemShotSpot = (m_LightgemShotSpot + 1) % DARKMOD_LG_MAX_RENDERPASSES;

	// we want to return the highest shot value
	float fRetVal = 0.0f;
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++) {
		if ( m_LightgemShotValue[i] > fRetVal ) {
			fRetVal = m_LightgemShotValue[i];
		}
	}
	return fRetVal;
}

//void R_IssueRenderCommands( frameData_t *frameData );

/*void LightGem::Render() {
	if (m_LightgemBackDraw->cmdHead == nullptr)
		return;

	PROFILE_BLOCK_START(LightGem_Render);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Rendering to lightgem render buffer\n");
	
	// switch in our own lightgem draw commands
	emptyCommand_t *bkpCmdHead = backendFrameData->cmdHead;
	emptyCommand_t *bkpCmdTail = backendFrameData->cmdTail;
	backendFrameData->cmdHead = m_LightgemBackDraw->cmdHead;
	backendFrameData->cmdTail = m_LightgemBackDraw->cmdTail;
	R_IssueRenderCommands(backendFrameData);
	backendFrameData->cmdHead = bkpCmdHead;
	backendFrameData->cmdTail = bkpCmdTail;

	m_LightgemBackDraw->cmdHead = m_LightgemBackDraw->cmdTail = nullptr;

	PROFILE_BLOCK_END(LightGem_Render);
}*/

/*void LightGem::EndFrame() {
	std::swap( m_LightgemFrontDraw, m_LightgemBackDraw );
}*/

void LightGem::AnalyzeRenderImage()
{
	//const byte *buffer = m_LightgemFrontDraw->imageBuffer;
	const byte *buffer = m_LightgemImgBuffer;
	
	// The lightgem will simply blink if the renderbuffer doesn't work.
	if ( buffer == nullptr ) {
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to read image from lightgem render-buffer\r");

		for ( int i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++ ) {
			m_fColVal[i] = (gameLocal.time % 1024 ) > 512;
		}

		return;
	}
	
	/* 	Split up the image into the 4 triangles

		 \11/	0 - east of lightgem render
		3 \/ 0	1 - north of lg
		3 /\ 0	2 - south of lg
		 /22\	3 - west of lg
	
		Note : Serp - This is a simplification of the early version which used two nested loops
	*/

	int in = 0;

	for ( int x = 0; x < DARKMOD_LG_RENDER_WIDTH; x++ ) {
		for ( int y = 0; y < DARKMOD_LG_RENDER_WIDTH; y++, buffer += DARKMOD_LG_BPP ) { // increment the buffer pos
			if ( y <= x && x + y >= (DARKMOD_LG_RENDER_WIDTH -1) ) {
				in = 0;
			} else if ( y < x ) {
				in = 1;
			} else if ( y > (DARKMOD_LG_RENDER_WIDTH -1) - x ) {
				in = 2;
			} else {
				in = 3;
			}

			// The order is RGB. 
			// #4395 Duzenko lightem pixel pack buffer optimization
			m_fColVal[in] += buffer[0] * DARKMOD_LG_RED +
							 buffer[1] * DARKMOD_LG_GREEN +
							 buffer[2] * DARKMOD_LG_BLUE;
		}
	}

	// Calculate the average for each value
	// Could be moved to the return
	// #4395 Duzenko lightem pixel pack buffer optimization
	m_fColVal[0] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
	m_fColVal[1] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
	m_fColVal[2] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
	m_fColVal[3] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
}

/*void LightGem::AddRenderToBufferCommand() {
	int width, height;
	renderSystem->GetCurrentRenderCropSize( width, height );
	width = (width + 3) & ~3; //opengl wants width padded to 4x

	copyRenderCommand_t &cmd = *(copyRenderCommand_t *)R_GetCommandBuffer(sizeof(cmd));
	cmd.commandId = RC_COPY_RENDER;
	cmd.buffer = m_LightgemFrontDraw->imageBuffer;
	cmd.usePBO = true;
	cmd.image = NULL;
	cmd.x = 0;
	cmd.y = 0;
	cmd.imageWidth = width;
	cmd.imageHeight = height;
}*/