#version 150

#define MAX_LIGHTS 16

//#define MAX_VERTICES 240 // 16*5*3 - no triangle can make it into all 6 faces, right?
#define MAX_VERTICES 128 // nVidia limit
layout(triangles) in;
layout(triangle_strip, max_vertices = MAX_VERTICES) out;
//layout(invocations = MAX_LIGHTS) in;

in VertOutput {
	vec2 texCoord;
	ivec2 clipX;
	ivec2 clipY;
} vert[];

out FragmentData {
	vec2 texCoord;
} frag;

uniform mat4 u_modelMatrix;

uniform int u_lightCount;
uniform float u_shadowTexelStep;
uniform vec3 u_lightOrigin[MAX_LIGHTS];
uniform vec4 u_shadowRect[MAX_LIGHTS];

const mat3 cubicTransformations[6] = mat3[6] (
	mat3(
		0, 0, -1,
		0, -1, 0,
		-1, 0, 0
	),
	mat3(
		0, 0, 1,
		0, -1, 0,
		1, 0, 0
	),
	mat3(
		1, 0, 0,
		0, 0, -1,
		0, 1, 0
	),
	mat3(
		1, 0, 0,
		0, 0, 1,
		0, -1, 0
	),
	mat3(
		1, 0, 0,
		0, -1, 0,
		0, 0, -1
	),
	mat3(
		-1, 0, 0,
		0, -1, 0,
		0, 0, 1
	)
);

const float clipEps = 0e-2;
const vec4 ClipPlanes[4] = vec4[4] (
	vec4(1, 0, -1, clipEps),
	vec4(-1, 0, -1, clipEps),
	vec4(0, 1, -1, clipEps),
	vec4(0, -1, -1, clipEps)
);
 
//int lightNo = gl_InvocationID;
int lightNo;
int faceNo;

// I suspect that the right way is to cook the projection matrices for each page on CPU and pass as uniforms to avoid this mess
vec2 ShadowAtlasForVector(vec3 v, int lightNo) {
	vec2 shadow2d = v.xy*.5 + vec2(-v.z)*.5;
	float pageSize = u_shadowRect[lightNo].w + u_shadowTexelStep;
	vec2 pageOrigin = u_shadowRect[lightNo].xy;
	pageOrigin.x += pageSize * faceNo;
	pageOrigin = pageOrigin * 2 - 1;
	shadow2d *= u_shadowRect[lightNo].w*2 + u_shadowTexelStep*5e-3; // FIXME rasterization rules workaround
	shadow2d.xy += -v.z * pageOrigin;
	return vec2(shadow2d);
}

void doVertex(int vertexNo) {
	vec3 inLightSpace = gl_in[vertexNo].gl_Position.xyz - u_lightOrigin[lightNo];
	vec4 inCubeFaceSpace = vec4(cubicTransformations[faceNo] * inLightSpace, 1);
	gl_Position.xy = ShadowAtlasForVector(inCubeFaceSpace.xyz, lightNo);
    gl_Position.z = (-inCubeFaceSpace.z - 2);
    gl_Position.w = -inCubeFaceSpace.z;

	gl_ClipDistance[0] = dot(inCubeFaceSpace, ClipPlanes[0]);
	gl_ClipDistance[1] = dot(inCubeFaceSpace, ClipPlanes[1]);
	gl_ClipDistance[2] = dot(inCubeFaceSpace, ClipPlanes[2]);
	gl_ClipDistance[3] = dot(inCubeFaceSpace, ClipPlanes[3]);
    
	frag.texCoord = vert[vertexNo].texCoord;
	
	EmitVertex();	
}

void doFace() {
	doVertex(0);
	doVertex(1);
	doVertex(2);
	EndPrimitive();
}

const ivec2 iZero2 = ivec2(0);
ivec2 clipX = vert[0].clipX & vert[1].clipX & vert[2].clipX;
ivec2 clipY = vert[0].clipY & vert[1].clipY & vert[2].clipY;

void doLight() {
	if(lightNo>=u_lightCount)
		return;
	if(1==1) {
		ivec2 test;
		test = clipX & (1 << lightNo);
		if(test != iZero2)
			return;
		test = clipY & (1 << lightNo);
		if(test != iZero2)
			return;
	}
	for(faceNo=0; faceNo<6; faceNo++) {
		doFace();
	}
}

void main() {
	for(lightNo=0;lightNo<u_lightCount;lightNo++)
		doLight();
}