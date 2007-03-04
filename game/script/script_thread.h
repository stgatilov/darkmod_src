/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.8  2007/01/12 05:57:17  gildoran
 * Added sys.waitForRender($entity)
 *
 * Revision 1.7  2006/05/03 21:35:03  sparhawk
 * Added support for booleans for scriptfunctions.
 *
 * Revision 1.6  2006/03/25 08:14:03  gildoran
 * New update for declarations... Improved the documentation/etc for xdata decls, and added some basic code for tdm_matinfo decls.
 *
 * Revision 1.5  2006/02/04 23:52:32  sparhawk
 * Added support for arbitrary arguments being passed to a scriptfunction.
 *
 * Revision 1.4  2006/01/29 04:18:10  ishtvan
 * added scriptfunction to get and dynamically set soundprop losses at portals
 *
 * Revision 1.3  2005/11/11 22:01:38  sparhawk
 * SDK 1.3 Merge
 *
 * Revision 1.2  2005/03/29 07:53:32  ishtvan
 * AI Relations: Added AI relations scripting functions to get and change the relationship between two teams.  The script functions are called from the global $sys object.
 *
 * Revision 1.1.1.1  2004/10/30 15:52:33  sparhawk
 * Initial release
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __SCRIPT_THREAD_H__
#define __SCRIPT_THREAD_H__

extern const idEventDef EV_Thread_Execute;
extern const idEventDef EV_Thread_SetCallback;
extern const idEventDef EV_Thread_SetRenderCallback;
extern const idEventDef EV_Thread_TerminateThread;
extern const idEventDef EV_Thread_Pause;
extern const idEventDef EV_Thread_Wait;
extern const idEventDef EV_Thread_WaitFrame;
extern const idEventDef EV_Thread_WaitFor;
extern const idEventDef EV_Thread_WaitForThread;
extern const idEventDef EV_Thread_Print;
extern const idEventDef EV_Thread_PrintLn;
extern const idEventDef EV_Thread_Say;
extern const idEventDef EV_Thread_Assert;
extern const idEventDef EV_Thread_Trigger;
extern const idEventDef EV_Thread_SetCvar;
extern const idEventDef EV_Thread_GetCvar;
extern const idEventDef EV_Thread_Random;
extern const idEventDef EV_Thread_GetTime;
extern const idEventDef EV_Thread_KillThread;
extern const idEventDef EV_Thread_SetThreadName;
extern const idEventDef EV_Thread_GetEntity;
extern const idEventDef EV_Thread_Spawn;
extern const idEventDef EV_Thread_SetSpawnArg;
extern const idEventDef EV_Thread_SpawnString;
extern const idEventDef EV_Thread_SpawnFloat;
extern const idEventDef EV_Thread_SpawnVector;
extern const idEventDef EV_Thread_AngToForward;
extern const idEventDef EV_Thread_AngToRight;
extern const idEventDef EV_Thread_AngToUp;
extern const idEventDef EV_Thread_Sine;
extern const idEventDef EV_Thread_Cosine;
extern const idEventDef EV_Thread_Normalize;
extern const idEventDef EV_Thread_VecLength;
extern const idEventDef EV_Thread_VecDotProduct;
extern const idEventDef EV_Thread_VecCrossProduct;
extern const idEventDef EV_Thread_OnSignal;
extern const idEventDef EV_Thread_ClearSignal;
extern const idEventDef EV_Thread_SetCamera;
extern const idEventDef EV_Thread_FirstPerson;
extern const idEventDef EV_Thread_TraceFraction;
extern const idEventDef EV_Thread_TracePos;
extern const idEventDef EV_Thread_FadeIn;
extern const idEventDef EV_Thread_FadeOut;
extern const idEventDef EV_Thread_FadeTo;
extern const idEventDef EV_Thread_Restart;

extern const idEventDef EV_AI_GetRelationSys;
extern const idEventDef EV_AI_SetRelation;
extern const idEventDef EV_AI_OffsetRelation;

extern const idEventDef EV_TDM_SetPortSoundLoss;
extern const idEventDef EV_TDM_GetPortSoundLoss;

class idThread : public idClass {
private:
	static idThread				*currentThread;

	idThread					*waitingForThread;
	int							waitingFor;
	int							waitingUntil;
	idInterpreter				interpreter;

	idDict						spawnArgs;
								
	int 						threadNum;
	idStr 						threadName;

	int							lastExecuteTime;
	int							creationTime;

	bool						manualControl;

	static int					threadIndex;
	static idList<idThread *>	threadList;

	static trace_t				trace;

	void						Init( void );
	void						Pause( void );

	void						Event_Execute( void );
	void						Event_SetThreadName( const char *name );

	//
	// script callable Events
	//
	void						Event_TerminateThread( int num );
	void						Event_Pause( void );
	void						Event_Wait( float time );
	void						Event_WaitFrame( void );
	void						Event_WaitFor( idEntity *ent );
	void						Event_WaitForThread( int num );
	void						Event_WaitForRender( idEntity *ent );
	void						Event_Print( const char *text );
	void						Event_PrintLn( const char *text );
	void						Event_Say( const char *text );
	void						Event_Assert( float value );
	void						Event_Trigger( idEntity *ent );
	void						Event_SetCvar( const char *name, const char *value ) const;
	void						Event_GetCvar( const char *name ) const;
	void						Event_Random( float range ) const;
	void						Event_GetTime( void );
	void						Event_KillThread( const char *name );
	void						Event_GetEntity( const char *name );
	void						Event_Spawn( const char *classname );
	void						Event_CopySpawnArgs( idEntity *ent );
	void						Event_SetSpawnArg( const char *key, const char *value );
	void						Event_SpawnString( const char *key, const char *defaultvalue );
	void						Event_SpawnFloat( const char *key, float defaultvalue );
	void						Event_SpawnVector( const char *key, idVec3 &defaultvalue );
	void						Event_ClearPersistantArgs( void );
	void 						Event_SetPersistantArg( const char *key, const char *value );
	void 						Event_GetPersistantString( const char *key );
	void 						Event_GetPersistantFloat( const char *key );
	void 						Event_GetPersistantVector( const char *key );
	void						Event_AngToForward( idAngles &ang );
	void						Event_AngToRight( idAngles &ang );
	void						Event_AngToUp( idAngles &ang );
	void						Event_GetSine( float angle );
	void						Event_GetCosine( float angle );
	void						Event_GetSquareRoot( float theSquare );
	void						Event_VecNormalize( idVec3 &vec );
	void						Event_VecLength( idVec3 &vec );
	void						Event_VecDotProduct( idVec3 &vec1, idVec3 &vec2 );
	void						Event_VecCrossProduct( idVec3 &vec1, idVec3 &vec2 );
	void						Event_VecToAngles( idVec3 &vec );
	void						Event_OnSignal( int signal, idEntity *ent, const char *func );
	void						Event_ClearSignalThread( int signal, idEntity *ent );
	void						Event_SetCamera( idEntity *ent );
	void						Event_FirstPerson( void );
	void						Event_Trace( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idEntity *passEntity );
	void						Event_TracePoint( const idVec3 &start, const idVec3 &end, int contents_mask, idEntity *passEntity );
	void						Event_GetTraceFraction( void );
	void						Event_GetTraceEndPos( void );
	void						Event_GetTraceNormal( void );
	void						Event_GetTraceEntity( void );
	void						Event_GetTraceJoint( void );
	void						Event_GetTraceBody( void );
	void						Event_FadeIn( idVec3 &color, float time );
	void						Event_FadeOut( idVec3 &color, float time );
	void						Event_FadeTo( idVec3 &color, float alpha, float time );
	void						Event_SetShaderParm( int parmnum, float value );
	void						Event_StartMusic( const char *name );
	void						Event_Warning( const char *text );
	void						Event_Error( const char *text );
	void 						Event_StrLen( const char *string );
	void 						Event_StrLeft( const char *string, int num );
	void 						Event_StrRight( const char *string, int num );
	void 						Event_StrSkip( const char *string, int num );
	void 						Event_StrMid( const char *string, int start, int num );
	void						Event_StrToFloat( const char *string );
	void						Event_RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, const char *damageDefName, float dmgPower );
	void						Event_IsClient( void );
	void 						Event_IsMultiplayer( void );
	void 						Event_GetFrameTime( void );
	void 						Event_GetTicsPerSecond( void );
	void						Event_CacheSoundShader( const char *soundName );
	void						Event_DebugLine( const idVec3 &color, const idVec3 &start, const idVec3 &end, const float lifetime );
	void						Event_DebugArrow( const idVec3 &color, const idVec3 &start, const idVec3 &end, const int size, const float lifetime );
	void						Event_DebugCircle( const idVec3 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const float lifetime );
	void						Event_DebugBounds( const idVec3 &color, const idVec3 &mins, const idVec3 &maxs, const float lifetime );
	void						Event_DrawText( const char *text, const idVec3 &origin, float scale, const idVec3 &color, const int align, const float lifetime );
	void						Event_InfluenceActive( void );

	// For test purposes only.
	void						Event_DebugTDM_MatInfo( const char *mat );

	/**
	* The following events are a frontend for the AI relationship
	* manager (stored in game_local).
	* See CRelations definition for descriptions of functions called
	**/
	void						Event_GetRelation( int team1, int team2 );
	void						Event_SetRelation( int team1, int team2, int val );
	void						Event_OffsetRelation( int team1, int team2, int offset );

	/**
	* TDM Soundprop Events:
	* Set or get the acoustical loss for a portal with a given handle.
	* Handle must be greater than zero and less than the number of portals in the map.
	**/
	void						Event_SetPortSoundLoss( int handle, float value );
	void						Event_GetPortSoundLoss( int handle );
	
	/**
	* TDM priority queue events
	**/
	void						idThread::Event_pqNew();
	void						idThread::Event_pqDelete(int pqueueID);
	void						idThread::Event_pqPush( int queueID, const char* task, int priority );
	void						idThread::Event_pqPeek( int queueID );
	void						idThread::Event_pqPop( int queueID );


public:							
								CLASS_PROTOTYPE( idThread );
								
								idThread();
								idThread( idEntity *self, const function_t *func );
								idThread( const function_t *func );
								idThread( idInterpreter *source, const function_t *func, int args );
								idThread( idInterpreter *source, idEntity *self, const function_t *func, int args );

	virtual						~idThread();

								// tells the thread manager not to delete this thread when it ends
	void						ManualDelete( void );

	// save games
	void						Save( idSaveGame *savefile ) const;				// archives object for save game file
	void						Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void						EnableDebugInfo( void ) { interpreter.debug = true; };
	void						DisableDebugInfo( void ) { interpreter.debug = false; };

	void						WaitMS( int time );
	void						WaitSec( float time );
	void						WaitFrame( void );
								
								// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction(const function_t	*func, bool clearStack );

	bool						CallFunctionArgs(const function_t *func, bool clearStack, const char *fmt, ...);
	bool						CallFunctionArgsVN(const function_t *func, bool clearStack, const char *fmt, va_list args);

								// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction( idEntity *obj, const function_t *func, bool clearStack );

	void						DisplayInfo();
	static idThread				*GetThread( int num );
	static void					ListThreads_f( const idCmdArgs &args );
	static void					Restart( void );
	static void					ObjectMoveDone( int threadnum, idEntity *obj );
								
	static idList<idThread*>&	GetThreads ( void );
	
	bool						IsDoneProcessing ( void );
	bool						IsDying			 ( void );	
								
	void						End( void );
	static void					KillThread( const char *name );
	static void					KillThread( int num );
	bool						Execute( void );
	void						ManualControl( void ) { manualControl = true; CancelEvents( &EV_Thread_Execute ); };
	void						DoneProcessing( void ) { interpreter.doneProcessing = true; };
	void						ContinueProcessing( void ) { interpreter.doneProcessing = false; };
	bool						ThreadDying( void ) { return interpreter.threadDying; };
	void						EndThread( void ) { interpreter.threadDying = true; };
	bool						IsWaiting( void );
	void						ClearWaitFor( void );
	bool						IsWaitingFor( idEntity *obj );
	void						ObjectMoveDone( idEntity *obj );
	void						ThreadCallback( idThread *thread );
	void						DelayedStart( int delay );
	bool						Start( void );
	idThread					*WaitingOnThread( void );
	void						SetThreadNum( int num );
	int 						GetThreadNum( void );
	void						SetThreadName( const char *name );
	const char					*GetThreadName( void );

	void						Error( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

	void						Warning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

								
	static idThread				*CurrentThread( void );
	static int					CurrentThreadNum( void );
	static bool					BeginMultiFrameEvent( idEntity *ent, const idEventDef *event );
	static void					EndMultiFrameEvent( idEntity *ent, const idEventDef *event );

	static void					ReturnString( const char *text );
	static void					ReturnFloat( float value );
	static void					ReturnInt( int value );
	static void					ReturnVector( idVec3 const &vec );
	static void					ReturnEntity( idEntity *ent );
};

/*
================
idThread::WaitingOnThread
================
*/
ID_INLINE idThread *idThread::WaitingOnThread( void ) {
	return waitingForThread;
}

/*
================
idThread::SetThreadNum
================
*/
ID_INLINE void idThread::SetThreadNum( int num ) {
	threadNum = num;
}

/*
================
idThread::GetThreadNum
================
*/
ID_INLINE int idThread::GetThreadNum( void ) {
	return threadNum;
}

/*
================
idThread::GetThreadName
================
*/
ID_INLINE const char *idThread::GetThreadName( void ) {
	return threadName.c_str();
}

/*
================
idThread::GetThreads
================
*/
ID_INLINE idList<idThread*>& idThread::GetThreads ( void ) {
	return threadList;
}	

/*
================
idThread::IsDoneProcessing
================
*/
ID_INLINE bool idThread::IsDoneProcessing ( void ) {
	return interpreter.doneProcessing;
}

/*
================
idThread::IsDying
================
*/
ID_INLINE bool idThread::IsDying ( void ) {
	return interpreter.threadDying;
}

#endif /* !__SCRIPT_THREAD_H__ */
