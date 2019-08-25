#ifndef _QGL_H_
#define _QGL_H_

// All GL/WGL/GLX features available at compile time must be enumerated here.
// Versions:
#define GL_VERSION_1_0 1
#define GL_VERSION_1_1 1
#define GL_VERSION_1_2 1
#define GL_VERSION_1_3 1
#define GL_VERSION_1_4 1
#define GL_VERSION_1_5 1
#define GL_VERSION_2_0 1
#define GL_VERSION_2_1 1
#define GL_VERSION_3_0 1
// Mandatory extensions:
#define GL_EXT_texture_compression_s3tc			1
#define GL_ARB_vertex_program					1
#define GL_ARB_fragment_program					1
#define GL_ARB_draw_instanced					1	//core since 3.1
// Optional extensions:
#define GL_EXT_texture_filter_anisotropic		1
#define GL_EXT_texture_lod_bias					1
#define GL_EXT_stencil_wrap						1
#define GL_EXT_depth_bounds_test				1
#define GL_EXT_gpu_shader4						1
#define GL_ARB_geometry_shader4					1	//similar feature core since 3.2
#define GL_ARB_timer_query						1	//core since 3.3
#define GL_KHR_debug							1	//core since 4.3
#define GL_ARB_sync								1	//core since 3.2
#define GL_ARB_stencil_texturing				1	//core since 4.3
#define GL_ARB_compatibility					1	//check context profile
#include "glad.h"

#ifdef _WIN32
// Versions:
#define WGL_VERSION_1_0 1
// Mandatory extensions:
#define WGL_ARB_create_context					1
#define WGL_ARB_create_context_profile			1
#define WGL_ARB_pixel_format					1
// Optional extensions:
#define WGL_EXT_swap_control					1
#include "glad_wgl.h"
#endif

#ifdef __linux__
// Versions:
#define GLX_VERSION_1_0 1
#define GLX_VERSION_1_1 1
#define GLX_VERSION_1_2 1
#define GLX_VERSION_1_3 1
#define GLX_VERSION_1_4 1
#include "glad_glx.h"
#endif

#define QGL_REQUIRED_VERSION_MAJOR 3
#define QGL_REQUIRED_VERSION_MINOR 1

// Loads all GL/WGL/GLX functions from OpenGL library (using glad-generated loader).
// Note: no requirements are checked here, run GLimp_CheckRequiredFeatures afterwards.
void GLimp_LoadFunctions(bool inContext = true);

// Check and load all optional extensions.
// Fills extensions flags in glConfig and loads optional function pointers.
void GLimp_CheckRequiredFeatures();

// Unloads OpenGL library (does NOTHING in glad loader).
inline void GLimp_UnloadFunctions() {}

#endif
