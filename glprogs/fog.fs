#version 140

uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec3 u_fogColor;
uniform bool u_newFog;
uniform float u_fogEnter;
uniform float u_fogDensity;
uniform vec4 u_frustumPlanes[6];
uniform vec3 u_viewOrigin;

in vec4 var_TexCoord0;
in vec4 var_TexCoord1;
in vec4 var_worldPosition;

out vec4 FragColor;

void main() {
	vec4 texel0 = texture(u_texture0, var_TexCoord0.xy);
	vec4 texel1 = texture(u_texture1, var_TexCoord1.xy);
	FragColor = vec4(u_fogColor, 1)*texel0*texel1;	
	if(u_newFog) {

	vec3 dirToViewer = normalize(u_viewOrigin - var_worldPosition.xyz);
	float minRayCoord = length(u_viewOrigin - var_worldPosition.xyz); 
	vec4 points[2] = vec4[2] (vec4(u_viewOrigin,1), var_worldPosition);
	for(int i=0; i<6; i++) {  
//			continue;
		float top = dot(u_frustumPlanes[i], points[0]);
		float bottom = dot(u_frustumPlanes[i], points[1]);
		if(top <= 0 && bottom <= 0) // both inside, no clip on this plane
			continue;
		if(top > 0 && bottom > 0) {	// both outside, no fog
			FragColor = vec4(0);
			return;
		}
		float distance = length(points[1]-points[0]);
		if(top > 0) 
			points[0] = mix(points[0], points[1], top/(top-bottom));
		if(bottom > 0) 
			points[1] = mix(points[1], points[0], bottom/(bottom-top));
	}

		float fogged = length(points[1]-points[0]) * u_fogDensity * 5;
		FragColor = vec4(u_fogColor, fogged);
	}
}