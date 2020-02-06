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


#include "ImmediateRendering.h"
#include "GLSLProgramManager.h"
#include "GLSLProgram.h"
#include "glsl.h"
#include "tr_local.h"


//OpenGL can be only called from one thread anyway
//so it is perfectly OK to have two global buffers
static idList<ImmediateRendering::VertexData> buffers[2];
static uintptr_t lastThreadId = 0;

static bool redirectToGL = false;

ImmediateRendering::ImmediateRendering() {
	redirectToGL = r_glCoreProfile.GetInteger() == 0;
	if (redirectToGL)
		return;

	vertexList.Swap(buffers[0]);
	tempList.Swap(buffers[1]);

	qglGetIntegerv(GL_VERTEX_ARRAY_BINDING, &restore_vao);
	qglGetIntegerv(GL_ARRAY_BUFFER_BINDING, &restore_vbo);
}

ImmediateRendering::~ImmediateRendering() {
	if (redirectToGL)
		return;

	Flush();

	qglBindVertexArray(restore_vao);
	qglBindBuffer(GL_ARRAY_BUFFER, restore_vbo);

	vertexList.SetNum(0, false);
	tempList.SetNum(0, false);
	vertexList.Swap(buffers[0]);
	tempList.Swap(buffers[1]);
}

void ImmediateRendering::Flush() {
	if (redirectToGL)
		return;
	//Note: currently we send geometry for drawing straight in glEnd
	//however, it might be faster to collect it into larger batch and send all at once
}

void ImmediateRendering::glBegin(GLenum mode) {
	if (redirectToGL)
		return qglBegin(mode);

	state_currentMode = mode;
	vertexList.SetNum(0, false);
	//Note: it seems that color state persists...
}

void ImmediateRendering::glEnd() {
	if (redirectToGL)
		return qglEnd();

	GLenum actualMode = state_currentMode;

	if (actualMode == GL_QUADS) {
		actualMode = GL_TRIANGLES;
		int n = vertexList.Num() / 4;
		tempList.SetNum(6 * n, false);
		for (int i = 0; i < n; i++) {
			tempList[6 * i + 0] = vertexList[4 * i + 0];
			tempList[6 * i + 1] = vertexList[4 * i + 1];
			tempList[6 * i + 2] = vertexList[4 * i + 2];
			tempList[6 * i + 3] = vertexList[4 * i + 0];
			tempList[6 * i + 4] = vertexList[4 * i + 2];
			tempList[6 * i + 5] = vertexList[4 * i + 3];
		}
		vertexList.Swap(tempList);
	}
	if (actualMode == GL_POLYGON) {
		actualMode = GL_TRIANGLES;
		int n = idMath::Imax(vertexList.Num() - 2, 0);
		tempList.SetNum(3 * n, false);
		for (int i = 0; i < n; i++) {
			tempList[3 * i + 0] = vertexList[0];
			tempList[3 * i + 1] = vertexList[i + 1];
			tempList[3 * i + 2] = vertexList[i + 2];
		}
		vertexList.Swap(tempList);
	}

	GLuint vbo = 0;
	GLuint vao = 0;
	qglGenBuffers(1, &vbo);
	qglGenVertexArrays(1, &vao);

	qglBindBuffer(GL_ARRAY_BUFFER, vbo);
	qglBufferData(GL_ARRAY_BUFFER, vertexList.Num() * sizeof(VertexData), vertexList.Ptr(), GL_STREAM_DRAW);

	qglBindVertexArray(vao);
	qglEnableVertexAttribArray(0);
	qglEnableVertexAttribArray(3);
	qglVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, vertex));
	qglVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, color));

	qglDrawArrays(actualMode, 0, vertexList.Num());

	qglDeleteVertexArrays(1, &vao);
	qglDeleteBuffers(1, &vbo);
}

void ImmediateRendering::glVertex4f(float x, float y, float z, float w) {
	if (redirectToGL)
		return qglVertex4f(x, y, z, w);

	VertexData v = {idVec4(x, y, z, w), state_currentColor};
	vertexList.AddGrow(v);
}

void ImmediateRendering::glColor4f(float r, float g, float b, float a) {
	if (redirectToGL)
		return qglColor4f(r, g, b, a);

	state_currentColor.Set(r, g, b, a);
}

void ImmediateRendering::glColor4ub(byte r, byte g, byte b, byte a) {
	if (redirectToGL)
		return qglColor4ub(r, g, b, a);

	static const float coeff = 1.0f / 255.0f;
	state_currentColor.Set(r*coeff, g*coeff, b*coeff, a*coeff);
}
