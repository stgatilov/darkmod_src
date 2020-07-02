cmake_minimum_required (VERSION 3.9.6)
cmake_policy(SET CMP0079 NEW)   # allow adding dependencies to libzipsync (in parent directories)
cmake_policy(SET CMP0091 NEW)   # for MSVC_RUNTIME_LIBRARY
project(tdm_installer)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TDM: use prebuilt artefacts for third-party libraries
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../ThirdParty/cmake_find_package")

set(ZIPSYNC_OPTION_BUILD_TESTS OFF CACHE BOOL "")
set(ZIPSYNC_OPTION_BUILD_TOOL OFF CACHE BOOL "")
add_subdirectory(zipsync)

set(sources
    InstallerMain.cpp 

    GuiFluidAutoGen.cxx     # auto-generated
    GuiFluidAutoGen.h       # auto-generated
    GuiGlobal.cpp
    GuiGlobal.h
    GuiPageSettings.cpp
    GuiPageSettings.h

    Actions.h
    Actions.cpp

    OsUtils.cpp
    OsUtils.h
    LogUtils.cpp
    LogUtils.h
)

find_package(fltk REQUIRED)
find_package(libjpeg REQUIRED)
add_executable(tdm_installer WIN32 ${sources})
target_include_directories(tdm_installer PRIVATE zipsync ${fltk_INCLUDE_DIRS} ${libjpeg_INCLUDE_DIRS})
target_link_libraries(tdm_installer PRIVATE libzipsync libzipsyncextra ${fltk_LIBRARIES} ${libjpeg_LIBRARIES})
set_property(TARGET tdm_installer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(MSVC)
    target_compile_definitions(tdm_installer PRIVATE _CRT_SECURE_NO_DEPRECATE)
    target_link_libraries(libzipsync INTERFACE ws2_32)                  # libzipsync -> libcurl -> ?
else()
    target_link_libraries(tdm_installer PRIVATE m dl X11 Xext pthread)  # fltk -> *
endif()

# fix libcurl dependencies
find_package(mbedtls REQUIRED)  # libzipsync -> libcurl -> mbedtls
find_package(zlib REQUIRED)     # libzipsync -> libcurl -> zlib
target_link_libraries(libzipsync INTERFACE ${mbedtls_LIBRARIES} ${zlib_LIBRARIES})

# ensure same type of runtime on used zipsync libraries
set_property(TARGET libzipsync libzipsyncextra PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
