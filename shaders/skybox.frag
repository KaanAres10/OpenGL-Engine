#version 450 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap;


void main() {
    vec3 color = texture(environmentMap, localPos).rgb;
    FragColor = vec4(color, 1.0);
}
