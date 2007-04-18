/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#ifdef __cplusplus

//-----------------------------------------------------

#ifdef _WIN32

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion
#define _WIN32_WINNT 0x0501
#ifndef _D3SDK
#ifndef GAME_DLL

#define WINVER				0x501

#ifdef	ID_DEDICATED
// dedicated sets windows version here
#define	_WIN32_WINNT WINVER
#define	WIN32_LEAN_AND_MEAN
#else
// non-dedicated includes MFC and sets windows verion here
#include "../tools/comafx/StdAfx.h"			// this will go away when MFC goes away
#endif

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#define DIRECTINPUT_VERSION  0x0700
#define DIRECTSOUND_VERSION  0x0800

#include "../mssdk/include/dsound.h"
#include "../mssdk/include/dinput.h"
#include "../mssdk/include/dxerr8.h"

#endif /* !GAME_DLL */
#endif /* !_D3SDK */

#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for qgl.h
#undef FindText								// stupid namespace poluting Microsoft monkeys

#endif /* _WIN32 */

//-----------------------------------------------------

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>

//-----------------------------------------------------

// non-portable system services
#include "../sys/sys_public.h"

// id lib
#include "../idlib/lib.h"

#if !defined( _D3SDK ) && defined( __WITH_PB__ )
	#include "../punkbuster/pbcommon.h"
#endif

// framework
#include "../framework/buildversion.h"
#include "../framework/builddefines.h"
#include "../framework/licensee.h"
#include "../framework/cmdsystem.h"
#include "../framework/cvarsystem.h"
#include "../framework/common.h"
#include "../framework/file.h"
#include "../framework/filesystem.h"
#include "../framework/usercmdgen.h"

// decls
#include "../framework/declmanager.h"
#include "../framework/decltable.h"
#include "../framework/declskin.h"
#include "../framework/declentitydef.h"
#include "../framework/declfx.h"
#include "../framework/declparticle.h"
#include "../framework/declaf.h"
#include "../framework/declpda.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "../renderer/qgl.h"
#include "../renderer/cinematic.h"
#include "../renderer/material.h"
#include "../renderer/model.h"
#include "../renderer/modelmanager.h"
#include "../renderer/rendersystem.h"
#include "../renderer/renderworld.h"

// sound engine
#include "../sound/sound.h"

// asynchronous networking
#include "../framework/async/networksystem.h"

// user interfaces
#include "../ui/listgui.h"
#include "../ui/userinterface.h"

// collision detection system
#include "../cm/collisionmodel.h"

// AAS files and manager
#include "../tools/compilers/aas/aasfile.h"
#include "../tools/compilers/aas/aasfilemanager.h"

// game
#include "../game/game.h"

//-----------------------------------------------------

#ifndef _D3SDK

#ifdef GAME_DLL

#if defined(_D3XP)
#include "../d3xp/Game_local.h"
#else
#include "../game/Game_local.h"
#endif

#else

#include "../framework/DemoChecksum.h"

// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/Console.h"
#include "../framework/DemoFile.h"
#include "../framework/Session.h"

// asynchronous networking
#include "../framework/async/AsyncNetwork.h"

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../tools/edit_public.h"

// Compilers for map, model, video etc. processing.
#include "../tools/compilers/compiler_public.h"

#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------

#endif	/* __cplusplus */

#endif /* !__PRECOMPILED_H__ */
