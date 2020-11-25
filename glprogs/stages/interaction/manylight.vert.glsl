#version 330 core

in vec4 attr_Position;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Normal;
in vec3 attr_Bitangent;
in vec4 attr_Color;
in int attr_DrawId;

#pragma tdm_include "stages/interaction/manylight.params.glsl"

out vec3 var_WorldPosition;
out vec4 var_ClipPosition;
out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_Color;
out mat3 var_TangentBitangentNormalMatrix;
flat out int var_DrawId;

void sendTBN() {
    // construct tangent-bitangent-normal 3x3 matrix in world space
    vec3 worldTangent = normalize((params[attr_DrawId].modelMatrix * vec4(attr_Tangent, 0)).xyz);
    vec3 worldNormal = normalize((params[attr_DrawId].modelMatrix * vec4(attr_Normal, 0)).xyz);
    vec3 worldBitangent = normalize((params[attr_DrawId].modelMatrix * vec4(attr_Bitangent, 0)).xyz);
    //vec3 worldBitangent = cross(worldTangent, worldNormal);
    var_TangentBitangentNormalMatrix = mat3(worldTangent, worldBitangent, worldNormal);
}

void main() {
	// transform vertex position into homogenous clip-space
	var_ClipPosition = u_projectionMatrix * (params[attr_DrawId].modelViewMatrix * attr_Position);
    gl_Position = var_ClipPosition;

	var_WorldPosition = (params[attr_DrawId].modelMatrix * attr_Position).xyz;

	// normal map texgen
	var_TexNormal.x = dot(attr_TexCoord, params[attr_DrawId].bumpMatrix[0]);
	var_TexNormal.y = dot(attr_TexCoord, params[attr_DrawId].bumpMatrix[1]);

	// diffuse map texgen
	var_TexDiffuse.x = dot(attr_TexCoord, params[attr_DrawId].diffuseMatrix[0]);
	var_TexDiffuse.y = dot(attr_TexCoord, params[attr_DrawId].diffuseMatrix[1]);

	// specular map texgen
	var_TexSpecular.x = dot(attr_TexCoord, params[attr_DrawId].specularMatrix[0]);
	var_TexSpecular.y = dot(attr_TexCoord, params[attr_DrawId].specularMatrix[1]);

	// construct tangent-bitangent-normal 3x3 matrix
	sendTBN();

	// primary color
	var_Color = (attr_Color * params[attr_DrawId].colorModulate) + params[attr_DrawId].colorAdd;

	var_DrawId = attr_DrawId;
}
