#version 450 core
out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D screenTexture;

uniform float exposure;

void main() {
	const float gamma = 2.2;
	vec3 hdrColor = texture(screenTexture, texCoords).rgb;

	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
	mapped = pow(mapped, vec3(1.0 / gamma));

	FragColor = vec4(mapped, 1.0);
}
