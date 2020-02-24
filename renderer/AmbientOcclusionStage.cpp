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

idCVar r_ssao("r_ssao", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Enable screen space ambient occlusion");
idCVar r_ssao_radius("r_ssao_radius", "24", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
					 "View space sample radius - larger values provide a softer, spread effect, but risk causing unwanted halo shadows around objects");
idCVar r_ssao_cutoff("r_ssao_cutoff", "8", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
					 "Max view space depth difference - larger distances are considered to not occlude");
idCVar r_ssao_bias("r_ssao_bias", "0.025", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
				   "Min depth difference to count for occlusion, used to avoid some acne effects");
idCVar r_ssao_power("r_ssao_power", "1.5", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
					"SSAO exponential factor, the higher the value, the stronger the effect");
idCVar r_ssao_base("r_ssao_base", "0.1", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE,
				   "Minimum baseline visibility below which AO cannot drop");
idCVar r_ssao_kernelSize("r_ssao_kernelSize", "8", CVAR_INTEGER | CVAR_RENDERER | CVAR_ARCHIVE,
						 "Size of sample kernel (max 128) - higher values will impact performance!");

extern idCVar r_fboResolution;

AmbientOcclusionStage ambientOcclusionImpl;
AmbientOcclusionStage *ambientOcclusion = &ambientOcclusionImpl;

namespace {
	struct AOUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF(AOUniforms)

		DEFINE_UNIFORM(sampler, depthTexture)
		DEFINE_UNIFORM(sampler, noiseTexture)
		DEFINE_UNIFORM(float, sampleRadius)
		DEFINE_UNIFORM(float, depthCutoff)
		DEFINE_UNIFORM(float, depthBias)
		DEFINE_UNIFORM(float, baseValue)
		DEFINE_UNIFORM(vec3, sampleKernel)
		DEFINE_UNIFORM(int, kernelSize)
		DEFINE_UNIFORM(float, power)
	};

	const int MAX_KERNEL_SIZE = 128;

	void CreateHemisphereSampleKernel(AOUniforms *uniforms) {
		// Create random vectors within a unit hemisphere. Used in the SSAO shader
		// to sample the surrounding geometry.
		std::uniform_real_distribution<float> uniformRandom(0.f, 1.f);
		std::mt19937 generator (543210);
		idList<idVec3> kernel;
		for ( int i = 0; i < MAX_KERNEL_SIZE; ++i ) {
			// generate a random point on the unit hemisphere
			float u = uniformRandom(generator), v = uniformRandom(generator);
			float theta = idMath::ACos(idMath::Sqrt(1 - u));
			float phi = v * idMath::TWO_PI;
			idVec3 sample(idMath::Sin(theta)*idMath::Cos(phi), idMath::Sin(theta)*idMath::Sin(phi), idMath::Cos(theta));
			// now choose a distribution with points concentrated closer to the origin
			float scale = i / (float)MAX_KERNEL_SIZE;
			scale = Lerp(0.1f, 1.0f, scale * scale);
			kernel.Append(sample * scale);
		}
		std::shuffle(kernel.begin(), kernel.end(), generator);
		uniforms->sampleKernel.SetArray(MAX_KERNEL_SIZE, kernel[ 0 ].ToFloatPtr());
	}

	void LoadSSAOShader(GLSLProgram *ssaoShader) {
		ssaoShader->Init();
		ssaoShader->AttachVertexShader("ssao.vert.glsl");
		ssaoShader->AttachFragmentShader("ssao.frag.glsl");
		Attributes::Default::Bind(ssaoShader);
		ssaoShader->Link();
		ssaoShader->Activate();
		AOUniforms *uniforms = ssaoShader->GetUniformGroup<AOUniforms>();
		uniforms->depthTexture.Set(0);
		uniforms->noiseTexture.Set(1);
		CreateHemisphereSampleKernel(uniforms);
		ssaoShader->Deactivate();
	}

	void CreateSSAOColorBuffer(idImage *image) {
		image->type = TT_2D;
		GLuint curWidth = r_fboResolution.GetFloat() * glConfig.vidWidth;
		GLuint curHeight = r_fboResolution.GetFloat() * glConfig.vidHeight;
		image->GenerateAttachment(curWidth, curHeight, GL_RED);
	}

	void CreateSSAONoiseTexture(idImage *image) {
		// Create a small noise texture to introduce some randomness in the SSAO
		// sampling. Allows us to get away with fewer samples by blurring over the
		// randomness afterwards.
		idRandom rnd(12345);
		idList<idVec3> noise;
		for ( int i = 0; i < 16; ++i ) {
			idVec3 randomVec(rnd.RandomFloat(), rnd.RandomFloat(), 0.5);
			noise.Append(randomVec);
		}
		image->type = TT_2D;
		image->uploadWidth = 4;
		image->uploadHeight = 4;
		qglGenTextures(1, &image->texnum);
		qglBindTexture(GL_TEXTURE_2D, image->texnum);
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.Ptr());
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	}
}

AmbientOcclusionStage::AmbientOcclusionStage() : ssaoFBO(0), ssaoBlurFBO(0), ssaoResult(nullptr), ssaoBlurred(nullptr),
												 ssaoNoise(nullptr), ssaoShader(nullptr) {
}

void AmbientOcclusionStage::Init() {
	ssaoResult = globalImages->ImageFromFunction("SSAO ColorBuffer", CreateSSAOColorBuffer);
	ssaoResult->ActuallyLoadImage();
	ssaoBlurred = globalImages->ImageFromFunction("SSAO Blurred", CreateSSAOColorBuffer);
	ssaoBlurred->ActuallyLoadImage();
	ssaoNoise = globalImages->ImageFromFunction("SSAO Noise", CreateSSAONoiseTexture);

	qglGenFramebuffers(1, &ssaoFBO);
	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoResult->texnum, 0);
	qglGenFramebuffers(1, &ssaoBlurFBO);
	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurred->texnum, 0);
	qglBindFramebuffer(GL_FRAMEBUFFER, 0);

	ssaoShader = programManager->Find("ssao");
	if ( ssaoShader == nullptr ) {
		ssaoShader = programManager->LoadFromGenerator("ssao", LoadSSAOShader);
	}
	ssaoShader->Activate();

	ssaoBlurShader = programManager->Find("ssao_blur");
	if ( ssaoBlurShader == nullptr ) {
		ssaoBlurShader = programManager->LoadFromFiles("ssao_blur", "ssao.vert.glsl", "ssao_blur.frag.glsl");
	}
}

void AmbientOcclusionStage::Shutdown() {
	if ( ssaoFBO != 0 ) {
		qglDeleteFramebuffers(1, &ssaoFBO);
		ssaoFBO = 0;
	}
	if ( ssaoBlurFBO != 0 ) {
		qglDeleteFramebuffers(1, &ssaoBlurFBO);
		ssaoBlurFBO = 0;
	}
	if ( ssaoResult != nullptr ) {
		ssaoResult->PurgeImage();
	}
	if ( ssaoBlurred != nullptr ) {
		ssaoBlurred->PurgeImage();
	}
	if ( ssaoNoise != nullptr ) {
		ssaoNoise->PurgeImage();
	}
}

extern GLuint fboPrimary;
extern bool primaryOn;

void AmbientOcclusionStage::ComputeSSAOFromDepth() {
	GL_PROFILE("AmbientOcclusionStage");

	if ( ssaoFBO != 0 && (globalImages->currentDepthImage->uploadWidth != ssaoResult->uploadWidth ||
						  globalImages->currentDepthImage->uploadHeight != ssaoResult->uploadHeight)) {
		// resolution changed, need to recreate our resources
		Shutdown();
	}

	if ( ssaoFBO == 0 ) {
		Init();
	}

	SSAOPass();
	BlurPass();

	// FIXME: this is a bit hacky, needs better FBO control
	qglBindFramebuffer(GL_FRAMEBUFFER, primaryOn ? fboPrimary : 0);
}

void AmbientOcclusionStage::SSAOPass() {
	GL_PROFILE("SSAOPass");

	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	globalImages->currentDepthImage->Bind();
	GL_SelectTexture(1);
	ssaoNoise->Bind();

	ssaoShader->Activate();
	AOUniforms *uniforms = ssaoShader->GetUniformGroup<AOUniforms>();
	uniforms->sampleRadius.Set(r_ssao_radius.GetFloat());
	uniforms->depthCutoff.Set(r_ssao_cutoff.GetFloat());
	uniforms->depthBias.Set(r_ssao_bias.GetFloat());
	uniforms->baseValue.Set(r_ssao_base.GetFloat());
	uniforms->power.Set(r_ssao_power.GetFloat());
	int kernelSize = std::max(1, std::min(MAX_KERNEL_SIZE, r_ssao_kernelSize.GetInteger()));
	uniforms->kernelSize.Set(kernelSize);

	RB_DrawFullScreenQuad();
}

void AmbientOcclusionStage::BlurPass() {
	GL_PROFILE("BlurPass");

	qglBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	qglClear(GL_COLOR_BUFFER_BIT);
	GL_SelectTexture(0);
	ssaoResult->Bind();
	ssaoBlurShader->Activate();
	RB_DrawFullScreenQuad();
}

void AmbientOcclusionStage::BindSSAOTexture(int index) {
	GL_SelectTexture(index);
	ssaoBlurred->Bind();
}
