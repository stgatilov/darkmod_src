/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma once

class FrameBuffer {
public:
	using Generator = std::function<void( FrameBuffer* )>;

	// do not use directly, use FrameBufferManager::CreateFromGenerator
	FrameBuffer(const idStr &name, const Generator &generator);
	~FrameBuffer();

	void Init(int width, int height, int msaa = 1);
	void Destroy();

	void AddColorRenderBuffer(int attachment, GLenum format);
	void AddColorRenderTexture(int attachment, idImage *texture, int mipLevel = 0);
	void AddDepthStencilRenderBuffer(GLenum format);
	void AddDepthStencilRenderTexture(idImage *texture);
	void AddDepthRenderTexture(idImage *texture);
	void AddStencilRenderTexture(idImage *texture);

	void Validate();

	void Bind();
	void BindDraw();

	void BlitTo(FrameBuffer *target, GLbitfield mask, GLenum filter = GL_LINEAR);

	int Width() const { return width; }
	int Height() const { return height; }
	int MultiSamples() const { return msaa; }

	const char *Name() const { return name.c_str(); }

	static void CreateDefaultFrameBuffer(FrameBuffer *fbo);

	static const int MAX_COLOR_ATTACHMENTS = 8;
private:
	idStr name;
	Generator generator;
	bool initialized = false;

	GLuint fbo = 0;
	int width = 0;
	int height = 0;
	int msaa = 0;

	GLuint colorRenderBuffers[MAX_COLOR_ATTACHMENTS] = { 0 };
	GLuint depthRenderBuffer = 0;

	void Generate();

	void AddRenderBuffer(GLuint &buffer, GLenum attachment, GLenum format, const idStr &name);
	void AddRenderTexture(idImage *texture, GLenum attachment, int mipLevel);
};

extern idCVar r_showFBO;
extern idCVar r_fboColorBits;
extern idCVarBool r_fboSRGB;
extern idCVar r_fboDepthBits;
extern idCVarInt r_shadowMapSize;
extern idCVar r_fboResolution;
extern idCVarBool r_tonemap;

extern renderCrop_t ShadowAtlasPages[42];

void FB_RenderTexture(idImage *texture);
void FB_DebugShowContents();
void FB_ApplyScissor();
