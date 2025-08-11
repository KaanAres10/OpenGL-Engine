#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
    mat3 TBN;
} vs_out;

uniform mat4 model;

void main() {
    vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vs_out.fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.TBN = mat3(T, B, N);
    vs_out.texCoords = aTexCoords;
}
