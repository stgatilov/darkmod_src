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

#include "precompiled_engine.h"
#pragma hdrstop
static bool versioned = RegisterVersionedFile( "$Id$" );
#include "tr_local.h"
#include "draw_soft_shadows.h"



/*  ---+-+-+-+-+-+-+-+|  GENERIC STUFF  |+-+-+-+-+-+-+-+---  */
/*           Probably to be moved to a shared header         */


// Debugging convenience: Set openGL debugger to break on glIsFramebuffer()
#ifdef _DEBUG
#define __opengl_breakpoint assert( !qglIsFramebuffer(0) ); 
#else
#define __opengl_breakpoint ;
#endif

// Need to undo some Microsoft namespace pollution to be able to embed GLSL
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// 'GLSL' macro allows easy embedding of GLSL in c++ code. Compatibility mode 
// is for accessing the engine's GLSL 1.2 matrix stack and glVertexPointers etc
#define GLSL(src)	 "#version 150 core\n" #src 
#define GLSLold(src) "#version 150 compatibility\n" #src 


static bool ShaderOK( GLuint shdr )
{
	GLint status;
	qglGetShaderiv( shdr, GL_COMPILE_STATUS, &status );

	if ( status != GL_TRUE )
	{
		char buffer[512];
		qglGetShaderInfoLog( shdr, 512, NULL, buffer );
		common->Error( "GLSL Shader error: %s\n", buffer );
		return false;
	}
	return true;
}

static bool LinkedOK(GLuint program )
{
	GLint status;
	qglGetProgramiv( program, GL_LINK_STATUS, &status );
	
	if (status == GL_FALSE)
	{
		GLint maxLength = 0;
		qglGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
 
		//The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		qglGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
		common->Error( "GLSL Program error: %s\n", &infoLog[0] );
		return false;
	}
	return true;
}



/*  ---+-+-+-+-+-+-+-+|  SOFT SHADOW RESOURCE MANAGER  |+-+-+-+-+-+-+-+---  */


void SoftShadowManager::Init()
{
	width = glConfig.vidWidth;
	height = glConfig.vidHeight;
	potWidth = MakePowerOfTwo( width );
	potHeight = MakePowerOfTwo( height );
	InitRenderTargets();
	InitShaders();
	InitFBOs();
	InitVBOs();
	initialized = true;
}


void SoftShadowManager::UnInit()
{
	if (!initialized)
	{
		return;
	}

	qglDeleteFramebuffers( NumFramebuffers, fbo );
	
	qglDeleteRenderbuffers( 1, &depthRbo );

	for ( int i=0; i<NumTextures; ++i )
	{
		tex[i]->PurgeImage();
	}
	
	for ( int i=0; i<NumGLSLPrograms; ++i )
	{
		qglDeleteProgram( glslProgs[i] );
	}

	for ( int i=0; i<NumShaders; ++i )
	{
		qglDeleteShader( shaders[i] );
	}

	vertexCache.Free( ScreenQuadVerts );
	vertexCache.Free( ScreenQuadIndexes );

	initialized = false;
}


void SoftShadowManager::NewFrame()
{
	if ( initialized && (width != glConfig.vidWidth || height != glConfig.vidHeight) )
	{
		UnInit();
	}
	if ( !initialized )
	{
		Init();
	}
}


void SoftShadowManager::ResetLightScissor( const viewLight_t* vLight )
{
	backEnd.currentScissor = vLight->scissorRect;
	if( r_useScissor.GetBool() ) {
		qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	}
}


void SoftShadowManager::SetLightPosition( const idVec4* pos)
{
	qglUniform4fv( UNF_SHADOW_lightPos, 1, pos->ToFloatPtr() ); //~TODO: store this and defer upload until use. 
}



void SoftShadowManager::InitRenderTargets()
{
	// Allocates new textures and renderbuffers.
	qglActiveTextureARB( GL_TEXTURE0_ARB );

	// Depth-Stencil renderbuffer object for use when drawing shadow volumes and interactions
	qglGenRenderbuffers(1, &depthRbo);
	qglBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
	qglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	qglBindRenderbuffer( GL_RENDERBUFFER, 0 );

	// Penumbra size map
	tex[penumbraSize_tx] = globalImages->RenderTargetImage("penumbraSize_tx", potWidth, potHeight, GL_RG16F, GL_RGBA, GL_FLOAT );
}


void SoftShadowManager::InitShaders()
{
	// Stencil shadow shader program
	// Does the usual job of projecting the shadow volume away from the light,
	// but also outputs penumbra size to a color buffer.
	// Use "old" GLSL compatibility mode so we can access the built-in matrix stack.
	const GLchar* SoftShadowVP = GLSLold(
		uniform vec4 lightPos;			/* in model space */
		uniform float lightRadius;		/* size of the light source */
		uniform float lightReach;		/* max distance that light can hit */
		out float penumbraSize;

		//debug
		out float light2edge;
		out float edge2shadow;
		

		void main()
		{
			/* Project verts with w=0 away from light beyond its reach, while leaving verts with w=1 where they are */
			vec3 position = gl_Vertex.xyz;
			if ( gl_Vertex.w == 0.0 )
			{
				vec3 shadowDirection = position - lightPos.xyz;
				position += shadowDirection * ( lightReach / length(shadowDirection) );
			}
			/* Output vertex position */
			gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0); 
			/* Output penumbra size to be interpolated across the shadow volume. 
			   0 at the near edge (w=1 vert), max at the far (w=0) vert. */
			float distFromLight = distance( lightPos.xyz, gl_Vertex.xyz );
			penumbraSize = ( 1.0 - gl_Vertex.w ) * ( lightReach - distFromLight ) * lightRadius / distFromLight;

			//debug only
			edge2shadow = ( 1.0 - gl_Vertex.w ) * ( lightReach - distFromLight );
			light2edge = distFromLight;
		}
	);

	/* Stencil shadow fragment shader tests whether the fragment is very close to the scene depth, 
	   and writes the penumbra size at that point if so. The idea is to produce a line marking the 
	   centre of each penumbra with size info. */
	const GLchar* SoftShadowFP = GLSLold(
		in float penumbraSize;
		uniform sampler2D depthtex;
		uniform vec2 invDepthImageSize;
		
		//debug only
		in float light2edge;
		in float edge2shadow;
		uniform float threshold;
		
		void main()
		{
			/* sample scene depth */
			vec2 texcoord = gl_FragCoord.xy * invDepthImageSize;
			float SceneDepth = texture( depthtex, texcoord ).x;
			float DepthDiff = abs( SceneDepth - gl_FragCoord.z );
			float maxDepthDelta = fwidth(SceneDepth) + fwidth(gl_FragCoord.z); // greatest relative depth change between shadow vol and scene
			float sceneDelta = dFdx(SceneDepth) + dFdy(SceneDepth);

			bool sharpDepthDiscontinuity = fwidth(SceneDepth) > threshold; 
			// debug
			// show linear depth of fragment and scene
			/*float linear_scenedepth = -1.0 / ( ( 2 * SceneDepth - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
			float linear_fragdepth = -1.0 /  ( ( 2 * gl_FragCoord.z - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
			float linear_fragchange = -1.0 /  ( ( 2 * fwidth(gl_FragCoord.z) - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
			float linear_depthdiff =  -1.0 /  ( ( 2 * DepthDiff - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
			*/
			//if ( fwidth(gl_FragCoord.z) >= DepthDiff )
			//{
			//	gl_FragColor = vec4( 1.0, linear_fragchange / 256.0, 0.0, 1.0 );  // debug
			//}
			float screenCoverage = penumbraSize * gl_FragCoord.w;
			if ( screenCoverage < 8.0 * invDepthImageSize.x || screenCoverage < 8.0 * invDepthImageSize.y ) 
			{
				screenCoverage = 0.0;
			}

			if ( !sharpDepthDiscontinuity && screenCoverage > 0.0 /* && penumbraSize > 0.25 */ && DepthDiff < maxDepthDelta )
			{
				//gl_FragColor = vec4( light2edge, edge2shadow, 0.0, 1.0 ); 
				gl_FragColor = vec4( penumbraSize, 1.0, 0.0, 1.0 ); 
			} else {
				gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0 );
			}

			// Debug test: color according to dz
			/*
			float linear_fragdepth = -1.0 /  ( ( 2 * gl_FragCoord.z - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
			gl_FragColor = vec4( dFdx(linear_fragdepth), dFdy(linear_fragdepth), 0.0, 0.1 );
			*/
		}
	);
	
	shaders[shadow_vp] = qglCreateShader( GL_VERTEX_SHADER );
	qglShaderSource( shaders[shadow_vp], 1, &SoftShadowVP, NULL );
	qglCompileShader( shaders[shadow_vp] );
	assert( ShaderOK( shaders[shadow_vp] ) );

	shaders[shadow_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
	qglShaderSource( shaders[shadow_fp], 1, &SoftShadowFP, NULL );
	qglCompileShader( shaders[shadow_fp] );
	assert( ShaderOK( shaders[shadow_fp] ) );

	// Link the stencil shadow program
	const GLuint prg_shadow =  qglCreateProgram();
	glslProgs[stencilShadow_pr] = prg_shadow;
	qglAttachShader( prg_shadow, shaders[shadow_vp] );
	qglAttachShader( prg_shadow, shaders[shadow_fp] );
	qglLinkProgram( prg_shadow );
	assert( LinkedOK(prg_shadow) );
	UNF_SHADOW_lightPos = qglGetUniformLocation( prg_shadow, "lightPos" );
	UNF_SHADOW_lightRadius = qglGetUniformLocation( prg_shadow, "lightRadius" );
	UNF_SHADOW_lightReach = qglGetUniformLocation( prg_shadow, "lightReach" );
	UNF_SHADOW_depthtex = qglGetUniformLocation( prg_shadow, "depthtex" );
	UNF_SHADOW_invDepthImageSize = qglGetUniformLocation( prg_shadow, "invDepthImageSize" );
	// debug threshold
	UNF_SHADOW_threshold = qglGetUniformLocation( prg_shadow, "threshold" );

	// Simplest screen quad.
	const GLchar* ScreenQuadVP = GLSL(
		in      vec2 pos;
		uniform vec2 maxTexcoord;
		out     vec2 texcoord;
		const   vec2 posNormalizer = vec2( 0.5, 0.5 );

		void main()
		{
			vec2 npos = pos * posNormalizer + posNormalizer;
			texcoord = npos * maxTexcoord;
			gl_Position = vec4( pos, 0.0, 1.0 );
		}
	);

	const GLchar* ScreenQuadFP = GLSL(
		in vec2 texcoord;
		uniform sampler2D tex;
		out	vec4 outColor;

		void main()
		{
			vec4 col = texture( tex, texcoord );
			outColor = col;
		}
	);

	shaders[quad_vp] = qglCreateShader( GL_VERTEX_SHADER );
	qglShaderSource( shaders[quad_vp], 1, &ScreenQuadVP, NULL );
	qglCompileShader( shaders[quad_vp] );
	assert( ShaderOK( shaders[quad_vp] ) );

	shaders[quad_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
	qglShaderSource( shaders[quad_fp], 1, &ScreenQuadFP, NULL );
	qglCompileShader( shaders[quad_fp] );
	assert( ShaderOK( shaders[quad_fp] ) );

	const GLuint prg_quad =  qglCreateProgram();
	glslProgs[quad_pr] = prg_quad;
	qglAttachShader( prg_quad, shaders[quad_vp] );
	qglAttachShader( prg_quad, shaders[quad_fp] );
	qglLinkProgram( prg_quad );
	assert( LinkedOK(prg_quad) );
	UNF_QUAD_maxTexcoord = qglGetUniformLocation( prg_quad, "maxTexcoord" );
	UNF_QUAD_tex = qglGetUniformLocation( prg_quad, "tex" );
}


void SoftShadowManager::InitFBOs()
{
	qglGenFramebuffers( NumFramebuffers, fbo );

	// Stencil shadow pass FBO
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbra_size_fb] );
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo );
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[penumbraSize_tx]->texnum, 0 );
	assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
	
	// Reset to default framebuffer
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
}


/*
==================
SoftShadowManager::InitVBOs

This belongs in a generic GLSL module when we have one.
Verts and Indexes for drawing a screen-aligned quad much more simply than the 
old method of messing with the projection matrix.
==================
*/
void SoftShadowManager::InitVBOs()
{
	// Verts, in normalized device coordinates
	float vertices[] = {
		-1.0f,  1.0f,
		 1.0f,  1.0f,
		 1.0f, -1.0f,
		-1.0f, -1.0f
	};

	vertexCache.Alloc( &vertices, sizeof( vertices ), &ScreenQuadVerts, false );
	vertexCache.Touch( ScreenQuadVerts );

	// Indexes
	GLuint indexes[] = {
		0, 1, 2,
		2, 3, 0
	};

	vertexCache.Alloc( &indexes, sizeof( indexes ), &ScreenQuadIndexes, true );
	vertexCache.Touch( ScreenQuadIndexes );
}


void SoftShadowManager::DrawInteractions( const viewLight_t* vLight )
{
	// Draws the interactions for one light
	qglDisable( GL_VERTEX_PROGRAM_ARB );
	qglDisable( GL_FRAGMENT_PROGRAM_ARB );

	/**********
		Step 1. Get a copy of the depth buffer. We can use global _currentDepth as a sampler, but we need a separate
		active depth buffer for our shadow volume drawing. 
	 **********/
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo[penumbra_size_fb] );
	qglBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	qglBlitFramebuffer( 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 
				 	    GL_DEPTH_BUFFER_BIT, GL_NEAREST );

	/**********
		Step 2. Draw shadows into the stencil using existing technique, but using a new shader that draws 
		estimated penumbra size into a color buffer.
	 **********/
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbra_size_fb] );
	assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
	GL_SelectTexture( 0 );
	globalImages->currentDepthImage->Bind();
	qglUseProgram( glslProgs[stencilShadow_pr] );
	qglUniform1f( UNF_SHADOW_lightRadius, r_softShadows.GetFloat() ); //~TODO: Get this from a spawnarg
	qglUniform1i( UNF_SHADOW_depthtex, 0 );
	qglUniform2f( UNF_SHADOW_invDepthImageSize, 
				  1.0 / (float)globalImages->currentDepthImage->uploadWidth, 
				  1.0 / (float)globalImages->currentDepthImage->uploadHeight );
	qglUniform1f( UNF_SHADOW_lightReach, 10000.0f ); //~TODO: use correct light size
	//~debug
	qglUniform1f( UNF_SHADOW_threshold, /* r_ignore.GetFloat() */ 0.0005 );
	ResetLightScissor( vLight );
	//qglBlendEquation( GL_MAX ); //~TODO cvar. This and clear color need to be in sync
	qglClearColor( 0.0, 0.0, 0.0, 0.0 );
	//glClearColor( 1.0, 1.0, 1.0, 1.0 );
	qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT );
	RB_StencilShadowPass( vLight->globalShadows );
	RB_StencilShadowPass( vLight->localShadows );
	qglBlendEquation( GL_FUNC_ADD );
	//~TODO: split off rendering pass for non-self-shadow surfaces. For the POC, all shadows hit 
	// everything. Can maintain 2 stencils and alpha masks

	__opengl_breakpoint
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	qglUseProgram( 0 );
	globalImages->BindNull();
}


void SoftShadowManager::DrawQuad( idImage* tex, const GLuint shaderProg, const bool powerOfTwo )
{
	// Texcoord setup
	float max_s, max_t;
	if ( powerOfTwo ) {
		max_s = static_cast<float>(glConfig.vidWidth) / MakePowerOfTwo( glConfig.vidWidth );
		max_t = static_cast<float>(glConfig.vidHeight) / MakePowerOfTwo( glConfig.vidHeight );
	} else {
		max_s = max_t = 1.0f;
	}
	
	GL_SelectTexture( 0 );
	if ( tex )
	{
		tex->Bind();
	} else {
		globalImages->blackImage->Bind();
	}

	// shader inputs: verts
	qglUseProgram( shaderProg );
	qglVertexAttribPointerARB( UNF_QUAD_pos, 2, GL_FLOAT, GL_FALSE, 0, vertexCache.Position( ScreenQuadVerts ) );
	qglEnableVertexAttribArrayARB( UNF_QUAD_pos );

	// shader inputs: uniforms
	qglUniform2f( UNF_QUAD_maxTexcoord, max_s, max_t );
	qglUniform1i( UNF_QUAD_tex, 0 );

	// Draw
	__opengl_breakpoint
	qglDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, vertexCache.Position( ScreenQuadIndexes ) );

	// Clean up
	qglDisableVertexAttribArrayARB( UNF_QUAD_pos );
	globalImages->BindNull();
	qglUseProgram( 0 );
	GL_State(GLS_DEFAULT);
}


void SoftShadowManager::DrawDebugOutput()
{
	// Draw penumbra centre line from penumbraSize_tx
	// It's already got alpha 0 where there is no line.
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE );
	// Leave light scissor in place
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	DrawQuad( tex[penumbraSize_tx], glslProgs[quad_pr] );
}



/*  ---+-+-+-+-+-+-+-+|  EXTERNAL API  |+-+-+-+-+-+-+-+---  */

// Instantiate our singleton
SoftShadowManager softShadowManager;
SoftShadowManager* softShadowMgr = &softShadowManager;

