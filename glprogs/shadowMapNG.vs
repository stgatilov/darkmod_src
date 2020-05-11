#version 150

#define MAX_LIGHTS 16

uniform mat4 u_modelMatrix;

uniform int u_lightCount;
uniform mat4 u_lightProjectionFalloff[MAX_LIGHTS];

in vec4 attr_Position;
in vec4 attr_TexCoord;

out VertOutput {
	vec2 texCoord;
	ivec2 clipX;
	ivec2 clipY;
} vert;

void main() {
	gl_Position = u_modelMatrix * attr_Position;
	vert.clipX = vert.clipY = ivec2(0);
	for(int i = 0; i<u_lightCount; i++) {
		vec3 b0 = u_lightProjectionFalloff[i][0].xyz;
		vec3 b1 = u_lightProjectionFalloff[i][1].xyz;
		vec3 v = gl_Position.xyz;
		if(v.x < b0.x)
			vert.clipX.x |= 1 << i;
		if(v.x > b1.x)
			vert.clipX.y |= 1 << i;
		if(v.y < b0.y)
			vert.clipY.x |= 1 << i;
		if(v.y > b1.y)
			vert.clipX.y |= 1 << i;
	}
	vert.texCoord = (attr_TexCoord).st;
}