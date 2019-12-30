#version 150

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in vec2 texCoord[];

out FragmentData {
	vec2 texCoord;
} frag;

uniform float u_lightRadius;
 
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
 
void main() {
    for(int j = 0; j < 6; j++) {
		//if(j!=outputSide)
			//continue;
        //gl_Layer = j;
        for(int i = 0; i < 3; i++) {
            vec4 frag_pos = vec4(cubicTransformations[j] * gl_in[i].gl_Position.xyz, 1);
            gl_Position.x = frag_pos.x / 6 + frag_pos.z * 5/6 - frag_pos.z / 3 * j;;
            gl_Position.y = frag_pos.y;
            gl_Position.z = -frag_pos.z - 2*u_lightRadius;
            gl_Position.w = -frag_pos.z;
		    frag.texCoord = texCoord[i];
			gl_ClipDistance[0] = dot(frag_pos, ClipPlanes[0]);
			gl_ClipDistance[1] = dot(frag_pos, ClipPlanes[1]);
			gl_ClipDistance[2] = dot(frag_pos, ClipPlanes[2]);
			gl_ClipDistance[3] = dot(frag_pos, ClipPlanes[3]);
            EmitVertex();
        }
        EndPrimitive();
    }
}