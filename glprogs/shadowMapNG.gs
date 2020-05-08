#version 400

#define MAX_LIGHTS 16

//#define MAX_VERTICES 240 // 16*5*3 - no triangle can make it into all 6 faces, right?
#define MAX_VERTICES 128 // nVidia limit
layout(triangles) in;
layout(triangle_strip, max_vertices = MAX_VERTICES) out;
layout(invocations = MAX_LIGHTS) in;

in VertOutput {
	vec2 texCoord;
	int test1;
	int test2;
	int test3;
	int test4;
} vert[];

out FragmentData {
	vec2 texCoord;
} frag;

uniform mat4 u_modelMatrix;

uniform int u_lightCount;
uniform float u_shadowTexelStep;
uniform float u_lightRadius[MAX_LIGHTS];
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
 
int lightNo = gl_InvocationID;
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

int zPassed;
vec4 outPosition[3]; 
vec4 outClipDistance[3]; 

void doVertex(int vertexNo) {
		vec4 clip;
		vec3 inLightSpace = gl_in[vertexNo].gl_Position.xyz - u_lightOrigin[lightNo];

			vec4 inCubeFaceSpace = vec4(cubicTransformations[faceNo] * inLightSpace, 1);
			for(int k=0; k<4; k++) {
				clip[k] = dot(inCubeFaceSpace, ClipPlanes[k]);
			}
			outPosition[vertexNo].xy = ShadowAtlasForVector(inCubeFaceSpace.xyz, lightNo);
            outPosition[vertexNo].z = (-inCubeFaceSpace.z - 2*u_lightRadius[lightNo]);
            outPosition[vertexNo].w = -inCubeFaceSpace.z;

	outClipDistance[vertexNo][0] = clip.x;
	outClipDistance[vertexNo][1] = clip.y;
	outClipDistance[vertexNo][2] = clip.z;
	outClipDistance[vertexNo][3] = clip.w;
    
	if(outPosition[vertexNo].z > 0)
		zPassed++;

	frag.texCoord = vert[vertexNo].texCoord;
}

void saveVertex(int no) {
	gl_Position = outPosition[no];//*vec4(0,1,1,1);
	gl_ClipDistance[0] = outClipDistance[no].x;
	gl_ClipDistance[1] = outClipDistance[no].y;
	gl_ClipDistance[2] = outClipDistance[no].z;
	gl_ClipDistance[3] = outClipDistance[no].w;
	EmitVertex();
}

void doLight() {
	zPassed = 0;
	doVertex(0);
	doVertex(1);
	doVertex(2);
//	if(zPassed < 1) return;
	saveVertex(0);
	saveVertex(1);
	saveVertex(2);
	EndPrimitive();
}

void main() {
	if(lightNo>=u_lightCount)
		return;
	if(1==1) {
		int test = vert[0].test1 & vert[1].test1 & vert[2].test1 & (1 << lightNo);
		if(test != 0)
			return;
		test = vert[0].test2 & vert[1].test2 & vert[2].test2 & (1 << lightNo);
		if(test != 0)
			return;
		test = vert[0].test3 & vert[1].test3 & vert[2].test3 & (1 << lightNo);
		if(test != 0)
			return;
		test = vert[0].test4 & vert[1].test4 & vert[2].test4 & (1 << lightNo);
		if(test != 0)
			return;
	}
//	}
	for(faceNo=0; faceNo<6; faceNo++) {
		doLight();
	}
}