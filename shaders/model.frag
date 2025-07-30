#version 450 core
out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D texture_diffuse1;

void main() {
    vec4 color = texture(texture_diffuse1, texCoords);
    if (color.a < 0.5) {
        discard;
    }

    FragColor = color;
}
