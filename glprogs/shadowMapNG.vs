#version 150

#define MAX_LIGHTS 16

uniform mat4 u_modelMatrix;

uniform int u_lightCount;
uniform mat4 u_lightProjectionFalloff[MAX_LIGHTS];

in vec4 attr_Position;
in vec4 attr_TexCoord;

out VertOutput {
	vec2 texCoord;
	int test1;
	int test2;
	int test3;
	int test4;
//	float x;
} vert;

void main() {
	vert.test2 = 1;
	gl_Position = u_modelMatrix * attr_Position;
	vert.test1 = vert.test2 = vert.test3 = vert.test4 = 0;
	for(int i = 0; i<u_lightCount; i++) {
/*		int n = 0;
		vec4 v = u_lightProjectionFalloff[i] * gl_Position;
		v.xy /= v.w;
		n |= v.x < 0 ? 1 : 0;
		n |= v.x > 1 ? 2 : 0;
		n |= v.y < 0 ? 4 : 0;
		n |= v.y > 1 ? 8 : 0;
		n |= v.z < 0 ? 16 : 0;
		n |= v.z > 1 ? 32 : 0;
/*		lightNorm[i] = n;*/
		vec3 b0 = u_lightProjectionFalloff[i][0].xyz;
		vec3 b1 = u_lightProjectionFalloff[i][1].xyz;
		vec3 v = gl_Position.xyz;
		if(v.x < b0.x)
			vert.test1 |= 1 << i;
		if(v.x > b1.x)
			vert.test2 |= 1 << i;
		if(v.y < b0.y)
			vert.test3 |= 1 << i;
		if(v.y > b1.y)
			vert.test4 |= 1 << i;
	}
	//boundsTest = ivec4(0);
	//gl_Position = gl_Vertex;
	vert.texCoord = (attr_TexCoord).st;
}