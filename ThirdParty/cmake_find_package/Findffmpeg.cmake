include(${CMAKE_CURRENT_LIST_DIR}/tdm_find_package.cmake)

set(ffmpeg_FOUND 1)
set(ffmpeg_INCLUDE_DIRS "${ARTEFACTS_DIR}/ffmpeg/include")
set(ffmpeg_LIBRARY_DIR "${ARTEFACTS_DIR}/ffmpeg/lib/${PACKAGE_PLATFORM}")
if(MSVC)
	set(ffmpeg_LIBRARIES
		"${ffmpeg_LIBRARY_DIR}/libavformat.lib"
		"${ffmpeg_LIBRARY_DIR}/libavcodec.lib"
		"${ffmpeg_LIBRARY_DIR}/libavutil.lib"
		"${ffmpeg_LIBRARY_DIR}/libswresample.lib"
		"${ffmpeg_LIBRARY_DIR}/libswscale.lib"
	)
else()
	set(ffmpeg_LIBRARIES
		"${ffmpeg_LIBRARY_DIR}/libavformat.a"
		"${ffmpeg_LIBRARY_DIR}/libavcodec.a"
		"${ffmpeg_LIBRARY_DIR}/libavutil.a"
		"${ffmpeg_LIBRARY_DIR}/libswresample.a"
		"${ffmpeg_LIBRARY_DIR}/libswscale.a"
	)
endif()
