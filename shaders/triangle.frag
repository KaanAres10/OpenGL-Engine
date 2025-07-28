#version 450 core
in vec3 vColor;
in vec3 pos;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
