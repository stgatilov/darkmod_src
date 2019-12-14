set(ZLIB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/zlib/include")
set(ZLIB_LIBRARY_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/zlib/lib/${PACKAGE_PLATFORM}")
set(ZLIB_LIBRARIES "${ZLIB_LIBRARY_DIR}/libz.a" "${ZLIB_LIBRARY_DIR}/libminizip.a")
