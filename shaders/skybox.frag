#version 450 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap;

uniform float lod;

void main() {
    vec3 color = textureLod(environmentMap, localPos, lod).rgb;
    FragColor = vec4(color, 1.0);
}
