#version 430

layout(binding=0) uniform sampler2D s_projection;
layout(binding=1) uniform sampler2D s_falloff;
layout(binding=2) uniform sampler2D s_depth;
layout(binding=5) uniform sampler2D u_shadowMap;

layout (location = 0) uniform mat4[2] u_MVP;
layout (location = 2) uniform vec3 u_viewOrigin;
layout (location = 3) uniform mat4 u_lightProject;
layout (location = 4) uniform vec4 u_lightFrustum[6];
layout (location = 10) uniform vec4 u_shadowRect;
layout (location = 11) uniform vec3 u_lightOrigin;
layout (location = 12) uniform int u_sampleCount;
layout (location = 13) uniform float u_lightRadius;
layout (location = 14) uniform vec4 u_lightColor;

in vec4 csThis;
in vec4 lightProject;
in vec4 worldPosition;

#define SHOW_PRECISION_ISSUE 1

#ifdef SHOW_PRECISION_ISSUE

void main() {
	float minDist = 0; 
	for(int i=0; i<6; i++) {
		float dist = dot(u_lightFrustum[i], worldPosition);
		minDist = min(dist, dist);
	}
	gl_FragColor.rg = minDist * vec2(1, -1) * 1e-1;
	gl_FragColor.b = .5;
}

#else

void ShadowAtlasForVector(vec3 v, out vec4 depthSamples, out vec2 sampleWeights) {
	vec3 v1 = abs(v);
	float maxV = max(v1.x, max(v1.y, v1.z));
	int cubeSide = 0;
	if(maxV == v.x) {
		v1 = -v.zyx;
	} 
	else if(maxV == -v.x) {
		v1 = v.zyx * vec3(1, -1, 1);
		cubeSide = 1;
	}
	else if(maxV == v.y) {
		v1 = v.xzy * vec3(1, 1, -1);
		cubeSide = 2;
	}
	else if(maxV == -v.y) {
		v1 = v.xzy * vec3(1, -1, 1);
		cubeSide = 3;
	}
	else if(maxV == v.z) {
		v1 = v.xyz * vec3(1, -1, -1);
		cubeSide = 4;
	}
	else { //if(maxV == -v.z) {
		v1 = v.xyz * vec3(-1, -1, 1);
		cubeSide = 5;
	}
	v1.xy /= -v1.z;
	vec2 texSize = textureSize(u_shadowMap, 0);
	vec2 shadow2d = (v1.xy * (.5-.0/texSize.x) + vec2(.5) ) * u_shadowRect.ww + u_shadowRect.xy;
	shadow2d.x += u_shadowRect.w * cubeSide;
	vec4 d = textureGather(u_shadowMap, shadow2d);
	vec4 one = vec4(1.000001);
	depthSamples = u_lightRadius / (one - d);
	sampleWeights = fract(shadow2d * texSize + -0.5);
}

mat4 projMatrixInv = inverse(u_MVP[1]);

// this is supposed to get the world position from the depth buffer
vec3 ViewPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

	vec2 TexCoord = gl_FragCoord.xy / textureSize(s_depth, 0);
    vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
	return viewSpacePosition.xyz;
}

void main() {
	vec2 wrCoord = csThis.xy/csThis.w * .5 + .5;
	float depth = texture2D(s_depth, wrCoord ).r;
	
	vec3 dirToViewer = normalize(u_viewOrigin - worldPosition.xyz);

	// where does the fragment-viewer ray leave the light frustum?
	float minRayCoord = length(u_viewOrigin - worldPosition.xyz); 
	for(int i=0; i<6; i++) {  // https://stackoverflow.com/questions/23975555/how-to-do-ray-plane-intersection
		float top = dot(u_lightFrustum[i], worldPosition);
		float bottom = dot(u_lightFrustum[i].xyz, -dirToViewer);
		if(abs(bottom) < 1e-3) { // special case, ray parallel to plane
			continue; 
		}
		float rayCoord = top / bottom;
		if(rayCoord < 1e-1) { // negative should mean that the intersection is behind the fragment - ignored
			continue;         // at least one of these must be zero-ish (the plane being rendered now)
		}
		if(rayCoord > minRayCoord) // the intersection closest to the fragment is the exit point
			continue;
		minRayCoord = rayCoord;
	}
	vec4 exitPoint = vec4(minRayCoord * dirToViewer + worldPosition.xyz, 1);

	// get the nearest solid surface
	float solidDistance = length(ViewPosFromDepth(depth));
	// start position lies on the light frustum	
	float fragmentDistance = distance(u_viewOrigin, worldPosition.xyz);
	vec4 startPos = worldPosition;
	if(solidDistance < fragmentDistance) // frustum occluded, need to shift
		startPos = mix(vec4(u_viewOrigin, 1), worldPosition, solidDistance / fragmentDistance);
	if(distance(exitPoint.xyz, u_viewOrigin) >= solidDistance)
		return;

	// get N samples from the fragment-view ray inside the frustum
	float color = 0;
	for(float i=0; i<u_sampleCount; i++) {
		vec4 samplePos = mix(startPos, exitPoint, i/u_sampleCount);
			// shadow test
			vec4 depthSamples;
			vec2 sampleWeights;
			vec3 light2fragment = samplePos.xyz - u_lightOrigin;
			ShadowAtlasForVector(normalize(light2fragment), depthSamples, sampleWeights);
			vec3 absL = abs(light2fragment);
			float maxAbsL = max(absL.x, max(absL.y, absL.z));
			vec4 lit4 = vec4(lessThan(vec4(maxAbsL), depthSamples));
			float lit = mix(mix(lit4.w, lit4.z, sampleWeights.x),
                	mix(lit4.x, lit4.y, sampleWeights.x),
                   	sampleWeights.y);
			//lit = dot(lit4, vec4(.25));
			vec4 lightProject = samplePos * u_lightProject;
			vec4 t0 = texture2DProj(s_projection, lightProject.xyz );
			vec4 t1 = texture2D(s_falloff, vec2(lightProject.w, 0.5) );
			color += t0.r * t1.r * lit;
			//color += dot(sin(startPos),startPos)*1e-0 - lit;
			//color += distance(samplePos.xyz, u_viewOrigin)*1e-3; 
	}

	gl_FragColor.rgb = u_lightColor.rgb * vec3(color) / u_sampleCount * 3e-1;
	//gl_FragColor.b = (u_lightColor.rgb * vec3(color) / N * minRayCoord * 3e-3).b;
}

#endif