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
serialized=['CC', 'CXX', 'JOBS', 'BUILD', 'GL_HARDLINK',
	'DEBUG_MEMORY', 'LIBC_MALLOC', 'ID_MCHECK', 'NOCURL',
	'BUILD_ROOT', 'BASEFLAGS', 'SILENT', 'NO_GCH', 'OPENMP',
	'TARGET_ARCH' ]

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

TARGET_ARCH (default: "x86")
	Build for either x86 or x64 architecture.

GL_HARDLINK (default 0)
	Instead of dynamically loading the OpenGL libraries, use implicit dependencies
	NOTE: no GL logging capability and no r_glDriver with GL_HARDLINK 1

DEBUG_MEMORY (default 0)
	Enables memory logging to file
	
LIBC_MALLOC (default 1)
	Toggle idHeap memory / libc malloc usage
	When libc malloc is on, memory size statistics are wrong ( no _msize )

ID_MCHECK (default 2)
	Perform heap consistency checking
	0: on in Debug / off in Release
	1 forces on, 2 forces off
	note that Doom has it's own block allocator/checking
	this should not be considered a replacement, but an additional tool

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
GL_HARDLINK = '0'
DEBUG_MEMORY = '0'
LIBC_MALLOC = '1'
ID_MCHECK = '2'
BUILD_ROOT = 'build'
NOCONF = '0'
NOCURL = '0'
BASEFLAGS = ''
SILENT = '0'
NO_GCH = '0'
OPENMP = '0'
TARGET_ARCH = 'x86'

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

# stgatilov: avoid annoying human errors when you set target='x32'
# and get surprised that it does not build properly =)
if TARGET_ARCH == 'x32':
	TARGET_ARCH = 'x86'

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

# general configuration, target selection --------

g_build = BUILD_ROOT + '/scons_' + TARGET_ARCH + '/' + BUILD

# 
# Use an absolute path to a single signature file to avoid a problem
# whereby everything gets rebuilt every time due to the signature file
# not being detected.
# 
SConsignFile(os.path.join(Dir('#').abspath, 'scons.signatures'))

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
BASELINKFLAGS = [ ]
CORELINKFLAGS = [ ]

# for release build, further optimisations that may not work on all files
OPTCPPFLAGS = [ ]

BASECPPFLAGS.append( BASEFLAGS )
BASECPPFLAGS.append( '-pipe' )
# warn all
BASECPPFLAGS.append( '-Wall' )
BASECPPFLAGS.append( '-Wno-unknown-pragmas' )               # MSVC-specific pragmas
BASECPPFLAGS.append( '-Wno-unused-variable' )               # too many, often happens from commenting out code
BASECPPFLAGS.append( '-Wno-unused-but-set-variable' )       # useful for debugging, also may remain after commenting stuff
BASECPPFLAGS.append( '-Wno-sign-compare' )                  # very nasty warning in the world of STL and size_t

# this define is necessary to make sure threading support is enabled in X
CORECPPFLAGS.append( '-DXTHREADS' )
# don't wrap gcc messages
BASECPPFLAGS.append( '-fmessage-length=0' )
# C++11 features
BASECPPFLAGS.append( '-std=c++11' )
# maintain this dangerous optimization off at all times
BASECPPFLAGS.append( '-fno-strict-aliasing' )

if ( g_os == 'Linux' ):
	# use old ABI for std::string and std::list (which is not fully compliant with C++11 standard)
	# this allows to run TDM binary on OSes which have old glibcxx (e.g. Ubuntu 14.04 from years 2014-2016)
	BASECPPFLAGS.append( '-D_GLIBCXX_USE_CXX11_ABI=0' )
	# gcc 4.x option only - only export what we mean to from the game SO
	BASECPPFLAGS.append( '-fvisibility=hidden' )
	# get the 64 bits machine on the distcc array to produce 32 bit binaries :)
	if ( TARGET_ARCH == 'x86' ):
		BASECPPFLAGS.append( '-m32' )
		BASELINKFLAGS.append( '-m32' )
		BASECPPFLAGS.append( '-msse2' );
	if ( TARGET_ARCH == 'x64' ):
		BASECPPFLAGS.append( '-m64' )
		BASELINKFLAGS.append( '-m64' )
		# current ffmpeg dep was built without -fPIC, so can't use position-independent code generation
		BASELINKFLAGS.append( '-no-pie' )
    
	if ( OPENMP != '0' ):
		# openmp support for changes made to the renderer
		BASECPPFLAGS.append( '-fopenmp' )
		BASELINKFLAGS.append( '-fopenmp' )

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
	# -fomit-frame-pointer: not set because if prohibits debugging (which is necessary even on release build)
	# -finline-functions: implicit at -O3
	# -fschedule-insns2: implicit at -O2
	# -fno-unsafe-math-optimizations: that should be on by default really. hit some wonko bugs in physics code because of that
	OPTCPPFLAGS = [ '-g', '-O3', '-ffast-math', '-fno-unsafe-math-optimizations' ] 
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

if ( ID_MCHECK == '1' ):
	BASECPPFLAGS.append( '-DID_MCHECK' )
	
# create the build environements
g_env_base = Environment( ENV = os.environ, CC = CC, CXX = CXX, LINK = LINK, CPPFLAGS = BASECPPFLAGS, LINKFLAGS = BASELINKFLAGS, CPPPATH = CORECPPPATH, LIBPATH = CORELIBPATH )
scons_utils.SetupUtils( g_env_base )

g_env_base.Prepend(CPPPATH=['.'])
g_env_base.Append(CPPPATH = '#/include')
g_env_base.Append(CPPPATH = '#/include/zlib')
g_env_base.Append(CPPPATH = '#/include/libjpeg')
g_env_base.Append(CPPPATH = '#/include/libpng')
g_env_base.Append(CPPPATH = '#/include/devil')
g_env_base.Append(CPPPATH = '#/include/ffmpeg')
g_env_base.Append(CPPPATH = '#/')

g_env_base['CPPFLAGS'] += OPTCPPFLAGS
g_env_base['CPPFLAGS'] += CORECPPFLAGS
g_env_base['LINKFLAGS'] += CORELINKFLAGS


#if ( int(JOBS) > 1 ):
#	print 'Using buffered process output'
#	silent = False
#	if ( SILENT == '1' ):
#		silent = True
#	scons_utils.SetupBufferedOutput( g_env, silent )
#	scons_utils.SetupBufferedOutput( g_game_env, silent )

# mark the globals

# curl usage. there is a global toggle flag
local_curl = 0
curl_lib = []

GLOBALS = 'g_env_base g_os ID_MCHECK curl_lib local_curl NO_GCH TARGET_ARCH'

# end general configuration ----------------------

# targets ----------------------------------------

Export( 'GLOBALS ' + GLOBALS )

thedarkmod = None

# insert SVN revision number into header (requires svnversion)
# svnversion must be run from SVN root, otherwise revision information may be incomplete
os.chdir(Dir('#').abspath)
svnversion_command = r'sed "s/\!SVNVERSION\!/$(svnversion)/g" idlib/svnversion_template.h >idlib/svnversion.h'
if os.system(svnversion_command) != 0:
	exit()
print("Inserted SVN revision number into svnversion.h")


# build curl if needed
if ( NOCURL == '0' ):
	# 1: debug, 2: release
	if ( BUILD == 'release' ):
		local_curl = 2
	else:
		local_curl = 1
	Export( 'GLOBALS ' + GLOBALS )
	if ( TARGET_ARCH == 'x86' ):
		curl_lib = [ '#linux/libcurl/libcurl.a' ] # Use the static one built for TDM
	if ( TARGET_ARCH == 'x64' ):
		curl_lib = [ '#linux/libcurl/lib64/libcurl.a' ]


VariantDir( g_build + '/core/glimp', '.', duplicate = 1 )
SConscript( g_build + '/core/glimp/sys/scons/SConscript.gl' )
VariantDir( g_build + '/core', '.', duplicate = 0 )
thedarkmod = SConscript( g_build + '/core/sys/scons/SConscript.darkmod' )

exe_name = 'thedarkmod.' + ('x64' if TARGET_ARCH == 'x64' else cpu)
# Note: this target only runs if you append ".." (without quotes) as the last argument to scons command line
# It copies executable into ../darkmod, which is default location of darkmod installation in development environment
InstallAs( '../darkmod/' + exe_name, thedarkmod )
# this runs always and produces TDM binary in local directory
if ( BUILD == 'release' ):	# strip debug info in release
	Command(exe_name, thedarkmod, "strip $SOURCE -o $TARGET")
else:
	InstallAs( '#' + exe_name, thedarkmod )

# end targets ------------------------------------
