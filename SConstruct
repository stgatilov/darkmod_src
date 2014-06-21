# -*- mode: python -*-
# DOOM build script
# TTimo <ttimo@idsoftware.com>
# http://scons.sourceforge.net

import sys, os, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile, string
import SCons

sys.path.append( 'sys/scons' )
import scons_utils

conf_filename='site.conf'
# choose configuration variables which should be saved between runs
# ( we handle all those as strings )
serialized=['CC', 'CXX', 'JOBS', 'BUILD', 'IDNET_HOST', 'GL_HARDLINK', 'DEDICATED',
	'DEBUG_MEMORY', 'LIBC_MALLOC', 'ID_NOLANADDRESS', 'ID_MCHECK', 'ALSA',
	'TARGET_CORE', 'TARGET_GAME', 'TARGET_MONO', 'TARGET_DEMO', 'NOCURL',
	'BUILD_ROOT', 'BUILD_GAMEPAK', 'BASEFLAGS', 'SILENT', 'NO_GCH', 'OPENMP' ]

# global build mode ------------------------------

g_sdk = not os.path.exists( 'sys/scons/SConscript.core' )

# ------------------------------------------------

# help -------------------------------------------

help_string = """
Usage: scons [OPTIONS] [TARGET] [CONFIG]

[OPTIONS] and [TARGET] are covered in command line options, use scons -H

[CONFIG]: KEY="VALUE" [...]
a number of configuration options saved between runs in the """ + conf_filename + """ file
erase """ + conf_filename + """ to start with default settings again

CC (default gcc)
CXX (default g++)
	Specify C and C++ compilers (defaults gcc and g++)
	ex: CC="gcc-3.3"
	You can use ccache and distcc, for instance:
	CC="ccache distcc gcc" CXX="ccache distcc g++"

JOBS (default 1)
	Parallel build

BUILD (default debug)
	Use debug-all/debug/release/profile to select build settings
	ex: BUILD="release"
	debug-all: no optimisations, debugging symbols
	debug: -O -g
	profile: -pg -fprofile-arcs (to produce gmon.out, which can be analyzed with gprof)
	release: all optimisations, including CPU target etc.

BUILD_ROOT (default 'build')
	change the build root directory

TARGET_GAME (default 1)
	Build the base game code

BUILD_GAMEPAK (default 0)
	Build a game pak

BASEFLAGS (default '')
	Add compile flags

NOCONF (default 0, not saved)
	ignore site configuration and use defaults + command line only
	
SILENT ( default 0, saved )
	hide the compiler output, unless error

NO_GCH (default 0)
	Don't use precompiled headers when building.
    
OPENMP (default 0)
	Enable OpenMP builds.
"""

if ( not g_sdk ):
	help_string += """
DEDICATED (default 0)
	Control regular / dedicated type of build:
	0 - client
	1 - dedicated server
	2 - both

TARGET_CORE (default 1)
	Build the core

TARGET_MONO (default 0)
	Build a monolithic binary

TARGET_DEMO (default 0)
	Build demo client ( both a core and game, no mono )
	NOTE: if you *only* want the demo client, set TARGET_CORE and TARGET_GAME to 0

IDNET_HOST (default to source hardcoded)
	Override builtin IDNET_HOST with your own settings
	
GL_HARDLINK (default 0)
	Instead of dynamically loading the OpenGL libraries, use implicit dependencies
	NOTE: no GL logging capability and no r_glDriver with GL_HARDLINK 1

DEBUG_MEMORY (default 0)
	Enables memory logging to file
	
LIBC_MALLOC (default 1)
	Toggle idHeap memory / libc malloc usage
	When libc malloc is on, memory size statistics are wrong ( no _msize )

ID_NOLANADDRESS (default 0)
	Don't recognize any IP as LAN address. This is useful when debugging network
	code where LAN / not LAN influences application behaviour
	
ID_MCHECK (default 2)
	Perform heap consistency checking
	0: on in Debug / off in Release
	1 forces on, 2 forces off
	note that Doom has it's own block allocator/checking
	this should not be considered a replacement, but an additional tool

ALSA (default 1)
	enable ALSA sound backend support
	
SETUP (default 0, not saved)
    build a setup. implies release build

SDK (default 0, not saved)
	build an SDK release

NOCURL (default 0)
	set to 1 to disable usage of libcurl and http/ftp downloads feature
"""

Help( help_string )

# end help ---------------------------------------

# sanity -----------------------------------------

EnsureSConsVersion( 0, 96 )

# end sanity -------------------------------------

# system detection -------------------------------

# CPU type
cpu = commands.getoutput('uname -m')
exp = re.compile('.*i?86.*')
if exp.match(cpu):
	cpu = 'x86'
else:
	cpu = commands.getoutput('uname -p')
	if ( cpu == 'powerpc' ):
		cpu = 'ppc'
	else:
		cpu = 'cpu'
g_os = 'Linux'

# end system detection ---------------------------

# default settings -------------------------------

CC = 'gcc'
CXX = 'g++'
JOBS = '1'
BUILD = 'debug'
DEDICATED = '0'
TARGET_CORE = '1'
TARGET_GAME = '1'
TARGET_MONO = '0'
TARGET_DEMO = '0'
IDNET_HOST = ''
GL_HARDLINK = '0'
DEBUG_MEMORY = '0'
LIBC_MALLOC = '1'
ID_NOLANADDRESS = '0'
ID_MCHECK = '2'
BUILD_ROOT = 'build'
ALSA = '1'
SETUP = '0'
SDK = '0'
NOCONF = '0'
NOCURL = '0'
BUILD_GAMEPAK = '0'
BASEFLAGS = ''
SILENT = '0'
NO_GCH = '0'
OPENMP = '0'

# end default settings ---------------------------

# site settings ----------------------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	site_dict = {}
	if (os.path.exists(conf_filename)):
		site_file = open(conf_filename, 'r')
		p = pickle.Unpickler(site_file)
		site_dict = p.load()
		print 'Loading build configuration from ' + conf_filename + ':'
		for k, v in site_dict.items():
			exec_cmd = k + '=\'' + v + '\''
			print '  ' + exec_cmd
			exec(exec_cmd)
else:
	print 'Site settings ignored'

# end site settings ------------------------------

# command line settings --------------------------

for k in ARGUMENTS.keys():
	exec_cmd = k + '=\'' + ARGUMENTS[k] + '\''
	print 'Command line: ' + exec_cmd
	exec( exec_cmd )

# end command line settings ----------------------

# save site configuration ----------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	for k in serialized:
		exec_cmd = 'site_dict[\'' + k + '\'] = ' + k
		exec(exec_cmd)

	site_file = open(conf_filename, 'w')
	p = pickle.Pickler(site_file)
	p.dump(site_dict)
	site_file.close()

# end save site configuration ------------------

# configuration rules --------------------------

if ( SETUP != '0' ):
	DEDICATED	= '2'
	BUILD		= 'release'

if ( g_sdk or SDK != '0' ):
	TARGET_CORE = '0'
	TARGET_GAME = '1'
	TARGET_MONO = '0'
	TARGET_DEMO = '0'

# end configuration rules ----------------------

# general configuration, target selection --------

g_build = BUILD_ROOT + '/' + BUILD

SConsignFile( 'scons.signatures' )

if ( GL_HARDLINK != '0' ):
	g_build += '-hardlink'

if ( DEBUG_MEMORY != '0' ):
	g_build += '-debugmem'
	
if ( LIBC_MALLOC != '1' ):
	g_build += '-nolibcmalloc'

SetOption('num_jobs', JOBS)

LINK = CXX

# common flags
# BASE + CORE + OPT for engine
# BASE + GAME + OPT for game
# _noopt versions of the environements are built without the OPT

BASECPPFLAGS = [ ]
CORECPPPATH = [ ]
CORELIBPATH = [ ]
CORECPPFLAGS = [ ]
GAMECPPFLAGS = [ ]
BASELINKFLAGS = [ ]
CORELINKFLAGS = [ ]

# for release build, further optimisations that may not work on all files
OPTCPPFLAGS = [ ]

BASECPPFLAGS.append( BASEFLAGS )
BASECPPFLAGS.append( '-pipe' )
# warn all
BASECPPFLAGS.append( '-Wall' )
BASECPPFLAGS.append( '-Wno-unknown-pragmas' )
# this define is necessary to make sure threading support is enabled in X
CORECPPFLAGS.append( '-DXTHREADS' )
# don't wrap gcc messages
BASECPPFLAGS.append( '-fmessage-length=0' )
# gcc 4.0
BASECPPFLAGS.append( '-fpermissive' )

if ( g_os == 'Linux' ):
	# gcc 4.x option only - only export what we mean to from the game SO
	BASECPPFLAGS.append( '-fvisibility=hidden' )
	# get the 64 bits machine on the distcc array to produce 32 bit binaries :)
	BASECPPFLAGS.append( '-m32' )
	BASELINKFLAGS.append( '-m32' )
    
	if ( OPENMP != '0' ):
		# openmp support for changes made to the renderer
		BASECPPFLAGS.append( '-fopenmp' )
		BASELINKFLAGS.append( '-fopenmp' )

if ( g_sdk or SDK != '0' ):
	BASECPPFLAGS.append( '-D_D3SDK' )

if ( BUILD == 'debug-all' ):
	OPTCPPFLAGS = [ '-g', '-D_DEBUG' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'debug' ):
	OPTCPPFLAGS = [ '-g', '-O1', '-D_DEBUG' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'profile' ):
	# -fprofile-arcs is needed for gcc 3.x and 4.x
	OPTCPPFLAGS = [ '-pg', '-fprofile-arcs', '-ftest-coverage', '-O1', '-D_DEBUG' ]
	BASELINKFLAGS.append( '-pg' );
	BASELINKFLAGS.append( '-fprofile-arcs' );
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'release' ):
	# -fomit-frame-pointer: "-O also turns on -fomit-frame-pointer on machines where doing so does not interfere with debugging."
	#   on x86 have to set it explicitely
	# -finline-functions: implicit at -O3
	# -fschedule-insns2: implicit at -O2
	# no-unsafe-math-optimizations: that should be on by default really. hit some wonko bugs in physics code because of that
	# greebo: Took out -Winline, this is spamming real hard
	OPTCPPFLAGS = [ '-O3', '-march=pentium3', '-ffast-math', '-fno-unsafe-math-optimizations', '-fomit-frame-pointer' ] 
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '2'
else:
	print 'Unknown build configuration ' + BUILD
	sys.exit(0)

if ( GL_HARDLINK != '0' ):
	CORECPPFLAGS.append( '-DID_GL_HARDLINK' )

if ( DEBUG_MEMORY != '0' ):
	BASECPPFLAGS += [ '-DID_DEBUG_MEMORY', '-DID_REDIRECT_NEWDELETE' ]
	
if ( LIBC_MALLOC != '1' ):
	BASECPPFLAGS.append( '-DUSE_LIBC_MALLOC=0' )

if ( len( IDNET_HOST ) ):
	CORECPPFLAGS.append( '-DIDNET_HOST=\\"%s\\"' % IDNET_HOST)

if ( ID_NOLANADDRESS != '0' ):
	CORECPPFLAGS.append( '-DID_NOLANADDRESS' )
	
if ( ID_MCHECK == '1' ):
	BASECPPFLAGS.append( '-DID_MCHECK' )
	
# create the build environements
g_base_env = Environment( ENV = os.environ, CC = CC, CXX = CXX, LINK = LINK, CPPFLAGS = BASECPPFLAGS, LINKFLAGS = BASELINKFLAGS, CPPPATH = CORECPPPATH, LIBPATH = CORELIBPATH )
scons_utils.SetupUtils( g_base_env )

#######################################################################################
# greebo: Precompiled header related
#
# This is taken from GchBuilder 1.1, the SCons builder for gcc's precompiled headers
# Copyright (C) 2006  Tim Blechmann
#

import SCons.Action
import SCons.Builder
import SCons.Scanner.C
import SCons.Util
import SCons.Script
import os

GchAction = SCons.Action.Action('$GCHCOM', '$GCHCOMSTR')
GchShAction = SCons.Action.Action('$GCHSHCOM', '$GCHSHCOMSTR')

def gen_suffix(env, sources):
    return sources[0].get_suffix() + env['GCHSUFFIX']

GchShBuilder = SCons.Builder.Builder(action = GchShAction,
                                     source_scanner = SCons.Scanner.C.CScanner(),
                                     suffix = gen_suffix)

GchBuilder = SCons.Builder.Builder(action = GchAction,
                                   source_scanner = SCons.Scanner.C.CScanner(),
                                   suffix = gen_suffix)

def header_path(node):
    h_path = node.abspath
    idx = h_path.rfind('.gch')
    if idx != -1:
        h_path = h_path[0:idx]
        if not os.path.isfile(h_path):
            raise SCons.Errors.StopError("can't find header file: {0}".format(h_path))
        return h_path

    else:
        raise SCons.Errors.StopError("{0} file doesn't have .gch extension".format(h_path))


def static_pch_emitter(target,source,env):
    SCons.Defaults.StaticObjectEmitter( target, source, env )
    scanner = SCons.Scanner.C.CScanner()
    path = scanner.path(env)

    deps = scanner(source[0], env, path)
    if env.get('Gch'):
        h_path = header_path(env['Gch'])
        if h_path in [x.abspath for x in deps]:
            #print 'Found dep. on pch: ', target[0], ' -> ', env['Gch']
            env.Depends(target, env['Gch'])

    return (target, source)

def shared_pch_emitter(target,source,env):
    SCons.Defaults.SharedObjectEmitter( target, source, env )

    scanner = SCons.Scanner.C.CScanner()
    path = scanner.path(env)
    deps = scanner(source[0], env, path)

    if env.get('GchSh'):
        h_path = header_path(env['GchSh'])
        if h_path in [x.abspath for x in deps]:
            #print 'Found dep. on pch (shared): ', target[0], ' -> ', env['Gch']
            env.Depends(target, env['GchSh'])

    return (target, source)

def GchGenerate(env):
    """
    Add builders and construction variables for the Gch builder.
    """
    env.Append(BUILDERS = {
        'gch': env.Builder(
        action = GchAction,
        target_factory = env.fs.File,
        ),
        'gchsh': env.Builder(
        action = GchShAction,
        target_factory = env.fs.File,
        ),
        })

    try:
        bld = env['BUILDERS']['Gch']
        bldsh = env['BUILDERS']['GchSh']
    except KeyError:
        bld = GchBuilder
        bldsh = GchShBuilder
        env['BUILDERS']['Gch'] = bld
        env['BUILDERS']['GchSh'] = bldsh

    env['GCHCOM']     = '$CXX -Wall -o $TARGET -x c++-header -c $CXXFLAGS $CCFLAGS $_CCCOMCOM $SOURCE'
    env['GCHSHCOM']   = '$CXX -o $TARGET -x c++-header -c $SHCXXFLAGS $CCFLAGS $_CCCOMCOM $SOURCE'
    env['GCHSUFFIX']  = '.gch'

    for suffix in SCons.Util.Split('.c .C .cc .cxx .cpp .c++'):
        env['BUILDERS']['StaticObject'].add_emitter( suffix, static_pch_emitter )
        env['BUILDERS']['SharedObject'].add_emitter( suffix, shared_pch_emitter )

GchGenerate(g_base_env)

#######################################################################################

g_base_env.Prepend(CPPPATH=['.'])	# Makes sure the precompiled headers are found first
g_base_env.Append(CPPPATH = '#/include')
g_base_env.Append(CPPPATH = '#/include/zlib')
g_base_env.Append(CPPPATH = '#/include/minizip')
g_base_env.Append(CPPPATH = '#/include/libjpeg')
g_base_env.Append(CPPPATH = '#/include/devil')
g_base_env.Append(CPPPATH = '#/')

g_env = g_base_env.Clone()

g_env['CPPFLAGS'] += OPTCPPFLAGS
g_env['CPPFLAGS'] += CORECPPFLAGS
g_env['LINKFLAGS'] += CORELINKFLAGS

g_env_noopt = g_base_env.Clone()
g_env_noopt['CPPFLAGS'] += CORECPPFLAGS

g_game_env = g_base_env.Clone()
g_game_env['CPPFLAGS'] += OPTCPPFLAGS
g_game_env['CPPFLAGS'] += GAMECPPFLAGS

# maintain this dangerous optimization off at all times
g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_env_noopt.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_game_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

#if ( int(JOBS) > 1 ):
#	print 'Using buffered process output'
#	silent = False
#	if ( SILENT == '1' ):
#		silent = True
#	scons_utils.SetupBufferedOutput( g_env, silent )
#	scons_utils.SetupBufferedOutput( g_game_env, silent )

# mark the globals

local_dedicated = 0
# 0 for monolithic build
local_gamedll = 1
# carry around rather than using .a, avoids binutils bugs
idlib_objects = []
game_objects = []
local_demo = 0
# curl usage. there is a global toggle flag
local_curl = 0
curl_lib = []
# if idlib should produce PIC objects ( depending on core or game inclusion )
local_idlibpic = 0

GLOBALS = 'g_env g_env_noopt g_game_env g_os ID_MCHECK ALSA idlib_objects game_objects local_dedicated local_gamedll local_demo local_idlibpic curl_lib local_curl OPTCPPFLAGS NO_GCH'

# end general configuration ----------------------

# targets ----------------------------------------

Export( 'GLOBALS ' + GLOBALS )

doom = None
doomded = None
game = None
doom_mono = None
doom_demo = None
game_demo = None

# build curl if needed
if ( NOCURL == '0' and ( TARGET_CORE == '1' or TARGET_MONO == '1' ) ):
	# 1: debug, 2: release
	if ( BUILD == 'release' ):
		local_curl = 2
	else:
		local_curl = 1
	Export( 'GLOBALS ' + GLOBALS )
	curl_lib = [ '#linux/libcurl/libcurl.a' ] # Use the static one built for TDM

if ( TARGET_CORE == '1' ):
	local_gamedll = 1
	local_demo = 0
	local_idlibpic = 0
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		local_dedicated = 0
		Export( 'GLOBALS ' + GLOBALS )
		
		VariantDir( g_build + '/core/glimp', '.', duplicate = 1 )
		SConscript( g_build + '/core/glimp/sys/scons/SConscript.gl' )
		VariantDir( g_build + '/core', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/core/sys/scons/SConscript.idlib' )
		Export( 'GLOBALS ' + GLOBALS ) # update idlib_objects
		doom = SConscript( g_build + '/core/sys/scons/SConscript.core' )

		InstallAs( '#thedarkmod.' + cpu, doom )
		
	if ( DEDICATED == '1' or DEDICATED == '2' ):
		local_dedicated = 1
		Export( 'GLOBALS ' + GLOBALS )
		
		VariantDir( g_build + '/dedicated/glimp', '.', duplicate = 1 )
		SConscript( g_build + '/dedicated/glimp/sys/scons/SConscript.gl' )
		VariantDir( g_build + '/dedicated', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/dedicated/sys/scons/SConscript.idlib' )
		Export( 'GLOBALS ' + GLOBALS )
		doomded = SConscript( g_build + '/dedicated/sys/scons/SConscript.core' )

		InstallAs( '#doomded.' + cpu, doomded )

if ( TARGET_GAME == '1' ):
	local_gamedll = 1
	local_demo = 0
	local_dedicated = 0
	local_idlibpic = 1
	Export( 'GLOBALS ' + GLOBALS )
	dupe = 0
	if ( SDK == '1' ):
		# building an SDK, use scons for dependencies walking
		# clear the build directory to be safe
		g_env.PreBuildSDK( [ g_build + '/game' ] )
		dupe = 1
	VariantDir( g_build + '/game', '.', duplicate = dupe )
	idlib_objects = SConscript( g_build + '/game/sys/scons/SConscript.idlib' )
	if ( TARGET_GAME == '1' ):
		Export( 'GLOBALS ' + GLOBALS )
		game = SConscript( g_build + '/game/sys/scons/SConscript.game' )
		game_base = InstallAs( '#game%s-base.so' % cpu, game )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#tdm_game02.pk4', [ game_base, game ], Action( g_env.BuildGamePak ) )
	
if ( TARGET_MONO == '1' ):
	# NOTE: no D3XP atm. add a TARGET_MONO_D3XP
	local_gamedll = 0
	local_dedicated = 0
	local_demo = 0
	local_idlibpic = 0
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/mono/glimp', '.', duplicate = 1 )
	SConscript( g_build + '/mono/glimp/sys/scons/SConscript.gl' )
	VariantDir( g_build + '/mono', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/mono/sys/scons/SConscript.idlib' )
	game_objects = SConscript( g_build + '/mono/sys/scons/SConscript.game' )
	Export( 'GLOBALS ' + GLOBALS )
	doom_mono = SConscript( g_build + '/mono/sys/scons/SConscript.core' )
	InstallAs( '#doom-mon.' + cpu, doom_mono )

if ( TARGET_DEMO == '1' ):
	# NOTE: no D3XP atm. add a TARGET_DEMO_D3XP
	local_demo = 1
	local_dedicated = 0
	local_gamedll = 1
	local_idlibpic = 0
	local_curl = 0
	curl_lib = []
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/demo/glimp', '.', duplicate = 1 )
	SConscript( g_build + '/demo/glimp/sys/scons/SConscript.gl' )
	VariantDir( g_build + '/demo', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/demo/sys/scons/SConscript.idlib' )
	Export( 'GLOBALS ' + GLOBALS )
	doom_demo = SConscript( g_build + '/demo/sys/scons/SConscript.core' )

	InstallAs( '#doom-demo.' + cpu, doom_demo )
	
	local_idlibpic = 1
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/demo/game', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/demo/game/sys/scons/SConscript.idlib' )
	Export( 'GLOBALS ' + GLOBALS )
	game_demo = SConscript( g_build + '/demo/game/sys/scons/SConscript.game' )

	InstallAs( '#game%s-demo.so' % cpu, game_demo )

if ( SETUP != '0' ):
	brandelf = Program( 'brandelf', 'sys/linux/setup/brandelf.c' )
	if ( TARGET_CORE == '1' and TARGET_GAME == '1' ):
		setup = Command( 'setup', [ brandelf, doom, doomded, game, d3xp ], Action( g_env.BuildSetup ) )
	else:
		print 'Skipping main setup: TARGET_CORE == 0 or TARGET_GAME == 0'
	if ( TARGET_DEMO == '1' ):
		setup_demo = Command( 'setup-demo', [ brandelf, doom_demo, game_demo ], Action( g_env.BuildSetup ) )
		# if building two setups, make sure JOBS doesn't parallelize them
		try:
			g_env.Depends( setup_demo, setup )
		except:
			pass
	else:
		print 'Skipping demo setup ( TARGET_DEMO == 0 )'

if ( SDK != '0' ):
	setup_sdk = Command( 'sdk', [ ], Action( g_env.BuildSDK ) )
	g_env.Depends( setup_sdk, [ game, d3xp ] )

# end targets ------------------------------------
