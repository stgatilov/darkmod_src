cmake_minimum_required(VERSION 3.12)

if (UNIX)
	# we want debug symbols even with Release builds
	string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "-g -ffast-math")
	string(APPEND CMAKE_C_FLAGS_RELEASE_INIT "-g -ffast-math")
elseif(MSVC)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/sys/cmake")
include(CMakeDependentOption)
include(ucm)
include(PrecompiledHeader)
include(SvnVersion)

project(TheDarkMod)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_dependent_option(WITH_TOOLS "Compile with Tools" ON "MSVC" OFF)
set(GAME_DIR "${PROJECT_SOURCE_DIR}/../darkmod" CACHE STRING "The directory where the game assets are stored")
option(COPY_EXE "Copy executable to GAME_DIR after build" ON)

add_executable(TheDarkMod)

get_svn_revision(SVNVERSION)
configure_file("idlib/svnversion.cmake.h" "${PROJECT_SOURCE_DIR}/idlib/svnversion.h" @ONLY)

# gather list of source files
ucm_add_dirs(
		"cm"
		"framework"
		"game"
		"idlib"
		"renderer"
		"sound"
		"tests"
		"TypeInfo"
		"ui"
		"tools/compilers"
		TO TDM_SOURCE_FILES RECURSIVE
)
ucm_add_files("sys/sys_local.cpp" "sys/sys_local.h" "sys/sys_public.h" TO TDM_SOURCE_FILES)

if (WITH_TOOLS)
	ucm_add_dirs("tools" TO TDM_SOURCE_FILES RECURSIVE)
	ucm_remove_files(
			"tools/edit_stub.cpp"
			"tools/decl/DialogEntityDefEditor.cpp"
			"tools/common/RenderBumpFlatDialog.cpp"
			"tools/guied/GEWindowWrapper_stub.cpp"
			FROM TDM_SOURCE_FILES
	)
else ()
	ucm_add_files("tools/guied/GEWindowWrapper_stub.cpp" TO TDM_SOURCE_FILES)
	add_compile_definitions(NO_MFC)
endif ()

if (UNIX)
	ucm_add_dirs("sys/linux" "sys/posix" TO TDM_SOURCE_FILES RECURSIVE)
	ucm_remove_directories("idlib/sys/win32" FROM TDM_SOURCE_FILES)
	ucm_remove_files(
			"renderer/glad_wgl.c"
			"sys/linux/dedicated.cpp"
			"sys/linux/test_scheduler.c"
			"sys/posix/platform_osx.cpp"
			FROM TDM_SOURCE_FILES
	)
elseif (WIN32)
	ucm_add_dirs("sys/win32" TO TDM_SOURCE_FILES RECURSIVE)
	ucm_remove_directories("idlib/sys/posix" FROM TDM_SOURCE_FILES)
	ucm_remove_files("renderer/glad_glx.c" "tools/compilers/dmap/optimize_gcc.cpp" "idlib/bv/Frustum_gcc.cpp" FROM TDM_SOURCE_FILES)
endif ()

# blacklist dead code files that unfortunately still hang around the repository
ucm_remove_directories("framework/async" FROM TDM_SOURCE_FILES)
ucm_remove_files(
		"TypeInfo/main.cpp"
		"game/Game_network.cpp"
		"game/MultiplayerGame.cpp"
		"game/gamesys/Callbacks.cpp"
		"idlib/math/Simd_AltiVec.cpp"
		"renderer/draw_exp.cpp"
		"sys/win32/win_gamma.cpp"
		FROM TDM_SOURCE_FILES
)

target_sources(TheDarkMod PRIVATE ${TDM_SOURCE_FILES})

if (MSVC)
	ucm_add_dirs("sys/msvc/natvis" TO TDM_NATVIS_FILES RECURSIVE ADDITIONAL_EXT "natvis")
	target_sources(TheDarkMod INTERFACE ${TDM_NATVIS_FILES})
endif ()

# configure precompiled header
# note TypeInfo.cpp must be excluded from precompiled header because it uses some dirty hacks
add_precompiled_header(TheDarkMod
		"idlib/precompiled.h" "idlib/precompiled.cpp" "precompiled.h"
		EXCLUDE "game/gamesys/TypeInfo.cpp"
)

add_compile_definitions(
		"ExtLibs="
		IL_STATIC_LIB
		__DOOM__
		CURL_STATICLIB
		AL_LIBTYPE_STATIC
)

if (UNIX)
	# configure sensible warnings
	add_compile_options(
			-Wall
			-Wno-unknown-pragmas            # MSVC specific pragmas
			-Wno-unused-variable            # too many
			-Wno-unused-but-set-variable
			-Wno-sign-compare               # in particular, int/size_t conversions
			-Wno-class-memaccess            # memcpy into custom structs
	)
	# configure other compile options
	add_compile_options(
			-pipe
			-fno-strict-aliasing
			-fmessage-length=0
			-fvisibility=hidden
			-fno-unsafe-math-optimizations
			-std=c++14
			-msse2
	)
	# TODO: do we really want to keep using this
	# use old ABI for std::string and std::list to allow TDM to run on distros with old
	# glibcxx (e.g. Ubuntu 14.04). the old ABI is not fully compliant with C++11 standard.
	add_compile_definitions("_GLIBCXX_USE_CXX11_ABI=0")

	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PACKAGE_PLATFORM "lnx64_s_gcc5_rel_stdcpp")
		set(PACKAGE_PLATFORM_DEBUG "lnx64_s_gcc5_rel_stdcpp")
		# ffmpeg dep was compiled without -fPIC, so need to disable on 64bit builds
		set_target_properties(TheDarkMod PROPERTIES POSITION_INDEPENDENT_CODE OFF)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "thedarkmod.x64")
	else ()
		set(PACKAGE_PLATFORM "lnx32_s_gcc5_rel_stdcpp")
		set(PACKAGE_PLATFORM_DEBUG "lnx32_s_gcc5_rel_stdcpp")
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "thedarkmod.x86")
	endif ()
	# ensure threading support in X
	add_compile_definitions(XTHREADS)
	set(PACKAGE_CONFIG_PATH "${PROJECT_SOURCE_DIR}/sys/cmake/packages/linux")
elseif (MSVC)
	add_compile_definitions("WIN32" _WINDOWS
			_CRT_SECURE_NO_DEPRECATE
			_CRT_NONSTDC_NO_DEPRECATE
			_ALLOW_KEYWORD_MACROS
			NO_WARN_MBCS_MFC_DEPRECATION
			_MBCS
	)
	add_compile_options("/MP /Gm- /arch:SSE2")
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PACKAGE_PLATFORM "win64_s_vc15_rel_mt")
		set(PACKAGE_PLATFORM_DEBUG "win64_s_vc15_dbg_mtd")
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "TheDarkModx64")
	else ()
		set(PACKAGE_PLATFORM "win32_s_vc15_rel_mt")
		set(PACKAGE_PLATFORM_DEBUG "win32_s_vc15_dbg_mtd")
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "TheDarkMod")
	endif ()
	set(PACKAGE_CONFIG_PATH "${PROJECT_SOURCE_DIR}/sys/cmake/packages/vc15")
	# we want debug symbols even with Release builds
	ucm_add_linker_flags(EXE "/DEBUG" CONFIG Release)
endif ()
ucm_set_runtime(STATIC)

find_package(ZLIB REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(CURL REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(OPENAL REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(OGG REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(VORBIS REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(DEVIL REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(JPEG REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(FFMPEG REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(PUGIXML REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(PNG REQUIRED HINTS ${PACKAGE_CONFIG_PATH})
find_package(MBEDTLS REQUIRED HINTS ${PACKAGE_CONFIG_PATH})

include_directories(
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/idlib"
		"${PROJECT_SOURCE_DIR}/framework"
		"${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/doctest/include"
		${ZLIB_INCLUDE_DIRS}
		${CURL_INCLUDE_DIRS}
		${OPENAL_INCLUDE_DIRS}
		${OGG_INCLUDE_DIRS}
		${VORBIS_INCLUDE_DIRS}
		${DEVIL_INCLUDE_DIRS}
		${JPEG_INCLUDE_DIRS}
		${FFMPEG_INCLUDE_DIRS}
		${PUGIXML_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${MBEDTLS_INCLUDE_DIRS}
)

target_link_libraries(TheDarkMod
		${ZLIB_LIBRARIES}
		${CURL_LIBRARIES}
		${OPENAL_LIBRARIES}
		${VORBIS_LIBRARIES}
		${OGG_LIBRARIES}
		${DEVIL_LIBRARIES}
		${JPEG_LIBRARIES}
		${FFMPEG_LIBRARIES}
		${PNG_LIBRARIES}
		${MBEDTLS_LIBRARIES}
)
target_link_libraries(TheDarkMod debug ${PUGIXML_LIBRARIES_D} optimized ${PUGIXML_LIBRARIES})

if (WIN32)
	target_link_libraries(TheDarkMod ws2_32 iphlpapi winmm bcrypt dinput8 dbghelp)
	set_target_properties(TheDarkMod PROPERTIES WIN32_EXECUTABLE TRUE)
elseif (UNIX)
	target_link_libraries(TheDarkMod pthread dl stdc++fs X11 Xext Xxf86vm)
	# strip debug symbols from executable and put them into a separate symbols file
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND objcopy --only-keep-debug "$<TARGET_FILE:TheDarkMod>" "$<TARGET_FILE:TheDarkMod>.debug"
	)
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND strip --strip-debug --strip-unneeded "$<TARGET_FILE:TheDarkMod>"
	)
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND objcopy "--add-gnu-debuglink=$<TARGET_FILE:TheDarkMod>.debug" "$<TARGET_FILE:TheDarkMod>"
	)
endif ()

if (COPY_EXE)
	# copy produced executable to game directory '../darkmod'
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:TheDarkMod> "${GAME_DIR}"
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:TheDarkMod>.debug "${GAME_DIR}"
	)
endif ()
