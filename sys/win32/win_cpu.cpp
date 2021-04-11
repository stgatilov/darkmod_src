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

#include <float.h>
#include "win_local.h"

#include "intrin.h"
#include "immintrin.h"

/*
==============================================================

	Clock ticks

==============================================================
*/

/*
================
Sys_GetClockTicks
================
*/
double Sys_GetClockTicks( void ) {
#if 0

	LARGE_INTEGER li;

	QueryPerformanceCounter( &li );
	return = ( double ) li.LowPart + ( double ) 0xFFFFFFFF * li.HighPart;

#elif defined (_MSC_VER) && defined(_WIN64)

	// stgatilov: serialize pipeline with cpuid instruction
	int values[4];
	__cpuid( values, 0 );
	// greebo: Use the intrinsic provided by the VC++ compiler in x64
	unsigned __int64 ticks = __rdtsc();
	return static_cast<double>( ticks );

#else

	unsigned long lo, hi;

	__asm {
		push ebx
		xor eax, eax
		cpuid
		rdtsc
		mov lo, eax
		mov hi, edx
		pop ebx
	}
	return ( double ) lo + ( double ) 0xFFFFFFFF * hi;

#endif
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond( void ) {
	static double ticks = 0;
#if 0

	if ( !ticks ) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticks = li.QuadPart;
	}

#else

	if ( !ticks ) {
		HKEY hKey;
		uint64_t ProcSpeed;
		DWORD buflen, ret;

		if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey ) ) {
			ProcSpeed = 0;
			buflen = sizeof( ProcSpeed );
			ret = RegQueryValueEx( hKey, "~MHz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			// If we don't succeed, try some other spellings.
			if ( ret != ERROR_SUCCESS ) {
				ret = RegQueryValueEx( hKey, "~Mhz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			}
			if ( ret != ERROR_SUCCESS ) {
				ret = RegQueryValueEx( hKey, "~mhz", NULL, NULL, ( LPBYTE ) &ProcSpeed, &buflen );
			}
			RegCloseKey( hKey );
			if ( ret == ERROR_SUCCESS ) {
				ticks = ( double )( ( unsigned long )ProcSpeed ) * 1000000;
			}
		}
	}

#endif
	return ticks;
}


/*
==============================================================

	CPU

==============================================================
*/

/*
================
CPUCount

	logicalNum is the number of logical CPU per physical CPU
    physicalNum is the total number of physical processor
	returns one of the HT_* flags
================
*/
/*#define HT_NOT_CAPABLE				0
#define HT_ENABLED					1
#define HT_DISABLED					2
#define HT_SUPPORTED_NOT_ENABLED	3
#define HT_CANNOT_DETECT			4*/

/*int CPUCount( int &logicalNum, int &physicalNum )
{
	int statusFlag;
	SYSTEM_INFO info;

	physicalNum = 1;
	logicalNum = 1;
	statusFlag = HT_NOT_CAPABLE;

	info.dwNumberOfProcessors = 0;
	GetSystemInfo (&info);

	// Number of physical processors in a non-Intel system
	// or in a 32-bit Intel system with Hyper-Threading technology disabled
	physicalNum = info.dwNumberOfProcessors;  

	unsigned char HT_Enabled = 0;

	logicalNum = LogicalProcPerPhysicalProc();

	if ( logicalNum >= 1 ) {	// > 1 doesn't mean HT is enabled in the BIOS
		HANDLE hCurrentProcessHandle;
		DWORD  dwAffinityMask;

		// Calculate the appropriate  shifts and mask based on the 
		// number of logical processors.

		unsigned char i = 1, PHY_ID_MASK  = 0xFF, PHY_ID_SHIFT = 0;

		while( i < logicalNum ) {
			i *= 2;
 			PHY_ID_MASK  <<= 1;
			PHY_ID_SHIFT++;
		}
		
		hCurrentProcessHandle = GetCurrentProcess();

        DWORD_PTR  dwProcessAffinity;
        DWORD_PTR  dwSystemAffinity;
		GetProcessAffinityMask( hCurrentProcessHandle, &dwProcessAffinity, &dwSystemAffinity );

		// Check if available process affinity mask is equal to the
		// available system affinity mask
		if ( dwProcessAffinity != dwSystemAffinity ) {
			statusFlag = HT_CANNOT_DETECT;
			physicalNum = -1;
			return statusFlag;
		}

		dwAffinityMask = 1;
		while ( dwAffinityMask != 0 && dwAffinityMask <= dwProcessAffinity ) {
			// Check if this CPU is available
			if ( dwAffinityMask & dwProcessAffinity ) {
				if ( SetProcessAffinityMask( hCurrentProcessHandle, dwAffinityMask ) ) {
					unsigned char APIC_ID, LOG_ID, PHY_ID;

					Sleep( 0 ); // Give OS time to switch CPU

					APIC_ID = GetAPIC_ID();
					LOG_ID  = APIC_ID & ~PHY_ID_MASK;
					PHY_ID  = APIC_ID >> PHY_ID_SHIFT;

					if ( LOG_ID != 0 ) {
						HT_Enabled = 1;
					}
				}
			}
			dwAffinityMask = dwAffinityMask << 1;
		}
	        
		// Reset the processor affinity
		SetProcessAffinityMask( hCurrentProcessHandle, dwProcessAffinity );
	    
		if ( logicalNum == 1 ) {  // Normal P4 : HT is disabled in hardware
			statusFlag = HT_DISABLED;
		} else {
			if ( HT_Enabled ) {
				// Total physical processors in a Hyper-Threading enabled system.
				physicalNum /= logicalNum;
				statusFlag = HT_ENABLED;
			} else {
				statusFlag = HT_SUPPORTED_NOT_ENABLED;
			}
		}
	}
	return statusFlag;
}*/

/*
================
HasHTT
================
*/
/*static bool HasHTT( void ) {
	unsigned regs[4];
	int logicalNum, physicalNum, HTStatusFlag;

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 28 of EDX denotes HTT existence
	if ( !( regs[_REG_EDX] & ( 1 << 28 ) ) ) {
		return false;
	}

	HTStatusFlag = CPUCount( logicalNum, physicalNum );
	if ( HTStatusFlag != HT_ENABLED ) {
		return false;
	}
	return true;
}*/

/*===============================================================================*/

/*
========================
CountSetBits 

Helper function to count set bits in the processor mask.
Revelator: Was missing begin
========================
*/
DWORD CountSetBits( ULONG_PTR bitMask ) {
	DWORD LSHIFT = sizeof( ULONG_PTR ) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    

	for ( DWORD i = 0; i <= LSHIFT; i++ ) {
		bitSetCount += ( ( bitMask & bitTest ) ? 1 : 0 );
		bitTest /= 2;
	}
	return bitSetCount;
}

typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD );

enum LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL {
    localRelationProcessorCore,
    localRelationNumaNode,
    localRelationCache,
	localRelationProcessorPackage
};

struct cpuInfo_t {
	int processorPackageCount;
	int processorCoreCount;
	int logicalProcessorCount;
	int numaNodeCount;
	struct cacheInfo_t {
		int count;
		int associativity;
		int lineSize;
		int size;
	} cacheLevel[3];
};

/*
========================
GetCPUInfo
========================
*/
bool GetCPUInfo( cpuInfo_t & cpuInfo ) {
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	PCACHE_DESCRIPTOR Cache;
	LPFN_GLPI	glpi;
	BOOL		done = FALSE;
	DWORD		returnLength = 0;
	DWORD		byteOffset = 0;

	memset( & cpuInfo, 0, sizeof( cpuInfo ) );

	glpi = (LPFN_GLPI)GetProcAddress( GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation" );
	if ( NULL == glpi ) {
		common->Printf( "\nGetLogicalProcessorInformation is not supported.\n" );
		return 0;
	}

	while ( !done ) {
		DWORD rc = glpi( buffer, &returnLength );

		if ( FALSE == rc ) {
			if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
				if ( buffer ) {
					free( buffer );
				}

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc( returnLength );
			} else {
				common->Printf( "Sys_CPUCount error: %d\n", GetLastError() );
				return false;
			}
		} else {
			done = TRUE;
		}
	}
	ptr = buffer;

	while ( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength ) {
		switch ( (LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL) ptr->Relationship ) {
			case localRelationProcessorCore:
				cpuInfo.processorCoreCount++;

				// A hyperthreaded core supplies more than one logical processor.
				cpuInfo.logicalProcessorCount += CountSetBits( ptr->ProcessorMask );
				break;

			case localRelationNumaNode:
				// Non-NUMA systems report a single record of this type.
				cpuInfo.numaNodeCount++;
				break;

			case localRelationCache:
				// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
				Cache = &ptr->Cache;
				if ( Cache->Level >= 1 && Cache->Level <= 3 ) {
					int level = Cache->Level - 1;
					if ( cpuInfo.cacheLevel[level].count > 0 ) {
						cpuInfo.cacheLevel[level].count++;
					} else {
						cpuInfo.cacheLevel[level].associativity = Cache->Associativity;
						cpuInfo.cacheLevel[level].lineSize = Cache->LineSize;
						cpuInfo.cacheLevel[level].size = Cache->Size;
					}
				}
				break;

			case localRelationProcessorPackage:
				// Logical processors share a physical package.
				cpuInfo.processorPackageCount++;
				break;

			default:
				common->Printf( "Error: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n" );
				break;
		}
		byteOffset += sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
		ptr++;
	}
	free( buffer );

	return true;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)

Revelator: was missing end
========================
*/
void Sys_CPUCount( int & numLogicalCPUCores, int & numPhysicalCPUCores, int & numCPUPackages ) {
	cpuInfo_t cpuInfo;
	GetCPUInfo( cpuInfo );

	numPhysicalCPUCores = cpuInfo.processorCoreCount;
	numLogicalCPUCores = cpuInfo.logicalProcessorCount;
	numCPUPackages = cpuInfo.processorPackageCount;
}

/*
===============================================================================

	FPU

===============================================================================
*/

typedef struct bitFlag_s {
	char 		*name;
	int			bit;
} bitFlag_t;

static byte fpuState[128], *statePtr = fpuState;
static char fpuString[2048];
static bitFlag_t controlWordFlags[] = {
	{ "Invalid operation", 0 },
	{ "Denormalized operand", 1 },
	{ "Divide-by-zero", 2 },
	{ "Numeric overflow", 3 },
	{ "Numeric underflow", 4 },
	{ "Inexact result (precision)", 5 },
	{ "Infinity control", 12 },
	{ "", 0 }
};
static char *precisionControlField[] = {
	"Single Precision (24-bits)",
	"Reserved",
	"Double Precision (53-bits)",
	"Double Extended Precision (64-bits)"
};
static char *roundingControlField[] = {
	"Round to nearest",
	"Round down",
	"Round up",
	"Round toward zero"
};
static bitFlag_t statusWordFlags[] = {
	{ "Invalid operation", 0 },
	{ "Denormalized operand", 1 },
	{ "Divide-by-zero", 2 },
	{ "Numeric overflow", 3 },
	{ "Numeric underflow", 4 },
	{ "Inexact result (precision)", 5 },
	{ "Stack fault", 6 },
	{ "Error summary status", 7 },
	{ "FPU busy", 15 },
	{ "", 0 }
};

/*
===============
Sys_FPU_PrintStateFlags
===============
*/
int Sys_FPU_PrintStateFlags( char *ptr, int ctrl, int stat, int tags, int inof, int inse, int opof, int opse ) {
	int i, length = 0;

	length += sprintf( ptr + length,	"CTRL = %08x\n"
	                   "STAT = %08x\n"
	                   "TAGS = %08x\n"
	                   "INOF = %08x\n"
	                   "INSE = %08x\n"
	                   "OPOF = %08x\n"
	                   "OPSE = %08x\n"
	                   "\n",
	                   ctrl, stat, tags, inof, inse, opof, opse );

	length += sprintf( ptr + length, "Control Word:\n" );
	for ( i = 0; controlWordFlags[i].name[0]; i++ ) {
		length += sprintf( ptr + length, "  %-30s = %s\n", controlWordFlags[i].name, ( ctrl & ( 1 << controlWordFlags[i].bit ) ) ? "true" : "false" );
	}
	length += sprintf( ptr + length, "  %-30s = %s\n", "Precision control", precisionControlField[( ctrl >> 8 ) & 3] );
	length += sprintf( ptr + length, "  %-30s = %s\n", "Rounding control", roundingControlField[( ctrl >> 10 ) & 3] );

	length += sprintf( ptr + length, "Status Word:\n" );
	for ( i = 0; statusWordFlags[i].name[0]; i++ ) {
		ptr += sprintf( ptr + length, "  %-30s = %s\n", statusWordFlags[i].name, ( stat & ( 1 << statusWordFlags[i].bit ) ) ? "true" : "false" );
	}
	length += sprintf( ptr + length, "  %-30s = %d%d%d%d\n", "Condition code", ( stat >> 8 ) & 1, ( stat >> 9 ) & 1, ( stat >> 10 ) & 1, ( stat >> 14 ) & 1 );
	length += sprintf( ptr + length, "  %-30s = %d\n", "Top of stack pointer", ( stat >> 11 ) & 7 );

	return length;
}

// Only do this in 32 bit builds
#if defined(_MSC_VER) && !defined(_WIN64)

#define MXCSR_DAZ	(1 << 6)
#define MXCSR_FTZ	(1 << 15)

#define STREFLOP_FSTCW(cw) do { short tmp; __asm { fstcw tmp }; (cw) = tmp; } while (0)
#define STREFLOP_FLDCW(cw) do { short tmp = (cw); __asm { fclex }; __asm { fldcw tmp }; } while (0)
#define STREFLOP_STMXCSR(cw) do { int tmp; __asm { stmxcsr tmp }; (cw) = tmp; } while (0)
#define STREFLOP_LDMXCSR(cw) do { int tmp = (cw); __asm { ldmxcsr tmp }; } while (0)

static void EnableMXCSRFlag( int flag, bool enable, const char *name ) {
	int sse_mode;

	STREFLOP_STMXCSR( sse_mode );

	if ( enable && ( sse_mode & flag ) == flag ) {
		common->Printf( "%s mode is already enabled\n", name );
		return;
	}

	if ( !enable && ( sse_mode & flag ) == 0 ) {
		common->Printf( "%s mode is already disabled\n", name );
		return;
	}

	if ( enable ) {
		common->Printf( "enabling %s mode\n", name );
		sse_mode |= flag;
	} else {
		common->Printf( "disabling %s mode\n", name );
		sse_mode &= ~flag;
	}
	STREFLOP_LDMXCSR( sse_mode );
}
#endif

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable ) {
#if defined(_MSC_VER) && !defined(_WIN64)
	if ( !HasDAZ() ) {
		common->Printf( "this CPU doesn't support Denormals-Are-Zero\n" );
		return;
	}

	EnableMXCSRFlag( MXCSR_DAZ, enable, "Denormals-Are-Zero" );
#endif
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable ) {
#if defined(_MSC_VER) && !defined(_WIN64)
	EnableMXCSRFlag( MXCSR_FTZ, enable, "Flush-To-Zero" );
#endif
}

/*
===============
Sys_FPU_SetPrecision
===============
*/
void Sys_FPU_SetPrecision() {
#if defined(_MSC_VER) && defined(_M_IX86)
	_controlfp( _PC_64, _MCW_PC );
#endif
}


void Sys_FPU_SetExceptions(bool enable) {
	static const DWORD bits = _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID;
	_controlfp(enable ? ~bits : ~0, _MCW_EM);
}
