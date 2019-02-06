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
#include "../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "../sys_local.h"
#include "local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <execinfo.h>
#include <stdio.h>

#ifdef ID_MCHECK
#include <mcheck.h>
#endif

static idStr	basepath;
static idStr	savepath;
static idStr	modSavepath; // greebo: Added for TDM mission handling

/*
===========
Sys_InitScanTable
===========
*/
void Sys_InitScanTable( void ) {
	common->DPrintf( "TODO: Sys_InitScanTable\n" );
}

/*
=================
Sys_AsyncThread
=================
*/
THREAD_RETURN_TYPE Sys_AsyncThread(void*) {
	int now;
	int next;
	int	want_sleep;

	// multi tick compensate for poor schedulers (Linux 2.4)
	int ticked, to_ticked;
	now = Sys_Milliseconds();
	ticked = now >> 4;
	while (1) {
		// sleep
		now = Sys_Milliseconds();		
		next = ( now & 0xFFFFFFF0 ) + 0x10;
		want_sleep = ( next-now-1 ) * 1000;
		if ( want_sleep > 0 ) {
			usleep( want_sleep ); // sleep 1ms less than true target
		}
		
		// compensate if we slept too long
		now = Sys_Milliseconds();
		to_ticked = now >> 4;
		
		// show ticking statistics - every 100 ticks, print a summary
		#if 0
			#define STAT_BUF 100
			static int stats[STAT_BUF];
			static int counter = 0;
			// how many ticks to play
			stats[counter] = to_ticked - ticked;
			counter++;
			if (counter == STAT_BUF) {
				Sys_DebugPrintf("\n");
				for( int i = 0; i < STAT_BUF; i++) {
					if ( ! (i & 0xf) ) {
						Sys_DebugPrintf("\n");
					}
					Sys_DebugPrintf( "%d ", stats[i] );
				}
				Sys_DebugPrintf("\n");
				counter = 0;
			}
		#endif
		
		while ( ticked < to_ticked ) {
			common->Async();
			ticked++;
			Sys_TriggerEvent( TRIGGER_EVENT_ONE );
		}
		// thread exit
		pthread_testcancel();
	}

    return (THREAD_RETURN_TYPE)0;
}

/*
==============
Sys_DefaultSavePath
==============
*/
const char *Sys_DefaultSavePath(void) {
    if ( savepath.IsEmpty() ) {
        idStr fsMod = cvarSystem->GetCVarString("fs_mod");

        sprintf( savepath, "%s", Sys_DefaultBasePath() );

        if (!fsMod.IsEmpty() && idStr::Cmp( fsMod.c_str(), BASE_TDM ) ) {
            // mod isn't "darkmod", so add the mod dir
            sprintf( savepath, "%s/%s", savepath.c_str(), fsMod.c_str() );
        }
    }

	return savepath.c_str();
}

/*
==============
Sys_ModSavePath
==============
*/
const char* Sys_ModSavePath() {
    if ( modSavepath.IsEmpty() ) {
        sprintf( modSavepath, "%s/%s", Sys_DefaultSavePath(), "fms" );
    }

	return modSavepath.c_str();
}

/*
==============
Sys_EXEPath
==============
*/
const char *Sys_EXEPath( void ) {
	static char	buf[ 2048 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf( linkpath, "/proc/%d/exe", getpid() );
	len = readlink( linkpath.c_str(), buf, sizeof( buf ) );
	if ( len == -1 ) {
		Sys_Printf("Couldn't stat exe path link %s\n", linkpath.c_str());
		buf[ 0 ] = '\0';
	}
	return buf;
}

/*
================
Sys_DefaultBasePath

Get the default base path
- binary image path
- current directory
- hardcoded
Try to be intelligent: if there is no BASE_TDM ("darkmod"), try the next path
================
*/
const char *Sys_DefaultBasePath(void) {
    if ( basepath.IsEmpty() ) {
        struct stat st;
        idStr testbase;

        // first the path where the executable resides
        basepath = Sys_EXEPath();
        if ( basepath.Length() ) {
            basepath.StripFilename();
            testbase = basepath;
            testbase.StripFilename(); // strip a second time to get rid of mod dir
            testbase += "/"; testbase += BASE_TDM;
            if ( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) ) {
                return basepath.c_str();
            } else {
                common->Printf( "no '%s' directory in exe path %s, skipping\n", BASE_TDM, basepath.c_str() );
            }

            /* we don't want the binary to live anywhere except darkmod/

            // next try "path_to_executable/.." (one up), maybe the executable is in the same dir as the "darkmod" directory
            // strip the last part of basepath
            // "/home/user/games/tdm/darkmod/" => "/home/user/games/tdm/"
            if (basepath.StripTrailingOnce("/darkmod"))
            {
                testbase = basepath + "/" + BASE_TDM;
                if ( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) ) {
                    return basepath.c_str();
                } else {
                    common->Printf( "exe inside 'darkmod/' path, but could not traverse up, skipping\n" );
                }
            }
            */
        }

        // next try the current path
        if ( basepath != Posix_Cwd() ) {
            basepath = Posix_Cwd();
            testbase = basepath;
            testbase.StripFilename();
            testbase += "/"; testbase += BASE_TDM;
            if ( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) ) {
                return basepath.c_str();
            } else {
                common->Printf("no '%s' directory in cwd path %s, skipping\n", BASE_TDM, basepath.c_str());
            }
        }
    } else {
        return basepath.c_str();
    }

    // try the current path
	common->Printf( "WARNING: using hardcoded default base path\n" );
	return LINUX_DEFAULT_PATH;
}

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey( bool shifted ) {
	return shifted ? '~' : '`';
}

/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown( void ) {
	basepath.Clear();
	savepath.Clear();
	Posix_Shutdown();
}

/*
===============
Sys_GetProcessorId
===============
*/
cpuid_t Sys_GetProcessorId( void ) {
	return CPUID_GENERIC;
}

/*
===============
Sys_GetProcessorString
===============
*/
const char *Sys_GetProcessorString( void ) {
	return "generic";
}

/*
===============
Sys_FPE_handler
===============
*/
void Sys_FPE_handler( int signum, siginfo_t *info, void *context ) {
	assert( signum == SIGFPE );
	Sys_Printf( "FPE\n" );
}

/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks( void ) {
	unsigned int hi, lo;
	__asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return (double) (((uint64_t)hi << 32) | lo);
}

/*
===============
MeasureClockTicks
===============
*/
double MeasureClockTicks( void ) {
	double t0, t1;

	t0 = Sys_GetClockTicks( );
	Sys_Sleep( 1000 );
	t1 = Sys_GetClockTicks( );	
	return t1 - t0;
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
double Sys_ClockTicksPerSecond(void) {
	static bool		init = false;
	static double	ret;

	int		fd, len, pos, end;
	char	buf[ 4096 ];

	if ( init ) {
		return ret;
	}

	fd = open( "/proc/cpuinfo", O_RDONLY );
	if ( fd == -1 ) {
		common->Printf( "couldn't read /proc/cpuinfo\n" );
		ret = MeasureClockTicks();
		init = true;
		common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
		return ret;		
	}
	len = read( fd, buf, 4096 );
	close( fd );
	pos = 0;
	while ( pos < len ) {
		if ( !idStr::Cmpn( buf + pos, "cpu MHz", 7 ) ) {
			pos = strchr( buf + pos, ':' ) - buf + 2;
			end = strchr( buf + pos, '\n' ) - buf;
			if ( pos < len && end < len ) {
				buf[end] = '\0';
				ret = atof( buf + pos );
			} else {
				common->Printf( "failed parsing /proc/cpuinfo\n" );
				ret = MeasureClockTicks();
				init = true;
				common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
				return ret;		
			}
			common->Printf( "/proc/cpuinfo CPU frequency: %g MHz\n", ret );
			ret *= 1000000;
			init = true;
			return ret;
		}
		pos = strchr( buf + pos, '\n' ) - buf + 1;
	}
	common->Printf( "failed parsing /proc/cpuinfo\n" );
	ret = MeasureClockTicks();
	init = true;
	common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
	return ret;		
}

/*
==================
Sys_DoStartProcess
if we don't fork, this function never returns
the no-fork lets you keep the terminal when you're about to spawn an installer

if the command contains spaces, system() is used. Otherwise the more straightforward execl ( system() blows though )
==================
*/
void Sys_DoStartProcess( const char *exeName, bool dofork ) {	
	bool use_system = false;
	if ( strchr( exeName, ' ' ) ) {
		use_system = true;
	} else {
		// set exec rights when it's about a single file to execute
		struct stat buf;
		if ( stat( exeName, &buf ) == -1 ) {
			printf( "stat %s failed: %s\n", exeName, strerror( errno ) );
		} else {
			if ( chmod( exeName, buf.st_mode | S_IXUSR ) == -1 ) {
				printf( "cmod +x %s failed: %s\n", exeName, strerror( errno ) );
			}
		}
	}
	if ( dofork ) {
		switch ( fork() ) {
		case -1:
			// main thread
            printf( "fork failed: %s\n", strerror( errno ) );
			break;
		case 0:
			if ( use_system ) {
				printf( "system %s\n", exeName );
				if (system( exeName ) == -1)
					printf( "system failed: %s\n", strerror( errno ) );
				_exit( 0 );
			} else {
				printf( "execl %s\n", exeName );
				execl( exeName, exeName, NULL );
				printf( "execl failed: %s\n", strerror( errno ) );
				_exit( -1 );
			}
			break;
        default:
            break;
		}
	} else {
		if ( use_system ) {
			printf( "system %s\n", exeName );
			if (system( exeName ) == -1)
				printf( "system failed: %s\n", strerror( errno ) );
			else
				sleep( 1 );	// on some systems I've seen that starting the new process and exiting this one should not be too close
		} else {
			printf( "execl %s\n", exeName );
			execl( exeName, exeName, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		// terminate
		_exit( 0 );
	}
}

/*
=================
Sys_OpenURL
=================
*/
void idSysLocal::OpenURL( const char *url, bool quit ) {
	const char	*script_path;
	idFile		*script_file;
	char		cmdline[ 1024 ];

	static bool	quit_spamguard = false;

	if ( quit_spamguard ) {
		common->DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf( "Open URL: %s\n", url );
	// opening an URL on *nix can mean a lot of things .. 
	// just spawn a script instead of deciding for the user :-)

	// look in the savepath first, then in the basepath
	script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), "", "openurl.sh" );
	script_file = fileSystem->OpenExplicitFileRead( script_path );
	if ( !script_file ) {
		script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_basepath" ), "", "openurl.sh" );
		script_file = fileSystem->OpenExplicitFileRead( script_path );
	}
	if ( !script_file ) {
		common->Printf( "Can't find URL script 'openurl.sh' in either savepath or basepath\n" );
		common->Printf( "OpenURL '%s' failed\n", url );
		return;
	}
	fileSystem->CloseFile( script_file );

	// if we are going to quit, only accept a single URL before quitting and spawning the script
	if ( quit ) {
		quit_spamguard = true;
	}

	common->Printf( "URL script: %s\n", script_path );

	// StartProcess is going to execute a system() call with that - hence the &
	idStr::snPrintf( cmdline, 1024, "%s '%s' &",  script_path, url );
	sys->StartProcess( cmdline, quit );
}

/*
==================
Sys_DoPreferences
==================
*/
void Sys_DoPreferences( void ) { }

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable ) {
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 6
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<6)	// clear DAX bit
		or		eax, ecx		// set the DAZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable ) {
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 15
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<15)	// clear FTZ bit
		or		eax, ecx		// set the FTZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}

/*
================
stgatilov: stacktrace
================
*/

void Sys_CaptureStackTrace(int ignoreFrames, uint8_t *data, int &len) {
	int cnt = backtrace((void**)data, len / sizeof(void*));
	if (ignoreFrames > cnt)
		ignoreFrames = cnt;
	len = (cnt - ignoreFrames) * sizeof(void*);
	memmove(data, data + ignoreFrames * sizeof(void*), len);
}

int Sys_GetStackTraceFramesCount(uint8_t *data, int len) {
	return len / sizeof(void*);
}

void Sys_DecodeStackTrace(uint8_t *data, int len, debugStackFrame_t *frames) {
	//interpret input blob as array of addresses
	void **addresses = (void**)data;
	int framesCount = Sys_GetStackTraceFramesCount(data, len);
	//fill output with zeros
	memset(frames, 0, framesCount * sizeof(frames[0]));

	//decode the whole bunch of addresses
	char **messages = backtrace_symbols(addresses, framesCount);

	//the persistent set of addr2line processes
	struct Addr2lineProcess {
		char moduleName[248];
		FILE *pipe;
	};
	static const int MAX_DECODERS = 16;
	static int decodersCount = 0;
	static Addr2lineProcess decoders[MAX_DECODERS];

	for (int i = 0; i < framesCount; i++) {
		frames[i].pointer = addresses[i];

		if (!addresses[i])
			continue;	//null function?
		if (!messages[i])
			continue;	//no info from system?

		//copy results of backtrace (in case we fail to decode it)
		idStr::Copynz(frames[i].functionName, messages[i], sizeof(frames[i].functionName));

		//extract name of module from backtrace_symbols line
		int brOpen = strcspn(messages[i], "()[]");
		char moduleName[256];
		idStr::Copynz(moduleName, messages[i], sizeof(moduleName));
		moduleName[brOpen] = 0;

		//find existing addr2line process
		int idx;
		for (idx = 0; idx < decodersCount; idx++) {
			if (strcmp(decoders[idx].moduleName, moduleName) == 0)
				break;
		}
		if (idx == decodersCount) {	//not found
			if (decodersCount == MAX_DECODERS)
				continue;
			//create addr2line command line
			char cmdLine[1024];
			idStr::snPrintf(cmdLine, sizeof(cmdLine), "addr2line -e %s -f -C", moduleName);
			//printf("%d: %s\n", idx, cmdLine);

			//NOTE: in order to make it work, someone needs to make this pipe bidirectional
			//I f*cked with popen2 long enough to no avail =(

			//start process
			decoders[idx].pipe = popen(cmdLine, "r");
			if (!decoders[idx].pipe)
				continue;
			idStr::Copynz(decoders[idx].moduleName, moduleName, sizeof(decoders[idx].moduleName));
			decodersCount++;
		}
		FILE *f = decoders[idx].pipe;

		//ask addr2line process about this address
		fprintf(f, "%p\n", addresses[i]);
		fflush(f);
		char lines[2][1024];
		fgets(lines[0], sizeof(lines[0]), f);
		fgets(lines[1], sizeof(lines[1]), f);

		//parse first line (function name)
		int l = strcspn(lines[0], "()\n");
		if (l == 0) continue;
		lines[0][l] = 0;
		idStr::Copynz(frames[i].functionName, lines[0], sizeof(frames[i].functionName));

		//parse second line (file and line)
		l = strcspn(lines[1], ":");
		int r = l + strcspn(lines[1] + l, " ");
		if (l == 0 || r == l) continue;
		lines[1][l] = lines[1][r] = 0;
		int lineNo = 0;
		sscanf(lines[1] + l+1, "%d", &lineNo);
		idStr::Copynz(frames[i].fileName, lines[1], sizeof(frames[i].fileName));
		frames[i].lineNumber = lineNo;
	}

	//note: strings must not be freed
	free(messages);
}


/*
===============
mem consistency stuff
===============
*/

#ifdef ID_MCHECK

const char *mcheckstrings[] = {
	"MCHECK_DISABLED",
	"MCHECK_OK",
	"MCHECK_FREE",	// block freed twice
	"MCHECK_HEAD",	// memory before the block was clobbered
	"MCHECK_TAIL"	// memory after the block was clobbered
};

void abrt_func( mcheck_status status ) {
	Sys_Printf( "memory consistency failure: %s\n", mcheckstrings[ status + 1 ] );
	Posix_SetExit( EXIT_FAILURE );
	common->Quit();
}

#endif

/*
===============
main
===============
*/
int main(int argc, const char **argv) {
#ifdef ID_MCHECK
	// must have -lmcheck linkage
	mcheck( abrt_func );
	Sys_Printf( "memory consistency checking enabled\n" );
#endif
	
    // do not allow TDM to be run as root
    if ( getuid() == 0 ) {
        Sys_Printf( "The Dark Mod should not be run as root.\n" );
        Posix_Exit( EXIT_FAILURE );
    }

	Posix_EarlyInit( );

	if ( argc > 1 ) {
		common->Init( argc-1, &argv[1], NULL );
	} else {
		common->Init( 0, NULL, NULL );
	}

	Posix_LateInit( );

	while (1) {
		common->Frame();
	}
}
