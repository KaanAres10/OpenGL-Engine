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
    mat4 modelView = view * model;

    vec3 T = normalize(mat3(modelView) * aTangent);
    vec3 N = normalize(mat3(transpose(inverse(modelView))) * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    gl_Position = projection * modelView * vec4(aPos, 1.0);
    vs_out.fragPos = (modelView * vec4(aPos, 1.0)).xyz;
    vs_out.normal = N;
    vs_out.TBN = mat3(T, B, N);
    vs_out.texCoords = aTexCoords;
}
