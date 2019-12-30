#version 140
#extension GL_ARB_texture_gather: enable

#define STGATILOV_OCCLUDER_SEARCH 1
#define STGATILOV_USEGATHER 1

in vec3 var_Position;
in vec3 var_WorldLightDir;
in vec2 var_TexDiffuse;        
in vec2 var_TexNormal;        
in vec2 var_TexSpecular;       
in vec4 var_TexLight; 
in vec4 var_Color;        

out vec4 fragColor;        

uniform sampler2D u_normalTexture;
uniform sampler2D u_lightFalloffTexture;         
uniform sampler2D u_lightProjectionTexture;         
uniform samplerCube	u_lightProjectionCubemap;
uniform sampler2D u_diffuseTexture;         
uniform sampler2D u_specularTexture;        
uniform sampler2D u_depthTexture;        
uniform usampler2D u_stencilTexture;        
uniform sampler2D u_shadowMap;
         
uniform vec3 	u_lightOrigin;
uniform vec3 	u_lightOrigin2;
uniform vec4 	u_viewOrigin;
uniform vec4 	u_diffuseColor;
uniform vec4 	u_specularColor;

uniform bool 	u_shadows;
uniform bool 	u_shadowMapCullFront; 
uniform float	u_advanced;    
uniform float	u_cubic;
uniform int		u_softShadowsQuality;
uniform float	u_softShadowsRadius;
uniform vec4	u_shadowRect;
uniform mat4	u_modelMatrix;
uniform float	u_RGTC;

// common local variables
vec3 RawN, N, lightDir, viewDir, L, V, H;

#pragma tdm_include "tdm_bitangents.glsl"

float NdotH, NdotL, NdotV;

void InitGlobals() {
    lightDir	= u_lightOrigin.xyz - var_Position;
    viewDir	= u_viewOrigin.xyz - var_Position;

    // compute normalized light, view and half angle vectors 
    L = normalize( lightDir ); 
    V = normalize( viewDir ); 
    H = normalize( L + V );
	
	calcNormals(); // tdm_bitangents_fragment.glsl

    NdotH = clamp( dot( N, H ), 0.0, 1.0 );
    NdotL = clamp( dot( N, L ), 0.0, 1.0 );
    NdotV = clamp( dot( N, V ), 0.0, 1.0 );
}

vec3 lightColor() {
	// compute light projection and falloff 
	vec3 lightColor;
	if (u_cubic == 1.0) {
		vec3 cubeTC = var_TexLight.xyz * 2.0 - 1.0;
		lightColor = texture(u_lightProjectionCubemap, cubeTC).rgb;
		float att = clamp(1.0-length(cubeTC), 0.0, 1.0);
		lightColor *= att*att;
	} else {
		vec3 lightProjection = textureProj( u_lightProjectionTexture, var_TexLight.xyw ).rgb; 
		vec3 lightFalloff = texture( u_lightFalloffTexture, vec2( var_TexLight.z, 0.5 ) ).rgb;
		lightColor = lightProjection * lightFalloff;
	}
	return lightColor;
} 

vec3 simpleInteraction() {
	// compute the diffuse term    
	vec3 diffuse = texture( u_diffuseTexture, var_TexDiffuse ).rgb * u_diffuseColor.rgb;

	// compute the specular term
    float specularPower = 10.0;
    float specularContribution = pow( NdotH, specularPower );
	vec3 specular = texture( u_specularTexture, var_TexSpecular ).rgb * specularContribution * u_specularColor.rgb;

	// compute lighting model
    vec3 finalColor = ( diffuse + specular ) * NdotL * lightColor() * var_Color.rgb;
 
	return finalColor;
} 

vec3 advancedInteraction() {
	vec4 fresnelParms		= vec4( 1.0, .23, .5, 1.0  );			
	vec4 fresnelParms2		= vec4( .2, .023, 120.0, 4.0 );
	vec4 lightParms			= vec4( .7, 1.8, 10.0, 30.0 );

	vec3 diffuse = texture( u_diffuseTexture, var_TexDiffuse ).rgb;

	vec3 specular = vec3(0.026);	//default value if texture not set?...
	if (dot(u_specularColor, u_specularColor) > 0.0)
		specular = texture( u_specularTexture, var_TexSpecular ).rgb;

	vec3 localL = normalize(var_tc0);
	vec3 localV = normalize(var_tc6);
	//must be done in tangent space, otherwise smoothing will suffer (see #4958)
	float NdotL = clamp(dot(RawN, localL), 0.0, 1.0);
	float NdotV = clamp(dot(RawN, localV), 0.0, 1.0);
	float NdotH = clamp(dot(RawN, normalize(localV + localL)), 0.0, 1.0);

	// fresnel part, ported from test_direct.vfp
	float fresnelTerm = pow(1.0 - NdotV, fresnelParms2.w);
	float rimLight = fresnelTerm * clamp(NdotL - 0.3, 0.0, fresnelParms.z) * lightParms.y;
	float specularPower = mix(lightParms.z, lightParms.w, specular.z);
	float specularCoeff = pow(NdotH, specularPower) * fresnelParms2.z;
	float fresnelCoeff = fresnelTerm * fresnelParms.y + fresnelParms2.y;

	vec3 specularColor = specularCoeff * fresnelCoeff * specular * (diffuse * 0.25 + vec3(0.75));
	float R2f = clamp(localL.z * 4.0, 0.0, 1.0);
	float light = rimLight * R2f + NdotL;
	vec3 totalColor = (specularColor * R2f + diffuse) * light * u_diffuseColor.rgb * lightColor() * var_Color.rgb;

	return totalColor;
}

uniform vec2 u_softShadowsSamples[150];  //TODO: what cap is appropriate here?

vec3 CubeMapDirectionToUv(vec3 v, out int faceIdx) {
	vec3 v1 = abs(v);
	float maxV = max(v1.x, max(v1.y, v1.z));
	faceIdx = 0;
	if(maxV == v.x) {
		v1 = -v.zyx;
	} 
	else if(maxV == -v.x) {
		v1 = v.zyx * vec3(1, -1, 1);
		faceIdx = 1;
	}
	else if(maxV == v.y) {
		v1 = v.xzy * vec3(1, 1, -1);
		faceIdx = 2;
	}
	else if(maxV == -v.y) {
		v1 = v.xzy * vec3(1, -1, 1);
		faceIdx = 3;
	}
	else if(maxV == v.z) {
		v1 = v.xyz * vec3(1, -1, -1);
		faceIdx = 4;
	}
	else { //if(maxV == -v.z) {
		v1 = v.xyz * vec3(-1, -1, 1);
		faceIdx = 5;
	}
	v1.xy /= -v1.z;
	return v1;
}
float ShadowAtlasForVector(vec3 v) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(v, faceIdx);
	vec2 texSize = textureSize(u_shadowMap, 0);
	vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * u_shadowRect.ww + u_shadowRect.xy;
	shadow2d.x += (u_shadowRect.w + 1./texSize.x) * faceIdx;
	float d = textureLod(u_shadowMap, shadow2d, 0).r;
	return u_softShadowsRadius / (1 - d);
}
vec4 ShadowAtlasForVector4(vec3 v, out vec4 sampleWeights) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(v, faceIdx);
	vec2 texSize = textureSize(u_shadowMap, 0);
	vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * u_shadowRect.ww + u_shadowRect.xy;
	shadow2d.x += (u_shadowRect.w + 1./texSize.x) * faceIdx;
	vec4 d = textureGather(u_shadowMap, shadow2d);
	vec2 wgt = fract(shadow2d * texSize - 0.5);
	vec2 mwgt = vec2(1) - wgt;
	sampleWeights = vec4(mwgt.x, wgt.x, wgt.x, mwgt.x) * vec4(wgt.y, wgt.y, mwgt.y, mwgt.y);
	return vec4(u_softShadowsRadius) / (vec4(1) - d);
}

void UseShadowMap() {
	float shadowMapResolution = (textureSize(u_shadowMap, 0).x * u_shadowRect.w);

	//get unit direction from light to current fragment
	vec3 L = normalize(var_WorldLightDir);
	//find maximum absolute coordinate in light vector
	vec3 absL = abs(var_WorldLightDir);
	float maxAbsL = max(absL.x, max(absL.y, absL.z));

	//get interpolated normal / normal of triangle (using different normal affects near-tangent lighting greatly)
	//vec3 normal = normalize(cross(dFdx(var_WorldLightDir), dFdy(var_WorldLightDir)));
	vec3 normal = mat3(u_modelMatrix) * N;//var_TangentBitangentNormalMatrix[2];
	float lightFallAngle = -dot(normal, L);

	//note: choosing normal and how to cap angles is the hardest question for now
	//this has large effect on near-tangent surfaces (mostly the curved ones)
	fragColor.rgb *= smoothstep(0.01, 0.05, lightFallAngle);
	//some very generic error estimation...
	float errorMargin = 5.0 * maxAbsL / ( shadowMapResolution * max(lightFallAngle, 0.1) );
	if(u_shadowMapCullFront)
       errorMargin = -errorMargin;

	//process central shadow sample
	float centerFragZ = maxAbsL;
#if STGATILOV_USEGATHER
	vec4 wgt;
	vec4 centerBlockerZ = ShadowAtlasForVector4(L, wgt);
	float lit = dot(wgt, step(centerFragZ - errorMargin, centerBlockerZ));
#else
	float centerBlockerZ = ShadowAtlasForVector(L);
	float lit = float(centerBlockerZ >= centerFragZ - errorMargin);
#endif

	if (u_softShadowsQuality == 0) {
		fragColor.rgb *= lit;
		return;
	}
	float lightDist = length(var_WorldLightDir);
	//this is (1 / cos(phi)), where phi is angle between light direction and normal of the pierced cube face
	float secFallAngle = lightDist / maxAbsL;
	//find two unit directions orthogonal to light direction
	vec3 nonCollinear = vec3(1, 0, 0);
	if (absL.x == maxAbsL)
		nonCollinear = vec3(0, 1, 0);
	vec3 orthoAxisX = normalize(cross(L, nonCollinear));
	vec3 orthoAxisY = cross(L, orthoAxisX);

	#if STGATILOV_OCCLUDER_SEARCH
	//search for blockers in a cone with rather large angle
	float searchAngle = 0.03 * u_softShadowsRadius;    //TODO: this option is probably very important
	float avgBlockerZ = 0;
	int blockerCnt = 0;
	for (int i = 0; i < u_softShadowsQuality; i++) {
		//note: copy/paste from sampling code below
		vec3 perturbedLightDir = normalize(L + searchAngle * (u_softShadowsSamples[i].x * orthoAxisX + u_softShadowsSamples[i].y * orthoAxisY));
		float blockerZ = ShadowAtlasForVector(perturbedLightDir);
		float dotDpL = max(max(abs(perturbedLightDir.x), abs(perturbedLightDir.y)), abs(perturbedLightDir.z));
		float distCoeff = lightFallAngle / max(-dot(normal, perturbedLightDir), 1e-3) * (dotDpL * secFallAngle);
		float fragZ = centerFragZ * distCoeff;
		//note: only things which may potentially occlude are averaged
		if (blockerZ < fragZ - errorMargin) {
			avgBlockerZ += blockerZ;
			blockerCnt++;
		}
	}
	//shortcut: no blockers in search angle => fully lit
	if (blockerCnt == 0)
		return;
	/* Bad optimization!
	 * Go to St. Alban's Collateral and execute:
	 *   setviewpos  -114.57 1021.61 130.95   -1.0 147.8 0.0
	 * and you'll notice artefacts if you enable this piece of code.
	//shortcut: all blockers in search angle => fully occluded
	if (blockerCnt == u_softShadowsQuality && lit == 0) {
		fragColor.rgb *= 0.0f;
		return;
	}
	*/
	avgBlockerZ /= blockerCnt;  
	#endif

	//radius of light source
	float lightRadius = u_softShadowsRadius;
	//radius of one-point penumbra at the considered point (in world coordinates)
	//note that proper formula is:  lightRadius * (lightDist - occlDist) / occlDist;
	float blurRadiusWorld = lightRadius * lightDist / 200.0;
	#if STGATILOV_OCCLUDER_SEARCH
	blurRadiusWorld = lightRadius * (centerFragZ - avgBlockerZ) / avgBlockerZ;
	#endif
	//blur radius relative to light distance
	float blurRadiusRel = blurRadiusWorld / lightDist;
	#if STGATILOV_OCCLUDER_SEARCH
	//note: it is very important to limit blur angle <= search angle !
	blurRadiusRel = min(blurRadiusRel, searchAngle);
	#endif
	//minor radius of the ellipse, which is: the intersection of penumbra cone with cube's face
	float blurRadiusCube = blurRadiusRel * secFallAngle;
	//the same radius in shadowmap pixels
	float blurRadiusPixels = blurRadiusCube * shadowMapResolution;

	//limit blur radius from below: blur must cover at least (2*M) pixels in any direction
	//otherwise user will see the shitty pixelated shadows, which is VERY ugly
	//we prefer blurring shadows to hell instead of showing pixelation...
	const float minBlurInShadowPixels = 5.0;
	if (blurRadiusPixels < minBlurInShadowPixels) {
		float coeff = minBlurInShadowPixels / (blurRadiusPixels + 1e-7);
		//note: other versions of blurRadius are not used below
		blurRadiusRel *= coeff;
	}
	for (int i = 0; i < u_softShadowsQuality; i++) {
		//unit vector L' = perturbed version of L
		vec3 perturbedLightDir = normalize(L + blurRadiusRel * (u_softShadowsSamples[i].x * orthoAxisX + u_softShadowsSamples[i].y * orthoAxisY));
		//this is:  (cos(D ^ L') / cos(N ^ L')) / (cos(D ^ L) / cos(N ^ L))
		//where L and L' is central/perturbed light direction, D is normal of active cubemap face, and N is normal of tangent plane
		float dotDpL = max(max(abs(perturbedLightDir.x), abs(perturbedLightDir.y)), abs(perturbedLightDir.z));
		float distCoeff = lightFallAngle / max(-dot(normal, perturbedLightDir), 1e-3) * (dotDpL * secFallAngle);
		float fragZ = centerFragZ * distCoeff;
		float blockerZ = ShadowAtlasForVector(perturbedLightDir);
		lit += float(blockerZ >= fragZ - errorMargin);
	}
	lit /= u_softShadowsQuality + 1;
	fragColor.rgb *= lit;
}

void main() {
    InitGlobals();
	
	if (u_advanced == 1.0)
		fragColor.rgb = advancedInteraction();
	else
		fragColor.rgb = simpleInteraction();
	                           
	if (u_shadows)
		UseShadowMap();
		
    fragColor.a = 1.0;              
}