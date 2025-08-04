#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;


layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
    vec4 fragPosLightSpace;
} vs_out;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vs_out.fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.texCoords = aTexCoords;
    vs_out.fragPosLightSpace = lightSpaceMatrix * vec4(vs_out.fragPos, 1.0);
}
