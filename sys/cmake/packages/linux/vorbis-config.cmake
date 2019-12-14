set(VORBIS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/vorbis/include")
set(VORBIS_LIBRARY_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/vorbis/lib/${PACKAGE_PLATFORM}")
set(VORBIS_LIBRARIES
        "${VORBIS_LIBRARY_DIR}/libvorbisfile.a"
        "${VORBIS_LIBRARY_DIR}/libvorbis.a"
        #"${VORBIS_LIBRARY_DIR}/libvorbisenc.a"
)
