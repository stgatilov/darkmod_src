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

#pragma once

extern idCVar r_bloom;
extern idCVar r_bloom_weight;

class BloomStage {
public:
	void Init();

	void Shutdown();

	void ComputeBloomFromRenderImage();
	void BindBloomTexture();

	static const int MAX_DOWNSAMPLING_STEPS = 16;
private:
	GLuint downsampleFBOs[MAX_DOWNSAMPLING_STEPS] = { 0 };
	GLuint upsampleFBOs[MAX_DOWNSAMPLING_STEPS] = { 0 };
	idImage *bloomDownSamplers = nullptr;
	idImage *bloomUpSamplers = nullptr;
	GLSLProgram *downsampleShader = nullptr;
	GLSLProgram *downsampleWithBrightPassShader = nullptr;
	GLSLProgram *blurShader = nullptr;
	GLSLProgram *upsampleShader = nullptr;
	int numDownsamplingSteps = 0;

	void Downsample();
	void Blur();
	void Upsample();
};

extern BloomStage *bloom;
