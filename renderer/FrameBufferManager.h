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

class FrameBuffer;

class FrameBufferManager
{
public:
	using Generator = std::function<void( FrameBuffer* )>;

	~FrameBufferManager();

	void Init();
	void Shutdown();

	FrameBuffer *CreateFromGenerator( const idStr &name, Generator generator );

	void PurgeAll();

	void BeginFrame();

	void EnterPrimary();
	void LeavePrimary(bool copyToDefault = true);
	void EnterShadowStencil();
	void LeaveShadowStencil();
	void ResolveShadowStencilAA();
	void EnterShadowMap();
	void LeaveShadowMap();

	void ResolvePrimary( GLbitfield mask = GL_COLOR_BUFFER_BIT, GLenum filter = GL_NEAREST );
	void UpdateCurrentRenderCopy();
	void UpdateCurrentDepthCopy();

	void CopyRender( const copyRenderCommand_t &cmd ); 

private:
	idList<FrameBuffer*> fbos;

	void UpdateResolutionAndFormats();
	void CreatePrimary(FrameBuffer *primary);
	void CreateResolve(FrameBuffer *resolve);
	void CreateGui(FrameBuffer *gui);

	void CopyRender( idImage *image, int x, int y, int imageWidth, int imageHeight );
	void CopyRender( unsigned char *buffer, int x, int y, int imageWidth, int imageHeight, bool usePBO );
	GLuint pbo = 0;


	// TODO: this should be moved to a dedicated shadow stage
	void CreateStencilShadow(FrameBuffer *shadow);
	void CreateMapsShadow(FrameBuffer *shadow);
	int shadowAtlasSize = 0;
	bool depthCopiedThisView = false;

public:
	int renderWidth = 0;
	int renderHeight = 0;
	GLenum colorFormat = 0;
	GLenum depthStencilFormat = 0;

	FrameBuffer *defaultFbo = nullptr;
	FrameBuffer *primaryFbo = nullptr;
	FrameBuffer *resolveFbo = nullptr;
	FrameBuffer *guiFbo = nullptr;
	FrameBuffer *shadowStencilFbo = nullptr;
	FrameBuffer *shadowMapFbo = nullptr;

	FrameBuffer *currentRenderFbo = nullptr;

	FrameBuffer *activeFbo = nullptr;
	FrameBuffer *activeDrawFbo = nullptr;
};

extern FrameBufferManager *frameBuffers;
