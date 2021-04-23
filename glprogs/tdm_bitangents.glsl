#ifdef VERTEX_SHADER

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

in mat3 var_TangentBitangentNormalMatrix; 
in vec3 var_LightDirLocal;  
in vec3 var_ViewDirLocal;

void calcNormals() {
    // compute normal from normal map, move from [0, 1] to [-1, 1] range, normalize 
	if (u_hasTextureDNS[1] != 0) {
/*		vec4 bumpTexel = texture (u_normalTexture, var_TexNormal.st) * 2. - 1.;
		RawN = u_RGTC == 1.0
			? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1. - bumpTexel.x*bumpTexel.x - bumpTexel.y*bumpTexel.y, 0)))
			: normalize(bumpTexel.wyz);
		N = var_TangentBitangentNormalMatrix * RawN;*/
		vec4 bumpTexel = texture ( u_normalTexture, var_TexNormal.st ) * 2. - 1.;
    	RawN = u_RGTC == 1. 
	    	? vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.-bumpTexel.x*bumpTexel.x-bumpTexel.y*bumpTexel.y, 0)))
		    : normalize( bumpTexel.wyz ); 
    	N = var_TangentBitangentNormalMatrix * RawN; 
	}
	else {
		RawN = vec3(0, 0, 1);
		N = var_TangentBitangentNormalMatrix[2];
	}
}

#endif