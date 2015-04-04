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
	// the same value, the fragment shader can skip the remaining samples. *** This is broken in 
	// the gpu gems code. The samples are not ordered in this way. Not yet fixed as we might not 
	// need the early exit, if the penumbra size estimation is good enough. 
	// Better explanation at http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter17.html
	static const long somevalue = 423480826;
	idRandom2 rand(somevalue);//~TODO: find a better rnd generator

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

	// Verts might already be on the garbage heap. Check.
	if ( ScreenQuadVerts->user ) { vertexCache.Free( ScreenQuadVerts ); }
	if ( ScreenQuadIndexes->user ) { vertexCache.Free( ScreenQuadIndexes ); }

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
    // Depth-Stencil renderbuffer object for use when drawing shadow volumes and interactions
    qglGenRenderbuffers(1, &depthRbo);
    qglBindRenderbuffer( GL_RENDERBUFFER, depthRbo );
    qglRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height );
    qglBindRenderbuffer( GL_RENDERBUFFER, 0 );

    // Penumbra size maps
    tex[penumbraSize_tx] = globalImages->RendertargetImage("penumbraSize_tx", potWidth, potHeight, GL_RG16F, GL_RGBA, GL_FLOAT );

    // Small-scale penumbra size maps
    const int smallSize[2] = { potWidth / MINISCALE, potHeight / MINISCALE };
    tex[penumbraSpread1_tx] = globalImages->RendertargetImage("penumbraSpread1_tx", smallSize[0], smallSize[1], GL_RG16F, GL_RGBA, GL_FLOAT );
    tex[penumbraSpread2_tx] = globalImages->RendertargetImage("penumbraSpread2_tx", smallSize[0], smallSize[1], GL_RG16F, GL_RGBA, GL_FLOAT );
	// These want linear filtering for when they get upsampled again. During the ping-pong passes, they'll be accessed without filtering anyway.
	tex[penumbraSpread1_tx]->filter = TF_LINEAR;
	tex[penumbraSpread1_tx]->SetImageFilterAndRepeat();
	tex[penumbraSpread2_tx]->filter = TF_LINEAR;
	tex[penumbraSpread2_tx]->SetImageFilterAndRepeat();

	// Color copy of the stencil buffer
	tex[colorStencil_tx] = globalImages->RendertargetImage("colorStencil_fb", potWidth, potHeight, GL_R8, GL_RGBA, GL_UNSIGNED_BYTE );
	tex[shadowBlur_tx] = globalImages->RendertargetImage("shadowBlur_tx", potWidth, potHeight, GL_R8, GL_RED, GL_UNSIGNED_BYTE );

	// Jitter map. Not strictly speaking a render target, but handy to put it here
	tex[jitterMap_tx] = globalImages->ImageFromFunction("jitterMap_tx", R_JitterMap);
}


void SoftShadowManager::InitShaders()
{
    // Stencil shadow shader program
    // Does the usual job of projecting the shadow volume away from the light,
    // but also outputs penumbra size to a color buffer.
    // Use "old" GLSL compatibility mode so we can access the built-in matrix stack.
    const GLchar* SoftShadowVP = GLSLold(
        uniform		vec4	lightPos;          /* in model space */
        uniform		float	lightRadius;      /* size of the light source */
        uniform		float	lightReach;       /* max distance that light can hit */
		out			vec2	screenSpaceSize;

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
            float penumbraSize = ( 1.0 - gl_Vertex.w ) * ( lightReach - distFromLight ) * lightRadius / distFromLight;
			/* Transform into screen space, without perspective divide */
			screenSpaceSize = ( gl_ProjectionMatrix * vec4( penumbraSize, penumbraSize, -1.0, 1.0 ) ).xy;
        }
    );

    /* Stencil shadow fragment shader tests whether the fragment is very close to the scene depth, 
       and writes the penumbra size at that point if so. The idea is to produce a line marking the 
       centre of each penumbra with size info. */
    const GLchar* SoftShadowFP = GLSLold(
        in float penumbraSize;
		in vec2	screenSpaceSize;
        uniform sampler2D depthtex;
        uniform vec2 invDepthImageSize;
		uniform ivec2 screensize;
        uniform float threshold;
        
        void main()
        {
            /* sample scene depth */
            vec2 texcoord = gl_FragCoord.xy * invDepthImageSize;
            float SceneDepth = texture( depthtex, texcoord ).x;
            float DepthDiff = abs( SceneDepth - gl_FragCoord.z );
            float maxDepthDelta = fwidth(SceneDepth) + fwidth(gl_FragCoord.z); // greatest relative depth change between shadow vol and scene
            float sceneDelta = dFdx(SceneDepth) + dFdy(SceneDepth);

            bool sharpDepthDiscontinuity = fwidth(SceneDepth) > 0.0005 || fwidth(gl_FragCoord.z) > threshold; 
            // debug
            // show linear depth of fragment and scene
            /*float linear_scenedepth = -1.0 / ( ( 2 * SceneDepth - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
            float linear_fragdepth = -1.0 /  ( ( 2 * gl_FragCoord.z - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
            float linear_fragchange = -1.0 /  ( ( 2 * fwidth(gl_FragCoord.z) - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
            float linear_depthdiff =  -1.0 /  ( ( 2 * DepthDiff - 1 + gl_ProjectionMatrix[2][2] ) / -gl_ProjectionMatrix[3][2] );
            */
            //if ( fwidth(gl_FragCoord.z) >= DepthDiff )
            //{
            //  gl_FragColor = vec4( 1.0, linear_fragchange / 256.0, 0.0, 1.0 );  // debug
            //}
            //ivec2 screenCoverage = ivec2( penumbraSize * 0.5 * gl_FragCoord.w * screensize ); // in pixels
			ivec2 screenCoverage = ivec2( screenSpaceSize * 0.5 * gl_FragCoord.w * screensize ); // in pixels. 0.5 because NDC coords have range of 2 (-1 to +1)

            if ( !sharpDepthDiscontinuity && max(screenCoverage.x, screenCoverage.y) > 1 /* && penumbraSize > 0.25 */ && DepthDiff < maxDepthDelta )
            {
                gl_FragColor = vec4( screenCoverage.x, 1.0, 0.0, 1.0 ); 
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
    UNF_SHADOW_invDepthImageSize = qglGetUniformLocation( prg_shadow, "invDepthImageSize" );
    // debug threshold
    UNF_SHADOW_threshold = qglGetUniformLocation( prg_shadow, "threshold" );
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_shadow );
    qglUniform1i( qglGetUniformLocation( prg_shadow, "depthtex" ), 0 );
	qglUniform2i( qglGetUniformLocation( prg_shadow, "screensize" ), width, height );
    qglUseProgram( 0 );

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
    qglUniform2fv( qglGetUniformLocation( prg_quad, "maxTexcoord" ), 1, maxTexcoord(true).ToFloatPtr() );
    qglUniform1i ( qglGetUniformLocation( prg_quad, "tex" ), 0 );
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

			float maxPenumbra = 0.0;

			if ( penumbraTest > 0.0 && penumbraTest < 5.0 )
			{
				for ( int i=0; i<16; ++i )
				{
					vec4 samp = vec4(
						texture( penumbraImg, texcoord[i] ).x,
						texture( penumbraImg, texcoord[i] + up ).x,
						texture( penumbraImg, texcoord[i] + right ).x,
						texture( penumbraImg, texcoord[i] + up + right ).x
					);
					maxPenumbra = max( maxPenumbra, max( max(samp.x, samp.y), max(samp.z, samp.w) ) );
				}
			}
			// MaxPenumbra is a count of pixels. Output remaining amount to spread in x, orig penumbra size in y.
			// Remaining spread is half pen width, scaled down by the minification factor of 8.
			float spread = maxPenumbra ? maxPenumbra / 16.0 + 1.0 : 0.0;
            oColor = vec4( spread, maxPenumbra, 0.0, 1.0 ); 
			//oColor = vec4( penumbraTest / 5.0, maxPenumbra, 0.0, 1.0 ); //~DEBUG

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
    qglUniform2fv( qglGetUniformLocation( prg_mini, "maxTexcoord" ), 1, maxTexcoord(true).ToFloatPtr() );
    qglUniform1i ( qglGetUniformLocation( prg_mini, "penumbraImg" ), 0 );
	qglUniform1i ( qglGetUniformLocation( prg_mini, "stencilImg" ), 1 );
    qglUniform2f( qglGetUniformLocation( prg_mini, "invTextureSize" ), 
                  1.0f / tex[penumbraSize_tx]->uploadWidth, 
                  1.0f / tex[penumbraSize_tx]->uploadHeight );
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
			if (result.x==0) result.x = sample.x; // hack to allow avg
			if (result.y==0) result.y = sample.y; // hack to allow avg
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

			//if (result.x == 0.0) result.x = 10000.0; // hack to allow min pen detection
			//if (result.y == 0.0) result.y = 10000.0; // hack to allow min pen detection

			// Try using min penumbra size but max spread
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
		uniform ivec2		screensize;
		in		vec2		texcoord;
        out     vec4		oColor;
        const	int			sampleCount = 64;
		
        void main()
        {
			float	penumbraSize = texture( penumbraImage, texcoord ).y / 2.0; // this is in pixels. Convert from diameter to radius.
			vec2	smpSize  = float(penumbraSize) / screensize;		 // as proportion of screen, i.e. normalized texcoord
			float	shadow = 0.0;
			vec3	jitterCoord = vec3( gl_FragCoord.xy * jitterMapScale, 0.0 );
			float	jitterMapZStep = 1.0 / (sampleCount / 2);
			
			if ( penumbraSize > 0.0 )
			{
				for ( int i = 0; i < sampleCount / 2; ++i, jitterCoord.z += jitterMapZStep )
				{
					vec4 offset = texture( jitterImage, jitterCoord ) * vec4( smpSize, smpSize);
					vec2 smpCoord = offset.xy + texcoord;
					shadow += texture(shadowImage, smpCoord).r;
					smpCoord = offset.zw + texcoord;
					shadow += texture(shadowImage, smpCoord).r; 
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
    // Set uniforms that won't change between initialisations
    qglUseProgram( prg_blur );
	qglUniform2fv( qglGetUniformLocation( prg_blur, "maxTexcoord" ), 1, maxTexcoord(true).ToFloatPtr() );
    qglUniform1i( qglGetUniformLocation( prg_blur, "penumbraImage" ), 0 );
	qglUniform1i( qglGetUniformLocation( prg_blur, "depthImage" ), 1 );
	qglUniform1i( qglGetUniformLocation( prg_blur, "shadowImage" ), 2 );
	qglUniform1i( qglGetUniformLocation( prg_blur, "jitterImage" ), 3 );
	qglUniform1f( qglGetUniformLocation( prg_blur, "jitterMapScale" ), 1.0 / JITTERMAPSIZE );
	qglUniform2i( qglGetUniformLocation( prg_blur, "screensize" ), width, height );

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
	qglUniform2fv( qglGetUniformLocation( prg_copyback, "maxTexcoord" ), 1, maxTexcoord(true).ToFloatPtr() );
    qglUniform1i( qglGetUniformLocation( prg_copyback, "alphaStencil" ), 0 );


	qglUseProgram( 0 );
}


void SoftShadowManager::InitFBOs()
{
    qglGenFramebuffers( NumFramebuffers, fbo );

    // Stencil shadow pass FBO
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSize_fb] );
    qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[penumbraSize_tx]->texnum, 0 );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );
    
    // FBO for penumbra spreading using a downsized image. 2 textures for ping-ponging
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[penumbraSpread_fb] );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[penumbraSpread1_tx]->texnum, 0 );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex[penumbraSpread2_tx]->texnum, 0 );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );


	// For copying the stencil contents to a color buffer
    qglBindFramebuffer( GL_FRAMEBUFFER, fbo[colorStencil_fb] );
    qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo );
    qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[colorStencil_tx]->texnum, 0 );
    assert( qglCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE );

	// For the blurred shadow stencil
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[shadowBlur_fb] );
	qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex[shadowBlur_tx]->texnum, 0 );


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
    // All input textures use active texture slot 0
    qglDisable( GL_VERTEX_PROGRAM_ARB );
    qglDisable( GL_FRAGMENT_PROGRAM_ARB );

	static const GLenum targets[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 }; // Used by multiple FBOs

	//~TODO: Activate depth bounds testing for these draws?

    /**********
        Step 1. Get a copy of the depth buffer. We can use the global _currentDepth image as a sampler 
        for our fragment shaders, but we need a separate active depth buffer for our shadow volume drawing. 
     **********/
    qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo[penumbraSize_fb] );
    qglBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
    qglBlitFramebuffer( 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 
                        GL_DEPTH_BUFFER_BIT, GL_NEAREST );
    /**********
        Step 2. Draw shadows into the stencil using existing technique, but using a new shader that draws 
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
    //~debug
    qglUniform1f( UNF_SHADOW_threshold, 0.2 /*r_ignore.GetFloat()*/ ); // best so far: 0.0005f for scene, 0.2 for shadvol edges (now we have sep cleanup for midair sparklies, was 0.005)
    ResetLightScissor( vLight );
    qglBlendEquation( GL_MAX ); // This and clear color need to be in sync
    qglClearColor( 0.0, 0.0, 0.0, 0.0 );
    //glClearColor( 1.0, 1.0, 1.0, 1.0 );
    qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT );
	// Use the standard stencil shadow functions to draw the volumes
    RB_StencilShadowPass( vLight->globalShadows );
    RB_StencilShadowPass( vLight->localShadows );
    qglBlendEquation( GL_FUNC_ADD );
    qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
    //~TODO: split off rendering pass for non-self-shadow surfaces. For the POC, all shadows hit 
    // everything. Can maintain 2 stencils and alpha masks

	/**********
        Step 3. Copy the shadow stencil to a colour sampler. Draw with stencil on and off to save clearing + overwriting.
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
        Step 4. Spread the penumbra information to all screen pixels that might be in penumbra.
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
        qglScissor( 0, 0, smallwidth, smallheight ); //~TODO: set scissor as downscaled light scissor
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
    // Then run through 7 cycles of spreading. Enough to spread penumbras by 2^7 = 128 pixels on the 
    // minified scale, or 8 * 128 = 1024 pixels at full screen res
	GL_SelectTexture( 0 );
	qglUseProgram( glslProgs[spread_pr] );
	for ( int i=0, target=1; i < 7; ++i, target = 1 - target )
	{
		qglDrawBuffers( 1, &targets[target] );
		current_pingpong_buffer = tex[penumbraSpread1_tx + target];
		qglUniform1i ( UNF_SPREAD_amount, (2<<i)/2 );
		DrawQuad( tex[penumbraSpread1_tx + (1 - target)], UNF_SPREAD_pos ); // Source is the non-target image
		__opengl_breakpoint
	}
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	qglViewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1, 
        tr.viewportOffset[1] + backEnd.viewDef->viewport.y1, 
        backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
        backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );
    ResetLightScissor( vLight );


	/**********
        Step 5. The shadow blur. Write to a texture that'll be applied to the alpha channel of the main color 
		buffer before light interactions are drawn. It'll also provide the new stencil for light drawing.
     **********/
	qglBindFramebuffer( GL_FRAMEBUFFER, fbo[shadowBlur_fb] );
	qglUseProgram( glslProgs[blur_pr] );
	GL_SelectTexture( 0 );
	current_pingpong_buffer->Bind();
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
        Step 6. Copy the resulting alpha mask back to the main color buffer. Create 
		the light interaction stencil at the same time.
     **********/
	qglBindFramebuffer(GL_FRAMEBUFFER, 0 );
	ResetLightScissor( vLight );
	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK ); // Alpha channel only
	//GL_State( GLS_DEPTHMASK ); // TEST -- Drawe colours too
	qglClear( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT ); // Stencil still clears to 128
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );
	GL_SelectTexture( 0 );
	qglUseProgram( glslProgs[copyback_pr] );
	DrawQuad( tex[shadowBlur_tx], UNF_COPYBACK_pos );
	qglUseProgram( 0 );
	globalImages->BindNull();


	/**********
		Step 7. Draw light interactions
	 **********/
	qglStencilFunc( GL_EQUAL, 0, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	RB_ARB2_CreateDrawInteractions( vLight->localInteractions, GLS_SRCBLEND_DST_ALPHA );
	RB_ARB2_CreateDrawInteractions( vLight->globalInteractions, GLS_SRCBLEND_DST_ALPHA );
	glDisable( GL_VERTEX_PROGRAM_ARB );	// if there weren't any globalInteractions, it would have stayed on

}


/*
==================
SoftShadowManager::maxTexcoord

For drawing quads using textures holding screen images. Locates the texcoord
of the upper right corner of the screen/game window. 
==================
*/
idVec2 SoftShadowManager::maxTexcoord( const bool powerOfTwo ) const
{
    return powerOfTwo ? idVec2( (float)width/potWidth, (float)height/potHeight ) : idVec2( 1.0, 1.0 );
}


/*
==================
SoftShadowManager::DrawQuad

Draw a screen-aligned quad filling the current viewport. Faster than buffer blitting. 

Uses GLSL3 conventions. Shaders and uniforms must be set up first, except the vertex 
position location in trhe shader. Pass that in. Activate the right texture slot before 
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


void SoftShadowManager::DrawDebugOutput()
{
	int level = r_softShadDebug.GetInteger();

	if ( !level )
	{
		return;
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	// Leave light scissor in place
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	qglUseProgram( glslProgs[quad_pr] );
	GL_SelectTexture( 0 );

	if ( level & 1 )	// penumbra lines
	{
		DrawQuad( tex[penumbraSize_tx], UNF_QUAD_pos );
	}
	if ( level & 2 )	// penumbra spread
	{
		DrawQuad( current_pingpong_buffer, UNF_QUAD_pos );
	}
	if ( level & 4 )
	{
		DrawQuad( tex[shadowBlur_tx], UNF_QUAD_pos );
	}
	qglUseProgram( 0 ); //~Remove
	__opengl_breakpoint
}



/*  ---+-+-+-+-+-+-+-+|  EXTERNAL API  |+-+-+-+-+-+-+-+---  */

// Instantiate our singleton
SoftShadowManager softShadowManager;
SoftShadowManager* softShadowMgr = &softShadowManager;

