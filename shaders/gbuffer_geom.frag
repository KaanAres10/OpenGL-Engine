#version 450 core

layout(location = 0) out vec3 gPosition; 
layout(location = 1) out vec3 gNormal; 
layout(location = 2) out vec4 gAlbedoSpec;

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoords;
    mat3 TBN;
} fs_in;


uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

uniform bool hasNormalMap;
uniform bool hasSpecularMap;

void main() {
    vec4 albedo = texture(diffuseMap, fs_in.texCoords);
    if (albedo.a < 0.5) discard;

    vec3 N = hasNormalMap ? normalize(fs_in.TBN * (texture(normalMap, fs_in.texCoords).rgb * 2.0 - 1.0)) 
                            : normalize(fs_in.normal);

    float specStrength = hasSpecularMap ? texture(specularMap, fs_in.texCoords).r: 1.0;

    gPosition = fs_in.fragPos;
    gNormal = N;
    gAlbedoSpec.rgb = albedo.rgb;
    gAlbedoSpec.a = albedo.a;
}
