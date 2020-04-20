#version 140

/**
 * This is a 9-tap Gaussian blur accelerated by making use of the GPU filtering hardware
 * See: http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
 */

out vec4 FragColor;

uniform sampler2D u_source;
uniform int u_mipLevel;

const float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
const float weight[3] = float[](0.227027027, 0.3162162162, 0.0702702703);

// (1, 0) or (0, 1)
uniform vec2 u_axis;

vec2 invImageSize = vec2(1) / vec2(textureSize(u_source, u_mipLevel));


void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    FragColor = textureLod(u_source, fragCoord * invImageSize, u_mipLevel) * weight[0];
    
    for (int i = 1; i < 3; ++i) {
        FragColor += textureLod(u_source, (fragCoord + u_axis * offset[i]) * invImageSize, u_mipLevel) * weight[i];
        FragColor += textureLod(u_source, (fragCoord - u_axis * offset[i]) * invImageSize, u_mipLevel) * weight[i];
    }
}
