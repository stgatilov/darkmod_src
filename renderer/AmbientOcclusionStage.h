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

extern idCVar r_ssao;

class AmbientOcclusionStage {
public:
	AmbientOcclusionStage();

	void Init();

	void Shutdown();

	void ComputeSSAOFromDepth();

	void BindSSAOTexture(int index);

private:
	GLuint ssaoFBO;
	GLuint ssaoBlurFBO;
	idImage *ssaoResult;
	idImage *ssaoBlurred;
	idImage *ssaoNoise;
	GLSLProgram *ssaoShader;
	GLSLProgram *ssaoBlurShader;

	void SSAOPass();

	void BlurPass();
};

extern AmbientOcclusionStage *ambientOcclusion;
