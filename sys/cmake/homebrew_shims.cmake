# CMake config shims for Homebrew packages.
# macOS has no conan artefacts: most Homebrew formulas ship pkg-config only.
# The main CMakeLists.txt expects these targets:
#   ZLIB::ZLIB minizip::minizip CURL::libcurl OpenAL::OpenAL vorbis::vorbis
#   Ogg::ogg JPEG::JPEG PNG::PNG ffmpeg::ffmpeg glfw pugixml::pugixml
#   doctest::doctest tracy::tracy
# On APPLE the find_package(... CONFIG) calls in CMakeLists.txt are skipped and
# this file defines everything instead.

find_package(PkgConfig REQUIRED)

# ---- OpenAL: Homebrew openal-soft ships a proper CMake config ----
if(NOT TARGET OpenAL::OpenAL)
	find_package(OpenAL REQUIRED CONFIG)
endif()

# ---- glfw3: Homebrew ships a proper CMake config (target: glfw) ----
if(NOT TARGET glfw)
	find_package(glfw3 REQUIRED CONFIG)
endif()

# ---- Ogg: Homebrew ships a proper CMake config (target: Ogg::ogg) ----
if(NOT TARGET Ogg::ogg)
	find_package(Ogg REQUIRED CONFIG)
endif()

# ---- doctest: Homebrew ships a proper CMake config (target: doctest::doctest) ----
if(NOT TARGET doctest::doctest)
	find_package(doctest REQUIRED CONFIG)
endif()

# ---- pugixml: Homebrew ships a proper CMake config (target: pugixml::pugixml) ----
if(NOT TARGET pugixml::pugixml)
	find_package(pugixml REQUIRED CONFIG)
endif()

# ---- tracy: local shim config at ~/tdm-deps/tracy-cmake (headers only) ----
if(NOT TARGET tracy::tracy)
	find_package(tracy REQUIRED CONFIG)
endif()

# ---- ZLIB (pkg-config only in Homebrew) ----
if(NOT TARGET ZLIB::ZLIB)
	pkg_check_modules(PC_ZLIB REQUIRED IMPORTED_TARGET zlib)
	add_library(ZLIB::ZLIB ALIAS PkgConfig::PC_ZLIB)
endif()

# ---- minizip (pkg-config only) ----
if(NOT TARGET minizip::minizip)
	pkg_check_modules(PC_MINIZIP REQUIRED IMPORTED_TARGET minizip)
	add_library(minizip::minizip ALIAS PkgConfig::PC_MINIZIP)
endif()

# ---- CURL (pkg-config only) ----
if(NOT TARGET CURL::libcurl)
	pkg_check_modules(PC_CURL REQUIRED IMPORTED_TARGET libcurl)
	add_library(CURL::libcurl ALIAS PkgConfig::PC_CURL)
endif()

# ---- Vorbis (pkg-config only) ----
if(NOT TARGET vorbis::vorbis)
	pkg_check_modules(PC_VORBIS REQUIRED IMPORTED_TARGET vorbis vorbisfile)
	add_library(vorbis::vorbis ALIAS PkgConfig::PC_VORBIS)
endif()

# ---- JPEG (pkg-config only) ----
if(NOT TARGET JPEG::JPEG)
	pkg_check_modules(PC_JPEG REQUIRED IMPORTED_TARGET libjpeg)
	add_library(JPEG::JPEG ALIAS PkgConfig::PC_JPEG)
endif()

# ---- PNG (pkg-config only) ----
if(NOT TARGET PNG::PNG)
	pkg_check_modules(PC_PNG REQUIRED IMPORTED_TARGET libpng)
	add_library(PNG::PNG ALIAS PkgConfig::PC_PNG)
endif()

# ---- ffmpeg (pkg-config only) ----
if(NOT TARGET ffmpeg::ffmpeg)
	pkg_check_modules(PC_FFMPEG REQUIRED IMPORTED_TARGET
		libavcodec libavformat libavutil libswscale libswresample)
	add_library(ffmpeg::ffmpeg ALIAS PkgConfig::PC_FFMPEG)
endif()
