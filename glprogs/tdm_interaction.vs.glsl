#pragma tdm_include "tdm_transform.glsl"

// Contains common formulas for computing interaction.
// Includes: illumination model, fetching surface and light properties
// Excludes: shadows



INATTR_POSITION  //in vec4 attr_Position;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec4 attr_Color;

uniform mat4 u_lightProjectionFalloff;
uniform vec4[2]	u_bumpMatrix;
uniform vec4[2]	u_diffuseMatrix;
uniform vec4[2]	u_specularMatrix;
uniform vec4 u_colorModulate;
uniform vec4 u_colorAdd;
uniform vec3 u_lightOrigin;
uniform vec4 u_viewOrigin;
uniform mat4 u_modelMatrix;
uniform vec3 u_lightOrigin2;

out vec3 var_Position;
out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_TexLight;
out vec4 var_Color;

#pragma tdm_include "tdm_bitangents.glsl"



void interactionProcessVertex() {
	// transform vertex position into homogenous clip-space
	gl_Position = tdm_transform(attr_Position);

	// transform vertex position into world space
	var_Position = attr_Position.xyz;

	// normal map texgen
	var_TexNormal.x = dot(attr_TexCoord, u_bumpMatrix[0]);
	var_TexNormal.y = dot(attr_TexCoord, u_bumpMatrix[1]);

	// diffuse map texgen
	var_TexDiffuse.x = dot(attr_TexCoord, u_diffuseMatrix[0]);
	var_TexDiffuse.y = dot(attr_TexCoord, u_diffuseMatrix[1]);

	// specular map texgen
	var_TexSpecular.x = dot(attr_TexCoord, u_specularMatrix[0]);
	var_TexSpecular.y = dot(attr_TexCoord, u_specularMatrix[1]);

	// light projection texgen
	var_TexLight = ( attr_Position * u_lightProjectionFalloff ).xywz;

	// construct tangent-bitangent-normal 3x3 matrix
	sendTBN();

	// primary color
	var_Color = (attr_Color * u_colorModulate) + u_colorAdd;
}
