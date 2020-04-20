/*****************************************************************************
					The Dark Mod GPL Source Code

 This file is part of the The Dark Mod Source Code, originally based
 on the Doom 3 GPL Source Code as published in 2011.

 The Dark Mod Source Code is free software: you can redistribute it
 and/or modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation, either version 3 of the License,
 or (at your option) any later version. For details, see LICENSE.TXT.

 Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"

#pragma hdrstop

#include "BloomStage.h"
#include "Image.h"
#include "tr_local.h"
#include "GLSLProgramManager.h"
#include "GLSLProgram.h"
#include "Profiling.h"
#include "GLSLUniforms.h"
#include "glsl.h"

idCVar r_bloom("r_bloom", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Enable Bloom effect");
idCVar r_bloom_threshold("r_bloom_threshold", "0.7", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Brightness threshold for Bloom effect");
idCVar r_bloom_threshold_falloff("r_bloom_threshold_falloff", "8", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Exponential factor with which values below the brightness threshold fall off");
idCVar r_bloom_detailblend("r_bloom_detailblend", "0.01", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Blend factor for mixing detail into the blurred Bloom result");
idCVar r_bloom_weight("r_bloom_weight", "0.7", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Multiplicative weight factor for adding Bloom to the final image");
idCVar r_bloom_downsample_limit("r_bloom_downsample_limit", "128", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Downsample render image until vertical resolution approximately reaches this value");
idCVar r_bloom_blursteps("r_bloom_blursteps", "3", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Number of blur steps to perform after downsampling");

extern idCVar r_fboResolution;

BloomStage bloomImpl;
BloomStage *bloom = &bloomImpl;

namespace {
	struct BloomDownsampleUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BloomDownsampleUniforms)

		DEFINE_UNIFORM(sampler, sourceTexture)
		DEFINE_UNIFORM(int, sourceMipLevel)
		DEFINE_UNIFORM(float, brightnessThreshold)
		DEFINE_UNIFORM(float, thresholdFalloff)
	};

	struct BloomUpsampleUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BloomUpsampleUniforms)

		DEFINE_UNIFORM(sampler, blurredTexture)
		DEFINE_UNIFORM(sampler, detailTexture)
		DEFINE_UNIFORM(int, mipLevel)
		DEFINE_UNIFORM(float, detailBlendWeight)
	};

	struct BloomBlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BloomBlurUniforms)

		DEFINE_UNIFORM(sampler, source)
		DEFINE_UNIFORM(int, mipLevel)
		DEFINE_UNIFORM(vec2, axis)
	};

	void LoadBloomDownsampleShader(GLSLProgram *downsampleShader) {
		downsampleShader->Init();
		downsampleShader->AttachVertexShader("bloom.vert.glsl");
		downsampleShader->AttachFragmentShader("bloom_downsample.frag.glsl");
		Attributes::Default::Bind(downsampleShader);
		downsampleShader->Link();
		downsampleShader->Activate();
		BloomDownsampleUniforms *uniforms = downsampleShader->GetUniformGroup<BloomDownsampleUniforms>();
		uniforms->sourceTexture.Set(0);
		downsampleShader->Deactivate();
	}

	void LoadBloomDownsampleWithBrightPassShader(GLSLProgram *downsampleShader) {
		downsampleShader->Init();
		downsampleShader->AttachVertexShader("bloom.vert.glsl");
		idDict defines;
		defines.Set( "BLOOM_BRIGHTPASS", "1" );
		downsampleShader->AttachFragmentShader("bloom_downsample.frag.glsl", defines);
		Attributes::Default::Bind(downsampleShader);
		downsampleShader->Link();
		downsampleShader->Activate();
		BloomDownsampleUniforms *uniforms = downsampleShader->GetUniformGroup<BloomDownsampleUniforms>();
		uniforms->sourceTexture.Set(0);
		downsampleShader->Deactivate();
	}

	void LoadBloomBlurShader(GLSLProgram *blurShader) {
		blurShader->Init();
		blurShader->AttachVertexShader("bloom.vert.glsl");
		blurShader->AttachFragmentShader("bloom_blur.frag.glsl");
		Attributes::Default::Bind(blurShader);
		blurShader->Link();
		blurShader->Activate();
		BloomBlurUniforms *uniforms = blurShader->GetUniformGroup<BloomBlurUniforms>();
		uniforms->source.Set(0);
		blurShader->Deactivate();
	}

	void LoadBloomUpsampleShader(GLSLProgram *upsampleShader) {
		upsampleShader->Init();
		upsampleShader->AttachVertexShader("bloom.vert.glsl");
		upsampleShader->AttachFragmentShader("bloom_upsample.frag.glsl");
		Attributes::Default::Bind(upsampleShader);
		upsampleShader->Link();
		upsampleShader->Activate();
		BloomUpsampleUniforms *uniforms = upsampleShader->GetUniformGroup<BloomUpsampleUniforms>();
		uniforms->blurredTexture.Set(0);
		uniforms->detailTexture.Set(1);
		upsampleShader->Deactivate();
	}

	int CalculateNumDownsamplingSteps(int imageHeight) {
		int numSteps = 0;
		int downsampleLimit = r_bloom_downsample_limit.GetInteger();
		while( (imageHeight >> numSteps) - downsampleLimit > downsampleLimit - (imageHeight >> (numSteps + 1)) ) {
			++numSteps;
		}
		return std::min(numSteps + 1, BloomStage::MAX_DOWNSAMPLING_STEPS);
	}

	void CreateBloomDownSamplingBuffer(idImage *image) {
		image->type = TT_2D;
		image->uploadWidth = r_fboResolution.GetFloat() * glConfig.vidWidth / 2;
		image->uploadHeight = r_fboResolution.GetFloat() * glConfig.vidHeight / 2;
		int numDownscalingSteps = CalculateNumDownsamplingSteps( image->uploadHeight );
		common->Printf( "Creating bloom downsampling texture %dx%d\n", image->uploadWidth, image->uploadHeight );
		qglGenTextures(1, &image->texnum);
		qglBindTexture(GL_TEXTURE_2D, image->texnum);
		for (int i = 0; i < numDownscalingSteps; ++i) {
			qglTexImage2D(GL_TEXTURE_2D, i, GL_RGBA16F, image->uploadWidth >> i, image->uploadHeight >> i, 0, GL_RGBA, GL_FLOAT, nullptr);
		}
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numDownscalingSteps - 1);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GL_SetDebugLabel(GL_TEXTURE, image->texnum, image->imgName);
	}

	void CreateBloomUpSamplingBuffer(idImage *image) {
		image->type = TT_2D;
		image->uploadWidth = r_fboResolution.GetFloat() * glConfig.vidWidth / 2;
		image->uploadHeight = r_fboResolution.GetFloat() * glConfig.vidHeight / 2; 
		int numDownscalingSteps = CalculateNumDownsamplingSteps( image->uploadHeight );
		common->Printf( "Creating bloom upsampling texture %dx%d\n", image->uploadWidth, image->uploadHeight );
		qglGenTextures(1, &image->texnum);
		qglBindTexture(GL_TEXTURE_2D, image->texnum);
		for (int i = 0; i < numDownscalingSteps; ++i) {
			qglTexImage2D(GL_TEXTURE_2D, i, GL_RGBA16F, image->uploadWidth >> i, image->uploadHeight >> i, 0, GL_RGBA, GL_FLOAT, nullptr);
		}
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numDownscalingSteps - 1);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GL_SetDebugLabel(GL_TEXTURE, image->texnum, image->imgName);
	}
}

void BloomStage::Init() {
	bloomDownSamplers = globalImages->ImageFromFunction("Bloom Downsampling", CreateBloomDownSamplingBuffer);
	bloomDownSamplers->ActuallyLoadImage();
	bloomUpSamplers = globalImages->ImageFromFunction("Bloom Upsampling", CreateBloomUpSamplingBuffer);
	bloomUpSamplers->ActuallyLoadImage();

	numDownsamplingSteps = CalculateNumDownsamplingSteps( bloomDownSamplers->uploadHeight );
	qglGenFramebuffers(numDownsamplingSteps, downsampleFBOs);
	for (int i = 0; i < numDownsamplingSteps; ++i) {
		qglBindFramebuffer(GL_FRAMEBUFFER, downsampleFBOs[i]);
		qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomDownSamplers->texnum, i);
	}
	qglGenFramebuffers(numDownsamplingSteps, upsampleFBOs);
	for (int i = 0; i < numDownsamplingSteps; ++i) {
		qglBindFramebuffer(GL_FRAMEBUFFER, upsampleFBOs[i]);
		qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomUpSamplers->texnum, i);
	}
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);

	downsampleShader = programManager->Find("bloom_downsample");
	if (downsampleShader == nullptr) {
		downsampleShader = programManager->LoadFromGenerator("bloom_downsample", LoadBloomDownsampleShader);
	}
	downsampleWithBrightPassShader = programManager->Find("bloom_downsample_brightpass");
	if (downsampleWithBrightPassShader == nullptr) {
		downsampleWithBrightPassShader = programManager->LoadFromGenerator("bloom_downsample_brightpass", LoadBloomDownsampleWithBrightPassShader);
	}
	blurShader = programManager->Find("bloom_blur");
	if (blurShader == nullptr) {
		blurShader = programManager->LoadFromGenerator("bloom_blur", LoadBloomBlurShader);
	}
	upsampleShader = programManager->Find("bloom_upsample");
	if (upsampleShader == nullptr) {
		upsampleShader = programManager->LoadFromGenerator("bloom_upsample", LoadBloomUpsampleShader);
	}
}

void BloomStage::Shutdown() {
	if (downsampleFBOs[0] != 0) {
		qglDeleteFramebuffers(numDownsamplingSteps, downsampleFBOs);
		downsampleFBOs[0] = 0;
	}
	if (upsampleFBOs[0] != 0) {
		qglDeleteFramebuffers(numDownsamplingSteps, upsampleFBOs);
		upsampleFBOs[0] = 0;
	}
	if (bloomDownSamplers != nullptr) {
		bloomDownSamplers->PurgeImage();
		bloomDownSamplers = nullptr;
	}
	if (bloomUpSamplers != nullptr) {
		bloomUpSamplers->PurgeImage();
		bloomUpSamplers = nullptr;
	}
}

extern GLuint fboPrimary;
extern bool primaryOn;

void BloomStage::ComputeBloomFromRenderImage() {
	GL_PROFILE("BloomStage");

	int expectedWidth = r_fboResolution.GetFloat() * glConfig.vidWidth / 2;
	int expectedHeight = r_fboResolution.GetFloat() * glConfig.vidHeight / 2;

	if (downsampleFBOs[0] != 0 && (bloomDownSamplers->uploadWidth != expectedWidth
		|| bloomDownSamplers->uploadHeight != expectedHeight)
		|| r_bloom_downsample_limit.IsModified() ) {

		r_bloom_downsample_limit.ClearModified();
		// resolution changed, need to recreate our resources
		Shutdown();
	}

	if (downsampleFBOs[0] == 0) {
		Init();
	}

	qglClearColor(0, 0, 0, 0);

	// To get a soft, wide blur for the Bloom effect cheaply, we use the following approach:
	// 1. do a bright pass on the original render image and downsample it in several steps
	// 2. apply a Gaussian blur filter multiple times on the downsampled image
	// 3. scale it back up to half resolution
	Downsample();
	for( int i = 0; i < r_bloom_blursteps.GetInteger(); ++i ) {
		Blur();
	}
	Upsample();

	qglViewport( 0, 0, globalImages->currentRenderImage->uploadWidth, globalImages->currentRenderImage->uploadHeight );
	// FIXME: this is a bit hacky, needs better FBO control
	qglBindFramebuffer(GL_FRAMEBUFFER, primaryOn ? fboPrimary : 0);
}

void BloomStage::BindBloomTexture() {
	bloomUpSamplers->Bind();
}

void BloomStage::Downsample() {
	GL_PROFILE( "BloomDownsampling" )

	// execute initial downsampling and bright pass on render image
	downsampleWithBrightPassShader->Activate();
	BloomDownsampleUniforms *uniforms = downsampleWithBrightPassShader->GetUniformGroup<BloomDownsampleUniforms>();
	qglBindFramebuffer(GL_FRAMEBUFFER, downsampleFBOs[0]);
	qglViewport( 0, 0, bloomDownSamplers->uploadWidth, bloomDownSamplers->uploadHeight );
	uniforms->sourceMipLevel.Set( 0 );
	uniforms->brightnessThreshold.Set( r_bloom_threshold.GetFloat() );
	uniforms->thresholdFalloff.Set( r_bloom_threshold_falloff.GetFloat() );
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenQuad();

	// generate additional downsampled mip levels
	bloomDownSamplers->Bind();
	downsampleShader->Activate();
	uniforms = downsampleShader->GetUniformGroup<BloomDownsampleUniforms>();
	for (int i = 1; i < numDownsamplingSteps; ++i) {
		uniforms->sourceMipLevel.Set( i - 1 );
		qglBindFramebuffer(GL_FRAMEBUFFER, downsampleFBOs[i]);
		qglViewport( 0, 0, bloomDownSamplers->uploadWidth >> i, bloomDownSamplers->uploadHeight >> i );
		qglClear(GL_COLOR_BUFFER_BIT);
		RB_DrawFullScreenQuad();
	}
}

void BloomStage::Blur() {
	GL_PROFILE("BloomBlur")

	int mipLevel = numDownsamplingSteps - 1;
	blurShader->Activate();
	BloomBlurUniforms *uniforms = blurShader->GetUniformGroup<BloomBlurUniforms>();
	uniforms->mipLevel.Set( mipLevel );

	// first horizontal Gaussian blur goes from downsampler[lowestMip] to upsampler[lowestMip]
	GL_SelectTexture( 0 );
	bloomDownSamplers->Bind();
	uniforms->axis.Set( 1, 0 );
	qglBindFramebuffer(GL_FRAMEBUFFER, upsampleFBOs[mipLevel]);
	qglViewport(0, 0, bloomUpSamplers->uploadWidth >> mipLevel, bloomUpSamplers->uploadHeight >> mipLevel);
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenQuad();

	// second vertical Gaussian blur goes from upsampler[lowestMip] to downsampler[lowestMip]
	uniforms->axis.Set( 0, 1 );
	qglBindFramebuffer(GL_FRAMEBUFFER, downsampleFBOs[mipLevel]);
	bloomUpSamplers->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenQuad();
}

void BloomStage::Upsample() {
	GL_PROFILE( "BloomUpsampling" )

	upsampleShader->Activate();
	BloomUpsampleUniforms *uniforms = upsampleShader->GetUniformGroup<BloomUpsampleUniforms>();
	uniforms->detailBlendWeight.Set( r_bloom_detailblend.GetFloat() );

	GL_SelectTexture( 1 );
	bloomDownSamplers->Bind();
	GL_SelectTexture( 0 );
	// first upsampling step goes from downsampler[lowestMip] to upsampler[lowestMip-1]
	bloomDownSamplers->Bind();

	for (int i = numDownsamplingSteps - 2; i >= 0; --i) {
		uniforms->mipLevel.Set( i );
		qglBindFramebuffer(GL_FRAMEBUFFER, upsampleFBOs[i]);
		qglViewport( 0, 0, bloomUpSamplers->uploadWidth >> i, bloomUpSamplers->uploadHeight >> i );
		RB_DrawFullScreenQuad();
		// next upsampling steps go from upsampler[mip+1] to upsampler[mip]
		bloomUpSamplers->Bind();
	}
}
