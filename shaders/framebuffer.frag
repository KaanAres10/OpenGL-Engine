#version 450 core
out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomTexture;

uniform float block_size;
uniform float exposure;

void main() {
    float gamma = 2.2;
	
    ivec2 texSize = textureSize(screenTexture, 0);
    vec2 resolution = vec2(texSize);

    // Snap UV to block
    vec2 uv_px = texCoords * resolution;
    vec2 block_uv = floor(uv_px / block_size) * block_size;
    vec2 uv_snapped = block_uv / resolution;

    vec3 hdrColor = texture(screenTexture, uv_snapped).rgb;
    vec3 bloomColor = texture(bloomTexture, uv_snapped).rgb;
    hdrColor += bloomColor;

    // Tone Mapping
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
	mapped = pow(mapped, vec3(1.0 / gamma));


    FragColor = vec4(mapped, 1.0);
}
