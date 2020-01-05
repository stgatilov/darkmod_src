#pragma tdm_include "tdm_transform.glsl"

// Contains common formulas for computing interaction.
// Includes: illumination model, fetching surface and light properties
// Excludes: shadows


in vec3 var_Position;
in vec4 var_Color;
in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;

uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;
uniform sampler2D u_lightFalloffTexture;
uniform sampler2D u_lightProjectionTexture;
uniform samplerCube	u_lightProjectionCubemap;

uniform float	u_advanced;
uniform float	u_cubic;
uniform float	u_RGTC;
uniform vec3	u_hasTextureDNS;
uniform vec3 	u_lightOrigin;
uniform vec4 	u_viewOrigin;
uniform vec4 	u_diffuseColor;
uniform vec4 	u_specularColor;
uniform int		u_testSpecularFix;	//stgatilov #5044: for testing only!
uniform int		u_testBumpmapLightTogglingFix;  //stgatilov #4825: for testing only


// output of fetchDNS
vec3 RawN, N;

// common variables
vec3 lightDir, viewDir;     //direction to light/eye in model coords
vec3 L, V, H;               //normalized light, view and half angle vectors 
float NdotH, NdotL, NdotV;

#pragma tdm_include "tdm_bitangents.glsl"

//fetch surface normal at fragment
void fetchDNS() {
/*	if (u_hasTextureDNS[1] != 0) {
		vec4 bumpTexel = texture (u_normalTexture, var_TexNormal.st) * 2. - 1.;
		RawN = u_RGTC == 1.0
			? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1. - bumpTexel.x*bumpTexel.x - bumpTexel.y*bumpTexel.y, 0)))
			: normalize(bumpTexel.wyz);
		N = var_TangentBitangentNormalMatrix * RawN;
	}
	else {
		RawN = vec3(0, 0, 1);
		N = var_TangentBitangentNormalMatrix[2];
	}
*/
	//initialize common variables (TODO: move somewhere else?)
	lightDir = u_lightOrigin.xyz - var_Position;
	viewDir = u_viewOrigin.xyz - var_Position;
	L = normalize(lightDir);
	V = normalize(viewDir);
	H = normalize(L + V);
	calcNormals();
	NdotH = clamp(dot(N, H), 0.0, 1.0);
	NdotL = clamp(dot(N, L), 0.0, 1.0);
	NdotV = clamp(dot(N, V), 0.0, 1.0);
}

//fetch color of the light source
vec3 lightColor() {
	// compute light projection and falloff 
	vec3 lightColor;
	if (u_cubic == 1.0) {
		vec3 cubeTC = var_TexLight.xyz * 2.0 - 1.0;
		lightColor = texture(u_lightProjectionCubemap, cubeTC).rgb;
		float att = clamp(1.0 - length(cubeTC), 0.0, 1.0);
		lightColor *= att * att;
	}
	else {
		vec3 lightProjection = textureProj(u_lightProjectionTexture, var_TexLight.xyw).rgb;
		vec3 lightFalloff = texture(u_lightFalloffTexture, vec2(var_TexLight.z, 0.5)).rgb;
		lightColor = lightProjection * lightFalloff;
	}
	return lightColor;
}

//illumination model with "simple interaction" setting
vec3 simpleInteraction() {
	// compute the diffuse term    
	vec3 diffuse = texture(u_diffuseTexture, var_TexDiffuse).rgb * u_diffuseColor.rgb;

	// compute the specular term
	float specularPower = 10.0;
	float specularContribution = pow(NdotH, specularPower);
	vec3 specular = texture(u_specularTexture, var_TexSpecular).rgb * specularContribution * u_specularColor.rgb;

	// compute lighting model
	vec3 finalColor = (diffuse + specular) * NdotL * lightColor() * var_Color.rgb;

	return finalColor;
}

//illumination model with "enhanced interaction" setting
vec3 advancedInteraction() {
	vec4 fresnelParms = vec4(1.0, .23, .5, 1.0);
	vec4 fresnelParms2 = vec4(.2, .023, 120.0, 4.0);
	vec4 lightParms = vec4(.7, 1.8, 10.0, 30.0);

	vec3 diffuse = texture(u_diffuseTexture, var_TexDiffuse).rgb;

	vec3 specular = vec3(0.026);	//default value if texture not set?...
	if (dot(u_specularColor, u_specularColor) > 0.0)
		specular = texture(u_specularTexture, var_TexSpecular).rgb;

	vec3 localL = normalize(var_LightDirLocal);
	vec3 localV = normalize(var_ViewDirLocal);
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

	float NdotL_adjusted = NdotL;
	if (u_testBumpmapLightTogglingFix != 0) {
		//stgatilov: hacky coefficient to make lighting smooth when L is almost in surface tangent plane
		vec3 meshNormal = normalize(var_TangentBitangentNormalMatrix[2]);
		float MNdotL = dot(meshNormal, L);
		if (MNdotL < min(0.25, NdotL))
			NdotL_adjusted = mix(MNdotL, NdotL, MNdotL / 0.25);
	}
	float light = rimLight * R2f + NdotL_adjusted;

	vec3 totalColor;
	if (u_testSpecularFix != 0)
		totalColor = (specularColor * u_specularColor.rgb * R2f + diffuse * u_diffuseColor.rgb) * light * lightColor() * var_Color.rgb;
	else
		totalColor = (specularColor * R2f + diffuse) * light * u_diffuseColor.rgb * lightColor() * var_Color.rgb;

	return totalColor;
}

vec3 computeInteraction() {
	vec3 res;
	if (u_advanced == 1.0)
		res = advancedInteraction();
	else
		res = simpleInteraction();
	return res;
}
