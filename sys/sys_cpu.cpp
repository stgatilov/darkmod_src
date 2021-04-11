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

#include "sys_public.h"

#if defined(_MSC_VER)
	#include <intrin.h>
#elif defined(__GNUC__) && (defined(__i386__) || defined (__x86_64__))
	#include <cpuid.h>
	#include <immintrin.h>
#endif


idCVar sys_cpustring( "sys_cpustring", "detect", CVAR_SYSTEM | CVAR_INIT, "" );
static cpuid_t cpuid;

/*
================
HasCPUID
================
*/
static bool HasCPUID( void ) {
#if defined(_MSC_VER) || defined(__i386__) || defined (__x86_64__)
	// Yes, we just have it, we're not compiling on 486-compatible hardware
	return true;
#else
	// PowerPC, Elbrus and other isoteric and unsupported architectures have no CPUID
	return false;
#endif
}

#define _REG_EAX		0
#define _REG_EBX		1
#define _REG_ECX		2
#define _REG_EDX		3

/*
================
CPUID
================
*/
static void CPUID( int func, unsigned regs[4] ) {
#if !defined (_MSC_VER)
	// stgatilov: use macro from cpuid.h on GCC (and possibly Clang)
	__cpuid( func, regs[_REG_EAX], regs[_REG_EBX], regs[_REG_ECX], regs[_REG_EDX] );
#else
	// greebo: Use intrinsics on VC++
	int values[4];
	__cpuid( values, func );
	regs[_REG_EAX] = values[0];
	regs[_REG_EBX] = values[1];
	regs[_REG_ECX] = values[2];
	regs[_REG_EDX] = values[3];
#endif
}


/*
================
IsAMD
================
*/
static bool IsAMD( void ) {
	char pstring[16];
	char processorString[13];

	// get name of processor
	CPUID( 0, ( unsigned int * ) pstring );
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

	if ( strcmp( processorString, "AuthenticAMD" ) == 0 ) {
		return true;
	}
	return false;
}

/*
================
HasSSE
================
*/
static bool HasSSE( void ) {
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 25 of EDX denotes SSE existence
	if ( regs[_REG_EDX] & ( 1 << 25 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSE2
================
*/
static bool HasSSE2( void ) {
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 26 of EDX denotes SSE2 existence
	if ( regs[_REG_EDX] & ( 1 << 26 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSE3
================
*/
static bool HasSSE3( void ) {
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 0 of ECX denotes SSE3 existence
	if ( regs[_REG_ECX] & ( 1 << 0 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSSE3
================
*/
static bool HasSSSE3( void ) {
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 9 of ECX denotes SSSE3 existence
	if ( regs[_REG_ECX] & ( 1 << 9 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSE41
================
*/
static bool HasSSE41( void ) {
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 19 of ECX denotes SSE4.1 existence
	if ( regs[_REG_ECX] & ( 1 << 19 ) ) {
		return true;
	}
	return false;
}

/*
================
HasAVX
================
*/
static bool HasAVX( void ) {
	unsigned regs[4];
	// get CPU feature bits
	CPUID( 1, regs );
	// https://github.com/Mysticial/FeatureDetector/blob/master/src/x86/cpu_x86.cpp#L53

	// check if CPU supports AVX instructions
	bool cpuAVXSupport = ( regs[_REG_ECX] & ( 1 << 28 ) ) != 0;
	if ( !cpuAVXSupport )
		return false;

	// check if xsave/xrstor instructions are enabled by OS for context switches
	bool osUsesXsaveXrstor = ( regs[_REG_ECX] & ( 1 << 27 ) ) != 0;
	if ( !osUsesXsaveXrstor )
		return false;

	uint64_t xcrFeatureMask = 0;

#if defined(_MSC_VER)
	xcrFeatureMask = _xgetbv( _XCR_XFEATURE_ENABLED_MASK );
#elif defined (__GNUC__) && (defined(__i386__) || defined (__x86_64__))
	// stgatilov: GCC did not have proper _xgetbv intrinsic until GCC 9
	// inline assembly taken from: https://gist.github.com/hi2p-perim/7855506
	int index = 0;	//_XCR_XFEATURE_ENABLED_MASK
	unsigned int eax, edx;
	__asm__ __volatile__(
		"xgetbv;"
		: "=a" (eax), "=d"(edx)
		: "c" (index)
	);
	xcrFeatureMask = ((unsigned long long)edx << 32) | eax;
#endif

	// check if OS is configured to save/restore YMM registers on context switches
	if ( ( xcrFeatureMask & 0x6 ) == 0x6 )
		return true;

	return false;
}

/*
================
HasAVX2
================
*/
static bool HasAVX2( void ) {
	unsigned regs[4];

	// check that cpuid instruction supports function 7
	CPUID( 0, regs );
	if ( regs[0] < 7 ) {
		return false;
	}

	// get CPU feature bits
	CPUID( 7, regs );

	// check if CPU supports AVX2 instructions
	bool cpuAVX2Support = ( regs[_REG_EBX] & ( 1 << 5 ) ) != 0;

	// check for AVX support too (which also checks OS support)
	if ( cpuAVX2Support && HasAVX() ) {
		return true;
	}
	return false;
}

/*
================
HasFMA3
================
*/
static bool HasFMA3( void ) {
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 12 of ECX denotes FMA3 support
	bool cpuFMA3Support = ( regs[_REG_ECX] & ( 1 << 12 ) ) != 0;

	// stgatilov: ensure that AVX2 is enabled too
	// (also checks for OS support of AVX registers)
	if ( cpuFMA3Support && HasAVX2() ) {
		return true;
	}
	return false;
}

/*
================
LogicalProcPerPhysicalProc
================
*/
// processors per physical processor when execute cpuid with
// eax set to 1
static unsigned char LogicalProcPerPhysicalProc( void ) {
	unsigned regs[4];

	CPUID( 1, regs );

	// EBX[23:16] Bit 16-23 in ebx contains the number of logical
	return ( unsigned char )( ( regs[_REG_EBX] & 0x00FF0000 ) >> 16 );
}

/*
================
GetAPIC_ID
================
*/
// initial APIC ID for the processor this code is running on.
// Default value = 0xff if HT is not supported
static unsigned char GetAPIC_ID( void ) {
	unsigned regs[4];

	CPUID( 1, regs );

	// EBX[31:24] Bits 24-31 (8 bits) return the 8-bit unique 
	return ( unsigned char )( ( regs[_REG_EBX] & 0xFF000000 ) >> 24 );
}

/*
================
HasDAZ
================
*/
static bool HasDAZ( void ) {
	ALIGNTYPE16 unsigned char FXSaveArea[512];
	unsigned char *FXArea = FXSaveArea;
	uint32 dwMask = 0;
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 24 of EDX denotes support for FXSAVE
	if ( !( regs[_REG_EDX] & ( 1 << 24 ) ) ) {
		return false;
	}

	memset( FXArea, 0, sizeof( FXSaveArea ) );

	// stgatilov: mxcsr was released with SSE, and did include only FTZ flag
	// when P4 with SSE2 came out, DAZ flag was added, but its support had to be checked via mxcsr_mask
	// so the fxsave instruction here extracts mxcsr_mask in order to check for DAZ support
	// most likely this check is unnecessary today...
#if defined(_MSC_VER)
	#if defined(_WIN64)
		_fxsave( FXArea );
	#else
		__asm {
			mov		eax, FXArea
			FXSAVE	[eax]
		}
	#endif
	dwMask = *( uint32 * )&FXArea[28];						// Read the MXCSR Mask
	return ( ( dwMask & ( 1 << 6 ) ) == ( 1 << 6 ) );	// Return if the DAZ bit is set
#else
	// stgatilov: if we have SSE2, then we hopefully have DAZ too
	return HasSSE2();
#endif
}


/*
================
Sys_GetCPUId
================
*/
static cpuid_t Sys_GetCPUId( void ) {
	int flags;

	// verify we're at least a Pentium or 486 with CPUID support
	// stgatilov: isoteric/unsupported architectures bail out here (must not execute CPUID instruction)
	if ( !HasCPUID() ) {
		return CPUID_UNSUPPORTED;
	}

	// check for an AMD
	if ( IsAMD() ) {
		flags = CPUID_AMD;
	} else {
		flags = CPUID_INTEL;
	}

	// check for Streaming SIMD Extensions
	if ( HasSSE() ) {
		flags |= CPUID_SSE | CPUID_FTZ;
	}

	// check for Streaming SIMD Extensions 2
	if ( HasSSE2() ) {
		flags |= CPUID_SSE2;
	}

	// check for Streaming SIMD Extensions 3 aka Prescott's New Instructions
	if ( HasSSE3() ) {
		flags |= CPUID_SSE3;
	}

	// check for Supplemental Streaming SIMD Extensions 3
	if ( HasSSSE3() ) {
		flags |= CPUID_SSSE3;
	}

	// check for Streaming SIMD Extensions 4.1
	if ( HasSSE41() ) {
		flags |= CPUID_SSE41;
	}

	// check for AVX
	if ( HasAVX() ) {
		flags |= CPUID_AVX;
	}

	// check for FMA3
	if ( HasFMA3() ) {
		flags |= CPUID_FMA3;
	}

	// check for AVX
	if ( HasAVX2() ) {
		flags |= CPUID_AVX2;
	}

	// check for Denormals-Are-Zero mode
	if ( HasDAZ() ) {
		flags |= CPUID_DAZ;
	}
	return ( cpuid_t )flags;
}



//===================================================================================

/*
================
Sys_InitCPUID
================
*/
void Sys_InitCPUID() {
	if ( !idStr::Icmp( sys_cpustring.GetString(), "detect" ) ) {
		idStr string;

		common->Printf( "%1.0f MHz ", Sys_ClockTicksPerSecond() / 1000000.0f );

		cpuid = Sys_GetCPUId();

		string.Clear();

		if ( cpuid & CPUID_AMD ) {
			string += "AMD CPU";
		} else if ( cpuid & CPUID_INTEL ) {
			string += "Intel CPU";
		} else if ( cpuid & CPUID_UNSUPPORTED ) {
			string += "unsupported CPU";
		} else {
			string += "generic CPU";
		}
		string += " with ";

		if ( cpuid & CPUID_SSE ) {
			string += "SSE & ";
		}
		if ( cpuid & CPUID_SSE2 ) {
			string += "SSE2 & ";
		}
		if ( cpuid & CPUID_SSE3 ) {
			string += "SSE3 & ";
		}
		if ( cpuid & CPUID_SSSE3 ) {
			string += "SSSE3 & ";
		}
		if ( cpuid & CPUID_SSE41 ) {
			string += "SSE41 & ";
		}
		if ( cpuid & CPUID_AVX ) {
			string += "AVX & ";
		}
		if ( cpuid & CPUID_AVX2 ) {
			string += "AVX2 & ";
		}
		if ( cpuid & CPUID_FMA3 ) {
			string += "FMA3 & ";
		}
		string.StripTrailing( " & " );
		string.StripTrailing( " with " );
		sys_cpustring.SetString( string );
	} else {
		common->Printf( "forcing CPU type to " );
		idLexer src( sys_cpustring.GetString(), idStr::Length( sys_cpustring.GetString() ), "sys_cpustring" );
		idToken token;

		int id = CPUID_NONE;
		while ( src.ReadToken( &token ) ) {
			if ( token.Icmp( "generic" ) == 0 ) {
				id |= CPUID_GENERIC;
			} else if ( token.Icmp( "intel" ) == 0 ) {
				id |= CPUID_INTEL;
			} else if ( token.Icmp( "amd" ) == 0 ) {
				id |= CPUID_AMD;
			} else if ( token.Icmp( "sse" ) == 0 ) {
				id |= CPUID_SSE;
			} else if ( token.Icmp( "sse2" ) == 0 ) {
				id |= CPUID_SSE2;
			} else if ( token.Icmp( "sse3" ) == 0 ) {
				id |= CPUID_SSE3;
			} else if ( token.Icmp( "ssse3" ) == 0 ) {
				id |= CPUID_SSSE3;
			} else if ( token.Icmp( "sse41" ) == 0 ) {
				id |= CPUID_SSE41;
			} else if ( token.Icmp( "avx" ) == 0 ) {
				id |= CPUID_AVX;
			} else if ( token.Icmp( "avx2" ) == 0 ) {
				id |= CPUID_AVX2;
			} else if ( token.Icmp( "fma3" ) == 0 ) {
				id |= CPUID_FMA3;
			}
		}
		if ( id == CPUID_NONE ) {
			common->Printf( "WARNING: unknown sys_cpustring '%s'\n", sys_cpustring.GetString() );
			id = CPUID_GENERIC;
		}
		cpuid = ( cpuid_t ) id;
	}

	common->Printf( "%s\n", sys_cpustring.GetString() );
}

/*
================
Sys_GetProcessorId
================
*/
cpuid_t Sys_GetProcessorId( void ) {
	return cpuid;
}

/*
================
Sys_GetProcessorString
================
*/
const char *Sys_GetProcessorString( void ) {
	return sys_cpustring.GetString();
}
