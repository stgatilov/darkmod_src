#pragma tdm_define "LEGACY_BITANGENTS"

#ifdef VERTEX_SHADER

#ifdef LEGACY_BITANGENTS

out mat3 var_TangentBitangentNormalMatrix; 
out vec3 var_LightDirLocal;  
out vec3 var_ViewDirLocal;  

void sendTBN() {
	// construct tangent-bitangent-normal 3x3 matrix   
	var_TangentBitangentNormalMatrix = mat3( clamp(attr_Tangent,-1,1), clamp(attr_Bitangent,-1,1), clamp(attr_Normal,-1,1) );
	var_LightDirLocal = (u_lightOrigin.xyz - var_Position).xyz * var_TangentBitangentNormalMatrix;
	var_ViewDirLocal = (u_viewOrigin.xyz - var_Position).xyz * var_TangentBitangentNormalMatrix;
}

#else

out vec3 var_Normal;  

void sendTBN() {
    var_Normal = attr_Normal;
}

#endif

#else

#ifdef LEGACY_BITANGENTS

in mat3 var_TangentBitangentNormalMatrix; 
in vec3 var_LightDirLocal;  
in vec3 var_ViewDirLocal;

void calcNormals() {
    // compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize 
	vec4 bumpTexel = texture ( u_normalTexture, var_TexNormal.st ) * 2. - 1.;
    RawN = u_RGTC == 1. 
	    ? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0)))
	    : normalize( bumpTexel.wyz ); 
    N = var_TangentBitangentNormalMatrix * RawN; 
}

#else

in vec3 var_Normal;  
vec3 var_LightDirLocal;  
vec3 var_ViewDirLocal;  

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv ) {
	/* get edge vectors of the pixel triangle */
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	/* solve the linear system */
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	/* construct a scale-invariant frame */
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord ) {
	/* assume N, the interpolated vertex normal and V, the view vector (vertex to eye) */
	RawN = texture( u_normalTexture, texcoord ).wyz;
	// WITH_NORMALMAP_UNSIGNED
	RawN = RawN * 2 - 1;
	// WITH_NORMALMAP_2CHANNEL
	// map.z = sqrt( 1. - dot( map.xy, map.xy ) );
	// WITH_NORMALMAP_GREEN_UP
	// map.y = -map.y;
	mat3 TBN = cotangent_frame( N, -V, texcoord );
	if (u_advanced == 1.0) {
		var_LightDirLocal = (u_lightOrigin.xyz - var_Position).xyz * TBN;
		var_ViewDirLocal = (u_viewOrigin.xyz - var_Position).xyz * TBN;	
	}
	return normalize( TBN * RawN );
}

void calcNormals() {
	N = perturb_normal( normalize( var_Normal ), V, var_TexNormal.st );
}

#endif

#endif