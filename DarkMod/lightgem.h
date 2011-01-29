#ifndef __LIGHTGEM_H__
#define __LIGHTGEM_H__

//----------------------------------
// Preprocessor Constants
//----------------------------------
// Number of passes that we can do at most. This is 6 because it's simply a cube that is rendered 
// from all sides. This is not needed though, because a top and a bottom render with a pyramidic
// shape would be sufficient to cover all lighting situations. For silouhette detection we might
// consider more stages though.
#define DARKMOD_LG_MAX_RENDERPASSES			2
#define DARKMOD_LG_MAX_IMAGESPLIT			4
#define DARKMOD_LG_RENDER_MODEL				"models/darkmod/misc/system/lightgem.lwo"
#define DARKMOD_LG_ENTITY_NAME				"lightgem_surface"
// The lightgem viewid defines the viewid that is to be used for the lightgem surfacetestmodel
#define DARKMOD_LG_VIEWID					-1
#define DARKMOD_LG_RENDER_WIDTH				50

#define DARKMOD_LG_FILENAME					"DM_lightgem_buffer"
// The colour is converted to a grayscale value which determines the state
// of the lightgem.
// LightGem = (0.29900*R+0.58700*G+0.11400*B) * 0.0625

#define DARKMOD_LG_MIN						1
#define DARKMOD_LG_MAX						32
#define DARKMOD_LG_FRACTION					(1.0f/32.0f)
#define DARKMOD_LG_RED						0.29900f
#define DARKMOD_LG_GREEN					0.58700f
#define DARKMOD_LG_BLUE						0.11400f
#define DARKMOD_LG_SCALE					(1.0/255.0)			// scaling factor for grayscale value

//----------------------------------
// Forward Declarations.
//----------------------------------
class LightGem;

//----------------------------------
// Type Definitions.
//----------------------------------

typedef void (LightGem::*LightGemState)	(void);

//----------------------------------
// Class Declarations.
//----------------------------------

class LightGem
{
private:
	// stgatilov: The buffer for captured lightgem image
	idList<char> *			m_LightgemRenderBuffer;

	int						m_LightgemShotSpot;
	float					m_LightgemShotValue[DARKMOD_LG_MAX_RENDERPASSES];
	idEntityPtr<idEntity>	m_LightgemSurface;
	idTimer					m_timer;

	LightGemState			m_pfnCurrentState;
	bool					m_bCaptureToGPUTexture;
	bool					m_bDoneProcessingThisFrame;

// 	enum eLightGemProcessStates
// 	{
// 		eLightGemProcessStates_NotStarted,
// 		eLightGemProcessStates_Setup,
// 		eLightGemProcessStates_RenderScene1,
// 		eLightGemProcessStates_RenderScene2,
// 		eLightGemProcessStates_CaptureRenderToFile1,
// 		eLightGemProcessStates_CaptureRenderToFile2,
// 		eLightGemProcessStates_AnalyzeRenderedImage1,
// 		eLightGemProcessStates_AnalyzeRenderedImage2,
// 		eLightGemProcessStates_RevertRenderSettings,
// 		eLightGemProcessStates_NumStates
// 	};

// 	eLightGemProcessStates m_eCurrentProcessingState;

	//----------------------------------
	// Frame Persistent variables
	//----------------------------------
	float m_fRetVal;
	float m_fColVal[DARKMOD_LG_MAX_IMAGESPLIT];

	int m_nHandlePlayerModel;			
	int m_nHandleHeadModel;		
	int m_nPlayerId;						// player viewid
	int m_nHeadId;							// head viewid
	int m_nPlayerShadowId;				// player shadow viewid
	int m_nHeadShadowId;				// head shadow viewid
	int m_iRenderPass;

	renderEntity_t *m_pRenderEntityPlayer;	
	renderEntity_t *m_pRenderEntityHead;

	renderView_t m_renderViewLighgem;


	//----------------------------------

public:
	//---------------------------------
	// Construction/Destruction
	//---------------------------------
	LightGem	();
	~LightGem	();

	//---------------------------------
	// Initiazation
	//---------------------------------
	void Initialize		();
	void Deinitialize	();
	void Clear			();

	//---------------------------------
	// Persistence
	//---------------------------------
	void Save			( idSaveGame &		a_saveGame );
	void Restore		( idRestoreGame &	a_savedGame );
	//---------------------------------
	
	//---------------------------------
	// SpawnlightgemEntity will create exactly one lightgem entity for the map and ensures
	//  that no multiple copies of it will exist.
	//---------------------------------
	void SpawnLightGemEntity( idMapFile *	a_mapFile );

	void InitializeLightGemEntity(); 

	float	Calculate		( idPlayer *	a_pPlayer );
	float	Process			( idPlayer *	a_pPlayer );

	ID_INLINE float				GetResult				( void ) const { return m_fRetVal; }
	ID_INLINE idList<char> &	GetLightgemRenderBuffer ( void ) const { return *m_LightgemRenderBuffer; }

private:
	void AnalyzeRenderImage	(float fColVal[DARKMOD_LG_MAX_IMAGESPLIT]);
	void Calculate_new		(float fColVal[DARKMOD_LG_MAX_IMAGESPLIT]);

	void State_Setup				(idPlayer *a_pPlayer);
	void State_Unsetup				(void);

	void State_RenderLightGem			(void);
	void State_CaptureRenderToBuffer	(void);
	void State_AnalyzeLGImage			(void);

	void UpdateCurrentState			(LightGemState a_newState);		// Returns true if the new state is supposed to be executed the same frame.

	//bool UpdateState				(void);


};

#endif // __LIGHTGEM_H__