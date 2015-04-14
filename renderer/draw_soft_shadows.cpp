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
#define GLSL(src)    "#version 150 core\n" #src 
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


static void R_JitterMap( idImage* img )
{
	// Create 3D texture for per-pixel jittered offset lookup   
	// Not a member of SoftShadowManager because we need idImage to use it as a generator function.
	// Code written by Yury Uralsky (yuralsky@nvidia.com)
	// Create a 3d texture containing sets of randomly jittered disk-distibuted sample 
	// offsets. Each texcoord s,t holds one normalized jittered sample disc, spread 
	// through the samples in the r dimension. The idea is to ensure an even distribution 
	// of samples -- smoother than purely random sampling -- without creating banding in 
	// the penumbrae.
	// The first 8 samples in each column are the outer ring of the disk. If they all return 
	// the same value, the fragment shader can skip the remaining samples. *** SL: This is broken in 
	// the gpu gems code. The samples are not ordered in this way. Not yet fixed as we might not 
	// need the early exit, if the penumbra size estimation is good enough. 
	// Better explanation at http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter17.html
	static const long somevalue = 423480826;
	idRandom2 rand(somevalue);

	const int size = SoftShadowManager::JITTERMAPSIZE;
	const int samples_u = 8;
	const int samples_v = 8;

    signed char* data = new signed char[size * size * samples_u * samples_v * 4 / 2];   
   
    for (int i = 0; i<size; i++) {   
        for (int j = 0; j<size; j++) {   
            for (int k = 0; k<samples_u*samples_v/2; k++) {   
   
                int x, y;
                idVec4 v;
   
                x = k % (samples_u / 2);
                y = (samples_v - 1) - k / (samples_u / 2);
   
                // generate points on a regular samples_u x samples_v rectangular grid   
                v[0] = (float)(x * 2 + 0.5f) / samples_u;   
                v[1] = (float)(y + 0.5f) / samples_v;   
                v[2] = (float)(x * 2 + 1 + 0.5f) / samples_u;   
                v[3] = v[1];   
                   
                // jitter position   
                v[0] += rand.CRandomFloat() * (0.5f / samples_u);   
                v[1] += rand.CRandomFloat() * (0.5f / samples_v);   
                v[2] += rand.CRandomFloat() * (0.5f / samples_u);   
                v[3] += rand.CRandomFloat() * (0.5f / samples_v);   
   
                // warp to disk   
                idVec4 d;
                d[0] = sqrtf(v[1]) * cosf(2 * idMath::PI * v[0]);   
                d[1] = sqrtf(v[1]) * sinf(2 * idMath::PI * v[0]);   
                d[2] = sqrtf(v[3]) * cosf(2 * idMath::PI * v[2]);   
                d[3] = sqrtf(v[3]) * sinf(2 * idMath::PI * v[2]);   
   
                data[(k * size * size + j * size + i) * 4 + 0] = (signed char)(d[0] * 127);   
                data[(k * size * size + j * size + i) * 4 + 1] = (signed char)(d[1] * 127);   
                data[(k * size * size + j * size + i) * 4 + 2] = (signed char)(d[2] * 127);   
                data[(k * size * size + j * size + i) * 4 + 3] = (signed char)(d[3] * 127);   
            }   
        }   
    }  
		
	img->GenerateDataCubeImage( static_cast<GLvoid*>(data), size, size, samples_u * samples_v / 2, TR_REPEAT, GL_RGBA8_SNORM, GL_RGBA, GL_BYTE ); 
    delete [] data;   
}  


/*  ---+-+-+-+-+-+-+-+|  SOFT SHADOW RESOURCE MANAGER  |+-+-+-+-+-+-+-+---  */

SoftShadowManager::SoftShadowManager() 
	:initialized(false), 
	 spamConsole(false) 
{
	// Defer real initialisation until the first call to NewFrame(), so 
	// that we know the GL context is initialised.
}

void SoftShadowManager::Init()
{
    width = glConfig.vidWidth;
    height = glConfig.vidHeight;
    potWidth = MakePowerOfTwo( width );
    potHeight = MakePowerOfTwo( height );
    smallwidth = ceil( (float)width / MINISCALE );
    smallheight = ceil( (float)height / MINISCALE );
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

	// Verts might already be on the garbage heap. Check whether user pointer is already null.
	if ( ScreenQuadVerts->user ) { vertexCache.Free( ScreenQuadVerts ); }
	if ( ScreenQuadIndexes->user ) { vertexCache.Free( ScreenQuadIndexes ); }

    initialized = false;
}


void SoftShadowManager::NewFrame()
{
    // Cater for annoying slight window resizing when windows user tabs in and out of game.
	if ( initialized && (width != glConfig.vidWidth || height != glConfig.vidHeight) )
    {
        UnInit();
    }

	// This is the main/only initialization path.
    if ( !initialized )
    {
        Init();
    }

	if ( spamConsole )
	{
		uint total = localShadowDrawCounter + globalShadowDrawCounter;
		common->Printf("Soft shadows: %d lights, %d soft shadow passes (%d no-self-shadow, %d other)\n", 
						lightCounter, total, localShadowDrawCounter, globalShadowDrawCounter ); // reporting prev frame
	}

	localShadowDrawCounter = 0;
	globalShadowDrawCounter = 0;
	lightCounter = 0;
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
	// Normal map, captured during early depth pass. Drawn into main color buffer so need to use GL_RGBA8
	tex[normal_tx] = globalImages->RendertargetImage("normal_tx", potWidth, potHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE );
	
	// Depth-Stencil renderbuffer object for use when drawing shadow volumes and interactions
    qglGenRenderbuffers(1, &depthRbo);
    qglBindRenderbuffer( GL_RENDERBUFFER, depthRbo );
    qglRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height );
    qglBindRenderbuffer( GL_RENDERBUFFER, 0 );

     // Color copy of the stencil buffer
	tex[colorStencil_tx] = globalImages->RendertargetImage("colorStencil_fb", potWidth, potHeight, GL_R8, GL_RED, GL_UNSIGNED_BYTE );

	// Penumbra size map
    tex[penumbraSize_tx] = globalImages->RendertargetImage("penumbraSize_tx", potWidth, potHeight, GL_RG16F, GL_RGBA, GL_FLOAT );

	// Small-scale penumbra size maps
    const int smallSize[2] = { potWidth / MINISCALE, potHeight / MINISCALE };
    tex[penumbraSpread1_tx] = globalImages->RendertargetImage("penumbraSpread1_tx", smallSize[0], smallSize[1], GL_RG16F, GL_RGBA, GL_FLOAT );
    tex[penumbraSpread2_tx] = globalImages->RendertargetImage("penumbraSpread2_tx", smallSize[0], smallSize[1], GL_RG16F, GL_RGBA, GL_FLOAT );

	// Alpha mask for the light interactions
	tex[shadowBlur_tx] = globalImages->RendertargetImage("shadowBlur_tx", potWidth, potHeight, GL_R8, GL_RED, GL_UNSIGNED_BYTE );

	// Jitter map. Not strictly speaking a render target, but handy to put it here
	tex[jitterMap_tx] = globalImages->ImageFromFunction("jitterMap_tx", R_JitterMap);
}


GLuint SoftShadowManager::CreateShader( GLuint type, const GLchar* src )
{
	const GLuint shd = qglCreateShader( type );
    qglShaderSource( shd, 1, &src, NULL );
    qglCompileShader( shd );
    assert( ShaderOK( shd ) );
	return shd;
}


GLuint SoftShadowManager::CreateShaderProg( GLuint vp, GLuint fp )
{
	const GLuint prg =  qglCreateProgram();
    qglAttachShader( prg, vp );
    qglAttachShader( prg, fp );
    qglLinkProgram( prg );
    assert( LinkedOK(prg) );
	return prg;
}


void SoftShadowManager::InitShaders()
{
    // Generic screen-quad shader
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
        out vec4 outColor;

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
    UNF_QUAD_pos = qglGetAttribLocation( prg_quad, "pos" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_quad );
    qglUniform2fv( qglGetUniformLocation( prg_quad, "maxTexcoord" ), 1, MaxTexcoord(true).ToFloatPtr() );
    qglUniform1i ( qglGetUniformLocation( prg_quad, "tex" ), 0 );
    qglUseProgram( 0 );
	
	// Shader to capture surface normals during the early depth pass
	const GLchar* NormalVP = GLSLold(
		in		vec3 normal;
        out     vec3 screenSpaceNormal;
		
		void main()
        {
            vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
			gl_Position = pos;
			screenSpaceNormal = ( gl_ModelViewProjectionMatrixInverseTranspose * vec4(normal, 0.0) ).xyz; 
			//~not sure why we don't need the invtrans here but this works that doesn't. invtrans almost always gives b of 0 or 255 and 
			//~Now not working! statue sternum is r 213=strongly +ve whch doesn't make sense. And b always -ve
			//WS values look alright
			//~DEBUG
			vec4 normalpos = gl_ModelViewProjectionMatrix * ( gl_Vertex + vec4(normalize(normal), 0.0) ); // looks like same result as invtrans
			screenSpaceNormal = normalpos.xyz - pos.xyz; // /w? made fior weird banding
		}
	);

	const GLchar* NormalFP = GLSLold(
		in		vec3 screenSpaceNormal;
		out     vec4 oColor;

		const vec3 rangeConverter = vec3( 0.5 ); // A mad converts range from -1..+1 to 0..1
		
		void main()
        {
            // Use the alpha channel to record whether depth hack is in effect, which restricts depth
			// range for player weapon. We won't draw soft shadows where depth range has been meddled with.
			vec3 norm = normalize(screenSpaceNormal) * rangeConverter + rangeConverter;
			oColor = vec4( norm, gl_DepthRange.diff );
        }
	);

	shaders[normal_vp] = qglCreateShader( GL_VERTEX_SHADER );
    qglShaderSource( shaders[normal_vp], 1, &NormalVP, NULL );
    qglCompileShader( shaders[normal_vp] );
    assert( ShaderOK( shaders[normal_vp] ) );

    shaders[normal_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
    qglShaderSource( shaders[normal_fp], 1, &NormalFP, NULL );
    qglCompileShader( shaders[normal_fp] );
    assert( ShaderOK( shaders[normal_fp] ) );

    const GLuint prg_normal =  qglCreateProgram();
    glslProgs[normal_pr] = prg_normal;
    qglAttachShader( prg_normal, shaders[normal_vp] );
    qglAttachShader( prg_normal, shaders[normal_fp] );
    qglLinkProgram( prg_normal );
    assert( LinkedOK(prg_normal) );
    UNF_NORM_normal = qglGetAttribLocation( prg_normal, "normal" );
	
	// Stencil shadow shader program
    // Does the usual job of projecting the shadow volume away from the light,
    // but also outputs penumbra size to a color buffer.
    // Use "old" GLSL compatibility mode so we can access the built-in matrix stack.
    const GLchar* SoftShadowVP = GLSLold(
        uniform		vec4	lightPos;			/* in model space */
        uniform		float	lightRadius;		/* size of the light source */
        uniform		float	lightReach;			/* max distance that light can hit */
		out			float	shadowSpread;		
		out			vec4	shadowEdgeViewPos;  /* in view space */
		out			vec2	fragmentViewPosXY;	/* in view space */

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
			/* Output penumbra spread and important coordinates*/
            float distFromLight = distance( lightPos.xyz, gl_Vertex.xyz );
			/* for attempt #2, instead of screenSpaceSize */
			shadowSpread = lightRadius / distFromLight; /* same for w=1 and w=0 verts */
			shadowEdgeViewPos = gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0); /* same for w=1 and w=0 verts. */
			fragmentViewPosXY = ( gl_ModelViewMatrix * vec4(position, 1.0) ).xy;
			
			/* Earlier attempt commented out. This calculated the penumbra size in 
			   screen space along the face of the shadow volume, to save the fragment shader 
			   doing it by calculating scene depth. Unfortunately it gives weird results where a
			   shadow volume edge is seen edge-on, which means a huge range of depths covered by one 
			   fragment. Left in to stop me making the same mistake again.
            float penumbraSize = ( 1.0 - gl_Vertex.w ) * ( lightReach - distFromLight ) * lightRadius / distFromLight;
			screenSpaceSize = ( gl_ProjectionMatrix * vec4( penumbraSize, penumbraSize, -1.0, 1.0 ) ).xy;
			*/
        }
    );

    /* Stencil shadow fragment shader tests whether the fragment is very close to the scene depth, 
       and writes the penumbra size at that point if so. The idea is to produce a line marking the 
       centre of each penumbra with size info. */
    const GLchar* SoftShadowFP = GLSLold(
		in		float	shadowSpread;			/* attempt #2: Just output this factor so the frag shader can calc the pen size based on scene depth */
		in		vec4	shadowEdgeViewPos;		/* also for attempt #2. in view space */
		in		vec2	fragmentViewPosXY;		/* also for attempt #2 */
        uniform	ivec2	screenSize;
		uniform sampler2D depthtex;
        uniform vec2 invDepthImageSize;
        uniform float threshold;
        
        void main()
        {
            /* sample scene depth */
            vec2 texcoord = gl_FragCoord.xy * invDepthImageSize;
            float sceneDepth = texture( depthtex, texcoord ).x;
            float depthDiff = abs( sceneDepth - gl_FragCoord.z );
            float maxDepthDelta = fwidth(sceneDepth) + fwidth(gl_FragCoord.z); // greatest relative depth change between shadow vol and scene
			bool sharpDepthDiscontinuity = fwidth(sceneDepth) > 0.0005 || fwidth(gl_FragCoord.z) > threshold;

			if ( depthDiff < maxDepthDelta && !sharpDepthDiscontinuity )
			{
				// debug
				// show linear depth of fragment and scene
				//float linear_scenedepth = -1.0 / ( ( 2 * sceneDepth - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
				//float linear_fragdepth = -1.0 /  ( ( 2 * gl_FragCoord.z - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
				//float linear_fragchange = -1.0 /  ( ( 2 * fwidth(gl_FragCoord.z) - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
				//float linear_depthdiff =  -1.0 /  ( ( 2 * depthDiff - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
				//
				float clampedDepth = min(sceneDepth, 0.99949); // 0.9995 is infinite depth in TDM's proj matrix
				float invViewDepth = ( 2 * clampedDepth - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2];
				float viewDepth = 1.0 / invViewDepth;
				vec4 scenePosition = vec4( fragmentViewPosXY, viewDepth, 1.0 );
				float pSize = distance( shadowEdgeViewPos, scenePosition ) * shadowSpread;
				vec2 pixelCoverageFor1Unit = ( gl_ProjectionMatrix * vec4( 1.0, 1.0, -1.0, 1.0 ) ).xy * screenSize * 0.5; //~TODO:Uniform
				ivec2 pixelCoverage = ivec2( pSize * pixelCoverageFor1Unit * -invViewDepth );

				if ( pSize > 0.1 )
				{
					gl_FragColor = vec4( pixelCoverage.x, pSize, 0.0, 1.0 );
				} else {
					gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0 );
				}
			} else {
				gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0 );
			}
        }
    );
    
    shaders[shadow_vp] = CreateShader( GL_VERTEX_SHADER, SoftShadowVP );
    shaders[shadow_fp] = CreateShader( GL_FRAGMENT_SHADER, SoftShadowFP );
	glslProgs[stencilShadow_pr] = CreateShaderProg( shaders[shadow_vp], shaders[shadow_fp] );
    UNF_SHADOW_lightPos = qglGetUniformLocation( glslProgs[stencilShadow_pr], "lightPos" );
    UNF_SHADOW_lightRadius = qglGetUniformLocation( glslProgs[stencilShadow_pr], "lightRadius" );
    UNF_SHADOW_lightReach = qglGetUniformLocation( glslProgs[stencilShadow_pr], "lightReach" );
    UNF_SHADOW_invDepthImageSize = qglGetUniformLocation( glslProgs[stencilShadow_pr], "invDepthImageSize" );
    // debug threshold
    UNF_SHADOW_threshold = qglGetUniformLocation( glslProgs[stencilShadow_pr], "threshold" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( glslProgs[stencilShadow_pr] );
    qglUniform1i( qglGetUniformLocation( glslProgs[stencilShadow_pr], "depthtex" ), 0 );
	qglUniform2i( qglGetUniformLocation( glslProgs[stencilShadow_pr], "screenSize" ), width, height );
	qglUseProgram( 0 );

    // Penumbra-Spread shaders
    // First the minification shader
    const GLchar* MiniVP = GLSL(
        in      vec2 pos;
        uniform vec2 maxTexcoord;
        uniform vec2 invTextureSize;
        out     vec2 texcoord[16];  // 16 guaranteed in spec. Each will be used for a 2x2 block of pixels.
        
        const   vec2 posNormalizer = vec2( 0.5, 0.5 ); // Scale NDC xy screen coords (-1 to +1) to the range 0 to 1.
        const   int  halfscale = 4; // 4 means 8x minification

        void main()
        {
            gl_Position = vec4( pos, 0.0, 1.0 );
            vec2 npos = pos * posNormalizer + posNormalizer;
            vec2 baseTexcoord = npos * maxTexcoord - invTextureSize * halfscale; // Subtract half a pixel else samples will be biased up and right
            for ( int x=0; x < halfscale; ++x )
            {
                for ( int y=0; y < halfscale; ++y )
                {
                    texcoord[ x * halfscale + y ] = baseTexcoord + vec2(x, y) * invTextureSize * 2.0;
                }
            }
        }
    );

    const GLchar* MiniFP = GLSL(
        uniform vec2 invTextureSize;
        uniform sampler2D penumbraImg;
		uniform sampler2D stencilImg;
        in      vec2 texcoord[16];
        out     vec4 oColor;
		const	float BIGFLOAT = 1.0e+20;

        void main()
        {
            // Our texcoords are the the bottom left of 2x2-pixel blocks. Create some 1-pixel texture displacements.
			vec2 up = vec2( 0.0, invTextureSize.y );
			vec2 right = vec2( invTextureSize.x, 0.0 );
			// First check whether this area contains a light/shadow division. If not, don't
			// take any samples from the penumbra line map. Helps clean up lines. Test the 4 
			// corners of the square plus the centre. Spread out a bit to overlap the neighboring 
			// sample, else it's possible for a valid line to sit exactly along the border and be missed by both.
			float penumbraTest = 0.0;
			penumbraTest += texture( stencilImg, texcoord[0] - up - right ).r;
			penumbraTest += texture( stencilImg, texcoord[3] + up * 2.0 - right ).r;
			penumbraTest += texture( stencilImg, texcoord[12] - up + right * 2.0 ).r;
			penumbraTest += texture( stencilImg, texcoord[15] + up * 2.0 + right * 2.0 ).r;
			penumbraTest += texture( stencilImg, texcoord[10] ).r; // rough centre

			vec2 minPenumbra = vec2(BIGFLOAT);

			if ( penumbraTest > 0.0 && penumbraTest < 5.0 )
			{
				for ( int i=0; i<16; ++i )
				{
					/*vec4 samp = vec4(
						texture( penumbraImg, texcoord[i] ).x,
						texture( penumbraImg, texcoord[i] + up ).x,
						texture( penumbraImg, texcoord[i] + right ).x,
						texture( penumbraImg, texcoord[i] + up + right ).x
					);
					maxPenumbra = max( maxPenumbra, max( max(samp.x, samp.y), max(samp.z, samp.w) ) );
					*/
					vec2 smp;
					smp = texture( penumbraImg, texcoord[i] ).rg;
					if ( smp.g > 0.0 ) { minPenumbra = min( minPenumbra, smp ); }
					smp = texture( penumbraImg, texcoord[i] + up ).rg;
					if ( smp.g > 0.0 ) { minPenumbra = min( minPenumbra, smp ); }
					smp = texture( penumbraImg, texcoord[i] + right ).rg;
					if ( smp.g > 0.0 ) { minPenumbra = min( minPenumbra, smp ); }
					smp = texture( penumbraImg, texcoord[i] + up + right ).rg;
					if ( smp.g > 0.0 ) { minPenumbra = min( minPenumbra, smp ); }
				}
			}
			// MaxPenumbra is a count of pixels. Output remaining amount to spread in x, orig penumbra size in y.
			// Remaining spread is half pen width, scaled down by the minification factor of 8.
			float size = minPenumbra.g < BIGFLOAT ? minPenumbra.g : 0.0; 
			float spread = minPenumbra.r < BIGFLOAT ? minPenumbra.r / 16.0 + 1.25 : 0.0; 
            oColor = vec4( spread, size, 0.0, 1.0 ); 
        }
    );

    shaders[mini_vp] = qglCreateShader( GL_VERTEX_SHADER );
    qglShaderSource( shaders[mini_vp], 1, &MiniVP, NULL );
    qglCompileShader( shaders[mini_vp] );
    assert( ShaderOK( shaders[mini_vp] ) );

    shaders[mini_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
    qglShaderSource( shaders[mini_fp], 1, &MiniFP, NULL );
    qglCompileShader( shaders[mini_fp] );
    assert( ShaderOK( shaders[mini_fp] ) );

    const GLuint prg_mini =  qglCreateProgram();
    glslProgs[mini_pr] = prg_mini;
    qglAttachShader( prg_mini, shaders[mini_vp] );
    qglAttachShader( prg_mini, shaders[mini_fp] );
    qglLinkProgram( prg_mini );
    assert( LinkedOK(prg_mini) );
    UNF_MINI_pos = qglGetAttribLocation( prg_mini, "pos" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_mini );
    qglUniform2fv( qglGetUniformLocation( prg_mini, "maxTexcoord" ), 1, MaxTexcoord(true).ToFloatPtr() );
    qglUniform1i ( qglGetUniformLocation( prg_mini, "penumbraImg" ), 0 );
	qglUniform1i ( qglGetUniformLocation( prg_mini, "stencilImg" ), 1 );
    qglUniform2f( qglGetUniformLocation( prg_mini, "invTextureSize" ), 
                  1.0f / tex[penumbraSize_tx]->uploadWidth, 
                  1.0f / tex[penumbraSize_tx]->uploadHeight );
    qglUseProgram( 0 );


	// A penumbra-minimizing step as a first pass befpore penumbra spreading. Gives each pixel the minimum 
	// penumbra size of itself and its neighbours. Should help with single-pixel bursts and intersections.
	const GLchar* minpVP = GLSL(
		in      vec2 pos;
		uniform vec2 maxTexcoord;
        uniform vec2 invTextureSize;
        out     vec2 texcoord[9];
        
        const   vec2 posNormalizer = vec2( 0.5, 0.5 );
		
		void main()
		{
            vec2 baseTexcoord = ( pos * posNormalizer + posNormalizer ) * maxTexcoord;
			texcoord[0] = baseTexcoord + invTextureSize * vec2(  0.0,  0.0 ); // out of sequence, but put "own" sample first
			texcoord[1] = baseTexcoord + invTextureSize * vec2( -1.0, -1.0 );
			texcoord[2] = baseTexcoord + invTextureSize * vec2(  0.0, -1.0 );
			texcoord[3] = baseTexcoord + invTextureSize * vec2(  1.0, -1.0 );
			texcoord[4] = baseTexcoord + invTextureSize * vec2( -1.0,  0.0 );
			texcoord[5] = baseTexcoord + invTextureSize * vec2(  1.0,  0.0 );
			texcoord[6] = baseTexcoord + invTextureSize * vec2( -1.0,  1.0 );
			texcoord[7] = baseTexcoord + invTextureSize * vec2(  0.0,  1.0 );
			texcoord[8] = baseTexcoord + invTextureSize * vec2(  1.0,  1.0 );
			gl_Position = vec4( pos, 0.0, 1.0 );
		}
	);

	const GLchar* minpFP = GLSL(
		in vec2 texcoord[9];
		uniform sampler2D tex;
		out	vec4 outColor;

		void main()
		{
			vec2 smp = texture( tex, texcoord[0] ).xy; // test "own" sample first
			vec2 result = smp;
			if ( smp.y > 0.0 )
			{
				for ( int i = 1; i < 9; ++i )
				{
					smp = texture( tex, texcoord[i] ).xy;
					if ( smp.y > 0.0 ) { result = min( result, smp ); }
				}
			}
			outColor = vec4( result, 0.0, 0.1 );
		}
	);

	shaders[minp_vp] = qglCreateShader( GL_VERTEX_SHADER );
    qglShaderSource( shaders[minp_vp], 1, &minpVP, NULL );
    qglCompileShader( shaders[minp_vp] );
    assert( ShaderOK( shaders[minp_vp] ) );

    shaders[minp_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
    qglShaderSource( shaders[minp_fp], 1, &minpFP, NULL );
    qglCompileShader( shaders[minp_fp] );
    assert( ShaderOK( shaders[minp_fp] ) );

    const GLuint prg_minp =  qglCreateProgram();
    glslProgs[minp_pr] = prg_minp;
    qglAttachShader( prg_minp, shaders[minp_vp] );
    qglAttachShader( prg_minp, shaders[minp_fp] );
    qglLinkProgram( prg_minp );
    assert( LinkedOK(prg_minp) );
	UNF_MINP_pos = qglGetAttribLocation( prg_minp, "pos" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_minp );
    qglUniform2fv( qglGetUniformLocation( prg_minp, "maxTexcoord" ), 1, MaxTexcoord(true).ToFloatPtr() );
    qglUniform1i ( qglGetUniformLocation( prg_minp, "tex" ), 0 );
    qglUniform2f( qglGetUniformLocation( prg_minp, "invTextureSize" ), 
                  1.0f / tex[penumbraSpread1_tx]->uploadWidth, 
                  1.0f / tex[penumbraSpread1_tx]->uploadHeight );
    qglUseProgram( 0 );


    // The penumbra-spreading shader
    const GLchar* SpreadVP = GLSL(
        in      vec2	pos;

        void main()
        {
            gl_Position = vec4( pos, 0.0, 1.0 );
        }
    );
    
    const GLchar* SpreadFP = GLSL(
        uniform sampler2D	tex;
		uniform int			amount; // This doubles with each iteration
        out     vec4		oColor;
        
		vec2 mergeSample( vec2 result, vec2 sample )
		{
			return max(result, vec2(sample.x-amount, sample.y)); // max spread and penumbra ... big, softer penumbrae eat chunks out of smaller ones.
			//return vec2( max(result.x, sample.x-amount), min(result.y, sample.y) ); // max spread min penumbra size ... causes blotchyness on overlaps
			//return min(result, vec2(sample.x-amount, sample.y)); // min spread and penumbra ... better, holes are gone, but still quivery and blotchy
			//if (result.x==0.0) result.x = sample.x; // hack to allow avg
			//if (result.y==0.0) result.y = sample.y; // hack to allow avg
			//return vec2( (result.x + sample.x - amount) / 2.0, max(result.y, sample.y) ); // avg spread, max pen ... best yet but quivery still and harder pen still getting softened
			//return vec2( max(result.x, sample.x - amount), (result.y + sample.y) / 2.0 ); // max spread, avg pen ... better not too quivery but some eating still
			//return vec2( (result.x + sample.x - amount) / 2.0, min(result.y, sample.y) ); // avg spread, min pen ... unstable 
			//return vec2( max(result.x, sample.x - amount), (result.y + sample.y + min(result.y, sample.y)) / 3.0 ); // max spread, avg pen biased towards smaller .. unstable still
		}

        void main()
        {
			// Output remaining amount to spread in x, original penumbra size in y.
			// The shader samples 8 points around a square to find penumbra regions
			// that have enough spread (x component) to reach the current pixel, and 
			// that have a greater penumbra value than the currently recorded one.
			// The "amount" is the distance in pixels between current pixel and edge samples.
			// By doubling it with each iteration, every pixel can spread info to every other
			// in just a few passes, at minified scale.
			ivec2 coord = ivec2( gl_FragCoord.xy ); // truncate the frag coord
			vec2 smp;
			vec2 result = texelFetch(tex, coord, 0).xy; // Existing value at this pixel

			smp = texelFetch(tex, coord + ivec2(amount, 0), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(-amount, 0), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(0, amount), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(0, -amount), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(amount, amount), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(-amount, amount), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(amount, -amount), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }
			smp = texelFetch(tex, coord + ivec2(-amount, -amount), 0).xy;
			if ( smp.x > amount ) { result = mergeSample(result, smp); }

			//if (result.x == 10000.0) result.x = 0.0; // hack to allow min spread detection
			//if (result.y == 10000.0) result.y = 0.0; // hack to allow min pen detection

			oColor = vec4(result, 0.0, 0.1);
        }
    );

	shaders[spread_vp] = qglCreateShader( GL_VERTEX_SHADER );
    qglShaderSource( shaders[spread_vp], 1, &SpreadVP, NULL );
    qglCompileShader( shaders[spread_vp] );
    assert( ShaderOK( shaders[spread_vp] ) );

    shaders[spread_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
    qglShaderSource( shaders[spread_fp], 1, &SpreadFP, NULL );
    qglCompileShader( shaders[spread_fp] );
    assert( ShaderOK( shaders[spread_fp] ) );

    const GLuint prg_spread =  qglCreateProgram();
    glslProgs[spread_pr] = prg_spread;
    qglAttachShader( prg_spread, shaders[spread_vp] );
    qglAttachShader( prg_spread, shaders[spread_fp] );
    qglLinkProgram( prg_spread );
    assert( LinkedOK(prg_spread) );
    UNF_SPREAD_pos = qglGetAttribLocation( prg_spread, "pos" );
	UNF_SPREAD_amount = qglGetUniformLocation( prg_spread, "amount" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_spread );
    qglUniform1i ( qglGetUniformLocation( prg_spread, "tex" ), 0 );
    qglUseProgram( 0 );


	// An averaging blur for the final minified pass. Samples from populated neighbours only, 
	// ignoring null pixels, so it doesn't spread the pattern or attentuate it at the edges. 
	// Uses gaussian weightings but normalizes the result for discarded samples.
	const GLchar* avgVP = GLSL(
		in      vec2	pos;
		
		void main()
		{
			gl_Position = vec4( pos, 0.0, 1.0 );
		}
	);

	const GLchar* avgFP = GLSL(
		uniform sampler2D tex;
		out	vec4 outColor;

		void main()
		{
			// Using 5-pixel gaussian weightings
			ivec2 coord = ivec2( gl_FragCoord.xy ); // truncate the frag coord
			float totalWeight = 0.150342;			// weighting for the centre pixel
			vec4 col = texelFetch(tex, coord, 0) * 0.150342;	// test current pixel for penumbra first
			if ( col.y > 0.0 )
			{
				vec4 smp;
				// The 4 corners
				const float weight1 = 0.003765;
				smp = texelFetch( tex, coord - ivec2(-2, -2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight1; totalWeight += weight1; }
				smp = texelFetch( tex, coord - ivec2(-2,  2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight1; totalWeight += weight1; }
				smp = texelFetch( tex, coord - ivec2( 2, -2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight1; totalWeight += weight1; }
				smp = texelFetch( tex, coord - ivec2( 2,  2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight1; totalWeight += weight1; }
				// 8 pixels adjacent to the corners
				const float weight2 = 0.015019;
				smp = texelFetch( tex, coord - ivec2(-2, -1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2(-1, -2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2(-2,  1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2(-1,  2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2( 1,  2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2( 2,  1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2( 2, -1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				smp = texelFetch( tex, coord - ivec2( 1, -2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight2; totalWeight += weight2; }
				// centre pixels on each side
				const float weight3 = 0.023792;
				smp = texelFetch( tex, coord - ivec2(-2,  0), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight3; totalWeight += weight3; }
				smp = texelFetch( tex, coord - ivec2( 2,  0), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight3; totalWeight += weight3; }
				smp = texelFetch( tex, coord - ivec2( 0, -2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight3; totalWeight += weight3; }
				smp = texelFetch( tex, coord - ivec2( 0,  2), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight3; totalWeight += weight3; }
				// 4 pixels diagonally adjacent to the corners
				const float weight4 = 0.059912;
				smp = texelFetch( tex, coord - ivec2(-1, -1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight4; totalWeight += weight4; }
				smp = texelFetch( tex, coord - ivec2(-1,  1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight4; totalWeight += weight4; }
				smp = texelFetch( tex, coord - ivec2( 1, -1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight4; totalWeight += weight4; }
				smp = texelFetch( tex, coord - ivec2( 1,  1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight4; totalWeight += weight4; }
				// 4 pixels adjacent to the centre
				const float weight5 = 0.094907;
				smp = texelFetch( tex, coord - ivec2( 0, -1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight5; totalWeight += weight5; }
				smp = texelFetch( tex, coord - ivec2( -1, 0), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight5; totalWeight += weight5; }
				smp = texelFetch( tex, coord - ivec2( 0,  1), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight5; totalWeight += weight5; }
				smp = texelFetch( tex, coord - ivec2( 1,  0), 0 );
				if ( smp.y > 0.0 ) { col.xy += smp.xy * weight5; totalWeight += weight5; }
			}
			outColor = col / totalWeight; // only xy-components will be used
		}
	);

	shaders[avg_vp] = qglCreateShader( GL_VERTEX_SHADER );
    qglShaderSource( shaders[avg_vp], 1, &avgVP, NULL );
    qglCompileShader( shaders[avg_vp] );
    assert( ShaderOK( shaders[avg_vp] ) );

    shaders[avg_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
    qglShaderSource( shaders[avg_fp], 1, &avgFP, NULL );
    qglCompileShader( shaders[avg_fp] );
    assert( ShaderOK( shaders[avg_fp] ) );

    const GLuint prg_avg =  qglCreateProgram();
    glslProgs[avg_pr] = prg_avg;
    qglAttachShader( prg_avg, shaders[avg_vp] );
    qglAttachShader( prg_avg, shaders[avg_fp] );
    qglLinkProgram( prg_avg );
    assert( LinkedOK(prg_avg) );
	UNF_AVG_pos = qglGetAttribLocation( prg_avg, "pos" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_avg );
    qglUniform1i( qglGetUniformLocation( prg_avg, "tex" ), 0 );
	qglUseProgram( 0 );

	// The shadow blur program
	const GLchar* BlurVP = GLSL(
        uniform vec2 maxTexcoord;
        in      vec2 pos;
		out     vec2 texcoord;
		const   vec2 posNormalizer = vec2( 0.5, 0.5 );

        void main()
        {
            texcoord = ( pos * posNormalizer + posNormalizer ) * maxTexcoord;
			gl_Position = vec4( pos, 0.0, 1.0 );
        }
    );
    
    const GLchar* BlurFP = GLSL(
        uniform sampler2D	penumbraImage;
		uniform sampler2D	depthImage;		//~TODO: limit depth
		uniform sampler2D	shadowImage;
		uniform sampler3D	jitterImage;
		uniform	float		jitterMapScale; //  = 1 / jitterMapSize
		uniform vec2		screenCoverage1Unit;
		uniform vec2		projMatrix; // Proj matrix c2r2 and c3r2 for depth calculations
		uniform vec4		lightScissor; // In screen proportions 0..1 instead of pixels. xyzw = x1, y1, x2, y2
		in		vec2		texcoord;
        out     vec4		oColor;
        const	int			sampleCount = 64;
		
		float InvViewDepth( float depthValue )
		{
			
			// 0.9995 is infinity in TDM's proj matrix. Avoid handling infinities (caulk sky)
			float dv = min(depthValue, 0.99949);
			return ( 2.0 * dv - 1.0 + projMatrix.x ) / -projMatrix.y;
		}

		float DepthValue( float viewDepth )
		{
			return (1.0 + ( projMatrix.x * viewDepth + projMatrix.y ) / -viewDepth ) / 2.0;
		}

		float GetCheckedSample( vec2 txCoord, float minDepth, float maxDepth )
		{
			//~Debug: Disable out of bounds check
			//return texture(shadowImage, txCoord).r;
			// Test whether sample is outside the current scissor, or out of depth bounds for this penumbra
			bool outOfBounds = false;
			if ( txCoord.x < lightScissor.x || txCoord.y < lightScissor.y || txCoord.x > lightScissor.z || txCoord.y > lightScissor.w )
			{
				outOfBounds = true;
			} else {
				//~Debug: Disable depth check until we correct sampling disc for surface normal
				/*float depth = texture( depthImage, txCoord ).r;
				if ( depth < minDepth || depth > maxDepth )
				{
					outOfBounds = true;
				}*/
			}
			if ( outOfBounds )
			{
				return 0.5;
			} else {
				return texture(shadowImage, txCoord).r;
			}
		}

        void main()
        {
			float penumbraSize = texture( penumbraImage, texcoord ).y / 2.0; // Convert from diameter to radius

			if ( penumbraSize > 0.0 )
			{
				float sceneDepthVal = texture(depthImage, texcoord).r;
				float invSceneDepth = InvViewDepth( sceneDepthVal );
				float sceneDepth = 1.0 / invSceneDepth;
				float minDepth = DepthValue( sceneDepth + penumbraSize ); // NB viewdepths are negative
				float maxDepth = DepthValue( sceneDepth - penumbraSize );
				vec2 screenCoverage = penumbraSize * screenCoverage1Unit * 0.5 * -invSceneDepth; // 0.5: Convert NDC range -1 to 1 to screen space 0 to 1
				
				float	shadow = 0.0;
				vec3	jitterCoord = vec3( gl_FragCoord.xy * jitterMapScale, 0.0 );
				float	jitterMapZStep = 1.0 / (sampleCount / 2);

				for ( int i = 0; i < sampleCount / 2; ++i, jitterCoord.z += jitterMapZStep )
				{
					vec4 offset = texture( jitterImage, jitterCoord ) * vec4( screenCoverage, screenCoverage );
					vec2 smpCoord = offset.xy + texcoord;
					shadow += GetCheckedSample( smpCoord, minDepth, maxDepth );
					smpCoord = offset.zw + texcoord;
					shadow += GetCheckedSample( smpCoord, minDepth, maxDepth );
				}
				shadow /= sampleCount;
				oColor = vec4( shadow, 0.0, 0.0, 1.0 );
			}
			else
			{
				// we're not in penumbra. Just use the current shadow result from the stencil
				oColor = vec4( texture(shadowImage, texcoord).r, 0.0, 0.0, 1.0 );
			}
        }
    );

	shaders[blur_vp] = qglCreateShader( GL_VERTEX_SHADER );
    qglShaderSource( shaders[blur_vp], 1, &BlurVP, NULL );
    qglCompileShader( shaders[blur_vp] );
    assert( ShaderOK( shaders[blur_vp] ) );

    shaders[blur_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
    qglShaderSource( shaders[blur_fp], 1, &BlurFP, NULL );
    qglCompileShader( shaders[blur_fp] );
    assert( ShaderOK( shaders[blur_fp] ) );

    const GLuint prg_blur =  qglCreateProgram();
    glslProgs[blur_pr] = prg_blur;
    qglAttachShader( prg_blur, shaders[blur_vp] );
    qglAttachShader( prg_blur, shaders[blur_fp] );
    qglLinkProgram( prg_blur );
    assert( LinkedOK(prg_blur) );
    UNF_BLUR_pos = qglGetAttribLocation( prg_blur, "pos" );
	UNF_BLUR_projMatrix = qglGetUniformLocation( prg_blur, "projMatrix" );
	UNF_BLUR_lightScissor = qglGetUniformLocation( prg_blur, "lightScissor" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_blur );
	qglUniform2fv( qglGetUniformLocation( prg_blur, "maxTexcoord" ), 1, MaxTexcoord(true).ToFloatPtr() );
    qglUniform1i( qglGetUniformLocation( prg_blur, "penumbraImage" ), 0 );
	qglUniform1i( qglGetUniformLocation( prg_blur, "depthImage" ), 1 );
	qglUniform1i( qglGetUniformLocation( prg_blur, "shadowImage" ), 2 );
	qglUniform1i( qglGetUniformLocation( prg_blur, "jitterImage" ), 3 );
	qglUniform1f( qglGetUniformLocation( prg_blur, "jitterMapScale" ), 1.0f / JITTERMAPSIZE );
	idVec3 screenCoverage1Unit;
	R_EyeToNormalizedDeviceCoordinates( idVec3( 1.0f, 1.0f, -1.0f ), screenCoverage1Unit );
	// This is coverage for a penumbra seen at 1 unit depth. Needs perspective divide with actual depth in the shader
    qglUniform2fv( qglGetUniformLocation( prg_blur, "screenCoverage1Unit" ), 1, screenCoverage1Unit.ToFloatPtr() ); 

    // Copy the resulting alpha mask back to the main color buffer, and create the
	// stencil for light interactions at the same time. Re-use the quad VP.
	const GLchar* CopybackFP = GLSL(
		in			vec2		texcoord;
		uniform		sampler2D	alphaStencil;

		void main()
		{
			float alpha = texture( alphaStencil, texcoord ).r;
			if ( alpha < 0.001 )
			{
				discard; // How we write to stencil
			}
			gl_FragColor = vec4( 0.0, 0.0, 0.0, alpha);
		}
	);

	shaders[copyback_fp] = qglCreateShader( GL_FRAGMENT_SHADER );
	qglShaderSource( shaders[copyback_fp], 1, &CopybackFP, NULL );
	qglCompileShader( shaders[copyback_fp] );
	assert( ShaderOK( shaders[copyback_fp] ) );

	GLuint prg_copyback = qglCreateProgram();
	glslProgs[copyback_pr] = prg_copyback;
	qglAttachShader( prg_copyback, shaders[quad_vp] );
	qglAttachShader( prg_copyback, shaders[copyback_fp] );
	qglLinkProgram( prg_copyback );
	assert( LinkedOK(prg_copyback) );
	UNF_COPYBACK_pos = qglGetAttribLocation( prg_copyback, "pos" );
    qglUseProgram( prg_copyback );
	qglUniform2fv( qglGetUniformLocation( prg_copyback, "maxTexcoord" ), 1, MaxTexcoord(true).ToFloatPtr() );
    qglUniform1i( qglGetUniformLocation( prg_copyback, "alphaStencil" ), 0 );


	qglUseProgram( 0 );
}


void SoftShadowManager::InitFBOs()
{
    qglGenFramebuffers( NumFramebuffers, fbo );

	// Normals captured during early depth pass
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[normal_fb] );
	qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo );
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[normal_tx]->texnum, 0 );
	assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );

    // Stencil shadow pass FBO
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSize_fb] );
    qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[penumbraSize_tx]->texnum, 0 );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
    
    // For copying the stencil contents to a color buffer
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[colorStencil_fb] );
    qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[colorStencil_tx]->texnum, 0 );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );

	// FBO for penumbra spreading using a downsized image. 2 textures for ping-ponging
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSpread_fb] );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[penumbraSpread1_tx]->texnum, 0 );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex[penumbraSpread2_tx]->texnum, 0 );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );

	// For the blurred shadow stencil
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[shadowBlur_fb] );
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[shadowBlur_tx]->texnum, 0 );
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


void SoftShadowManager::DepthPass( drawSurf_t **drawSurfs, int numDrawSurfs )
{
	// Use the usual depth pass code, but capture a surface normal map too, using an ARB fragment shader
	// Also write to the stencil buffer because we want to blank out the normal colors before we start to
	// add any light interactions
	
	// Stencil has been cleared to 1<<(glConfig.stencilBits-1) (=128) already, and will be set to always pass 
	// in RB_STD_FillDepthBuffer. Keep a record of where we drew normal colors, so we can black it out afterwards
	qglStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	qglUseProgram( glslProgs[normal_pr] );
	qglEnableVertexAttribArrayARB( UNF_NORM_normal ); // For surface normals
	__opengl_breakpoint
	RB_STD_FillDepthBuffer( drawSurfs, numDrawSurfs );

	qglDisableVertexAttribArrayARB( UNF_NORM_normal );
	qglUseProgram( 0 );

	// Capture the results.
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo[normal_fb] );
    qglBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
    qglBlitFramebuffer( 0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT, GL_NEAREST );
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );

	// Clear the normals from the main color buffer. Don't bother resetting stencil 
	// as individual lights will need to clear it anyway.
	qglStencilFunc( GL_NOTEQUAL, 1<<(glConfig.stencilBits-1), 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	GL_State( GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	qglUseProgram( glslProgs[quad_pr] );
	DrawQuad( globalImages->blackImage, UNF_QUAD_pos );
	qglUseProgram( 0 );
	__opengl_breakpoint
}


void SoftShadowManager::SetDepthNormalVertexAttribArray( const float* ptr )
{
	qglVertexAttribPointerARB( UNF_NORM_normal, 3, GL_FLOAT, false, sizeof( idDrawVert ), ptr );
}


void SoftShadowManager::MakeShadowStencil( const viewLight_t* vLight, const drawSurf_s* shadows, const bool clearStencil )
{
	static const GLenum targets[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 }; // Used by multiple FBOs

	ResetLightScissor( vLight );
	
	/**********
        Step 0. Early exit if we have no shadows to draw on this pass
     **********/
	if ( !shadows )
	{
		if ( clearStencil )
		{
			// We still need to clear the penumbra size image for any subsequent shadow passes from this light
			qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSize_fb] );
			assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
			qglClearStencil( 1<<(glConfig.stencilBits-1) );
			qglClearColor( 0.0, 0.0, 0.0, 0.0 );
			qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT );
			// As well as set up the empty stencil so the whole area is fully lit
			qglBindFramebuffer(GL_FRAMEBUFFER, 0 );
			qglClearColor( 0.0, 0.0, 0.0, 1.0 );
			qglClearStencil( 0 );
			GL_State( GLS_COLORMASK | GLS_DEPTHMASK ); // Alpha channel only
			qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT );
			// And set the stencil funcs ready for drawing light effects
			qglStencilFunc( GL_EQUAL, 0, 255 );
			qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		}
		return;
	}


	//~TODO: Activate depth bounds testing for these draws?

    /**********
        Step 1. Draw shadows into the stencil using existing technique, but using a new shader that draws 
        estimated penumbra size into a color buffer.
     **********/
    qglStencilFunc( GL_ALWAYS, 128, 255 );
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSize_fb] );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
    GL_SelectTexture( 0 );
    globalImages->currentDepthImage->Bind();
    qglUseProgram( glslProgs[stencilShadow_pr] );
    qglUniform1f( UNF_SHADOW_lightRadius, r_softShadows.GetFloat() ); //~TODO: Get this from a spawnarg
    qglUniform2f( UNF_SHADOW_invDepthImageSize, 
                  1.0f / globalImages->currentDepthImage->uploadWidth, 
                  1.0f / globalImages->currentDepthImage->uploadHeight );
    qglUniform1f( UNF_SHADOW_lightReach, 10000.0f ); //~TODO: use correct light size
    qglUniform1f( UNF_SHADOW_threshold, 0.05f /* r_ignore.GetFloat() */ ); // best so far: 0.0005f for scene, 0.05 for shadvoledges
    ResetLightScissor( vLight );
    qglBlendEquation( GL_MAX ); // This and clear color need to be in sync
	if ( clearStencil )
	{
		qglClearColor( 0.0, 0.0, 0.0, 0.0 );
		qglClearStencil( 1<<(glConfig.stencilBits-1) );
		qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT );
	}
	// Use the standard stencil shadow function to draw the volumes
    RB_StencilShadowPass( shadows );
    qglBlendEquation( GL_FUNC_ADD );
    qglBindFramebuffer( GL_FRAMEBUFFER, 0 );

	/**********
        Step 2. Copy the shadow stencil to a colour sampler. Draw with stencil on and off to save clearing + overwriting.
     **********/
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	ResetLightScissor( vLight );
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[colorStencil_fb] );
	qglUseProgram( glslProgs[quad_pr] );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	qglStencilFunc(GL_LESS, 128, 255);
	GL_SelectTexture( 0 );
    DrawQuad( globalImages->blackImage, UNF_QUAD_pos );
	qglStencilFunc(GL_GEQUAL, 128, 255);
	DrawQuad( globalImages->whiteImage, UNF_QUAD_pos );
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	__opengl_breakpoint

    /**********
        Step 3. Spread the penumbra information to all screen pixels that might be in penumbra.
        Do this at 1/8 size, so we get multiple passes while filling less than a screen's worth of pixels. 
        And there's no need to be super-accurate. We're identifying pixels that need to test whether they 
        are in penumbra, not drawing the penumbra.
		During the minification step, clean up any "penumbra lines" that are generated by side-on shadow volume
		faces and that are not part of an edge, by testing the corners of the sample area against the stencil.
     **********/
    // First downsample the penumbra size texture. Use a bespoke shader to preserve the max 
    // values of any pixels that are merged.
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSpread_fb] );
    qglViewport( 0, 0, smallwidth, smallheight );
    if( r_useScissor.GetBool() ) {
        qglScissor( 0, 0, smallwidth, smallheight ); //~TODO: set scissor as downscaled light scissor, and clear the mini texture instead
    }
    GL_SelectTexture( 0 );
	tex[penumbraSize_tx]->Bind();
	GL_SelectTexture( 1 );
	tex[colorStencil_tx]->Bind();
    qglUseProgram( glslProgs[mini_pr] );
    qglDrawBuffers( 1, &targets[0] );
    DrawQuad( NULL, UNF_MINI_pos );
	globalImages->BindNull(); // still on texture 1
	__opengl_breakpoint

	// The minified images get set to use linear samplimg when upsampled. Reset to nearest
	GL_SelectTexture( 0 );
	tex[penumbraSpread1_tx]->Bind();
	tex[penumbraSpread1_tx]->filter = TF_NEAREST;
	tex[penumbraSpread1_tx]->SetImageFilterAndRepeat();
	tex[penumbraSpread2_tx]->Bind();
	tex[penumbraSpread2_tx]->filter = TF_NEAREST;
	tex[penumbraSpread2_tx]->SetImageFilterAndRepeat();

	// state vars for the minified penumbra-propagation passes.
	uint tgt = 1;
	uint src = 0;
	idImage* srcimg = NULL;
	
	//*
	// Minifying operation as pre-pass
	// Try making penumbra size at each pixel the minimum of its own and its neighbour's values. 
	// This should cure 1-pixel outliers with extreme pen sizes, and should help with intersections of differently sized penumbrae.
	qglDrawBuffers( 1, &targets[tgt] );
	srcimg = tex[penumbraSpread1_tx + src];
	qglUseProgram( glslProgs[minp_pr] );
	DrawQuad( srcimg, UNF_MINP_pos ); 
	lastUsedSpreadTarget = tgt;
	std::swap( tgt, src );
	__opengl_breakpoint
	//*/

    // Then run through 6 cycles of spreading. Enough to spread penumbras by 2^6 = 64 pixels on the 
    // minified scale, or 8 * 64 = 512 pixels at full screen res
	GL_SelectTexture( 0 );
	qglUseProgram( glslProgs[spread_pr] );
	for ( int i=0; i < 6; ++i, std::swap( tgt, src ) )
	{
		qglDrawBuffers( 1, &targets[tgt] );
		qglUniform1i ( UNF_SPREAD_amount, (2<<i)/2 );
		srcimg = tex[penumbraSpread1_tx + src]; // Source is the non-target image
		DrawQuad( srcimg, UNF_SPREAD_pos );
	}
	lastUsedSpreadTarget = src;
	__opengl_breakpoint
	
	// Averaging blur as final pass
	qglDrawBuffers( 1, &targets[tgt] );
	srcimg = tex[penumbraSpread1_tx + src];
	qglUseProgram( glslProgs[avg_pr] );
	DrawQuad( srcimg, UNF_AVG_pos ); 
	lastUsedSpreadTarget = tgt;
	__opengl_breakpoint
	

	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	qglViewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1, 
        tr.viewportOffset[1] + backEnd.viewDef->viewport.y1, 
        backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
        backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );
	ResetLightScissor( vLight );


	/**********
        Step 4. The shadow blur. Write to a texture that'll be applied to the alpha channel of the main color 
		buffer before light interactions are drawn. It'll also provide the new stencil for light drawing.
     **********/
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[shadowBlur_fb] );
	qglUseProgram( glslProgs[blur_pr] );
	qglUniform2f( UNF_BLUR_projMatrix, backEnd.viewDef->projectionMatrix[10], backEnd.viewDef->projectionMatrix[14] );
	if ( r_useScissor.GetBool() ) 
	{
        qglUniform4f( UNF_BLUR_lightScissor,
					  float(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1) / potWidth,
					  float(backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1) / potHeight,
					  float(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x2) / potWidth,
					  float(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.y2) / potHeight );
	} else {
		qglUniform4f( UNF_BLUR_lightScissor, 0.0, 0.0, 1.0, 1.0 );
	}
	GL_SelectTexture( 0 );
	srcimg = tex[ penumbraSpread1_tx + lastUsedSpreadTarget ]; 
	srcimg->filter = TF_LINEAR; // Switch to linear from nearest
	srcimg->SetImageFilterAndRepeat();
	srcimg->Bind();
	GL_SelectTexture( 1 );
	globalImages->currentDepthImage->Bind();
	GL_SelectTexture( 2 );
	tex[colorStencil_tx]->Bind();
	GL_SelectTexture( 3 );
	tex[jitterMap_tx]->Bind();
	qglStencilFunc(GL_ALWAYS, 128, 255);
	DrawQuad( NULL, UNF_BLUR_pos );
	qglUseProgram( 0 );
	GL_SelectTexture( 3 );
	globalImages->BindNull();
	GL_SelectTexture( 2 );
	globalImages->BindNull();
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	__opengl_breakpoint

		
	/**********
        Step 5. Copy the resulting alpha mask back to the main color buffer. Create 
		the light interaction stencil at the same time.
     **********/
	qglBindFramebuffer(GL_FRAMEBUFFER, 0 );
	ResetLightScissor( vLight );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK ); // Alpha channel only
	//GL_State( GLS_DEPTHMASK ); // TEST -- Drawe colours too
	qglClearStencil( 1 );
	qglClearColor( 0.0, 0.0, 0.0, 0.0 );
	qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT ); 
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_ZERO ); // Set stencil to 0 where we draw any alpha value
	GL_SelectTexture( 0 );
	qglUseProgram( glslProgs[copyback_pr] );
	DrawQuad( tex[shadowBlur_tx], UNF_COPYBACK_pos );
	qglUseProgram( 0 );
	globalImages->BindNull();

	/**********
        Step 6. Set the stencil to draw on '0' areas, ready for interaction drawing
     **********/
	qglStencilFunc( GL_EQUAL, 0, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
}


void SoftShadowManager::DrawInteractions( const viewLight_t* vLight )
{
    // Draws the interactions for one light
    // All input textures use active texture slot 0
    qglDisable( GL_VERTEX_PROGRAM_ARB );
    qglDisable( GL_FRAGMENT_PROGRAM_ARB );

	++lightCounter;
	localShadowDrawCounter += vLight->localShadows ? 1 : 0;
	globalShadowDrawCounter += vLight->globalShadows ? 1 : 0;
	
	MakeShadowStencil( vLight, vLight->globalShadows, true); // Shadows that hit everything. Clear the stencil
	RB_ARB2_CreateDrawInteractions( vLight->localInteractions, GLS_SRCBLEND_DST_ALPHA ); // no-self-shadow surfaces
	MakeShadowStencil( vLight, vLight->localShadows, false); // Shadows cast by no-self-shadow surfaces. Don't clear the stencil, add to it.
	RB_ARB2_CreateDrawInteractions( vLight->globalInteractions, GLS_SRCBLEND_DST_ALPHA );
	qglDisable( GL_VERTEX_PROGRAM_ARB );	// if there weren't any globalInteractions, it would have stayed on
}


/*
==================
SoftShadowManager::maxTexcoord

For drawing quads using textures holding screen images. Locates the texcoord
of the upper right corner of the screen/game window. 
==================
*/
idVec2 SoftShadowManager::MaxTexcoord( const bool powerOfTwo ) const
{
    return powerOfTwo ? idVec2( (float)width/potWidth, (float)height/potHeight ) : idVec2( 1.0, 1.0 );
}


/*
==================
SoftShadowManager::DrawQuad

Draw a screen-aligned quad filling the current viewport. Faster than buffer blitting. 

Uses GLSL3 conventions. Shaders and uniforms must be set up first, except the vertex 
position location in the shader. Pass that in. Activate the right texture slot before 
calling too. 
TODO: GLSL program manager to store uniforms would be good for simplyfying this...
==================
*/
void SoftShadowManager::DrawQuad( idImage* tx, const GLuint vertexLoc )
{
    if ( tx )
    {
        tx->Bind();
    }

    // shader inputs
    qglVertexAttribPointerARB( vertexLoc, 2, GL_FLOAT, GL_FALSE, 0, vertexCache.Position( ScreenQuadVerts ) );
    qglEnableVertexAttribArrayARB( vertexLoc );

    // Draw
    qglDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, vertexCache.Position( ScreenQuadIndexes ) );

    // Clean up
    qglDisableVertexAttribArrayARB( vertexLoc );
    if ( tx )
    {
        globalImages->BindNull();
    }
}


void SoftShadowManager::DrawDebugOutput( const viewLight_t* vLight )
{
	int level = r_softShadDebug.GetInteger();

	if ( !level )
	{
		spamConsole = false;
		return;
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	ResetLightScissor( vLight );
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	qglUseProgram( glslProgs[quad_pr] );
	GL_SelectTexture( 0 );

	if ( level & 1 )	// penumbra lines
	{
		DrawQuad( tex[penumbraSize_tx], UNF_QUAD_pos );
	}
	if ( level & 2 )	// penumbra spread
	{
		DrawQuad( tex[penumbraSpread1_tx + lastUsedSpreadTarget], UNF_QUAD_pos );
	}
	if ( level & 4 )
	{
		DrawQuad( tex[shadowBlur_tx], UNF_QUAD_pos );
	}
	if ( level & 8 )
	{
		spamConsole = true;
	} else {
		spamConsole = false;
	}
	qglUseProgram( 0 );
	__opengl_breakpoint
}



/*  ---+-+-+-+-+-+-+-+|  EXTERNAL API  |+-+-+-+-+-+-+-+---  */

// Instantiate our singleton
SoftShadowManager softShadowManager;
SoftShadowManager* softShadowMgr = &softShadowManager;

