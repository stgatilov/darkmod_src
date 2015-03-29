/*****************************************************************************
	The Dark Mod GPL Source Code

	This file is part of the The Dark Mod Source Code.

	The Dark Mod Source Code is free software: you can redistribute it
	and/or modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of the License,
	or (at your option) any later version. For details, see LICENSE.TXT.

	Project: The Dark Mod (http://www.thedarkmod.com/)

	$Revision$ (Revision of last commit)
	$Date$ (Date of last commit)
	$Author$ (Author of last commit)

	******************************************************************************/

#ifndef __SOFT_SHADOWS__
#define __SOFT_SHADOWS__


/*  ---+-+-+-+-+-+-+-+|  SOFT SHADOW RESOURCE MANAGER  |+-+-+-+-+-+-+-+---  */

class SoftShadowManager {
public:
	void NewFrame();									// Call before use in each new frame. Initializes resources.
	void SetLightPosition( const idVec4* pos );
	void DrawInteractions( const viewLight_t* vLight );
	void UnInit();										// Releases all resources
	
	void DrawDebugOutput();

	SoftShadowManager() :initialized(false) {}			// Does not init resources, as gl context probably won't be valid yet
	~SoftShadowManager() { UnInit(); }

private:
	void Init();
	void InitRenderTargets();
	void InitShaders();
	void InitFBOs();
	void InitVBOs();

	void DrawQuad( idImage* tex, const GLuint shaderProg, const bool powerOfTwo = true );
	void ResetLightScissor( const viewLight_t* vLight );

	SoftShadowManager(const SoftShadowManager&);			// disallow copying
    SoftShadowManager& operator=(const SoftShadowManager&); // disallow copying


	int width, height;									// Screen/viewport dimensions
	int potWidth, potHeight;							// Enlarged to power-of-two
	bool initialized;
	
	/* Resources */

	// Frame buffers
	enum {
		penumbra_size_fb,
		NumFramebuffers
	};
	GLuint fbo[NumFramebuffers];

	// Render targets
	enum {
		penumbraSize_tx,
		NumTextures
	};
	idImage* tex[NumTextures];
	GLuint depthRbo;

	// Shaders and programs
	enum {
		shadow_vp,
		shadow_fp,
		quad_fp,
		quad_vp,
		NumShaders
	};
	GLuint shaders[NumShaders];
	enum {
		stencilShadow_pr,
		quad_pr,
		NumGLSLPrograms
	};
	GLuint glslProgs[NumGLSLPrograms];

	// Uniform / attribute locations
	GLuint UNF_SHADOW_lightPos;
	GLuint UNF_SHADOW_lightRadius;
	GLuint UNF_SHADOW_lightReach;
	GLuint UNF_SHADOW_depthtex;
	GLuint UNF_SHADOW_invDepthImageSize;
	GLuint UNF_QUAD_maxTexcoord;
	GLuint UNF_QUAD_tex;
	GLuint UNF_QUAD_pos; // Vertex data
	

	// VBOs for drawing screen quads
	vertCache_t* ScreenQuadVerts;
	vertCache_t* ScreenQuadIndexes;
};

extern SoftShadowManager* softShadowMgr;

#endif // !__SOFT_SHADOWS__