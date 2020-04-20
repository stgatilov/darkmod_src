#version 140

in vec4 var_TexCoord;
out vec4 draw_Color;
uniform sampler2D u_texture;
uniform sampler2D u_bloomTex;

uniform float u_gamma, u_brightness;
uniform float u_desaturation;
uniform float u_colorCurveBias;
uniform float u_colorCorrection, u_colorCorrectBias;
uniform float u_bloomWeight;

float mapColorComponent(float value) {
	float color = value;

	//stgatilov: apply traditional gamma/brightness settings
	color = pow(color, 1.0/u_gamma);
	color *= u_brightness;

	if (u_colorCurveBias != 0.0) {
		//---------------------------------------------------------
		//  Apply Smooth Exponential color falloff
		//---------------------------------------------------------
		float reduced1 = 1.0 - pow(2.718282, -3.0 * color * color);
		color = mix(color, reduced1, u_colorCurveBias);
	}
	if (u_colorCorrectBias != 0.0) {
		//---------------------------------------------------------
		//  Apply Smooth Exponential color correction with a bias
		//---------------------------------------------------------
		float reduced2 = 1.0 - pow(2.718282, -u_colorCorrection * color);
		color = mix(color, reduced2, u_colorCorrectBias);
	}
	return color;
}

void main() {
	vec4 color = texture(u_texture, var_TexCoord.xy);
	if (u_bloomWeight > 0) {
		vec4 bloom = texture(u_bloomTex, var_TexCoord.xy);
		color = color + u_bloomWeight * bloom;
	}

	color.r = mapColorComponent(color.r);
	color.g = mapColorComponent(color.g);
	color.b = mapColorComponent(color.b);

	float luma = clamp(dot(vec3(0.2125, 0.7154, 0.0721), color.rgb), 0.0, 1.0);
	color.rgb = mix(color.rgb, vec3(luma), u_desaturation);

	draw_Color = color;
}
