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

#ifndef __IMMEDIATE_RENDERING_H__
#define __IMMEDIATE_RENDERING_H__


class ImmediateRendering {
public:
	ImmediateRendering();
	~ImmediateRendering();

	//ensure that all geometry collected to this moment gets drawn
	//this should be called e.g. before changing textures/shaders/...
	void Flush();

	void glBegin(GLenum mode);
	void glEnd();

	void glVertex4f(float x, float y, float z, float w);
	void glColor4f(float r, float g, float b, float a);
	void glColor4ub(byte r, byte g, byte b, byte a);
	void glTexCoord4f(float s, float t, float r, float q);

	ID_FORCE_INLINE void glVertex3f(float x, float y, float z) { glVertex4f(x, y, z, 1.0f); }
	ID_FORCE_INLINE void glVertex2f(float x, float y) { glVertex4f(x, y, 0.0f, 1.0f); }
	ID_FORCE_INLINE void glColor3f(float r, float g, float b) { glColor4f(r, g, b, 1.0f); }
	ID_FORCE_INLINE void glColor3ub(byte r, byte g, byte b) { glColor4ub(r, g, b, 255U); }
	ID_FORCE_INLINE void glTexCoord3f(float s, float t, float r) { glTexCoord4f(s, t, r, 1.0f); }
	ID_FORCE_INLINE void glTexCoord2f(float s, float t) { glTexCoord4f(s, t, 0.0f, 1.0f); }

	ID_FORCE_INLINE void glVertex4fv(const float *ptr) { glVertex4f(ptr[0], ptr[1], ptr[2], ptr[3]); }
	ID_FORCE_INLINE void glColor4fv(const float *ptr) { glColor4f(ptr[0], ptr[1], ptr[2], ptr[3]); }
	ID_FORCE_INLINE void glColor4ubv(const byte *ptr) { glColor4ub(ptr[0], ptr[1], ptr[2], ptr[3]); }
	ID_FORCE_INLINE void glTexCoord4fv(const float *ptr) { glTexCoord4f(ptr[0], ptr[1], ptr[2], ptr[3]); }
	ID_FORCE_INLINE void glVertex3fv(const float *ptr) { glVertex3f(ptr[0], ptr[1], ptr[2]); }
	ID_FORCE_INLINE void glColor3fv(const float *ptr) { glColor3f(ptr[0], ptr[1], ptr[2]); }
	ID_FORCE_INLINE void glColor3ubv(const byte *ptr) { glColor3ub(ptr[0], ptr[1], ptr[2]); }
	ID_FORCE_INLINE void glTexCoord3fv(const float *ptr) { glTexCoord3f(ptr[0], ptr[1], ptr[2]); }
	ID_FORCE_INLINE void glVertex2fv(const float *ptr) { glVertex2f(ptr[0], ptr[1]); }
	ID_FORCE_INLINE void glTexCoord2fv(const float *ptr) { glTexCoord2f(ptr[0], ptr[1]); }

	struct VertexData {
		idVec4 vertex;
		byte color[4];
		idVec2 texCoord;
	};
private:

	GLenum state_currentMode = -1;
	byte state_currentColor[4] = {255, 255, 255, 255};
	idVec4 state_currentTexCoord = idVec4(0.0f, 0.0f, 0.0f, 1.0f);

	idList<VertexData> vertexList, tempList;

	GLint restore_vao = 0;
	GLint restore_vbo = 0;
};

#endif
