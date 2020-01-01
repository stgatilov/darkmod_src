#version 140

#pragma tdm_include "tdm_transform.glsl"

in vec3 var_Position;
in vec3 var_WorldLightDir;
in vec3 var_tc0;  
in vec3 var_tc6;  
in vec2 var_TexDiffuse;        
in vec2 var_TexNormal;        
in vec2 var_TexSpecular;       
in vec4 var_TexLight; 
in mat3 var_TangentBitangentNormalMatrix; 
in vec4 var_Color;        
     
out vec4 FragColor;

uniform sampler2D u_normalTexture;
uniform sampler2D u_lightFalloffTexture;         
uniform sampler2D u_lightProjectionTexture;         
uniform samplerCube	u_lightProjectionCubemap;
uniform sampler2D u_diffuseTexture;         
uniform sampler2D u_specularTexture;        
uniform sampler2D u_depthTexture;        
//uniform samplerCubeShadow u_shadowMap;
uniform samplerCube	u_shadowMap;

uniform usampler2D u_stencilTexture;        
         
uniform vec3 	u_lightOrigin;
uniform vec3 	u_lightOrigin2;
uniform vec4 	u_viewOrigin;
uniform vec4 	u_diffuseColor;
uniform vec4 	u_specularColor;

uniform bool	u_shadows;
uniform float	u_advanced;    
uniform float	u_cubic;
uniform int		u_softShadowsQuality;
uniform float	u_softShadowsRadius;
uniform int		u_shadowMipMap;
uniform vec2	u_renderResolution;
uniform mat4	u_modelMatrix;
uniform float	u_RGTC;
uniform vec3	u_hasTextureDNS;

// common local variables
vec3 lightDir	= u_lightOrigin.xyz - var_Position;
vec3 viewDir	= u_viewOrigin.xyz - var_Position;

// compute normalized light, view and half angle vectors 
vec3 L = normalize( lightDir ); 
vec3 V = normalize( viewDir ); 
vec3 H = normalize( L + V );

// compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize 
vec3 RawN, N;
float NdotH, NdotL, NdotV;

void fetchDNS() {
	if( u_hasTextureDNS[1] != 0) {
		vec4 bumpTexel = texture ( u_normalTexture, var_TexNormal.st ) * 2. - 1.;
		RawN = u_RGTC == 1. 
			? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0)))
		: normalize( bumpTexel.wyz ); 
		N = var_TangentBitangentNormalMatrix * RawN; 
	} else {
		RawN = vec3(0, 0, 1);
		N = var_TangentBitangentNormalMatrix[2];
	}
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

//returns eye Z coordinate with reversed sign (monotonically increasing with depth)
float depthToZ(float depth) {
	float clipZ = 2.0 * depth - 1.0;
	float A = u_projectionMatrix[2].z;
	float B = u_projectionMatrix[3].z;
	return B / (A + clipZ);
}

void StencilSoftShadow() {
	vec2 texSize = u_renderResolution;
	vec2 pixSize = vec2(1.0, 1.0) / texSize;
	vec2 baseTC = gl_FragCoord.xy * pixSize;

	float StTex = float(texture( u_stencilTexture, baseTC ).r);
	float stencil = clamp( 129. - StTex, 0., 1.);
	float sumWeight = 1.;

	float LightDist = min(length(lightDir), 1e3); // crutch !
	//radius of light source
	float lightRadius = u_softShadowsRadius;
	//radius of one-point penumbra at the consided point (in world coordinates)
	//note that proper formula is:  lightRadius * (LightDist - OcclDist) / OcclDist;
	float blurRadiusWorld = lightRadius * LightDist / 66.6666;  //TODO: revert?!

	//project direction to light onto surface
	vec3 normal = var_TangentBitangentNormalMatrix[2];
	vec3 alongDirW = normalize(lightDir - dot(lightDir, normal) * normal);
	//get orthogonal direction on surface
	vec3 orthoDirW = cross(normal, alongDirW);
	//multiply the two axes by penumbra radius
	alongDirW *= blurRadiusWorld / max(NdotL, 0.2);  //penumbra is longer by (1/cos(a)) in light direction
	orthoDirW *= blurRadiusWorld;

	//convert both vectors into clip space (get only X and Y components)
	mat4 modelViewProjectionMatrix = u_projectionMatrix*u_modelViewMatrix;
	vec2 alongDir = (mat3(modelViewProjectionMatrix) * alongDirW).xy;
	vec2 orthoDir = (mat3(modelViewProjectionMatrix) * orthoDirW).xy;
	//now also get W component from multiplication by gl_ModelViewProjectionMatrix
	vec3 mvpRow3 = vec3(modelViewProjectionMatrix[0][3], modelViewProjectionMatrix[1][3], modelViewProjectionMatrix[2][3]);
	float along_w = dot(mvpRow3, alongDirW);
	float ortho_w = dot(mvpRow3, orthoDirW);
	//this is perspective correction: it is necessary because W component in clip space also varies
	//if you remove it and look horizontally parallel to a wall, then vertical shadow boundaries on this wall won't be blurred
	vec2 thisNdc = (2 * baseTC - vec2(1));
	alongDir -= thisNdc * along_w;
	orthoDir -= thisNdc * ortho_w;
	//divide by clip W to get NDC coords (screen coords are half of them)
	alongDir *= gl_FragCoord.w / 2;
	orthoDir *= gl_FragCoord.w / 2;
	//Note: if you want to check the math just above, consider how screen position changes when a point moves in specified direction:
	//  F(t) = divideByW(gl_ModelViewProjectionMatrix * (var_Position + dir_world * t)).xy
	//the converted vector must be equal to the derivative by parameter:
	//  dir_screen = dF/dt (0)
	//(here [dir_world, dir_screen] are either [alongDirW, alongDir] or [orthoDirW, orthoDir])

	//estimate the length of spot ellipse vectors (in pixels)
	float lenX = length(alongDir * texSize);
	float lenY = length(orthoDir * texSize);
	//make sure vectors are sufficiently sampled
	float avgSampleDistInPixels = 2 * max(1e-3 * texSize.y, 1.0);
	float oversize = max(lenX, lenY) / (avgSampleDistInPixels * sqrt(0.0 + u_softShadowsQuality));
	if (oversize > 1) {
		alongDir /= oversize;
		orthoDir /= oversize;
	}

	//compute partial derivatives of eye -Z by screen X and Y (normalized)
	float Z00 = depthToZ(gl_FragCoord.z);
	vec2 dzdxy = vec2(dFdx(Z00), dFdy(Z00));
	//rescale to derivatives by texture coordinates (not pixels)
	dzdxy *= texSize;
	//compute Z derivatives on a theoretical wall visible under 45-degree angle
	vec2 tanFovHalf = vec2(1.0 / u_projectionMatrix[0][0], 1.0 / u_projectionMatrix[1][1]);
	vec2 canonDerivs = 2.0 * Z00 * tanFovHalf;

	for( int i = 0; i < u_softShadowsQuality; i++ ) {
		vec2 delta = u_softShadowsSamples[i].x * alongDir + u_softShadowsSamples[i].y * orthoDir;
		vec2 StTc = baseTC + delta;
		float Zdiff = depthToZ(texture(u_depthTexture, StTc).r) - Z00;
		float tangentZdiff = dot(dzdxy, delta);
		float deg45diff = dot(canonDerivs, abs(delta));
		float weight = float(abs(Zdiff - tangentZdiff) <= abs(tangentZdiff) * 0.5 + deg45diff * 0.2);
		float StTex = float(texture( u_stencilTexture, StTc ).r);
		stencil += clamp( 129. - StTex, 0., 1. ) * weight;
		sumWeight += weight;
	}
	FragColor.rgb *= stencil / sumWeight;

	/*vec2 StTc = baseTC + vec2(1, 0) * 1e-2;
	StTex = texture( u_stencilTexture, StTc ).r;
	stencil = .25*(128. - StTex);
	FragColor.rgb = vec3(stencil, -stencil, stencil==0?.3:0);
	*/
}

void main() {
	fetchDNS();

	if (u_advanced == 1.0)
		FragColor.rgb = advancedInteraction();
	else
		FragColor.rgb = simpleInteraction();
	                           
	if (u_shadows && u_softShadowsQuality > 0) 
		StencilSoftShadow();
    FragColor.a = 1.0;              
}