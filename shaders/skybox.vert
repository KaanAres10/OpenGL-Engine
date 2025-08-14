#version 450 core
layout(location = 0) in vec3 aPos;

layout(std140, binding = 0) uniform Matrices {
    mat4 projection;
    mat4 view;
};

out vec3 localPos;


void main() {
    localPos = aPos;    
    mat4 viewNt =  mat4(mat3(view));
    vec4 clipPos = projection * viewNt * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
}