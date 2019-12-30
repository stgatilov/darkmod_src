#version 150
#extension GL_ARB_gpu_shader5: enable

#define MAX_LIGHTS 16
#define INVOCATIONS 1

#ifdef INVOCATIONS
#define MAX_VERTICES 18
layout(triangles, invocations = MAX_LIGHTS) in;
layout(triangle_strip, max_vertices = MAX_VERTICES) out;
#else
#define MAX_VERTICES 240 // 16*5*3 - no triangle can make it into all 6 faces, right?
layout(triangles) in;
layout(triangle_strip, max_vertices = MAX_VERTICES) out;
#endif

in VertexData {
	vec4 position;
	vec2 texCoord;
	float culled[MAX_LIGHTS*6];
} vert[];

out FragmentData {
	vec2 texCoord;
} frag;

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
 
// I suspect that the right way is to cook the projection matrices for each page on CPU and pass as uniforms to avoid this mess
vec2 ShadowAtlasForVector(vec3 v, int lightNo, int targetFace) {
	vec2 shadow2d = v.xy*.5 + vec2(-v.z)*.5;
	float pageSize = u_shadowRect[lightNo].w + u_shadowTexelStep;
	vec2 pageOrigin = u_shadowRect[lightNo].xy - vec2(u_shadowTexelStep / 2);
	pageOrigin.x += pageSize * targetFace;
	pageOrigin = pageOrigin * 2 - vec2(1);
	shadow2d *= pageSize * 2;// + pageOrigin * -v.z;
	shadow2d.xy += -v.z * pageOrigin;
	return vec2(shadow2d);
}

#define CULL_BACK_FACES 1
//#define CULL_TRIANGLES_IN 1
#define CULL_TRIANGLES_OUT 1

void OneLight(int lightNo) {
#ifdef CULL_TRIANGLES_IN
    for(int j = 6*lightNo; j < 6*lightNo+6; j++) 
		if(vert[0].culled[j] + vert[1].culled[j] + vert[2].culled[j] == 3)
			return;
#endif
	vec3 vertsInLightSpace[3];
    for(int i = 0; i < 3; i++) 
		vertsInLightSpace[i] = vert[i].position.xyz - u_lightOrigin[lightNo];

	// cull back faces
#ifdef CULL_BACK_FACES
	vec3 edge1 = vertsInLightSpace[1].xyz - vertsInLightSpace[0].xyz;
	vec3 edge2 = vertsInLightSpace[2].xyz - vertsInLightSpace[0].xyz;
	if(dot(cross(edge1, edge2), vertsInLightSpace[0].xyz) < 0)
		return;	
#endif

    for(int j = 0; j < 6; j++) {
//		if(j!=2) continue;		// debug - single face

		vec4 clip[3], vertsInCubeFaceSpace[3];
		vec4 clipCount = vec4(0);
        for(int i = 0; i < 3; i++) {
			vec3 vertInLightSpace = vertsInLightSpace[i];
            vec4 vertInCubeFaceSpace = vec4(cubicTransformations[j] * vertInLightSpace, 1);
            //vertInCubeFaceSpace = vec4(vertInLightSpace, 1);
			vertsInCubeFaceSpace[i] = vertInCubeFaceSpace;
			for(int k=0; k<4; k++) {
				clip[i][k] = dot(vertInCubeFaceSpace, ClipPlanes[k]);
				clipCount[k] += float(clip[i][k] < 0);
			}
		}
#ifdef CULL_TRIANGLES_OUT
		if(max(max(clipCount[0], clipCount[1]), max(clipCount[2], clipCount[3])) == 3) continue;
#endif
        for(int i = 0; i < 3; i++) {
            vec4 vertInCubeFaceSpace = vertsInCubeFaceSpace[i];
			gl_Position.xy = ShadowAtlasForVector(vertInCubeFaceSpace.xyz, lightNo, j);
            gl_Position.z = (-vertInCubeFaceSpace.z - 2*u_lightRadius[lightNo]);
            gl_Position.w = -vertInCubeFaceSpace.z;
		    frag.texCoord = vert[i].texCoord;
			// clip the triangle to the atlas page 
			gl_ClipDistance[0] = clip[i].x;
			gl_ClipDistance[1] = clip[i].y;
			gl_ClipDistance[2] = clip[i].z;
			gl_ClipDistance[3] = clip[i].w;
			//gl_ClipDistance[0] = gl_ClipDistance[1] = gl_ClipDistance[2] = gl_ClipDistance[3] = 1; // debug - don't clip to atlas pages
            EmitVertex();
        }
        EndPrimitive();
    }
}

void main() {
#ifdef INVOCATIONS
	int lightNo = gl_InvocationID;
	if(lightNo >= u_lightCount)
		return;
//		if(lightNo!=0)	return; // debug - single light
	OneLight(lightNo);
#else
	for (int lightNo = 0; lightNo < u_lightCount; lightNo++) {
//		if(lightNo!=0)	return; // debug - single light
  		OneLight(lightNo);
	}
#endif
}