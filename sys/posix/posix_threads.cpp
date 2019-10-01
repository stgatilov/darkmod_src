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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <pthread.h>

#include "../../idlib/precompiled.h"
#include "posix_public.h"

#if defined(_DEBUG)
// #define ID_VERBOSE_PTHREADS 
#endif

/*
======================================================
locks
======================================================
*/

// we use an extra lock for the local stuff
const int MAX_LOCAL_CRITICAL_SECTIONS = MAX_CRITICAL_SECTIONS + 1;
static pthread_mutex_t global_lock[ MAX_LOCAL_CRITICAL_SECTIONS ];

/*
==================
Sys_EnterCriticalSection
==================
*/
void Sys_EnterCriticalSection( int index ) {
	assert( index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS );
#ifdef ID_VERBOSE_PTHREADS	
	if ( pthread_mutex_trylock( &global_lock[index] ) == EBUSY ) {
		Sys_Printf( "busy lock %d in thread '%s'\n", index, Sys_GetThreadName() );
		if ( pthread_mutex_lock( &global_lock[index] ) == EDEADLK ) {
			Sys_Printf( "FATAL: DEADLOCK %d, in thread '%s'\n", index, Sys_GetThreadName() );
		}
	}	
#else
	pthread_mutex_lock( &global_lock[index] );
#endif
}

/*
==================
Sys_LeaveCriticalSection
==================
*/
void Sys_LeaveCriticalSection( int index ) {
	assert( index >= 0 && index < MAX_LOCAL_CRITICAL_SECTIONS );
#ifdef ID_VERBOSE_PTHREADS
	if ( pthread_mutex_unlock( &global_lock[index] ) == EPERM ) {
		Sys_Printf( "FATAL: NOT LOCKED %d, in thread '%s'\n", index, Sys_GetThreadName() );
	}
#else
	pthread_mutex_unlock( &global_lock[index] );
#endif
}

/*
======================================================
wait and trigger events
we use a single lock to manipulate the conditions, MAX_LOCAL_CRITICAL_SECTIONS-1

the semantics match the win32 version. signals raised while no one is waiting stay raised until a wait happens (which then does a simple pass-through)

NOTE: we use the same mutex for all the events. I don't think this would become much of a problem
cond_wait unlocks atomically with setting the wait condition, and locks it back before exiting the function
the potential for time wasting lock waits is very low
======================================================
*/

pthread_cond_t	event_cond[ MAX_TRIGGER_EVENTS ];
bool			signaled[ MAX_TRIGGER_EVENTS ];
bool			waiting[ MAX_TRIGGER_EVENTS ];

/*
==================
Sys_WaitForEvent
==================
*/
void Sys_WaitForEvent( int index ) {
	assert( index >= 0 && index < MAX_TRIGGER_EVENTS );
	Sys_EnterCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
	assert( !waiting[ index ] );	// WaitForEvent from multiple threads? that wouldn't be good
	if ( signaled[ index ] ) {
		// emulate windows behaviour: signal has been raised already. clear and keep going
		signaled[ index ] = false;
	} else {
		waiting[ index ] = true;
		pthread_cond_wait( &event_cond[ index ], &global_lock[ MAX_LOCAL_CRITICAL_SECTIONS - 1 ] );
		waiting[ index ] = false;
	}
	Sys_LeaveCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
}

/*
==================
Sys_TriggerEvent
==================
*/
void Sys_TriggerEvent( int index ) {
	assert( index >= 0 && index < MAX_TRIGGER_EVENTS );
	Sys_EnterCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
	if ( waiting[ index ] ) {		
		pthread_cond_signal( &event_cond[ index ] );
	} else {
		// emulate windows behaviour: if no thread is waiting, leave the signal on so next wait keeps going
		signaled[ index ] = true;
	}
	Sys_LeaveCriticalSection( MAX_LOCAL_CRITICAL_SECTIONS - 1 );
}

/*
======================================================
thread create and destroy
======================================================
*/

typedef void *(*pthread_function_t) (void *);

/*
==================
Sys_CreateThread
==================
*/
uintptr_t Sys_CreateThread( xthread_t function, void* parms, xthreadPriority priority, const char* name, core_t core, int stackSize, bool suspended )
{
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	
	if( pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_attr_setdetachstate %s failed\n", name );
		return ( uintptr_t )0;
	}
	
	pthread_t handle;
	if( pthread_create( ( pthread_t* )&handle, &attr, ( pthread_function_t )function, parms ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_create %s failed\n", name );
		return ( uintptr_t )0;
	}
	
#if defined(DEBUG_THREADS)
	if( Sys_SetThreadName( handle, name ) != 0 )
	{
		idLib::common->Warning( "Warning: pthread_setname_np %s failed\n", name );
		return ( uintptr_t )0;
	}
#endif
	
	pthread_attr_destroy( &attr );
	
	
#if 0
	// RB: realtime policies require root privileges
	
	// all Linux threads have one of the following scheduling policies:
	
	// SCHED_OTHER or SCHED_NORMAL: the default policy,  priority: [-20..0..19], default 0
	
	// SCHED_FIFO: first in/first out realtime policy
	
	// SCHED_RR: round-robin realtime policy
	
	// SCHED_BATCH: similar to SCHED_OTHER, but with a throughput orientation
	
	// SCHED_IDLE: lower priority than SCHED_OTHER
	
	int schedulePolicy = SCHED_OTHER;
	struct sched_param scheduleParam;
	
	int error = pthread_getschedparam( handle, &schedulePolicy, &scheduleParam );
	if( error != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_getschedparam %s failed: %s\n", name, strerror( error ) );
		return ( uintptr_t )0;
	}
	
	schedulePolicy = SCHED_FIFO;
	
	int minPriority = sched_get_priority_min( schedulePolicy );
	int maxPriority = sched_get_priority_max( schedulePolicy );
	
	if( priority == THREAD_HIGHEST )
	{
		//  we better sleep enough to do this
		scheduleParam.__sched_priority = maxPriority;
	}
	else if( priority == THREAD_ABOVE_NORMAL )
	{
		scheduleParam.__sched_priority = Lerp( minPriority, maxPriority, 0.75f );
	}
	else if( priority == THREAD_NORMAL )
	{
		scheduleParam.__sched_priority = Lerp( minPriority, maxPriority, 0.5f );
	}
	else if( priority == THREAD_BELOW_NORMAL )
	{
		scheduleParam.__sched_priority = Lerp( minPriority, maxPriority, 0.25f );
	}
	else if( priority == THREAD_LOWEST )
	{
		scheduleParam.__sched_priority = minPriority;
	}
	
	// set new priority
	error = pthread_setschedparam( handle, schedulePolicy, &scheduleParam );
	if( error != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_setschedparam( name = %s, policy = %i, priority = %i ) failed: %s\n", name, schedulePolicy, scheduleParam.__sched_priority, strerror( error ) );
		return ( uintptr_t )0;
	}
	
	pthread_getschedparam( handle, &schedulePolicy, &scheduleParam );
	if( error != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_getschedparam %s failed: %s\n", name, strerror( error ) );
		return ( uintptr_t )0;
	}
#endif
	
	// Under Linux, we don't set the thread affinity and let the OS deal with scheduling
	
	return ( uintptr_t )handle;
}

/*
==================
Sys_DestroyThread
==================
*/
void Sys_DestroyThread( uintptr_t threadHandle )
{
	if( threadHandle == 0 )
	{
		return;
	}
	
	char	name[128];
	name[0] = '\0';
	
#if defined(DEBUG_THREADS)
	Sys_GetThreadName( ( pthread_t )threadHandle, name, sizeof( name ) );
#endif
	
#if 0 //!defined(__ANDROID__)
	if( pthread_cancel( ( pthread_t )threadHandle ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_cancel %s failed\n", name );
	}
#endif
	
	if( pthread_join( ( pthread_t )threadHandle, NULL ) != 0 )
	{
		idLib::common->FatalError( "ERROR: pthread_join %s failed\n", name );
	}
}

/*
==================
Sys_GetThreadName
find the name of the calling thread
==================
*/
static int Sys_GetThreadName( pthread_t handle, char* namebuf, size_t buflen )
{
	int ret = 0;
#ifdef __linux__
	ret = pthread_getname_np( handle, namebuf, buflen );
	if( ret != 0 )
		idLib::common->Printf( "Getting threadname failed, reason: %s (%i)\n", strerror( errno ), errno );
#elif defined(__FreeBSD__)
	// seems like there is no pthread_getname_np equivalent on FreeBSD
	idStr::snPrintf( namebuf, buflen, "Can't read threadname on this platform!" );
#endif
	/* TODO: OSX:
		// int pthread_getname_np(pthread_t, char*, size_t);
	*/
	
	return ret;
}

/*
=========================================================
Async Thread
=========================================================
*/

uintptr_t asyncThread;

/*
=================
Posix_StartAsyncThread
=================
*/
void Posix_StartAsyncThread() {
	if ( asyncThread == 0 ) {
		asyncThread = Sys_CreateThread( (xthread_t) Sys_AsyncThread, NULL, THREAD_NORMAL, "Async" );
	} else {
		common->Printf( "Async thread already running\n" );
	}
	common->Printf( "Async thread started\n" );
}