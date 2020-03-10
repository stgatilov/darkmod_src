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

#include "AmbientOcclusionStage.h"
#include "Image.h"
#include "tr_local.h"
#include "GLSLProgramManager.h"
#include "GLSLProgram.h"
#include "Profiling.h"
#include "GLSLUniforms.h"
#include "glsl.h"

idCVar r_ssao("r_ssao", "0", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE, "Screen space ambient occlusion: 0 - off, 1 - low, 2 - medium, 3 - high");
idCVar r_ssao_radius("r_ssao_radius", "32", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"View space sample radius - larger values provide a softer, spread effect, but risk causing unwanted halo shadows around objects");
idCVar r_ssao_bias("r_ssao_bias", "0.05", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"Min depth difference to count for occlusion, used to avoid some acne effects");
idCVar r_ssao_intensity("r_ssao_intensity", "1.0", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"SSAO intensity factor, the higher the value, the stronger the effect");
idCVar r_ssao_base("r_ssao_base", "0.1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
	"Minimum baseline visibility below which AO cannot drop");
idCVar r_ssao_edgesharpness("r_ssao_edgesharpness", "1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Edge sharpness in SSAO blur");

extern idCVar r_fboResolution;

AmbientOcclusionStage ambientOcclusionImpl;
AmbientOcclusionStage *ambientOcclusion = &ambientOcclusionImpl;

namespace {
	struct AOUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(AOUniforms)

		DEFINE_UNIFORM(sampler, depthTexture)
		DEFINE_UNIFORM(float, sampleRadius)
		DEFINE_UNIFORM(float, depthBias)
		DEFINE_UNIFORM(float, baseValue)
		DEFINE_UNIFORM(int, numSamples)
		DEFINE_UNIFORM(int, numSpiralTurns)
		DEFINE_UNIFORM(float, intensityDivR6)
		DEFINE_UNIFORM(int, maxMipLevel)
	};

	struct BlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(BlurUniforms)

		DEFINE_UNIFORM(sampler, source)
		DEFINE_UNIFORM(vec2, axis)
		DEFINE_UNIFORM(float, edgeSharpness)
	};

	struct DepthMipUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(DepthMipUniforms)

		DEFINE_UNIFORM(sampler, depth)
		DEFINE_UNIFORM(int, previousMipLevel)
	};

	void LoadSSAOShader(GLSLProgram *ssaoShader) {
		ssaoShader->Init();
		ssaoShader->AttachVertexShader("ssao.vert.glsl");
		ssaoShader->AttachFragmentShader("ssao.frag.glsl");
		Attributes::Default::Bind(ssaoShader);
		ssaoShader->Link();
		ssaoShader->Activate();
		AOUniforms *uniforms = ssaoShader->GetUniformGroup<AOUniforms>();
		uniforms->depthTexture.Set(0);
		ssaoShader->Deactivate();
	}

	void LoadSSAOBlurShader(GLSLProgram *blurShader) {
		blurShader->Init();
		blurShader->AttachVertexShader("ssao.vert.glsl");
		blurShader->AttachFragmentShader("ssao_blur.frag.glsl");
		Attributes::Default::Bind(blurShader);
		blurShader->Link();
		blurShader->Activate();
		BlurUniforms *uniforms = blurShader->GetUniformGroup<BlurUniforms>();
		uniforms->source.Set(0);
		blurShader->Deactivate();
	}

	void CreateSSAOColorBuffer(idImage *image) {
		image->type = TT_2D;
		GLuint curWidth = r_fboResolution.GetFloat() * glConfig.vidWidth;
		GLuint curHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;
		image->GenerateAttachment(curWidth, curHeight, GL_COLOR);
	}

	void CreateViewspaceDepthBuffer(idImage *image) {
		image->type = TT_2D;
		image->uploadWidth = r_fboResolution.GetFloat() * glConfig.vidWidth;
		image->uploadHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;
		qglGenTextures(1, &image->texnum);
		qglBindTexture(GL_TEXTURE_2D, image->texnum);
		for (int i = 0; i <= AmbientOcclusionStage::MAX_DEPTH_MIPS; ++i) {
			qglTexImage2D(GL_TEXTURE_2D, i, GL_R32F, image->uploadWidth >> i, image->uploadHeight >> i, 0, GL_RED, GL_FLOAT, nullptr);
		}
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, AmbientOcclusionStage::MAX_DEPTH_MIPS);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		GL_SetDebugLabel(GL_TEXTURE, image->texnum, image->imgName);
	}
}

AmbientOcclusionStage::AmbientOcclusionStage() : ssaoFBO(0), ssaoBlurFBO(0),
		ssaoResult(nullptr), ssaoBlurred(nullptr), viewspaceDepth(nullptr),
		ssaoShader(nullptr), ssaoBlurShader(nullptr), depthShader(nullptr) {
	for (int i = 0; i < MAX_DEPTH_MIPS; ++i) {
		depthMipFBOs[i] = 0;
	}
}

void AmbientOcclusionStage::Init() {
	ssaoResult = globalImages->ImageFromFunction("SSAO ColorBuffer", CreateSSAOColorBuffer);
	ssaoResult->ActuallyLoadImage();
	ssaoBlurred = globalImages->ImageFromFunction("SSAO Blurred", CreateSSAOColorBuffer);
	ssaoBlurred->ActuallyLoadImage();
	viewspaceDepth = globalImages->ImageFromFunction("SSAO Depth", CreateViewspaceDepthBuffer);

	qglGenFramebuffers(1, &ssaoFBO);
	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoResult->texnum, 0);
	qglGenFramebuffers(1, &ssaoBlurFBO);
	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurred->texnum, 0);
	qglGenFramebuffers(MAX_DEPTH_MIPS, depthMipFBOs);
	for (int i = 0; i <= MAX_DEPTH_MIPS; ++i) {
		qglBindFramebuffer(GL_FRAMEBUFFER, depthMipFBOs[i]);
		qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewspaceDepth->texnum, i);
		int status = qglCheckFramebufferStatus(GL_FRAMEBUFFER);
		common->Printf("Status for depth FBO level %d: %d\n", i, status);
	}
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);

	ssaoShader = programManager->Find("ssao");
	if (ssaoShader == nullptr) {
		ssaoShader = programManager->LoadFromGenerator("ssao", LoadSSAOShader);
	}
	ssaoBlurShader = programManager->Find("ssao_blur");
	if (ssaoBlurShader == nullptr) {
		ssaoBlurShader = programManager->LoadFromGenerator("ssao_blur", LoadSSAOBlurShader);
	}
	depthShader = programManager->Find("ssao_depth");
	if (depthShader == nullptr) {
		depthShader = programManager->LoadFromFiles("ssao_depth", "ssao.vert.glsl", "ssao_depth.frag.glsl");
	}
	depthMipShader = programManager->Find("ssao_depth_mip");
	if (depthMipShader == nullptr) {
		depthMipShader = programManager->LoadFromFiles("ssao_depth_mip", "ssao.vert.glsl", "ssao_depthmip.frag.glsl");
	}
	showSSAOShader = programManager->Find("ssao_show");
	if (showSSAOShader == nullptr) {
		showSSAOShader = programManager->LoadFromFiles("ssao_show", "ssao.vert.glsl", "ssao_show.frag.glsl");
	}
}

void AmbientOcclusionStage::Shutdown() {
	if (ssaoFBO != 0) {
		qglDeleteFramebuffers(1, &ssaoFBO);
		ssaoFBO = 0;
	}
	if (ssaoBlurFBO != 0) {
		qglDeleteFramebuffers(1, &ssaoBlurFBO);
		ssaoBlurFBO = 0;
	}
	if (depthMipFBOs[0] != 0) {
		qglDeleteFramebuffers(MAX_DEPTH_MIPS, depthMipFBOs);
		depthMipFBOs[0] = 0;
	}
	if (viewspaceDepth != nullptr) {
		viewspaceDepth->PurgeImage();
		viewspaceDepth = nullptr;
	}
	if (ssaoResult != nullptr) {
		ssaoResult->PurgeImage();
		ssaoResult = nullptr;
	}
	if (ssaoBlurred != nullptr) {
		ssaoBlurred->PurgeImage();
		ssaoBlurred = nullptr;
	}
}

extern GLuint fboPrimary;
extern bool primaryOn;

void AmbientOcclusionStage::ComputeSSAOFromDepth() {
	GL_PROFILE("AmbientOcclusionStage");

	if (ssaoFBO != 0 && (globalImages->currentDepthImage->uploadWidth != ssaoResult->uploadWidth ||
		globalImages->currentDepthImage->uploadHeight != ssaoResult->uploadHeight)) {
		// resolution changed, need to recreate our resources
		Shutdown();
	}

	if (ssaoFBO == 0) {
		Init();
	}

	PrepareDepthPass();
	SSAOPass();
	BlurPass();

	// FIXME: this is a bit hacky, needs better FBO control
	qglBindFramebuffer(GL_FRAMEBUFFER, primaryOn ? fboPrimary : 0);
}

void AmbientOcclusionStage::SSAOPass() {
	GL_PROFILE("SSAOPass");

	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	qglClearColor(1, 1, 1, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	viewspaceDepth->Bind();

	ssaoShader->Activate();
	SetQualityLevelUniforms();
	RB_DrawFullScreenQuad();
}

void AmbientOcclusionStage::BlurPass() {
	GL_PROFILE("BlurPass");

	ssaoBlurShader->Activate();
	BlurUniforms *uniforms = ssaoBlurShader->GetUniformGroup<BlurUniforms>();
	uniforms->edgeSharpness.Set(r_ssao_edgesharpness.GetFloat());
	uniforms->source.Set(0);
	uniforms->axis.Set(1, 0);

	// first horizontal pass
	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	qglClearColor(1, 1, 1, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	ssaoResult->Bind();
	RB_DrawFullScreenQuad();

	// second vertical pass
	uniforms->axis.Set(0, 1);
	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	qglClear(GL_COLOR_BUFFER_BIT);
	ssaoBlurred->Bind();
	RB_DrawFullScreenQuad();
}

void AmbientOcclusionStage::BindSSAOTexture(int index) {
	GL_SelectTexture(index);
	if (ShouldEnableForCurrentView()) {
		ssaoResult->Bind();
	}
	else {
		globalImages->whiteImage->Bind();
	}
}

bool AmbientOcclusionStage::ShouldEnableForCurrentView() const {
	return r_ssao.GetBool() && !backEnd.viewDef->IsLightGem() && !backEnd.viewDef->isSubview && !backEnd.viewDef->isXraySubview;
}

void AmbientOcclusionStage::PrepareDepthPass() {
	GL_PROFILE("PrepareDepthPass");

	qglBindFramebuffer(GL_FRAMEBUFFER, depthMipFBOs[0]);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	globalImages->currentDepthImage->Bind();

	depthShader->Activate();
	RB_DrawFullScreenQuad();

	if (r_ssao.GetInteger() > 1) {
		GL_PROFILE("DepthMips");
		// generate mip levels - used by the AO shader for distant samples to ensure we hit the texture cache as much as possible
		depthMipShader->Activate();
		DepthMipUniforms *uniforms = depthMipShader->GetUniformGroup<DepthMipUniforms>();
		uniforms->depth.Set(0);
		viewspaceDepth->Bind();
		for (int i = 1; i <= MAX_DEPTH_MIPS; ++i) {
			qglBindFramebuffer(GL_FRAMEBUFFER, depthMipFBOs[i]);
			uniforms->previousMipLevel.Set(i - 1);
			RB_DrawFullScreenQuad();
		}
	}
}

void AmbientOcclusionStage::ShowSSAO() {
	showSSAOShader->Activate();
	BindSSAOTexture(0);
	RB_DrawFullScreenQuad();
}

void AmbientOcclusionStage::SetQualityLevelUniforms() {
	AOUniforms *uniforms = ssaoShader->GetUniformGroup<AOUniforms>();
	uniforms->depthBias.Set(r_ssao_bias.GetFloat());
	uniforms->baseValue.Set(r_ssao_base.GetFloat());
	float sampleRadius;
	switch (r_ssao.GetInteger()) {
	case 1:
		 uniforms->maxMipLevel.Set(0);
		 uniforms->numSpiralTurns.Set(5);
		 uniforms->numSamples.Set(7);
		 sampleRadius = 0.5f * r_ssao_radius.GetFloat();
		 break;
	case 2:
		 uniforms->maxMipLevel.Set(MAX_DEPTH_MIPS);
		 uniforms->numSpiralTurns.Set(7);
		 uniforms->numSamples.Set(12);
		 sampleRadius = r_ssao_radius.GetFloat();
		 break;
	case 3:
	default:
		 uniforms->maxMipLevel.Set(MAX_DEPTH_MIPS);
		 uniforms->numSpiralTurns.Set(7);
		 uniforms->numSamples.Set(24);
		 sampleRadius = 2.0f * r_ssao_radius.GetFloat();
		 break;
	}
	uniforms->intensityDivR6.Set(r_ssao_intensity.GetFloat() / pow(sampleRadius * 0.02309f, 6));
	uniforms->sampleRadius.Set(sampleRadius);
}
