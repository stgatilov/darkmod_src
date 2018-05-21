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

#ifndef __SESSIONLOCAL_H__
#define __SESSIONLOCAL_H__
#include <thread>
#include <condition_variable>
#include <memory>

/*

IsConnectedToServer();
IsGameLoaded();
IsGuiActive();
IsPlayingRenderDemo();

if connected to a server
	if handshaking
	if map loading
	if in game
else if a game loaded
	if in load game menu
	if main menu up
else if playing render demo
else
	if error dialog
	full console

*/

typedef struct {
	usercmd_t	cmd;
	int			consistencyHash;
} logCmd_t;

struct fileTIME_T {
	int				index;
	ID_TIME_T			timeStamp;

					operator int() const { return timeStamp; }
};

#ifndef MULTIPLAYER
#define MAX_ASYNC_CLIENTS 1
#endif

typedef struct {
	idDict			serverInfo;
	idDict			syncedCVars;
	idDict			userInfo[MAX_ASYNC_CLIENTS];
	idDict			persistentPlayerInfo[MAX_ASYNC_CLIENTS];
	usercmd_t		mapSpawnUsercmd[MAX_ASYNC_CLIENTS];		// needed for tracking delta angles
} mapSpawnData_t;

typedef enum {
	TD_NO,
	TD_YES,
	TD_YES_THEN_QUIT
} timeDemo_t;

const int USERCMD_PER_DEMO_FRAME	= 2;
const int CONNECT_TRANSMIT_TIME		= 1000;
const int MAX_LOGGED_USERCMDS		= 60*60*60;	// one hour of single player, 15 minutes of four player

class idSessionLocal : public idSession {
public:

						idSessionLocal();
	virtual				~idSessionLocal();

	virtual void		Init();

	virtual void		Shutdown();

	virtual void		Stop();
	virtual void		TerminateFrontendThread();

	virtual void		UpdateScreen( bool outOfSequence = true );

	virtual void		PacifierUpdate(loadkey_t key, int count); // grayman #3763

	virtual void		Frame();

	virtual bool		IsMultiplayer();

	virtual bool		ProcessEvent( const sysEvent_t *event );

	virtual void		StartMenu( bool playIntro = false );
	virtual void		ExitMenu();
	virtual void		GuiFrameEvents();
	virtual void		SetGUI( idUserInterface *gui, HandleGuiCommand_t handle );

	virtual const char *MessageBox( msgBoxType_t type, const char *message, const char *title = NULL, bool wait = false, const char *fire_yes = NULL, const char *fire_no = NULL, bool network = false  );
	virtual void		StopBox( void );
	virtual void		DownloadProgressBox( backgroundDownload_t *bgl, const char *title, int progress_start = 0, int progress_end = 100 );
	virtual void		SetPlayingSoundWorld();

	virtual void		TimeHitch( int msec );

	virtual int			GetSaveGameVersion( void );
    
	virtual void		RunGameTic();
	virtual void		ActivateFrontend();
	virtual void		WaitForFrontendCompletion();
	virtual void		StartFrontendThread();

	virtual const char *GetCurrentMapName();

	//=====================================

	int					GetLocalClientNum();

	void				MoveToNewMap( const char *mapName );

	// loads a map and starts a new game on it
	void				StartNewGame( const char *mapName, bool devmap = false );
	void				PlayIntroGui();

	void				LoadSession( const char *name );
	void				SaveSession( const char *name );

	// called by Draw when the scene to scene wipe is still running
	void				DrawWipeModel();
	void				StartWipe( const char *materialName, bool hold = false);
	void				CompleteWipe();
	void				ClearWipe();

	void				ShowLoadingGui();

	void				ScrubSaveGameFileName( idStr &saveFileName ) const;
	idStr				GetAutoSaveName( const char *mapName ) const;

	bool				LoadGame(const char *saveName);
	bool				SaveGame(const char *saveName, bool autosave = false, bool skipCheck = false);

	//=====================================

	static idCVar		com_showAngles;
	static idCVar		com_showTics;
	static idCVar		com_minTics;
	static idCVar		com_fixedTic;
	static idCVar		com_showDemo;
	static idCVar		com_skipGameDraw;
	static idCVar		com_aviDemoWidth;
	static idCVar		com_aviDemoHeight;
	static idCVar		com_aviDemoSamples;
	static idCVar		com_aviDemoTics;
	static idCVar		com_wipeSeconds;
	static idCVar		com_guid;

	static idCVar		gui_configServerRate;

	//Obsttorte: cvar for disabling manual saves
	static idCVar		saveGameName;

	// SteveL #4161: Support > 1 quicksave
	static idCVar		com_numQuickSaves;

	int					timeHitch;

	bool				menuActive;
	idSoundWorld *		menuSoundWorld;			// so the game soundWorld can be muted

	bool				insideExecuteMapChange;	// draw loading screen and update
												// screen on prints
	//int				bytesNeededForMapLoad;	// grayman #3763 - no longer used

	float				pct;					// grayman #3763 - used by PacifierUpdate()
	float				pct_delta;				// grayman #3763 - used by PacifierUpdate()
	int					loadDoneTime;			// grayman #3763 - used by PacifierUpdate()

	// we don't want to redraw the loading screen for every single
	// console print that happens
	int					lastPacifierTime;

	// this is the information required to be set before ExecuteMapChange() is called,
	// which can be saved off at any time with the following commands so it can all be played back
	mapSpawnData_t		mapSpawnData;
	idStr				currentMapName;			// for checking reload on same level
	bool				mapSpawned;				// cleared on Stop()

	int					numClients;				// from serverInfo

	int					logIndex;
	logCmd_t			loggedUsercmds[MAX_LOGGED_USERCMDS];
	int					statIndex;
	logStats_t			loggedStats[MAX_LOGGED_STATS];
	int					lastSaveIndex;
	// each game tic, numClients usercmds will be added, until full

	bool				insideUpdateScreen;	// true while inside ::UpdateScreen()

	bool				loadingSaveGame;	// currently loading map from a SaveGame
	idFile *			savegameFile;		// this is the savegame file to load from
	int					savegameVersion;

	idFile *			cmdDemoFile;		// if non-zero, we are reading commands from a file

	int					latchedTicNumber;	// set to com_ticNumber each frame
	int					lastGameTic;		// while latchedTicNumber > lastGameTic, run game frames
	int					lastDemoTic;
	bool				syncNextGameFrame;


	bool				aviCaptureMode;		// if true, screenshots will be taken and sound captured
	idStr				aviDemoShortName;	// 
	float				aviDemoFrameCount;
	int					aviTicStart;

	timeDemo_t			timeDemo;
	int					timeDemoStartTime;
	int					numDemoFrames;		// for timeDemo and demoShot
	int					demoTimeOffset;
	renderView_t		currentDemoRenderView;
	// the next one will be read when 
	// com_frameTime + demoTimeOffset > currentDemoRenderView.

	// TODO: make this private (after sync networking removal and idnet tweaks)
	idUserInterface *	guiActive;
	HandleGuiCommand_t	guiHandle;

	idUserInterface *	guiInGame;
	idUserInterface *	guiMainMenu;
	idListGUI *			guiMainMenu_MapList;		// easy map list handling
	idUserInterface *	guiRestartMenu;
	idUserInterface *	guiLoading;
	idUserInterface *	guiTest;
	
	idUserInterface *	guiMsg;
	idUserInterface *	guiMsgRestore;				// store the calling GUI for restore
	idStr				msgFireBack[ 2 ];
	bool				msgRunning;
	int					msgRetIndex;
	bool				msgIgnoreButtons;
	
	bool				waitingOnBind;

	const idMaterial *	whiteMaterial;

	const idMaterial *	wipeMaterial;
	int					wipeStartTic;
	int					wipeStopTic;
	bool				wipeHold;

#if ID_CONSOLE_LOCK
	int					emptyDrawCount;				// watchdog to force the main menu to restart
#endif

	int					gameTicsToRun;
	std::thread			frontendThread;
	std::condition_variable signalFrontendThread;
	std::condition_variable signalMainThread;
	std::mutex			signalMutex;
	volatile bool		frontendActive;
	volatile bool		shutdownFrontend;
	std::shared_ptr<ErrorReportedException> frontendException;
	double				frontendTimeWaiting;
	double				frontendTimeGameTics;
	double				frontendTimeDrawing;
	double				frontendTimeSignal;

	void				FrontendThreadFunction();
	bool				IsFrontend() const;
	void				LogFrontendTimings( idFile& logFile ) const;

	//=====================================
	void				Clear();

	void				DrawCmdGraph();
	void				Draw();

	void				WriteCmdDemo( const char *name, bool save = false);
	void				StartPlayingCmdDemo( const char *demoName);
	void				TimeCmdDemo( const char *demoName);
	void				SaveCmdDemoToFile(idFile *file);
	void				LoadCmdDemoFromFile(idFile *file);
	void				StartRecordingRenderDemo( const char *name );
	void				StopRecordingRenderDemo();
	void				StartPlayingRenderDemo( idStr name );
	void				StopPlayingRenderDemo();
	void				CompressDemoFile( const char *scheme, const char *name );
	void				TimeRenderDemo( const char *name, bool twice = false );
	void				AVIRenderDemo( const char *name );
	void				AVICmdDemo( const char *name );
	void				AVIGame( const char *name );
	void				BeginAVICapture( const char *name );
	void				EndAVICapture();

	void				AdvanceRenderDemo( bool singleFrameOnly );
	void				RunGameTics();
	void				DrawFrame();

	void				FinishCmdLoad();
	void				LoadLoadingGui(const char *mapName);

	void				DemoShot( const char *name );

	void				TestGUI( const char *name );

//	int					GetBytesNeededForMapLoad( const char *mapName ); // #3763 debug - no longer used
	void				SetBytesNeededForMapLoad( const char *mapName, int bytesNeeded );

	void				ExecuteMapChange( bool noFadeWipe = false );
	void				UnloadMap();

	//------------------
	// Session_menu.cpp

	idStrList			loadGameList;
	idStrList			modsList;

	idUserInterface *	GetActiveMenu();

	void				DispatchCommand( idUserInterface *gui, const char *menuCommand, bool doIngame = true );
	void				MenuEvent( const sysEvent_t *event );
	bool				HandleSaveGameMenuCommand( idCmdArgs &args, int &icmd );
	void				HandleInGameCommands( const char *menuCommand );
	void				HandleMainMenuCommands( const char *menuCommand );
	void				HandleChatMenuCommands( const char *menuCommand );
	void				HandleRestartMenuCommands( const char *menuCommand );
	void				HandleMsgCommands( const char *menuCommand );
	void				GetSaveGameList( idStrList &fileList, idList<fileTIME_T> &fileTimes );
	void				UpdateMPLevelShot( void );

	void				SetSaveGameGuiVars( void );
	void				SetMainMenuGuiVars( void );
	void				SetModsMenuGuiVars( void );
	void				SetMainMenuSkin( void );
	void				SetPbMenuGuiVars( void );
	
private:
	bool				BoxDialogSanityCheck( void );
	idStr				authMsg;
};

extern idSessionLocal	sessLocal;

#endif /* !__SESSIONLOCAL_H__ */
