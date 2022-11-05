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

#include "tr_local.h"

class FrameBuffer;

class VolumetricStage {
public:
	VolumetricStage();
	~VolumetricStage();

	void Init();
	void Shutdown();

	bool RenderLight(const viewDef_t *viewDef, const viewLight_t *viewLight);

private:
	struct TemporaryData;

	void PrepareRaymarching(TemporaryData &data);
	void RenderRaymarching(const TemporaryData &data);
	void Blur(idImage *sourceTexture, bool vertical);
	void PerformCompositing(idImage *colorTexture);
	void SetScissor();
	void RenderFrustum(GLSLProgram *shader);

	idImage *workImage[2] = {nullptr, nullptr};
	FrameBuffer *workFBO[2] = {nullptr, nullptr};
	GLSLProgram *raymarchingShader = nullptr;
	GLSLProgram *blurShader = nullptr;
	GLSLProgram *compositingShader = nullptr;

	// temporary data
	mutable const viewDef_t *viewDef;
	mutable const viewLight_t *viewLight;
};

extern VolumetricStage *volumetric;
