#include "precompiled.h"
#include "qgl.h"
#include "tr_local.h"
#if defined(__linux__)
#include <dlfcn.h>
#endif

typedef void *FPTR;


#if defined(_WIN32)
HINSTANCE glimpOpenglDll;
PFNWGLGETPROCADDRESSPROC qlimpGetProcAddress;
#elif defined(__linux__)
void *glimpOpenglDll;
PFNGLXGETPROCADDRESSPROC qlimpGetProcAddress;
#endif
std::vector<FPTR*> glimpFuncPtrAddresses;


void GLimp_LoadDll() {
	const char *dllName = NULL;
#if defined(_WIN32)
	dllName = "opengl32.dll";
#elif defined(__linux__)
	dllName = "libGL.so.1";
#endif
	// r_glDriver is only intended for using instrumented OpenGL
	// dlls.  Normal users should never have to use it, and it is
	// not archived.
	if (r_glDriver.GetString()[0]) {
		dllName = r_glDriver.GetString();
	}

#if defined(_WIN32)
	glimpOpenglDll = LoadLibrary(dllName);
	if (!glimpOpenglDll) {
		common->Error("LoadLibrary(%s) failed: %d\n", dllName, GetLastError());
	}
#elif defined(__linux__)
	glimpOpenglDll = dlopen(dllName, RTLD_NOW | RTLD_GLOBAL);
	if (!glimpOpenglDll) {
		common->Error("dlopen(%s) failed: %s\n", dllName, dlerror());
	}
#endif
}

FPTR GLimp_AnyPointer(const char *name) {
	if (!glimpOpenglDll) {
		GLimp_LoadDll();
	}

#if defined(_WIN32)
	auto corefp = GetProcAddress(glimpOpenglDll, name);
#elif defined(__linux__)
	auto corefp = dlsym(glimpOpenglDll, name);
#endif
	if (corefp) {
		return corefp;
	}

	if (!qlimpGetProcAddress && !strstr(name, "GetProcAddress")) {
#if defined(_WIN32)
		qlimpGetProcAddress = (PFNWGLGETPROCADDRESSPROC)GLimp_AnyPointer("wglGetProcAddress");
#elif defined(__linux__)
		qlimpGetProcAddress = (PFNGLXGETPROCADDRESSPROC)GLimp_AnyPointer("glXGetProcAddress");
#endif
	}
	if (qlimpGetProcAddress) {
#if defined(_WIN32)
		FPTR extfp = qlimpGetProcAddress(name);
#else defined(__linux__)
		FPTR extfp = (FPTR)qlimpGetProcAddress((const GLubyte*)name);
#endif
		if (extfp) {
			return extfp;
		}
	}

	return NULL;
}


bool GLimp_LoadFunctionPointer(void *fpAddr, const char *name, bool mandatory) {
	auto &globalFuncPtr = *(FPTR*)fpAddr;

	FPTR fp = GLimp_AnyPointer(name);
	globalFuncPtr = fp;
	if (fp) {
		glimpFuncPtrAddresses.push_back(&globalFuncPtr);
	}

	if (mandatory && !fp) {
		common->Error("Failed to load mandatory function %s", name);
	}
	return fp != NULL;
}

void GLimp_UnloadBaseFunctions() {
	for (FPTR *address : glimpFuncPtrAddresses)
		*address = NULL;
	glimpFuncPtrAddresses.clear();

	qlimpGetProcAddress = NULL;
#if defined(_WIN32)
	FreeLibrary(glimpOpenglDll);
#elif defined(__linux__)
	dlclose(glimpOpenglDll);
#endif
	glimpOpenglDll = NULL;
}

void GLimp_LoadPreamble(double requiredVersion, const char **requiredExtensions) {
	GLimp_LoadFunctionPointer(&qglGetString, "glGetString");
	glConfig.vendor_string = (const char *)qglGetString(GL_VENDOR);
	glConfig.renderer_string = (const char *)qglGetString(GL_RENDERER);
	glConfig.version_string = (const char *)qglGetString(GL_VERSION);
	glConfig.extensions_string = (const char *)qglGetString(GL_EXTENSIONS);
	glConfig.glVersion = atof(glConfig.version_string);

	char comment[32];
	sprintf(comment, "GL version %0.1lf", requiredVersion);
	bool ok = GLimp_CheckExtension(comment, requiredVersion);
	for (int i = 0; requiredExtensions[i]; i++) {
		ok = ok && GLimp_CheckExtension(requiredExtensions[i]);
	}
	if (!ok)
		common->Error("OpenGL minimum requirements not satisfied");
}

bool GLimp_CheckExtension( const char *name, double coreSince ) {
	//check if the extension is available
	bool hasExtension = (strstr(glConfig.extensions_string, name) != NULL);
	//check if OpenGL version is high enough to include the feature into its core
	bool alreadyCore = (glConfig.glVersion >= coreSince - 1e-3);

	if ( !hasExtension && !alreadyCore ) {
		common->Printf( "^1X^0 - %s not found\n", name );
		return false;
	}
	common->Printf(
		"^2v^0 - using %s (%s)\n", name,
		hasExtension && alreadyCore ? "core+ext" : (hasExtension ? "ext" : "core")
	);
	return true;
}

void GLimp_LoadOptionalExtensions( void ) {
	common->Printf( "Checking optional OpenGL extensions...\n" );

	qglGetIntegerv( GL_MAX_TEXTURE_COORDS, &glConfig.maxTextureCoords );
	common->Printf( "Max texture coords: %d\n", glConfig.maxTextureCoords );
	qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &glConfig.maxTextureUnits );
	common->Printf( "Max texture units: %d\n", glConfig.maxTextureUnits );
	qglGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glConfig.maxTextures );
	common->Printf( "Max active textures: %d\n", glConfig.maxTextures );
	if ( glConfig.maxTextures < MAX_MULTITEXTURE_UNITS ) {
		common->Error( "   Too few!\n" );
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.anisotropicAvailable = GLimp_CheckExtension( "GL_EXT_texture_filter_anisotropic" );
	if ( glConfig.anisotropicAvailable ) {
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "    maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	} else {
		glConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias
	glConfig.textureLODBiasAvailable = GLimp_CheckExtension( "GL_EXT_texture_lod_bias" );

	// EXT_stencil_wrap
	// This isn't very important, but some pathological case might cause a clamp error and give a shadow bug.
	// Nvidia also believes that future hardware may be able to run faster with this enabled to avoid the
	// serialization of clamping.
	if ( GLimp_CheckExtension( "GL_EXT_stencil_wrap" ) ) {
		tr.stencilIncr = GL_INCR_WRAP_EXT;
		tr.stencilDecr = GL_DECR_WRAP_EXT;
	} else {
		tr.stencilIncr = GL_INCR;
		tr.stencilDecr = GL_DECR;
	}

	// GL_EXT_depth_bounds_test
	glConfig.depthBoundsTestAvailable = GLimp_CheckExtension( "GL_EXT_depth_bounds_test" );
	if ( glConfig.depthBoundsTestAvailable ) {
		GLimp_LoadFunctionPointer(&qglDepthBoundsEXT, "glDepthBoundsEXT", false);
	}

	//TODO: remove?
	glConfig.gpuShader4Available = GLimp_CheckExtension( "GL_EXT_gpu_shader4" );

	// geometry shaders
	glConfig.geometryShaderAvailable = GLimp_CheckExtension( "GL_ARB_geometry_shader4"/*, 3.2*/ );
	if (glConfig.geometryShaderAvailable) {
		GLimp_LoadFunctionPointer(&qglFramebufferTextureARB, "glFramebufferTextureARB", false);
		GLimp_LoadFunctionPointer(&qglFramebufferTextureLayer, "glFramebufferTextureLayerARB", false);
	}

	glConfig.timerQueriesAvailable = GLimp_CheckExtension( "GL_ARB_timer_query", 3.3 );
	if( glConfig.timerQueriesAvailable ) {
		GLimp_LoadFunctionPointer(&qglQueryCounter, "glQueryCounter", false);
		GLimp_LoadFunctionPointer(&qglGetQueryObjectui64v, "glGetQueryObjectui64v", false);
	}

	glConfig.debugGroupsAvailable = GLimp_CheckExtension( "GL_KHR_debug", 4.3 );
	if( glConfig.debugGroupsAvailable ) {
		GLimp_LoadFunctionPointer(&qglPushDebugGroup, "glPushDebugGroup", false);
		GLimp_LoadFunctionPointer(&qglPopDebugGroup, "glPopDebugGroup", false);
		GLimp_LoadFunctionPointer(&qglDebugMessageCallback, "glDebugMessageCallback", false);
	}

	glConfig.fenceSyncAvailable = GLimp_CheckExtension( "GL_ARB_sync", 3.2 );
	if ( glConfig.fenceSyncAvailable ) {
		GLimp_LoadFunctionPointer(&qglFenceSync, "glFenceSync", false);
		GLimp_LoadFunctionPointer(&qglClientWaitSync, "glClientWaitSync", false);
		GLimp_LoadFunctionPointer(&qglDeleteSync, "glDeleteSync", false);
	}

	int n;
	qglGetIntegerv( GL_MAX_GEOMETRY_OUTPUT_VERTICES, &n );
	common->Printf( "Max geometry output vertices: %d\n", n );
	qglGetIntegerv( GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &n );
	common->Printf( "Max geometry output components: %d\n", n );
	qglGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &n );
	common->Printf( "Max vertex attribs: %d\n", n );
	qglGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &n );
	common->Printf( "Max env parameters: %d\n", n );

#ifdef _WIN32
	GLimp_LoadFunctionPointer(&qwglSwapIntervalEXT, "wglSwapIntervalEXT", false);
#endif
}
